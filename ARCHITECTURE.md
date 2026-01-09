# Project Architecture Documentation

## Overall Architecture

This project uses a modular design, supporting three different packet reception methods, and provides a unified interface and statistics framework.

```
┌─────────────────────────────────────────┐
│         Main Program (main.c)           │
│  - Argument parsing                      │
│  - Receiver creation and management      │
│  - Signal handling                       │
└─────────────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        │           │           │
┌───────▼──────┐ ┌──▼──────┐ ┌─▼────────┐
│ Socket       │ │ AF_XDP  │ │ DPDK     │
│ Receiver     │ │Receiver │ │Receiver  │
└───────┬──────┘ └──┬──────┘ └─┬────────┘
        │           │           │
        └───────────┼───────────┘
                    │
        ┌───────────▼───────────┐
        │   Common Module       │
        │  - Statistics         │
        │  - Utilities          │
        │  - Configuration      │
        └───────────────────────┘
```

## Module Description

### 1. Socket Reception Module (`src/socket/`)

**Features:**
- Uses traditional RAW socket or libpcap
- Packets copied from kernel space to user space (1 copy)
- Simple implementation, good compatibility
- Suitable for low-rate scenarios

**Copy Count:** 1 (kernel → user space)

**Performance:** Medium (affected by kernel processing overhead)

### 2. AF_XDP Reception Module (`src/af_xdp/`)

**Features:**
- Uses XDP (eXpress Data Path) and AF_XDP socket
- Zero-copy or minimal-copy mode
- Requires Linux 4.18+ kernel
- Requires libbpf support

**Copy Count:** 0 (shared memory)

**Performance:** High (bypasses kernel protocol stack)

### 3. DPDK Reception Module (`src/dpdk/`)

**Features:**
- Uses DPDK userspace driver
- Completely bypasses kernel network stack
- Requires binding network card to DPDK driver
- Suitable for extremely high-performance scenarios

**Copy Count:** 0-1 (depends on configuration)

**Performance:** Very High (dedicated for high-performance networking)

## Design Pattern

### Strategy Pattern

Each reception module implements a unified `receiver_ops_t` interface:

```c
typedef struct {
    int (*init)(packet_receiver_t *receiver, const config_t *config);
    int (*start)(packet_receiver_t *receiver);
    int (*stop)(packet_receiver_t *receiver);
    void (*cleanup)(packet_receiver_t *receiver);
} receiver_ops_t;
```

Benefits of this design:
- Easy to extend new reception methods
- Unified interface facilitates testing and comparison
- Modular code, easy to maintain

## Statistics Framework

Unified statistics structure `stats_t` tracks:
- Packet count and byte count
- Dropped packet count
- **Memory copy count** (core metric)
- Throughput (PPS, BPS)
- Runtime

## Build System

Makefile supports:
- Compile modules separately
- Conditional compilation (optional DPDK)
- Dependency checking
- Clean functionality

## Testing Framework

`scripts/run_pktgen_test.sh` provides:
- Automated testing workflow
- Multi-mode testing
- Result collection

## Extension Suggestions

### 1. Add New Reception Method

1. Create new directory under `src/` (e.g., `src/pf_ring/`)
2. Implement `receiver_ops_t` interface
3. Add creation function in `main.c`
4. Update Makefile

### 2. Enhance Statistics

- Add latency measurement (requires timestamps)
- Add CPU usage monitoring
- Add memory usage monitoring
- Export JSON/CSV format results

### 3. Performance Optimization

- Use multi-threading/multi-processing
- Batch packet processing
- CPU affinity settings
- NUMA awareness

### 4. Testing Enhancements

- Automated benchmark testing
- Stress test scripts
- Result comparison analysis
- Chart generation

## Notes

1. **Permission Requirements**: All modes require root privileges
2. **Kernel Version**: AF_XDP requires Linux 4.18+
3. **Network Card Binding**: DPDK requires binding network card to DPDK driver
4. **Environment Isolation**: Recommended to run in dedicated test environment
5. **Resource Limits**: High-performance modes may affect other system components

## Performance Expectations

Based on general test results (for reference only):

| Mode    | Max PPS      | Copy Count | CPU Usage |
|---------|---------------|------------|-----------|
| Socket  | ~1-2M PPS     | 1          | High      |
| AF_XDP  | ~10-20M PPS   | 0          | Medium    |
| DPDK    | ~50-100M PPS+ | 0-1        | Low       |

*Actual performance depends on hardware, network card, packet size, and other factors*
