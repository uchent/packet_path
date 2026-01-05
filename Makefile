# Packet Path Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11
LDFLAGS = 

# Directory definitions
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = obj

# Include paths
INCLUDES = -I$(INCLUDE_DIR) -I$(SRC_DIR)/common

# Library paths and linking
LIBS = -lpthread -lm

# Socket mode dependencies
SOCKET_LIBS = -lpcap

# AF_XDP mode dependencies
AF_XDP_LIBS = -lbpf -lelf

# DPDK mode dependencies (adjust based on actual DPDK installation path)
DPDK_DIR ?= /usr/local/dpdk
DPDK_LIBS = -L$(DPDK_DIR)/lib -lrte_eal -lrte_mempool -lrte_ring \
            -lrte_ethdev -lrte_mbuf -lrte_pmd_virtio -lrte_pmd_ixgbe \
            -lrte_pmd_i40e -lrte_pmd_e1000 -lrte_pmd_ring

# Source files
COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.c)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/common/%.c=$(OBJ_DIR)/common/%.o)

SOCKET_SRCS = $(wildcard $(SRC_DIR)/socket/*.c)
SOCKET_OBJS = $(SOCKET_SRCS:$(SRC_DIR)/socket/%.c=$(OBJ_DIR)/socket/%.o)

AF_XDP_SRCS = $(wildcard $(SRC_DIR)/af_xdp/*.c)
AF_XDP_OBJS = $(AF_XDP_SRCS:$(SRC_DIR)/af_xdp/%.c=$(OBJ_DIR)/af_xdp/%.o)

DPDK_SRCS = $(wildcard $(SRC_DIR)/dpdk/*.c)
DPDK_OBJS = $(DPDK_SRCS:$(SRC_DIR)/dpdk/%.c=$(OBJ_DIR)/dpdk/%.o)

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(OBJ_DIR)/main.o

# Target executable
TARGET = $(BIN_DIR)/packet_receiver

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/common $(OBJ_DIR)/socket $(OBJ_DIR)/af_xdp $(OBJ_DIR)/dpdk

# Compile common module
$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile Socket module
$(OBJ_DIR)/socket/%.o: $(SRC_DIR)/socket/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile AF_XDP module
$(OBJ_DIR)/af_xdp/%.o: $(SRC_DIR)/af_xdp/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile DPDK module
$(OBJ_DIR)/dpdk/%.o: $(SRC_DIR)/dpdk/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -I$(DPDK_DIR)/include -c $< -o $@

# Compile main program
$(OBJ_DIR)/main.o: $(MAIN_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link executable
$(TARGET): $(COMMON_OBJS) $(SOCKET_OBJS) $(AF_XDP_OBJS) $(DPDK_OBJS) $(MAIN_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS) $(SOCKET_LIBS) $(AF_XDP_LIBS)

# Compile Socket mode only (simplified version, without DPDK)
socket: directories $(COMMON_OBJS) $(SOCKET_OBJS) $(MAIN_OBJ)
	$(CC) $(LDFLAGS) $(COMMON_OBJS) $(SOCKET_OBJS) $(MAIN_OBJ) -o $(TARGET) $(LIBS) $(SOCKET_LIBS)

# Compile AF_XDP mode only
af_xdp: directories $(COMMON_OBJS) $(AF_XDP_OBJS) $(MAIN_OBJ)
	$(CC) $(LDFLAGS) $(COMMON_OBJS) $(AF_XDP_OBJS) $(MAIN_OBJ) -o $(TARGET) $(LIBS) $(AF_XDP_LIBS)

# Clean
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

# Dependency check (Ubuntu/Debian)
check-deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@pkg-config --exists libpcap || echo "Warning: libpcap not installed (sudo apt-get install libpcap-dev)"
	@pkg-config --exists libbpf || echo "Warning: libbpf not installed (sudo apt-get install libbpf-dev)"
	@echo "Dependency check completed"

.PHONY: all directories clean socket af_xdp check-deps

