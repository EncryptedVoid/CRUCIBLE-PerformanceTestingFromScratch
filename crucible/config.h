#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <limits.h>

// Test mode enum
typedef enum
{
    BASELINE,
    STRESS,
    LOAD,
    SPIKE,
    ALL
} TestMode;

// Main configuration structure
typedef struct
{
    // Components to test
    bool cpu_enabled;
    bool memory_enabled;
    bool storage_enabled;
    bool network_enabled;
    bool io_enabled;

    // Test configuration
    TestMode mode;
    int duration_seconds;
    int sample_frequency_seconds;
    int diagnostics_duration_seconds;

    // Logging configuration
    char log_directory[PATH_MAX];
} TestConfig;

// Initialize configuration with defaults
void initialize_config(TestConfig *config);

// Parse components string (comma-separated list)
bool parse_components(const char *components, TestConfig *config);

// Parse test mode string
bool parse_mode(const char *mode, TestConfig *config);

// Validate configuration values
bool validate_config(TestConfig *config);

// Estimate total test duration
int estimate_test_duration(TestConfig *config);

// Sleep until next sample time
void sleep_until_next_sample(time_t *next_sample, int sample_interval);

#endif /* CONFIG_H */