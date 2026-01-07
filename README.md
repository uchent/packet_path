# Packet Datapath - Packet Reception Performance Test Project

This is a Linux C project for comparing the performance and copy count of different packet reception methods (Socket, AF_XDP, DPDK).

## Project Structure

```
packet_datapath/
├── src/
│   ├── socket/          # Traditional Socket reception implementation
│   ├── af_xdp/          # AF_XDP (XDP) reception implementation
│   ├── dpdk/            # DPDK reception implementation
│   └── common/          # Common utilities and statistics module
├── include/              # Header files
├── tests/                # Test related
├── scripts/              # Test scripts (pktgen configuration, etc.)
├── Makefile              # Build configuration
└── README.md
```

## Features

- **Socket Reception**: Uses traditional RAW socket to receive packets
- **AF_XDP Reception**: Uses XDP (eXpress Data Path) zero-copy reception
- **DPDK Reception**: Uses DPDK userspace driver reception
- **Performance Statistics**: Packet count, throughput, latency, copy count statistics
- **Pktgen Testing**: Integrated pktgen for stress testing

## Dependencies

### Basic Dependencies
- GCC compiler
- Make
- libpcap-dev (for Socket mode)
- libbpf-dev (for AF_XDP)
- DPDK (optional, for DPDK mode)

### Testing Tools
- Pktgen-DPDK or Pktgen (Linux kernel version)

## Build

```bash
# Build all modules
make all

# Build specific module
make socket
make af_xdp
make dpdk

# Clean
make clean
```

## Usage

### Socket Mode
```bash
sudo ./bin/packet_receiver --mode socket --interface eth0
```

### AF_XDP Mode
```bash
sudo ./bin/packet_receiver --mode af_xdp --interface eth0
```

### DPDK Mode
```bash
sudo ./bin/packet_receiver --mode dpdk --interface eth0
```

## Performance Testing

Use pktgen for testing:
```bash
./scripts/run_pktgen_test.sh --mode socket --interface eth0
```

## Statistics Output

The program will output the following statistics:
- Total packets received
- Packets per second (PPS)
- Bits per second (BPS)
- Average latency
- Memory copy count
- CPU usage

## Notes

- All modes require root privileges to run
- AF_XDP requires Linux 4.18+ kernel
- DPDK requires binding network card to DPDK driver
- Recommended to run in a dedicated test environment

## License

MIT License
