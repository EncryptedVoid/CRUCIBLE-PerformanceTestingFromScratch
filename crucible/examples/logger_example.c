/**
 * Logger Usage Example
 *
 * This file demonstrates how to use the logging framework in a typical
 * application. It shows initialization, logging at different levels,
 * metric logging, and cleanup.
 *
 * Author: Your Name
 * Date: March 20, 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* for sleep() */

/* Include our logger header */
#include "logger.h"
#include "config.h"

/* Function to simulate running a test */
void run_simulated_test(const Config *config);

int main(int argc, char *argv[])
{
    /* Parse command line arguments (from previous configuration example) */
    Config config;

    /* Initialize with default values for this example */
    config.component = COMP_CPU;
    config.mode = MODE_STRESS;
    config.duration = 10;  /* 10 seconds for this example */
    config.interval = 1;   /* 1 second sampling interval */
    config.intensity = 75; /* 75% intensity */
    config.log_dir = "test_logs";
    config.verbose = true;

    /* Initialize the logger */
    if (!logger_init(config.log_dir, config.verbose ? LOG_DEBUG : LOG_INFO, 10, true))
    {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    /* Log the test configuration */
    logger_info("Starting test with configuration:");
    logger_info("  Component: %s", component_to_string(config.component));
    logger_info("  Mode: %s", mode_to_string(config.mode));
    logger_info("  Duration: %d seconds", config.duration);
    logger_info("  Interval: %d seconds", config.interval);
    logger_info("  Intensity: %d%%", config.intensity);

    /* Run the test */
    logger_info("Test starting now");
    run_simulated_test(&config);
    logger_info("Test completed successfully");

    /* Clean up */
    logger_cleanup();

    printf("Test completed. Check logs in %s directory.\n", config.log_dir);

    return EXIT_SUCCESS;
}

/*
 * Run a simulated test that logs some sample data
 * This demonstrates how to use the logger in a real test
 */
void run_simulated_test(const Config *config)
{
    /* Record the start of the test */
    logger_debug("Initializing %s test", mode_to_string(config.mode));

    /* Log some sample metrics at the start */
    logger_metric("system_info", "cpu_cores=4,memory_total_mb=16384,os_version=Linux_5.15");

    /* Simulate a test running for the specified duration */
    for (int second = 0; second < config.duration; second++)
    {
        /* Log detailed debug info every interval */
        if (second % config.interval == 0)
        {
            logger_debug("Test progress: %d of %d seconds completed", second, config.duration);
        }

        /* Simulate some varying metrics */
        float cpu_usage = 50.0 + ((float)rand() / RAND_MAX) * 30.0;
        float memory_usage = 2048.0 + ((float)rand() / RAND_MAX) * 1024.0;
        int active_threads = 4 + (rand() % 3);

        /* Log performance metrics */
        logger_metric("performance", "cpu_percent=%.2f,memory_mb=%.2f,threads=%d",
                      cpu_usage, memory_usage, active_threads);

        /* Occasionally log a warning */
        if (cpu_usage > 75.0)
        {
            logger_warning("High CPU usage detected: %.2f%%", cpu_usage);
        }

        /* Simulate a rare error condition */
        if (second == config.duration / 2 && (rand() % 5) == 0)
        {
            logger_error("Simulated error occurred: resource allocation failed");
        }

        /* Wait one second */
        sleep(1);
    }

    /* Log a summary metric at the end */
    logger_metric("test_summary", "completed=true,duration=%d,success=true", config.duration);
}