#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

enum PerfTestType
{
    PTT_BASELINE, // Retrieve a baseline amount of data
    PTT_STRESS,   // Progressively increases load beyond normal operating capacity
    PTT_SPIKE,    // Suddenly applies a massive load increase, then drops back to normal levels
    PTT_LOAD,     // Gradually increases load to a predetermined level and maintains it for a specified duration
};

enum IOType
{
    ASYNC_IO,
    SYNC_IO
};

enum IOInterfaceType
{
    INTERFACE_USB3,
    INTERFACE_PCIE
};

// typedef struct
// {
//     int *cores;
//     int core_count;
//     char freq_min[16];
//     char freq_max[16];
//     char workload_type[16];
//     int threads_per_core;
//     bool test_thermal;
// } CPUOptions;

typedef struct
{
    char size[16];
    char pattern[16];
    char alloc_size[16];
    int alignment;
    bool numa_aware;
} MemoryOptions;

typedef struct
{
    char file_size[16];
    int read_ratio;
    char block_size[16];
    bool direct_io;
    char directory[256];
    int file_count;
} StorageOptions;

typedef struct
{
    char protocol[8];
    char target_ip[64];
    int port;
    char packet_size[16];
    int connection_count;
    char bandwidth_limit[16];
} NetworkOptions;

typedef struct
{
    char device_path[256];
    IOType io_type;
    char buffer_size[16];
    int operation_count;
    IOInterfaceType interface_type;
} IOOptions;

typedef struct
{
    int order;
    char component_type;
    PerfTestType test_type;
    int duration;
    union
    {
        CPUOptions cpu;
        MemoryOptions memory;
        StorageOptions storage;
        NetworkOptions network;
        IOOptions io;
    } options;
} ComponentConfig;

typedef struct
{
    ComponentConfig *components;
    int component_count;
    char log_directory[256];
    char file_name_base[256];
    char file_format[16];
} TestConfig;

// Function prototypes
bool parse_command_line(const char *cmd_line, TestConfig *config);
bool parse_component(const char *component_str, ComponentConfig *comp);
bool parse_options(const char *options_str, ComponentConfig *comp);
bool parse_global_option(const char *option_str, TestConfig *config);
void free_config(TestConfig *config);
void print_config(const TestConfig *config);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <component-configs>-<global-options>\n", argv[0]);
        fprintf(stderr, "Example: ./crucible *1c[t:stress-d600]*2m[t:baseline-d300]*D[/path/to/dir]*N[results]*F[JSON]\n");
        return 1;
    }

    TestConfig config = {0};

    if (!parse_command_line(argv[1], &config))
    {
        fprintf(stderr, "Error parsing command line\n");
        free_config(&config);
        return 1;
    }

    printf("Successfully parsed configuration:\n");
    print_config(&config);

    // TODO: Execute tests here

    free_config(&config);
    return 0;
}

bool parse_command_line(const char *cmd_line, TestConfig *config)
{
    char *input = strdup(cmd_line);
    if (!input)
        return false;

    char *ptr = input;
    int component_capacity = 10;
    config->components = malloc(sizeof(ComponentConfig) * component_capacity);
    if (!config->components)
    {
        free(input);
        return false;
    }

    // Default values
    strcpy(config->log_directory, ".");
    strcpy(config->file_name_base, "crucible_results");
    strcpy(config->file_format, "JSON");

    while (*ptr)
    {
        if (*ptr == '*')
        {
            ptr++; // Skip the asterisk

            // Find the end of this component/option
            char *end = strchr(ptr, '*');
            if (!end)
                end = ptr + strlen(ptr);

            char component_str[256];
            int len = end - ptr;
            if (len >= sizeof(component_str))
            {
                fprintf(stderr, "Component string too long\n");
                free(input);
                return false;
            }
            strncpy(component_str, ptr, len);
            component_str[len] = '\0';

            // Check if it's a component or global option
            if (isdigit(component_str[0]))
            {
                // It's a component
                if (config->component_count >= component_capacity)
                {
                    component_capacity *= 2;
                    ComponentConfig *new_components = realloc(config->components,
                                                              sizeof(ComponentConfig) * component_capacity);
                    if (!new_components)
                    {
                        free(input);
                        return false;
                    }
                    config->components = new_components;
                }

                if (!parse_component(component_str, &config->components[config->component_count]))
                {
                    fprintf(stderr, "Error parsing component: %s\n", component_str);
                    free(input);
                    return false;
                }
                config->component_count++;
            }
            else
            {
                // It's a global option
                if (!parse_global_option(component_str, config))
                {
                    fprintf(stderr, "Error parsing global option: %s\n", component_str);
                    free(input);
                    return false;
                }
            }

            ptr = end;
        }
        else
        {
            ptr++;
        }
    }

    free(input);
    return true;
}

bool parse_component(const char *component_str, ComponentConfig *comp)
{
    // Initialize component
    memset(comp, 0, sizeof(ComponentConfig));

    // Parse order and component type
    if (!isdigit(component_str[0]))
        return false;
    comp->order = atoi(component_str);

    char *type_pos = component_str;
    while (*type_pos && isdigit(*type_pos))
        type_pos++;
    if (!*type_pos)
        return false;
    comp->component_type = *type_pos;

    // Find the options section
    char *bracket_start = strchr(component_str, '[');
    if (!bracket_start)
        return false;

    char *bracket_end = strchr(bracket_start, ']');
    if (!bracket_end)
        return false;

    // Parse options
    char options_str[256];
    int len = bracket_end - bracket_start - 1;
    if (len >= sizeof(options_str))
        return false;
    strncpy(options_str, bracket_start + 1, len);
    options_str[len] = '\0';

    return parse_options(options_str, comp);
}

bool parse_options(const char *options_str, ComponentConfig *comp)
{
    char *options_copy = strdup(options_str);
    if (!options_copy)
        return false;

    char *token = strtok(options_copy, "-");

    while (token)
    {
        if (strncmp(token, "t:", 2) == 0)
        {
            char *test_type = token + 2;
            if (strcmp(test_type, "baseline") == 0)
                comp->test_type = PTT_BASELINE;
            else if (strcmp(test_type, "stress") == 0)
                comp->test_type = PTT_STRESS;
            else if (strcmp(test_type, "spike") == 0)
                comp->test_type = PTT_SPIKE;
            else if (strcmp(test_type, "load") == 0)
                comp->test_type = PTT_LOAD;
            else
            {
                free(options_copy);
                return false;
            }
        }
        else if (token[0] == 'd' && isdigit(token[1]))
        {
            comp->duration = atoi(token + 1);
        }
        else if (strncmp(token, "{", 1) == 0)
        {
            // Parse component-specific options
            char *end_brace = strchr(token, '}');
            if (!end_brace)
            {
                free(options_copy);
                return false;
            }

            char suboptions[256];
            int len = end_brace - token - 1;
            if (len >= sizeof(suboptions))
            {
                free(options_copy);
                return false;
            }
            strncpy(suboptions, token + 1, len);
            suboptions[len] = '\0';

            // Parse component-specific suboptions
            char *subtoken = strtok(suboptions, "-");
            while (subtoken)
            {
                switch (comp->component_type)
                {
                case 'c': // CPU
                    if (strncmp(subtoken, "cr:", 3) == 0)
                    {
                        // Parse core list
                        char *core_list = subtoken + 3;
                        char *core_token = strtok(core_list, ",");
                        int core_count = 0;

                        // Count cores first
                        char *temp_list = strdup(core_list);
                        char *temp_token = strtok(temp_list, ",");
                        while (temp_token)
                        {
                            core_count++;
                            temp_token = strtok(NULL, ",");
                        }
                        free(temp_list);

                        comp->options.cpu.cores = malloc(sizeof(int) * core_count);
                        comp->options.cpu.core_count = core_count;

                        // Now parse the actual cores
                        core_list = strdup(subtoken + 3);
                        core_token = strtok(core_list, ",");
                        int i = 0;
                        while (core_token && i < core_count)
                        {
                            comp->options.cpu.cores[i++] = atoi(core_token);
                            core_token = strtok(NULL, ",");
                        }
                        free(core_list);
                    }
                    else if (strncmp(subtoken, "f:", 2) == 0)
                    {
                        char *freq_range = subtoken + 2;
                        char *comma = strchr(freq_range, ',');
                        if (comma)
                        {
                            strncpy(comp->options.cpu.freq_min, freq_range, comma - freq_range);
                            comp->options.cpu.freq_min[comma - freq_range] = '\0';
                            strcpy(comp->options.cpu.freq_max, comma + 1);
                        }
                    }
                    else if (strncmp(subtoken, "w:", 2) == 0)
                    {
                        strcpy(comp->options.cpu.workload_type, subtoken + 2);
                    }
                    else if (strncmp(subtoken, "th:", 3) == 0)
                    {
                        comp->options.cpu.threads_per_core = atoi(subtoken + 3);
                    }
                    else if (strncmp(subtoken, "tt:", 3) == 0)
                    {
                        comp->options.cpu.test_thermal = (strcmp(subtoken + 3, "true") == 0);
                    }
                    break;

                case 'm': // Memory
                    if (strncmp(subtoken, "sz:", 3) == 0)
                    {
                        strcpy(comp->options.memory.size, subtoken + 3);
                    }
                    else if (strncmp(subtoken, "p:", 2) == 0)
                    {
                        strcpy(comp->options.memory.pattern, subtoken + 2);
                    }
                    else if (strncmp(subtoken, "a:", 2) == 0)
                    {
                        strcpy(comp->options.memory.alloc_size, subtoken + 2);
                    }
                    break;

                // Add cases for other component types...
                default:
                    break;
                }

                subtoken = strtok(NULL, "-");
            }
        }

        token = strtok(NULL, "-");
    }

    free(options_copy);
    return true;
}

bool parse_global_option(const char *option_str, TestConfig *config)
{
    if (option_str[0] == 'D' && option_str[1] == '[')
    {
        char *end = strchr(option_str, ']');
        if (!end)
            return false;

        int len = end - option_str - 2;
        if (len >= sizeof(config->log_directory))
            return false;
        strncpy(config->log_directory, option_str + 2, len);
        config->log_directory[len] = '\0';
    }
    else if (option_str[0] == 'N' && option_str[1] == '[')
    {
        char *end = strchr(option_str, ']');
        if (!end)
            return false;

        int len = end - option_str - 2;
        if (len >= sizeof(config->file_name_base))
            return false;
        strncpy(config->file_name_base, option_str + 2, len);
        config->file_name_base[len] = '\0';
    }
    else if (option_str[0] == 'F' && option_str[1] == '[')
    {
        char *end = strchr(option_str, ']');
        if (!end)
            return false;

        int len = end - option_str - 2;
        if (len >= sizeof(config->file_format))
            return false;
        strncpy(config->file_format, option_str + 2, len);
        config->file_format[len] = '\0';
    }
    else
    {
        return false;
    }

    return true;
}

void free_config(TestConfig *config)
{
    if (config->components)
    {
        for (int i = 0; i < config->component_count; i++)
        {
            if (config->components[i].component_type == 'c' &&
                config->components[i].options.cpu.cores)
            {
                free(config->components[i].options.cpu.cores);
            }
        }
        free(config->components);
    }
}

void print_config(const TestConfig *config)
{
    printf("Test Configuration:\n");
    printf("Log Directory: %s\n", config->log_directory);
    printf("File Name Base: %s\n", config->file_name_base);
    printf("File Format: %s\n", config->file_format);
    printf("\nComponents (%d):\n", config->component_count);

    for (int i = 0; i < config->component_count; i++)
    {
        ComponentConfig *comp = &config->components[i];
        printf("  [%d] Order: %d, Type: %c, Duration: %d seconds\n",
               i + 1, comp->order, comp->component_type, comp->duration);
        printf("      Test Type: ");
        switch (comp->test_type)
        {
        case PTT_BASELINE:
            printf("Baseline\n");
            break;
        case PTT_STRESS:
            printf("Stress\n");
            break;
        case PTT_SPIKE:
            printf("Spike\n");
            break;
        case PTT_LOAD:
            printf("Load\n");
            break;
        }

        if (comp->component_type == 'c')
        {
            printf("      CPU Options: cores=");
            for (int j = 0; j < comp->options.cpu.core_count; j++)
            {
                printf("%d", comp->options.cpu.cores[j]);
                if (j < comp->options.cpu.core_count - 1)
                    printf(",");
            }
            printf(", freq=%s-%s, workload=%s\n",
                   comp->options.cpu.freq_min, comp->options.cpu.freq_max,
                   comp->options.cpu.workload_type);
        }
        // Add printing for other component types...
    }
}

// gcc -o crucible crucible.c
// ./crucible '*1c[t:stress-d600-{cr:1,2,3-f:min,max-w:avx}]*2m[t:baseline-d300-{sz:2g-p:seq-a:4k}]*D[/path/to/dir]*N[results]*F[JSON]'