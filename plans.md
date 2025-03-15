# Performance Test Tool Implementation Details

## File Structure and Implementation

### main.c
- Command-line argument parsing using `getopt_long()`
- Configuration loading and validation
- High-level test orchestration
- Exit code handling for Jenkins integration

### config.h/c
- Struct definitions for test configuration
- Command-line parsing helpers
- Configuration validation
- Default values management

### logger.h/c
- File-based logging implementation
- Timestamp functions using `time()`
- Log rotation if needed
- Structured log format (CSV or JSON)
- Directory creation/validation

### diagnostics.h/c
- System probing for baseline metrics
- Library/function availability checking
- Short test runs with statistical validation
- Environment validation (temperature, load, etc.)

### cpu.h/c
- Temperature reading via `libsensors`
- Thread/core utilization via `/proc/stat` parsing
- CPU stress testing (computational loops)
- Idle/runtime measurements

### memory.h/c
- Memory information via `sysinfo()`
- Memory allocation/deallocation benchmarking
- Memory stress testing (allocation patterns)
- Speed measurements with timing functions

### storage.h/c
- Disk read/write operations
- Storage information via `statvfs()`
- I/O stress patterns
- Consistency testing logic

### network.h/c
- Socket operations for throughput testing
- Connection establishment timing
- Network stress testing (data transfer)
- Packet analysis if needed

### io.h/c
- USB device detection
- External storage benchmarking
- I/O stress patterns
- Device path management

## Build System

### Makefile
- Compilation rules
- Library linking
- Cross-compilation support for Raspberry Pi 4 AARCH64
- Installation targets

## Key Data Structures

```c
typedef struct {
    bool cpu_enabled;
    bool memory_enabled;
    bool storage_enabled;
    bool network_enabled;
    bool io_enabled;

    enum TestMode {
        BASELINE,
        STRESS,
        LOAD,
        SPIKE,
        ALL
    } mode;

    int duration_seconds;
    int sample_frequency_seconds;
    int diagnostics_duration_seconds;

    char log_directory[PATH_MAX];
} TestConfig;

typedef struct {
    double value;
    char unit[16];
    time_t timestamp;
    bool is_valid;
} MetricReading;
```

## Required Libraries

- Standard C libraries
- `libsensors` for hardware monitoring
- Socket libraries for network testing
- POSIX file I/O