# Crucible

<p align="center">
  <img src="docs/logo.png" alt="Crucible Logo" width="200" height="200"/>
  <br>
  <em>Comprehensive System Performance Testing</em>
</p>

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Version](https://img.shields.io/badge/version-0.1.0-orange)]()

## Overview

Crucible is a cross-platform, C-based performance testing tool designed for automated system benchmarking and stress testing. It provides comprehensive metrics across CPU, memory, storage, network, and I/O components with detailed logging for analysis.

Perfect for:
- CI/CD pipelines
- Hardware qualification
- System stability validation
- Performance regression testing
- Long-term reliability testing

## Features

- **Cross-Platform**: Compile and run on any architecture
- **Multiple Test Components**:
  - CPU (temperature, thread/core usage, idle time)
  - Memory (usage, speed)
  - Storage (read/write times, consistency)
  - Network (upload/download speeds)
  - I/O (device read/write speeds)
- **Flexible Test Modes**:
  - Baseline (normal operation)
  - Stress (maximum load)
  - Load (consistent pressure)
  - Spike (intermittent high pressure)
- **Automation Ready**:
  - Jenkins integration
  - Configurable through command line
  - Structured logging
- **Pre-Test Diagnostics**:
  - System health validation
  - Probe verification
  - Hardware capability detection

## Installation

### Prerequisites

- GCC or compatible C compiler
- libsensors-dev (for hardware monitoring)
- pthread development libraries
- make

### Compilation

```bash
git clone https://github.com/yourusername/crucible.git
cd crucible
make
```

For cross-compilation to Raspberry Pi 4 (AARCH64):

```bash
make TARGET=arm64
```

## Usage

Basic usage:

```bash
./crucible --components=cpu,memory,storage --mode=stress --duration=3600 --log-dir=/path/to/logs
```

Full options:

```
Usage: crucible [OPTIONS]

Options:
  --components=LIST     Comma-separated list of components to test
                        (cpu,memory,storage,network,io,all)
  --mode=MODE           Test mode (baseline,stress,load,spike,all)
  --duration=SECONDS    Test duration in seconds
  --sample-freq=SECONDS Sampling frequency in seconds
  --diagnostics=SECONDS Diagnostic duration in seconds (0 to skip)
  --log-dir=PATH        Directory for log output
  --help                Display this help message
  --version             Display version information
```

### Jenkins Integration

Example Jenkins pipeline stage:

```groovy
stage('Performance Testing') {
    steps {
        sh 'scp ./crucible user@target:/tmp/'
        sh 'ssh user@target "cd /tmp && ./crucible --components=all --mode=stress --duration=1800 --log-dir=/tmp/logs"'
        sh 'scp -r user@target:/tmp/logs ./performance_results'
    }
    post {
        always {
            archiveArtifacts artifacts: 'performance_results/**/*', allowEmptyArchive: true
        }
    }
}
```

## Log Format

Crucible produces structured logs with timestamps and categorized metrics:

```
[2023-06-15 14:32:45] [INFO] Starting CPU tests
[2023-06-15 14:32:45] [DATA] CPU,Usage,23.45,%
[2023-06-15 14:32:45] [DATA] CPU,Temperature,42.31,C
[2023-06-15 14:32:50] [DATA] CPU,Usage,78.92,%
[2023-06-15 14:32:50] [DATA] CPU,Temperature,56.87,C
```

## Architecture

Crucible is designed with modularity in mind:

```
crucible/
├── src/
│   ├── main.c           # Entry point, argument parsing
│   ├── config.c/h       # Configuration handling
│   ├── logger.c/h       # Logging functionality
│   ├── diagnostics.c/h  # System diagnostics
│   ├── cpu.c/h          # CPU testing
│   ├── memory.c/h       # Memory testing
│   ├── storage.c/h      # Storage testing
│   ├── network.c/h      # Network testing
│   └── io.c/h           # I/O testing
├── include/             # Public headers
└── Makefile             # Build system
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the GNU General Public License v3.0 - see the LICENSE file for details. This license ensures that all derivative works remain open source, protecting the project's commitment to collaborative development.

## Acknowledgments

* Inspired by various benchmarking tools like stress-ng and sysbench
* Special thanks to all contributors