# Crucible Project Architecture: Simple Breakdown

## Project File Structure

```
crucible/
├── src/
│   ├── main.c               # Entry point
│   ├── config.c             # Configuration handling
│   ├── logger.c             # Logging functionality
│   ├── cpu_test.c           # CPU testing functions
│   ├── memory_test.c        # Memory testing functions
│   ├── storage_test.c       # Storage testing functions
│   └── network_test.c       # Network testing functions
├── include/
│   ├── config.h             # Configuration declarations
│   ├── logger.h             # Logging declarations
│   ├── cpu_test.h           # CPU test declarations
│   ├── memory_test.h        # Memory test declarations
│   ├── storage_test.h       # Storage test declarations
│   └── network_test.h       # Network test declarations
└── Makefile                 # Build instructions
```

## How The Code Works Together

1. **main.c**: Parses arguments, initializes modules, runs selected tests
2. **config.c/h**: Stores all test settings
3. **logger.c/h**: Creates log files and writes to them
4. **[component]_test.c/h**: Contains stress and probe functions for each component

## Key Function Signatures and Their Purpose

### 1. main.c - Program Entry Point

```c
int main(int argc, char *argv[]) {
    // Parse command-line args into config
    parse_arguments(argc, argv, &config);

    // Initialize logger
    logger_init(config.log_dir);

    // Run the selected test based on component
    if (strcmp(config.component, "cpu") == 0) {
        run_cpu_test();
    } else if (strcmp(config.component, "memory") == 0) {
        run_memory_test();
    } // etc.

    // Cleanup and exit
    logger_cleanup();
    return 0;
}
```

### 2. config.c/h - Configuration Management

```c
// config.h
typedef struct {
    char component[32];    // "cpu", "memory", etc.
    char mode[32];         // "stress", "load", "spike"
    int duration;          // Test duration in seconds
    int interval;          // Sampling interval in seconds
    int intensity;         // Test intensity (0-100%)
    char log_dir[256];     // Directory for logs
} CrucibleConfig;

void config_init(CrucibleConfig *config);
int parse_arguments(int argc, char *argv[], CrucibleConfig *config);
```

### 3. logger.c/h - Logging Functions

```c
// logger.h
typedef enum {
    LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR
} LogLevel;

int logger_init(const char *log_dir);
void logger_log(LogLevel level, const char *format, ...);
void logger_log_metric(const char *component, const char *metric, double value, const char *unit);
void logger_cleanup(void);
```

### 4. cpu_test.c/h - CPU Testing Functions

```c
// cpu_test.h
int run_cpu_test(void);                  // Main entry for CPU tests
int cpu_stress_prime(int intensity);     // CPU stress via prime calculation
int cpu_stress_float(int intensity);     // CPU stress via floating point math
void cpu_probe_temperature(void);        // Measure and log CPU temperature
void cpu_probe_usage(void);              // Measure and log CPU usage
```

## Concrete Example: CPU Testing Implementation

Let's look at what these functions actually do:

### In cpu_test.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/cpu_test.h"
#include "../include/logger.h"
#include "../include/config.h"

// Global config (initialized in main.c)
extern CrucibleConfig config;

// Flag to control test duration
static int keep_running = 0;

// Worker function for thread to stress CPU
void* cpu_stress_worker(void* arg) {
    int intensity = *((int*)arg);
    unsigned long long i = 0;
    double result = 0.0;

    // Calculate intensity - higher means more iterations
    unsigned long long max_iter = 10000000 * (intensity / 10);

    while (keep_running) {
        // Simple but intensive calculation
        for (i = 0; i < max_iter; i++) {
            result += 1.0 / (i + 1.0);
        }
    }

    return NULL;
}

// CPU stress function
int cpu_stress_prime(int intensity) {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[16]; // Max 16 threads

    // Use at most num_cores threads
    int thread_count = (num_cores * intensity) / 100;
    if (thread_count < 1) thread_count = 1;

    logger_log(LOG_INFO, "Starting CPU stress with %d threads at %d%% intensity",
               thread_count, intensity);

    // Start the stress threads
    keep_running = 1;
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, cpu_stress_worker, &intensity);
    }

    // Wait for the specified duration
    sleep(config.duration);

    // Stop the stress threads
    keep_running = 0;
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    logger_log(LOG_INFO, "CPU stress completed");
    return 0;
}

// Function to read CPU temperature
double get_cpu_temperature(void) {
    // This would normally read from /sys/class/thermal
    // For simplicity, let's return a random value
    return 40.0 + (rand() % 20);
}

// CPU temperature probe function
void cpu_probe_temperature(void) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/cpu_temperature.csv", config.log_dir);

    FILE* log_file = fopen(log_path, "w");
    if (!log_file) {
        logger_log(LOG_ERROR, "Failed to create CPU temperature log");
        return;
    }

    // Write CSV header
    fprintf(log_file, "timestamp,temperature,unit\n");

    // Probe loop
    time_t start_time = time(NULL);
    while ((time(NULL) - start_time) < config.duration) {
        // Get current time
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

        // Measure temperature
        double temp = get_cpu_temperature();

        // Log to file and through logger
        fprintf(log_file, "%s,%.2f,C\n", timestamp, temp);
        fflush(log_file);

        logger_log_metric("CPU", "Temperature", temp, "C");

        // Wait for next interval
        sleep(config.interval);
    }

    fclose(log_file);
}

// Main CPU test function
int run_cpu_test(void) {
    logger_log(LOG_INFO, "Starting CPU test in %s mode", config.mode);

    // Start probe in separate thread (simplified - actual code would use pthread)
    // For simplicity, we'll just call it directly and alternate with stress
    cpu_probe_temperature();

    // Apply stress based on mode
    if (strcmp(config.mode, "stress") == 0) {
        // Continuous stress
        cpu_stress_prime(config.intensity);
    }
    else if (strcmp(config.mode, "load") == 0) {
        // Alternating moderate stress
        for (int i = 0; i < 5; i++) {
            cpu_stress_prime(config.intensity / 2);
            sleep(5);
        }
    }
    else if (strcmp(config.mode, "spike") == 0) {
        // Short bursts of heavy stress
        for (int i = 0; i < 3; i++) {
            cpu_stress_prime(config.intensity);
            sleep(10);
        }
    }

    logger_log(LOG_INFO, "CPU test completed");
    return 0;
}
```

## How It All Ties Together

1. User runs: `./crucible --component=cpu --mode=stress --duration=60 --intensity=80`

2. Flow of execution:
   - `main.c` parses these arguments into the `config` structure
   - `main.c` calls `logger_init()` to set up log files
   - `main.c` sees "cpu" component and calls `run_cpu_test()`
   - `run_cpu_test()` starts CPU temperature probing
   - `run_cpu_test()` calls the appropriate stress function based on mode
   - Stress function creates threads to load the CPU
   - Probe function measures and logs temperature periodically
   - When test duration is reached, everything cleans up and exits

## Similar Pattern for Other Components

The memory, storage, and network modules follow the same pattern:

- A main test function called from main.c
- Stress functions that apply load in different ways
- Probe functions that measure and log metrics
- All functions using the global config structure

Does this simpler explanation help you understand how the pieces fit together? The main thing to remember is that C programs are just collections of functions that call each other, with data being passed between them through arguments and global variables.
