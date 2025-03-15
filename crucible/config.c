#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "logger.h"

// Initialize configuration with default values
void initialize_config(TestConfig *config)
{
    if (config == NULL)
    {
        return;
    }

    // Default to all components enabled
    config->cpu_enabled = true;
    config->memory_enabled = true;
    config->storage_enabled = true;
    config->network_enabled = true;
    config->io_enabled = true;

    // Default test configuration
    config->mode = ALL;
    config->duration_seconds = 300;            // 5 minutes
    config->sample_frequency_seconds = 5;      // 5 seconds
    config->diagnostics_duration_seconds = 60; // 1 minute

    // Default log directory to current directory
    getcwd(config->log_directory, PATH_MAX);
}

// Parse components string (comma-separated list)
bool parse_components(const char *components, TestConfig *config)
{
    if (components == NULL || config == NULL)
    {
        return false;
    }

    // First, disable all components
    config->cpu_enabled = false;
    config->memory_enabled = false;
    config->storage_enabled = false;
    config->network_enabled = false;
    config->io_enabled = false;

    // Make a copy of the components string for tokenization
    char *components_copy = strdup(components);
    if (components_copy == NULL)
    {
        return false;
    }

    // Split by comma
    char *token = strtok(components_copy, ",");
    while (token != NULL)
    {
        // Trim whitespace
        while (isspace(*token))
            token++;
        char *end = token + strlen(token