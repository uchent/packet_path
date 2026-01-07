# Project Summary

## Project Overview

This is a Linux C project for comparing the performance and memory copy count of different packet reception methods (Socket, AF_XDP, DPDK). The project uses a modular design and provides a unified interface and statistics framework.

## Project Structure

```
packet_datapath/
├── include/                    # Header files
│   ├── common.h               # Common definitions and statistics structure
│   └── packet_receiver.h      # Receiver interface definition
│
├── src/                        # Source code
│   ├── common/                # Common module
│   │   └── stats.c            # Statistics implementation
│   ├── socket/                 # Socket reception module
│   │   ├── socket_receiver.c  # Socket implementation
│   │   └── socket_receiver.h  # Socket header file
│   ├── af_xdp/                 # AF_XDP reception module
│   │   ├── af_xdp_receiver.c  # AF_XDP implementation
│   │   └── af_xdp_receiver.h  # AF_XDP header file
│   ├── dpdk/                   # DPDK reception module
│   │   ├── dpdk_receiver.c    # DPDK implementation (framework)
│   │   └── dpdk_receiver.h    # DPDK header file
│   └── main.c                  # Main program
│
├── scripts/                    # Scripts
│   └── run_pktgen_test.sh     # Pktgen test script
│
├── Makefile                    # Build configuration
├── README.md                   # Project description
├── ARCHITECTURE.md             # Architecture documentation
├── TIPS.md                     # Development suggestions
├── QUICKSTART.md               # Quick start guide
└── .gitignore                  # Git ignore file
```

## Core Features

### 1. Three Reception Modes

- **Socket Mode**: Traditional RAW socket, 1 memory copy
- **AF_XDP Mode**: XDP zero-copy reception, 0 memory copies
- **DPDK Mode**: Userspace driver, 0-1 memory copies

### 2. Unified Interface

All reception modes implement the same `receiver_ops_t` interface:
- `init()`: Initialize
- `start()`: Start reception
- `stop()`: Stop reception
- `cleanup()`: Cleanup resources
- `get_stats()`: Get statistics

### 3. Statistics Framework

Tracks the following metrics:
- Packet count and byte count
- Dropped packet count
- **Memory copy count** (core metric)
- Throughput (PPS, BPS)
- Runtime

## Design Features

### 1. Modular Design
- Each reception mode implemented independently
- Easy to add new reception methods
- Clear code, easy to maintain

### 2. Strategy Pattern
- Unified interface design
- Runtime selection of reception mode
- Convenient for testing and comparison

### 3. Extensibility
- Easy to add new features
- Statistics framework is extensible
- Testing framework is extensible

## Use Cases

1. **Performance Research**: Compare performance differences of different reception methods
2. **Learning Purpose**: Understand Linux network programming and high-performance networking
3. **Benchmark Testing**: Serve as a benchmark testing tool for network reception performance
4. **Optimization Reference**: Understand the impact of memory copies on performance

## Technology Stack

- **Language**: C11
- **Platform**: Linux
- **Dependencies**: 
  - libpcap (Socket mode)
  - libbpf (AF_XDP mode)
  - DPDK (DPDK mode, optional)

## Development Status

- ✅ Project architecture design completed
- ✅ Socket mode implementation completed
- ✅ AF_XDP mode framework completed (requires full libbpf implementation)
- ✅ DPDK mode framework completed (requires full DPDK implementation)
- ✅ Statistics framework completed
- ✅ Build system completed
- ✅ Documentation completed

## Future Development Suggestions

### Short-term Goals
1. Complete AF_XDP implementation (full libbpf/XDP support)
2. Complete DPDK implementation (full DPDK integration)
3. Add unit tests
4. Optimize statistics output format

### Medium-term Goals
1. Add packet parsing functionality
2. Add filtering functionality (BPF filter)
3. Add multi-threading support
4. Add result export (JSON/CSV)

### Long-term Goals
1. Add graphical interface
2. Add real-time monitoring
3. Add automated test suite
4. Performance optimization

## Notes

1. **Permission Requirements**: All modes require root privileges
2. **Kernel Version**: AF_XDP requires Linux 4.18+
3. **Environment Requirements**: DPDK requires special environment configuration
4. **Test Environment**: Recommended to run in dedicated test environment

## License

MIT License

## Contributing

Welcome to submit Issues and Pull Requests!
