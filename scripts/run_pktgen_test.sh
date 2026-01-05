#!/bin/bash

# Pktgen Test Script

set -e

# Default configuration
MODE="socket"
INTERFACE="eth0"
DURATION=10
PKTGEN_RATE="100%"
PKT_SIZE=64

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --mode)
            MODE="$2"
            shift 2
            ;;
        --interface)
            INTERFACE="$2"
            shift 2
            ;;
        --duration)
            DURATION="$2"
            shift 2
            ;;
        --rate)
            PKTGEN_RATE="$2"
            shift 2
            ;;
        --pkt-size)
            PKT_SIZE="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --mode <socket|af_xdp|dpdk>  Receive mode"
            echo "  --interface <name>           Network interface"
            echo "  --duration <seconds>         Test duration"
            echo "  --rate <rate>                Pktgen send rate (e.g.: 100%, 10G)"
            echo "  --pkt-size <bytes>           Packet size"
            exit 0
            ;;
        *)
            echo "Unknown parameter: $1"
            exit 1
            ;;
    esac
done

echo "=========================================="
echo "Pktgen Performance Test"
echo "=========================================="
echo "Mode: $MODE"
echo "Interface: $INTERFACE"
echo "Duration: $DURATION seconds"
echo "Rate: $PKTGEN_RATE"
echo "Packet size: $PKT_SIZE bytes"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Error: Root privileges required"
    exit 1
fi

# Check if receiver program exists
if [ ! -f "./bin/packet_receiver" ]; then
    echo "Error: packet_receiver not found, please build first"
    exit 1
fi

# Start receiver (background)
echo "Starting packet receiver..."
sudo ./bin/packet_receiver --mode "$MODE" --interface "$INTERFACE" --duration "$DURATION" > receiver.log 2>&1 &
RECEIVER_PID=$!

sleep 2

# Check if receiver is still running
if ! kill -0 $RECEIVER_PID 2>/dev/null; then
    echo "Error: Receiver failed to start"
    cat receiver.log
    exit 1
fi

echo "Receiver PID: $RECEIVER_PID"

# Use pktgen (if available)
if command -v pktgen &> /dev/null; then
    echo "Using kernel pktgen..."
    # Need to configure based on actual pktgen
    echo "Note: Manual pktgen configuration required"
elif command -v pktgen-dpdk &> /dev/null; then
    echo "Using DPDK pktgen..."
    # DPDK pktgen configuration
    echo "Note: Manual DPDK pktgen configuration required"
else
    echo "Warning: pktgen not found, using alternative test method"
    echo "You can manually send test packets using:"
    echo "  ping -f $INTERFACE"
    echo "  or use other packet generation tools"
fi

# Wait for test to complete
echo "Waiting for test to complete ($DURATION seconds)..."
sleep $DURATION

# Stop receiver
echo "Stopping receiver..."
kill -SIGTERM $RECEIVER_PID 2>/dev/null || true
wait $RECEIVER_PID 2>/dev/null || true

# Display results
echo ""
echo "=========================================="
echo "Test Results:"
echo "=========================================="
if [ -f receiver.log ]; then
    cat receiver.log
fi

echo ""
echo "Test completed"

