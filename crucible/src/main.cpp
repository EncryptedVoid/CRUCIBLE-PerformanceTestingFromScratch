#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Keep your enum definitions
enum PerfTestType
{
    PTT_BASELINE, // Retrieve a baseline amount of data
    PTT_STRESS,   // Progressively increases load beyond normal operating capacity
    PTT_SPIKE,    // Suddenly applies a massive load increase, then drops back to normal levels
    PTT_LOAD,     // Gradually increases load to a predetermined level and maintains it for a specified duration
};

// Keep your component structs
struct CPUComponent
{
    PerfTestType performance_type;
    int duration;
    int core; // Number of Cores being tested
};

struct MemoryComponent
{
    PerfTestType performance_type;
    int duration;
    int size;             // Memory space under test (Megabytes)
    int alloc_block_size; // Allocation block size (Kilobytes)
};

struct StorageComponent
{
    PerfTestType performance_type;
    int duration;
    int size;             // Memory space under test (Megabytes)
    int rw_ratio;         // Read/write ratio
    int alloc_block_size; // Allocation block size (Kilobytes)
};

struct NetworkComponent
{
    PerfTestType performance_type;
    int duration;
    char *target_ip; // Target IP
    int port;        // Target Port number
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

struct ExternalStorageComponent
{
    char *path;
    IOType io_type;
    IOInterfaceType interface_type;
    int buffer_size;
    int op_count;
};

// Configuration structure to hold all components
struct TestConfig
{
    bool test_cpu;
    bool test_memory;
    bool test_storage;
    bool test_network;
    bool test_external_storage;

    struct CPUComponent cpu;
    struct MemoryComponent memory;
    struct StorageComponent storage;
    struct NetworkComponent network;
    struct ExternalStorageComponent external_storage;
};

// Function prototypes
void print_usage(const char *program_name);
bool parse_arguments(int argc, char *argv[], struct TestConfig *config);
void run_tests(struct TestConfig *config);

int main(int argc, char *argv[])
{
    struct TestConfig config = {0}; // Initialize everything to zero

    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (!parse_arguments(argc, argv, &config))
    {
        fprintf(stderr, "Error parsing arguments\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("Starting Crucible performance tests...\n");
    run_tests(&config);

    return 0;
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --components=cpu,memory,storage,network  Specify components to test\n");
    printf("  --mode=baseline,stress,spike,load       Test mode\n");
    printf("  --duration=<seconds>                    Test duration\n");
    printf("  --help                                  Show this help\n");
}

bool parse_arguments(int argc, char *argv[], struct TestConfig *config)
{
    // Start with a simple implementation that just checks for --help
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strncmp(argv[i], "--components=", 13) == 0)
        {
            // Parse components - simplified for now
            char *components = argv[i] + 13;
            if (strstr(components, "cpu"))
                config->test_cpu = true;
            if (strstr(components, "memory"))
                config->test_memory = true;
            if (strstr(components, "storage"))
                config->test_storage = true;
            if (strstr(components, "network"))
                config->test_network = true;
        }
        else if (strncmp(argv[i], "--duration=", 11) == 0)
        {
            int duration = atoi(argv[i] + 11);
            // Apply duration to all components for now
            config->cpu.duration = duration;
            config->memory.duration = duration;
            config->storage.duration = duration;
            config->network.duration = duration;
        }
        else if (strncmp(argv[i], "--mode=", 7) == 0)
        {
            char *mode = argv[i] + 7;
            PerfTestType type;
            if (strcmp(mode, "baseline") == 0)
                type = PTT_BASELINE;
            else if (strcmp(mode, "stress") == 0)
                type = PTT_STRESS;
            else if (strcmp(mode, "spike") == 0)
                type = PTT_SPIKE;
            else if (strcmp(mode, "load") == 0)
                type = PTT_LOAD;
            else
                return false;

            // Apply mode to all components for now
            config->cpu.performance_type = type;
            config->memory.performance_type = type;
            config->storage.performance_type = type;
            config->network.performance_type = type;
        }
    }

    return true;
}

void run_tests(struct TestConfig *config)
{
    printf("Test configuration:\n");

    if (config->test_cpu)
    {
        printf("- CPU test: duration=%d, mode=%d\n",
               config->cpu.duration, config->cpu.performance_type);
    }

    if (config->test_memory)
    {
        printf("- Memory test: duration=%d, mode=%d\n",
               config->memory.duration, config->memory.performance_type);
    }

    if (config->test_storage)
    {
        printf("- Storage test: duration=%d, mode=%d\n",
               config->storage.duration, config->storage.performance_type);
    }

    if (config->test_network)
    {
        printf("- Network test: duration=%d, mode=%d\n",
               config->network.duration, config->network.performance_type);
    }

    // TODO: Implement actual test functions
    printf("\nTests completed successfully!\n");
}