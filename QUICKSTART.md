# Quick Start Guide

## 1. Install Dependencies

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    gcc \
    make \
    libpcap-dev \
    libbpf-dev \
    pkg-config
```

### Check Dependencies
```bash
make check-deps
```

## 2. Build Project

### Build All Modules
```bash
make all
```

### Build Socket Mode Only (Simplest)
```bash
make socket
```

### Build AF_XDP Mode Only
```bash
make af_xdp
```

## 3. Run Tests

### Socket Mode Test
```bash
# View available network interfaces
ip link show

# Run receiver (requires root privileges)
sudo ./bin/packet_receiver --mode socket --interface eth0 --duration 10

# Send test packets from another terminal
ping -f <target_IP>
# Or use other tools to send packets
```

### AF_XDP Mode Test
```bash
# Check kernel version (requires 4.18+)
uname -r

# Run receiver
sudo ./bin/packet_receiver --mode af_xdp --interface eth0 --duration 10
```

### View Help
```bash
./bin/packet_receiver --help
```

## 4. Use Pktgen Testing

### Prepare Pktgen Environment

#### Method 1: Use Kernel Pktgen
```bash
# Load module
sudo modprobe pktgen

# Configure (via /proc/net/pktgen/)
# See pktgen documentation for details
```

#### Method 2: Use DPDK Pktgen
```bash
# Download and compile DPDK Pktgen
# Configure and run
```

### Run Automated Test Script
```bash
sudo ./scripts/run_pktgen_test.sh \
    --mode socket \
    --interface eth0 \
    --duration 10 \
    --rate 100%
```

## 5. View Results

After the program finishes, it will automatically display statistics:
- Total packets received
- Packet rate (PPS)
- Bit rate (Mbps)
- **Memory copy count** (core metric)
- Average copies per packet

## 6. Comparison Testing

### Test Script Example
```bash
#!/bin/bash

INTERFACE="eth0"
DURATION=30

echo "Testing Socket mode..."
sudo ./bin/packet_receiver --mode socket --interface $INTERFACE --duration $DURATION > socket.log 2>&1

echo "Testing AF_XDP mode..."
sudo ./bin/packet_receiver --mode af_xdp --interface $INTERFACE --duration $DURATION > af_xdp.log 2>&1

echo "Testing completed, viewing results:"
echo "Socket:"
tail -20 socket.log
echo "AF_XDP:"
tail -20 af_xdp.log
```

## 7. Common Issues

### Build Errors
```bash
# Ensure all dependencies are installed
make check-deps

# Clean and rebuild
make clean
make all
```

### Runtime Permission Errors
```bash
# All modes require root privileges
sudo ./bin/packet_receiver --mode socket --interface eth0
```

### Network Interface Not Found
```bash
# View available network interfaces
ip link show

# Use correct interface name
sudo ./bin/packet_receiver --mode socket --interface <correct_interface_name>
```

## 8. Next Steps

- Read [ARCHITECTURE.md](ARCHITECTURE.md) to understand the architecture design
- Read [TIPS.md](TIPS.md) for development suggestions
- Modify and extend code as needed
- Perform more detailed performance testing
