#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include "config.h"
#include "logger.h"
#include "diagnostics.h"
#include "cpu.h"
#include "memory.h"
#include "storage.h"
#include "network.h"
#include "io.h"

#define VERSION "0.1.0"

void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  --components=LIST     Comma-separated list of components to test\n");
    printf("                        (cpu,memory,storage,network,io,all)\n");
    printf("  --mode=MODE           Test mode (baseline,stress,load,spike,all)\n");
    printf("  --duration=SECONDS    Test duration in seconds\n");
    printf("  --sample-freq=SECONDS Sampling frequency in seconds\n");
    printf("  --diagnostics=SECONDS Diagnostic duration in seconds (0 to skip)\n");
    printf("  --log-dir=PATH        Directory for log output\n");
    printf("  --help                Display this help message\n");
    printf("  --version             Display version information\n");
}

int main(int argc, char *argv[])
{
    TestConfig config;

    // Set default configuration
    initialize_config(&config);

    // Long options for getopt_long
    static struct option long_options[] = {
        {"components", required_argument, 0, 'c'},
        {"mode", required_argument, 0, 'm'},
        {"duration", required_argument, 0, 'd'},
        {"sample-freq", required_argument, 0, 's'},
        {"diagnostics", required_argument, 0, 'g'},
        {"log-dir", required_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c;

    // Parse command line arguments
    while ((c = getopt_long(argc, argv, "c:m:d:s:g:l:hv", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'c':
            parse_components(optarg, &config);
            break;
        case 'm':
            parse_mode(optarg, &config);
            break;
        case 'd':
            config.duration_seconds = atoi(optarg);
            break;
        case 's':
            config.sample_frequency_seconds = atoi(optarg);
            break;
        case 'g':
            config.diagnostics_duration_seconds = atoi(optarg);
            break;
        case 'l':
            strncpy(config.log_directory, optarg, PATH_MAX - 1);
            config.log_directory[PATH_MAX - 1] = '\0';
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        case 'v':
            printf("%s version %s\n", argv[0], VERSION);
            return EXIT_SUCCESS;
        case '?':
            // Invalid option or missing argument handled by getopt
            return EXIT_FAILURE;
        default:
            // Should not happen
            return EXIT_FAILURE;
        }
    }

    // Validate configuration
    if (!validate_config(&config))
    {
        fprintf(stderr, "Invalid configuration. See --help for usage.\n");
        return EXIT_FAILURE;
    }

    // Initialize logging
    if (!initialize_logging(&config))
    {
        fprintf(stderr, "Failed to initialize logging to %s\n", config.log_directory);
        return EXIT_FAILURE;
    }

    log_message(LOG_INFO, "Starting performance test with configuration:");
    log_config(&config);

    // Calculate and log test duration estimate
    int total_duration = estimate_test_duration(&config);
    log_message(LOG_INFO, "Estimated test duration: %d seconds", total_duration);

    // Run diagnostics if enabled
    if (config.diagnostics_duration_seconds > 0)
    {
        log_message(LOG_INFO, "Running system diagnostics for %d seconds...",
                    config.diagnostics_duration_seconds);

        if (!run_diagnostics(&config))
        {
            log_message(LOG_ERROR, "Diagnostics failed, aborting test");
            finalize_logging();
            return EXIT_FAILURE;
        }

        log_message(LOG_INFO, "Diagnostics completed successfully");
    }
    else
    {
        log_message(LOG_INFO, "Diagnostics skipped");
    }

    // Run the tests for each enabled component
    time_t start_time = time(NULL);

    if (config.cpu_enabled)
    {
        log_message(LOG_INFO, "Starting CPU tests");
        run_cpu_tests(&config);
    }

    if (config.memory_enabled)
    {
        log_message(LOG_INFO, "Starting memory tests");
        run_memory_tests(&config);
    }

    if (config.storage_enabled)
    {
        log_message(LOG_INFO, "Starting storage tests");
        run_storage_tests(&config);
    }

    if (config.network_enabled)
    {
        log_message(LOG_INFO, "Starting network tests");
        run_network_tests(&config);
    }

    if (config.io_enabled)
    {
        log_message(LOG_INFO, "Starting I/O tests");
        run_io_tests(&config);
    }

    time_t end_time = time(NULL);

    // Log test completion
    log_message(LOG_INFO, "All tests completed in %ld seconds", (end_time - start_time));

    // Clean up
    finalize_logging();

    return EXIT_SUCCESS;
}