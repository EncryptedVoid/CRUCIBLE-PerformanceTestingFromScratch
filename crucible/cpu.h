#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <pthread.h>
#include <math.h>

#include "config.h"

// CPU usage tracking
typedef struct
{
    unsigned long total;
    unsigned long idle;
} CpuUsage;

// CPU information
typedef struct
{
    int cores;
    int threads;
} CpuInfo;

// Function to run all CPU tests
bool run_cpu_tests(TestConfig *config);

#endif /* CPU_H */