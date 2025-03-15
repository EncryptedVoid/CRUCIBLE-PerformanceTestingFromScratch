#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sensors/sensors.h>

#include "config.h"
#include "logger.h"
#include "cpu.h"

// File path for CPU information
#define PROC_STAT "/proc/stat"
#define CPU_TEMP_CHIP "coretemp-isa-0000" // This may need adjustment for Raspberry Pi
#define CPU_TEMP_FEATURE "temp1"          // This may need adjustment for Raspberry Pi

// CPU stress testing configuration
typedef struct
{
    int num_threads;
    int stress_duration;
    int sample_interval;
} CpuStressConfig;

// Parse /proc/stat to get CPU usage
static bool get_cpu_usage(CpuUsage *usage)
{
    FILE *fp;
    char buffer[1024];
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

    fp = fopen(PROC_STAT, "r");
    if (fp == NULL)
    {
        log_message(LOG_ERROR, "Failed to open %s", PROC_STAT);
        return false;
    }

    // Read the first line which contains total CPU info
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        log_message(LOG_ERROR, "Failed to read from %s", PROC_STAT);
        fclose(fp);
        return false;
    }

    // Parse CPU line
    // Format: "cpu user nice system idle iowait irq softirq steal guest guest_nice"
    sscanf(buffer, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

    // Calculate total and idle times
    unsigned long total_idle = idle + iowait;
    unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;

    // Store current values
    usage->total = total;
    usage->idle = total_idle;

    fclose(fp);
    return true;
}

// Calculate CPU percentage from two consecutive readings
static float calculate_cpu_percentage(CpuUsage *prev, CpuUsage *curr)
{
    unsigned long total_delta = curr->total - prev->total;
    unsigned long idle_delta = curr->idle - prev->idle;

    if (total_delta == 0)
    {
        return 0.0;
    }

    return 100.0 * (1.0 - ((float)idle_delta / (float)total_delta));
}

// Get CPU temperature using libsensors
static float get_cpu_temperature()
{
    float temp = -1.0;
    const sensors_chip_name *chip;
    const sensors_feature *feature;
    const sensors_subfeature *subfeature;
    int chip_nr = 0;

    // Try to initialize sensors library
    if (sensors_init(NULL) != 0)
    {
        log_message(LOG_ERROR, "Failed to initialize libsensors");
        return temp;
    }

    // Look for the CPU temperature sensor
    while ((chip = sensors_get_detected_chips(NULL, &chip_nr)) != NULL)
    {
        if (strstr(chip->prefix, CPU_TEMP_CHIP) != NULL)
        {
            int feature_nr = 0;

            while ((feature = sensors_get_features(chip, &feature_nr)) != NULL)
            {
                if (feature->type == SENSORS_FEATURE_TEMP)
                {
                    subfeature = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);

                    if (subfeature)
                    {
                        if (sensors_get_value(chip, subfeature->number, &temp) != 0)
                        {
                            log_message(LOG_ERROR, "Failed to read CPU temperature");
                        }
                        break;
                    }
                }
            }
            break;
        }
    }

    sensors_cleanup();
    return temp;
}

// Get number of cores and threads
static bool get_cpu_info(CpuInfo *info)
{
    FILE *fp;
    char buffer[1024];
    int cores = 0;
    int threads = 0;

    // Read core count from /proc/cpuinfo
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
    {
        log_message(LOG_ERROR, "Failed to open /proc/cpuinfo");
        return false;
    }

    // Count the number of processor entries
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (strncmp(buffer, "processor", 9) == 0)
        {
            threads++;
        }
        else if (strncmp(buffer, "core id", 7) == 0)
        {
            int core_id;
            sscanf(buffer, "core id : %d", &core_id);
            if (core_id + 1 > cores)
            {
                cores = core_id + 1;
            }
        }
    }

    fclose(fp);

    if (threads == 0)
    {
        log_message(LOG_ERROR, "Failed to detect CPU thread count");
        return false;
    }

    // If we couldn't determine cores, assume threads = cores
    if (cores == 0)
    {
        cores = threads;
    }

    info->cores = cores;
    info->threads = threads;

    return true;
}

// CPU stress function for a single thread
static void *cpu_stress_thread(void *arg)
{
    CpuStressConfig *config = (CpuStressConfig *)arg;
    time_t end_time = time(NULL) + config->stress_duration;

    // Simple busy loop
    while (time(NULL) < end_time)
    {
        // Perform CPU-intensive calculations
        double result = 0.0;
        for (int i = 0; i < 10000000; i++)
        {
            result += sqrt((double)i);
        }

        // Prevent compiler from optimizing away the calculation
        if (result < 0)
        {
            printf("%f", result);
        }
    }

    return NULL;
}

// Run CPU stress test with the given configuration
static bool run_cpu_stress(TestConfig *config, TestMode mode)
{
    CpuStressConfig stress_config;
    CpuInfo cpu_info;
    pthread_t *threads;
    int i;

    // Get CPU info to determine thread count
    if (!get_cpu_info(&cpu_info))
    {
        log_message(LOG_ERROR, "Failed to get CPU information");
        return false;
    }

    // Configure stress test based on mode
    switch (mode)
    {
    case BASELINE:
        // No stress for baseline, just monitoring
        stress_config.num_threads = 0;
        stress_config.stress_duration = config->duration_seconds;
        break;

    case STRESS:
        // Use all threads for maximum stress
        stress_config.num_threads = cpu_info.threads;
        stress_config.stress_duration = config->duration_seconds;
        break;

    case LOAD:
        // Use half the threads for moderate load
        stress_config.num_threads = cpu_info.threads / 2;
        stress_config.stress_duration = config->duration_seconds;
        break;

    case SPIKE:
        // Alternate between high and low stress
        stress_config.num_threads = cpu_info.threads;
        stress_config.stress_duration = 30; // 30 second spikes
        break;

    default:
        log_message(LOG_ERROR, "Invalid test mode for CPU stress test");
        return false;
    }

    // If no threads to stress, just return (baseline mode)
    if (stress_config.num_threads == 0)
    {
        return true;
    }

    // Create and start stress threads
    threads = (pthread_t *)malloc(stress_config.num_threads * sizeof(pthread_t));
    if (threads == NULL)
    {
        log_message(LOG_ERROR, "Memory allocation failed for CPU stress threads");
        return false;
    }

    log_message(LOG_INFO, "Starting CPU stress test with %d threads for %d seconds",
                stress_config.num_threads, stress_config.stress_duration);

    for (i = 0; i < stress_config.num_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, cpu_stress_thread, &stress_config) != 0)
        {
            log_message(LOG_ERROR, "Failed to create CPU stress thread %d", i);
            // Clean up created threads
            for (int j = 0; j < i; j++)
            {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], NULL);
            }
            free(threads);
            return false;
        }
    }

    // Wait for all threads to complete
    for (i = 0; i < stress_config.num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    log_message(LOG_INFO, "CPU stress test completed");

    return true;
}

// Monitor CPU metrics at regular intervals
static bool monitor_cpu_metrics(TestConfig *config)
{
    CpuUsage prev_usage, curr_usage;
    CpuInfo cpu_info;
    time_t end_time = time(NULL) + config->duration_seconds;
    time_t next_sample = time(NULL);
    float cpu_percent, cpu_temp;

    // Get initial CPU info
    if (!get_cpu_info(&cpu_info))
    {
        log_message(LOG_ERROR, "Failed to get CPU information for monitoring");
        return false;
    }

    log_message(LOG_INFO, "CPU info: %d cores, %d threads", cpu_info.cores, cpu_info.threads);

    // Get initial CPU usage
    if (!get_cpu_usage(&prev_usage))
    {
        log_message(LOG_ERROR, "Failed to get initial CPU usage");
        return false;
    }

    // Monitoring loop
    while (time(NULL) < end_time)
    {
        // Wait until next sample time
        sleep_until_next_sample(&next_sample, config->sample_frequency_seconds);

        // Get current CPU usage
        if (!get_cpu_usage(&curr_usage))
        {
            log_message(LOG_ERROR, "Failed to get CPU usage");
            continue;
        }

        // Calculate CPU usage percentage
        cpu_percent = calculate_cpu_percentage(&prev_usage, &curr_usage);

        // Get CPU temperature
        cpu_temp = get_cpu_temperature();

        // Log metrics
        log_message(LOG_DATA, "CPU,Usage,%.2f,%%", cpu_percent);

        if (cpu_temp > 0)
        {
            log_message(LOG_DATA, "CPU,Temperature,%.2f,C", cpu_temp);
        }

        // Update previous usage for next iteration
        prev_usage = curr_usage;
    }

    return true;
}

// Run all CPU tests according to configuration
bool run_cpu_tests(TestConfig *config)
{
    bool success = true;
    pthread_t monitor_thread;

    // Create separate thread for monitoring
    pthread_create(&monitor_thread, NULL, (void *(*)(void *))monitor_cpu_metrics, config);

    // Run stress test based on configured mode
    if (config->mode == ALL)
    {
        // Run all test modes in sequence
        log_message(LOG_INFO, "Running CPU baseline test");
        success &= run_cpu_stress(config, BASELINE);

        log_message(LOG_INFO, "Running CPU load test");
        success &= run_cpu_stress(config, LOAD);

        log_message(LOG_INFO, "Running CPU stress test");
        success &= run_cpu_stress(config, STRESS);

        log_message(LOG_INFO, "Running CPU spike test");
        success &= run_cpu_stress(config, SPIKE);
    }
    else
    {
        // Run single test mode
        success = run_cpu_stress(config, config->mode);
    }

    // Wait for monitoring thread to complete
    pthread_join(monitor_thread, NULL);

    return success;
}