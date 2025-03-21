/**
 * Configuration Management System
 *
 * This file implements a command-line configuration parser for a system testing tool.
 * It parses command-line arguments, validates them, and stores them in a central
 * configuration structure for use throughout the program.
 *
 * Author: Your Name
 * Date: March 20, 2025
 */

/*
 * Include Statements:
 * These are header files that provide access to standard functions and types
 */
#include <stdio.h>   // Standard input/output functions (printf, fprintf)
#include <stdlib.h>  // General utilities (malloc, free, exit)
#include <string.h>  // String manipulation functions (strcasecmp, strdup)
#include <config.h>  // Command-line option parsing functionality
#include <stdbool.h> // Boolean type support (true/false) - not built into C by default
#include <errno.h>   // Error handling variables and macros
#include <limits.h>  // Constants for numeric type limits

/*
 * Preprocessor Directives:
 * These are instructions for the compiler that are processed before compilation.
 * #define creates a constant that gets replaced by its value at compile-time.
 */
#define VERSION "1.0.0"

/*
 * Enumeration Types:
 * Enums in C are a way to define named integer constants.
 * By default, the first enum value is 0, the second is 1, and so on.
 * The 'typedef' keyword creates a new type name (Component) for this enum.
 */
typedef enum
{
    COMP_CPU,     // Value is 0
    COMP_MEMORY,  // Value is 1
    COMP_STORAGE, // Value is 2
    COMP_NETWORK, // Value is 3
    COMP_ALL,     // Value is 4
    COMP_INVALID  // Value is 5
} Component;

/*
 * Another enumeration for test modes
 */
typedef enum
{
    MODE_STRESS,   // Value is 0
    MODE_LOAD,     // Value is 1
    MODE_SPIKE,    // Value is 2
    MODE_BASELINE, // Value is 3
    MODE_INVALID   // Value is 4
} TestMode;

/*
 * Structure Definition:
 * A struct in C is a composite data type that groups variables under one name.
 * The 'typedef' creates a type name (Config) for this struct.
 * Unlike Java or Python:
 * - There's no automatic constructor/destructor
 * - Memory for string fields must be manually allocated and freed
 * - There are no methods within structs
 */
typedef struct
{
    Component component; // Component to test
    TestMode mode;       // Test mode
    int duration;        // Total test duration in seconds
    int interval;        // Sampling/logging interval in seconds
    int intensity;       // Stress intensity (0-100)
    char *log_dir;       // Directory for logs (pointer to char - a string)
    bool verbose;        // Verbose output flag
} Config;

/*
 * Function Prototypes:
 * C requires functions to be declared before they're used unless
 * the function is defined before its first use. These declarations
 * tell the compiler what return type and parameters each function has.
 */
void print_usage(const char *program_name);
void print_version(void);
bool parse_int(const char *str, int *value, int min, int max);
bool validate_config(Config *config);
void init_default_config(Config *config);
Component parse_component(const char *comp_str);
TestMode parse_test_mode(const char *mode_str);
const char *component_to_string(Component comp);
const char *mode_to_string(TestMode mode);

/*
 * Main Function:
 * The main function is the entry point of C programs.
 * It returns an integer (typically 0 for success, non-zero for errors) and takes:
 * - argc: The count of command-line arguments
 * - argv: An array of strings containing those arguments
 */
int main(int argc, char *argv[])
{
    /* Define and initialize our configuration structure */
    Config config;

    /*
     * Initialize config with default values.
     * In C, we must explicitly initialize our structures.
     */
    init_default_config(&config);

    /*
     * Define Long Options:
     * This array defines the command-line options our program accepts.
     * Each entry specifies:
     * 1. Option name (e.g., "component")
     * 2. Argument requirement (required_argument, optional_argument, or no_argument)
     * 3. Flag variable (NULL in this case)
     * 4. Short option character (e.g., 'c')
     *
     * The 'static' keyword means this array is only visible within this function.
     */
    static struct option long_options[] = {
        {"component", required_argument, NULL, 'c'},
        {"mode", required_argument, NULL, 'm'},
        {"duration", required_argument, NULL, 'd'},
        {"interval", required_argument, NULL, 'i'},
        {"intensity", required_argument, NULL, 'n'},
        {"log-dir", required_argument, NULL, 'l'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0} /* Terminating entry */
    };

    int opt;
    int option_index = 0;

    /*
     * Parse Command Line Options:
     * This loop processes all provided command-line options.
     * getopt_long() returns:
     * - The next option character
     * - -1 when there are no more options
     *
     * The string "c:m:d:i:n:l:vhV" specifies the short options:
     * - Letters with colons (:) require an argument
     * - Letters without colons don't take arguments
     */
    while ((opt = getopt_long(argc, argv, "c:m:d:i:n:l:vhV", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'c': /* component */
            /*
             * Parse the component option and handle errors
             * optarg is a global variable set by getopt_long() to point to the option argument
             */
            config.component = parse_component(optarg);
            if (config.component == COMP_INVALID)
            {
                fprintf(stderr, "Error: Invalid component. Valid options are: cpu, memory, storage, network, all\n");
                return EXIT_FAILURE;
            }
            break;

        case 'm': /* mode */
            config.mode = parse_test_mode(optarg);
            if (config.mode == MODE_INVALID)
            {
                fprintf(stderr, "Error: Invalid test mode. Valid options are: stress, load, spike, baseline\n");
                return EXIT_FAILURE;
            }
            break;

        case 'd': /* duration */
            /*
             * Parse integer options with range validation
             * parse_int() validates and converts the string to an integer
             */
            if (!parse_int(optarg, &config.duration, 1, 86400))
            { /* Max 24 hours */
                fprintf(stderr, "Error: Invalid test duration. Must be between 1 and 86400 seconds.\n");
                return EXIT_FAILURE;
            }
            break;

        case 'i': /* interval */
            if (!parse_int(optarg, &config.interval, 1, 3600))
            { /* Max 1 hour */
                fprintf(stderr, "Error: Invalid sampling interval. Must be between 1 and 3600 seconds.\n");
                return EXIT_FAILURE;
            }
            break;

        case 'n': /* intensity */
            if (!parse_int(optarg, &config.intensity, 0, 100))
            {
                fprintf(stderr, "Error: Invalid intensity. Must be between 0 and 100 percent.\n");
                return EXIT_FAILURE;
            }
            break;

        case 'l': /* log-dir */
            /*
             * String Handling in C:
             * 1. First free any previously allocated memory to prevent leaks
             * 2. Then allocate new memory and copy the string
             *
             * This is necessary because in C:
             * - Strings are null-terminated character arrays
             * - strdup() allocates memory on the heap
             * - You must free() this memory later to prevent leaks
             */
            free(config.log_dir); /* Free previous value if set */
            config.log_dir = strdup(optarg);

            /*
             * Error Handling:
             * C has no exceptions like Java or Python. Error handling is done by:
             * - Checking return values
             * - Using the global errno variable
             * - Returning error codes
             * - Writing error messages to stderr
             */
            if (!config.log_dir)
            {
                fprintf(stderr, "Error: Memory allocation failed.\n");
                return EXIT_FAILURE;
            }
            break;

        case 'v': /* verbose */
            config.verbose = true;
            break;

        case 'h': /* help */
            print_usage(argv[0]);
            return EXIT_SUCCESS;

        case 'V': /* version */
            print_version();
            return EXIT_SUCCESS;

        case '?':
            /*
             * Invalid option - getopt_long already printed an error message
             * We show usage information and exit
             */
            print_usage(argv[0]);
            return EXIT_FAILURE;

        default:
            fprintf(stderr, "Error: Unexpected getopt behavior.\n");
            return EXIT_FAILURE;
        }
    }

    /*
     * Check for Non-Option Arguments:
     * After getopt_long() finishes, optind is the index of the first non-option argument
     * We check if there are any unexpected arguments
     */
    if (optind < argc)
    {
        fprintf(stderr, "Error: Unexpected arguments: ");
        while (optind < argc)
        {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /*
     * Validate Configuration:
     * Perform additional validation to ensure the configuration is consistent
     */
    if (!validate_config(&config))
    {
        fprintf(stderr, "Error: Invalid configuration.\n");
        free(config.log_dir); /* Clean up before exit */
        return EXIT_FAILURE;
    }

    /*
     * Display Configuration:
     * If verbose mode is enabled, show the configured parameters
     */
    if (config.verbose)
    {
        printf("Configuration:\n");
        printf("  Component: %s\n", component_to_string(config.component));
        printf("  Test Mode: %s\n", mode_to_string(config.mode));
        printf("  Duration: %d seconds\n", config.duration);
        printf("  Interval: %d seconds\n", config.interval);
        printf("  Intensity: %d%%\n", config.intensity);
        printf("  Log Directory: %s\n", config.log_dir ? config.log_dir : "current directory");
        printf("  Verbose Mode: enabled\n");
    }

    /*
     * Your test logic would go here
     * This is where you would use the configuration to set up and run your tests
     */
    printf("Starting test with the configured parameters...\n");

    /*
     * Memory Management:
     * In C, you must manually free dynamically allocated memory when done
     * If you don't, you'll have a memory leak.
     * This is different from Python or Java, which have garbage collectors.
     */
    free(config.log_dir);

    /*
     * Return Value:
     * C programs typically return 0 (or EXIT_SUCCESS) for successful execution
     * and non-zero values (often EXIT_FAILURE) for errors.
     */
    return EXIT_SUCCESS;
}

/*
 * Initialize config with default values
 *
 * This function sets sensible defaults for all configuration options.
 * In C, struct members are not automatically initialized.
 *
 * Parameters:
 *   config - Pointer to the configuration structure to initialize
 */
void init_default_config(Config *config)
{
    /* Checking for NULL is a common defensive programming practice in C */
    if (!config)
        return;

    config->component = COMP_ALL; /* Default: test all components */
    config->mode = MODE_STRESS;   /* Default: stress test */
    config->duration = 60;        /* Default: 60 seconds */
    config->interval = 5;         /* Default: 5 seconds */
    config->intensity = 80;       /* Default: 80% */
    config->log_dir = NULL;       /* Default: current directory */
    config->verbose = false;      /* Default: non-verbose mode */
}

/*
 * Parse component string
 *
 * Converts a string like "cpu" to the corresponding Component enum value.
 * Uses strcasecmp() for case-insensitive comparison.
 *
 * Parameters:
 *   comp_str - String to parse
 *
 * Returns:
 *   Component enum value, or COMP_INVALID if invalid
 */
Component parse_component(const char *comp_str)
{
    if (!comp_str)
        return COMP_INVALID;

    /*
     * String Comparison:
     * C doesn't have built-in operators for string comparison. You must use functions like:
     * - strcmp(): Case-sensitive comparison
     * - strcasecmp(): Case-insensitive comparison (not standard C, but available on many systems)
     */
    if (strcasecmp(comp_str, "cpu") == 0)
        return COMP_CPU;
    if (strcasecmp(comp_str, "memory") == 0)
        return COMP_MEMORY;
    if (strcasecmp(comp_str, "storage") == 0)
        return COMP_STORAGE;
    if (strcasecmp(comp_str, "network") == 0)
        return COMP_NETWORK;
    if (strcasecmp(comp_str, "all") == 0)
        return COMP_ALL;

    return COMP_INVALID;
}

/*
 * Parse test mode string
 *
 * Converts a string like "stress" to the corresponding TestMode enum value.
 * Uses strcasecmp() for case-insensitive comparison.
 *
 * Parameters:
 *   mode_str - String to parse
 *
 * Returns:
 *   TestMode enum value, or MODE_INVALID if invalid
 */
TestMode parse_test_mode(const char *mode_str)
{
    if (!mode_str)
        return MODE_INVALID;

    if (strcasecmp(mode_str, "stress") == 0)
        return MODE_STRESS;
    if (strcasecmp(mode_str, "load") == 0)
        return MODE_LOAD;
    if (strcasecmp(mode_str, "spike") == 0)
        return MODE_SPIKE;
    if (strcasecmp(mode_str, "baseline") == 0)
        return MODE_BASELINE;

    return MODE_INVALID;
}

/*
 * Convert component enum to string
 *
 * Returns a human-readable string representation of a Component enum value.
 *
 * Parameters:
 *   comp - Component enum value
 *
 * Returns:
 *   Constant string representation (do not free this memory)
 */
const char *component_to_string(Component comp)
{
    /*
     * Constants and String Literals:
     * The 'const' keyword indicates that this function won't modify the returned string.
     * In C, string literals are stored in read-only memory and shouldn't be modified.
     */
    switch (comp)
    {
    case COMP_CPU:
        return "CPU";
    case COMP_MEMORY:
        return "Memory";
    case COMP_STORAGE:
        return "Storage";
    case COMP_NETWORK:
        return "Network";
    case COMP_ALL:
        return "All Components";
    default:
        return "Unknown";
    }
}

/*
 * Convert test mode enum to string
 *
 * Returns a human-readable string representation of a TestMode enum value.
 *
 * Parameters:
 *   mode - TestMode enum value
 *
 * Returns:
 *   Constant string representation (do not free this memory)
 */
const char *mode_to_string(TestMode mode)
{
    switch (mode)
    {
    case MODE_STRESS:
        return "Stress Test";
    case MODE_LOAD:
        return "Load Test";
    case MODE_SPIKE:
        return "Spike Test";
    case MODE_BASELINE:
        return "Baseline Test";
    default:
        return "Unknown";
    }
}

/*
 * Parse and validate an integer argument
 *
 * Safely converts a string to an integer and validates it's within range.
 *
 * Parameters:
 *   str   - String to parse
 *   value - Pointer to store the parsed value
 *   min   - Minimum allowed value
 *   max   - Maximum allowed value
 *
 * Returns:
 *   true if successful, false if parsing failed or value out of range
 */
bool parse_int(const char *str, int *value, int min, int max)
{
    /*
     * Pointers:
     * Pointers are variables that store memory addresses. They're used:
     * - To modify variables from within functions (pass-by-reference)
     * - To work with dynamically allocated memory
     * - To handle strings (which are character arrays)
     *
     * The '*' in this context means "pointer to", so 'int *value' is a pointer to an integer.
     */
    if (!str || !value)
        return false;

    char *endptr;
    errno = 0;

    /*
     * String to Integer Conversion:
     * strtol() converts a string to a long integer.
     * It's safer than atoi() because it:
     * - Provides error detection
     * - Supports different numeric bases
     * - Tells you where parsing stopped (via endptr)
     */
    long result = strtol(str, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE || *endptr != '\0' || str == endptr)
    {
        return false;
    }

    /* Check if the value is within the allowed range */
    if (result < min || result > max)
    {
        return false;
    }

    *value = (int)result;
    return true;
}

/*
 * Validate the complete configuration
 *
 * Checks the entire configuration for consistency and validity.
 *
 * Parameters:
 *   config - Pointer to the configuration to validate
 *
 * Returns:
 *   true if the configuration is valid, false otherwise
 */
bool validate_config(Config *config)
{
    if (!config)
        return false;

    /* Ensure component is valid */
    if (config->component == COMP_INVALID)
    {
        fprintf(stderr, "Error: Invalid component selected.\n");
        return false;
    }

    /* Ensure test mode is valid */
    if (config->mode == MODE_INVALID)
    {
        fprintf(stderr, "Error: Invalid test mode selected.\n");
        return false;
    }

    /* Ensure duration is valid */
    if (config->duration <= 0)
    {
        fprintf(stderr, "Error: Test duration must be positive.\n");
        return false;
    }

    /* Ensure interval is valid and less than duration */
    if (config->interval <= 0)
    {
        fprintf(stderr, "Error: Sampling interval must be positive.\n");
        return false;
    }
    if (config->interval > config->duration)
    {
        fprintf(stderr, "Error: Sampling interval cannot be greater than test duration.\n");
        return false;
    }

    /* Ensure intensity is within range */
    if (config->intensity < 0 || config->intensity > 100)
    {
        fprintf(stderr, "Error: Intensity must be between 0 and 100 percent.\n");
        return false;
    }

    /* Add additional validation as needed */

    return true;
}

/*
 * Print usage information
 *
 * Displays help information about command-line options.
 *
 * Parameters:
 *   program_name - Name of the program (usually argv[0])
 */
void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  --component=COMP     Component to test (cpu, memory, storage, network, all)\n");
    printf("  --mode=MODE          Test mode (stress, load, spike, baseline)\n");
    printf("  --duration=SECONDS   Total test duration (1-86400, default: 60)\n");
    printf("  --interval=SECONDS   Sampling/logging interval (1-3600, default: 5)\n");
    printf("  --intensity=PERCENT  Stress intensity (0-100, default: 80)\n");
    printf("  --log-dir=PATH       Directory for logs (default: current directory)\n");
    printf("  --verbose, -v        Enable verbose output\n");
    printf("  --help, -h           Display this help and exit\n");
    printf("  --version, -V        Display version information and exit\n");
}

/*
 * Print version information
 *
 * Displays the program version and copyright information.
 */
void print_version(void)
{
    printf("Performance Test Tool version %s\n", VERSION);
    printf("Copyright (C) 2025 Your Organization\n");
    printf("License: MIT\n");
}