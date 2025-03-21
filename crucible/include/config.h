/**
 * Configuration Management System Header
 *
 * This header file defines the data structures and function declarations
 * for the configuration management system. Include this file in other
 * source files that need to access configuration data.
 *
 * Author: Your Name
 * Date: March 20, 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

/* Component types enumeration */
typedef enum
{
    COMP_CPU,
    COMP_MEMORY,
    COMP_STORAGE,
    COMP_NETWORK,
    COMP_ALL,
    COMP_INVALID
} Component;

/* Test mode enumeration */
typedef enum
{
    MODE_STRESS,
    MODE_LOAD,
    MODE_SPIKE,
    MODE_BASELINE,
    MODE_INVALID
} TestMode;

/* Configuration structure */
typedef struct
{
    Component component; /* Component to test */
    TestMode mode;       /* Test mode */
    int duration;        /* Total test duration in seconds */
    int interval;        /* Sampling/logging interval in seconds */
    int intensity;       /* Stress intensity (0-100) */
    char *log_dir;       /* Directory for logs */
    bool verbose;        /* Verbose output flag */
} Config;

/* Function declarations */
void init_default_config(Config *config);
Component parse_component(const char *comp_str);
TestMode parse_test_mode(const char *mode_str);
const char *component_to_string(Component comp);
const char *mode_to_string(TestMode mode);
bool validate_config(Config *config);

#endif /* CONFIG_H */