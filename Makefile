# Packet Datapath Makefile

CC = gcc
CFLAGS = -Wno-unused-variable -Wall -Wextra -O2 -g -std=c11
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
AF_XDP_LIBS = -lxdp -lbpf -lelf

# DPDK mode dependencies
DPDK_INCLUDE_DIR ?= /usr/include/dpdk
DPDK_CONFIG_DIR ?= /usr/include/x86_64-linux-gnu/dpdk
DPDK_LIB_DIR ?= /usr/lib/x86_64-linux-gnu
DPDK_LIBS = -L$(DPDK_LIB_DIR) -lrte_eal -lrte_mempool -lrte_ring \
            -lrte_ethdev -lrte_mbuf

# Source files
COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.c)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/common/%.c=$(OBJ_DIR)/common/%.o)

SOCKET_SRCS = $(wildcard $(SRC_DIR)/socket/*.c)
SOCKET_OBJS = $(SOCKET_SRCS:$(SRC_DIR)/socket/%.c=$(OBJ_DIR)/socket/%.o)

AF_XDP_SRCS = $(filter-out %_stub.c, $(wildcard $(SRC_DIR)/af_xdp/*.c))
AF_XDP_OBJS = $(AF_XDP_SRCS:$(SRC_DIR)/af_xdp/%.c=$(OBJ_DIR)/af_xdp/%.o)

DPDK_SRCS = $(filter-out %_stub.c, $(wildcard $(SRC_DIR)/dpdk/*.c))
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
	@if [ ! -d "$(DPDK_INCLUDE_DIR)" ] || [ ! -f "$(DPDK_INCLUDE_DIR)/rte_eal.h" ]; then \
		echo "Warning: DPDK headers not found at $(DPDK_INCLUDE_DIR)"; \
		echo "         DPDK mode will not be available"; \
		echo "         Set DPDK_INCLUDE_DIR in Makefile or environment"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) -msse4.2 $(INCLUDES) -I$(DPDK_INCLUDE_DIR) -I$(DPDK_CONFIG_DIR) -c $< -o $@

# Compile main program
$(OBJ_DIR)/main.o: $(MAIN_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link executable
$(TARGET): $(COMMON_OBJS) $(SOCKET_OBJS) $(AF_XDP_OBJS) $(DPDK_OBJS) $(MAIN_OBJ)
	@if [ -n "$(DPDK_OBJS)" ] && [ "$(shell ls $(DPDK_OBJS) 2>/dev/null | grep -v stub | wc -l)" -gt 0 ] && [ -d "$(DPDK_INCLUDE_DIR)" ]; then \
		$(CC) $(LDFLAGS) $^ -o $@ $(LIBS) $(SOCKET_LIBS) $(AF_XDP_LIBS) $(DPDK_LIBS); \
	else \
		$(CC) $(LDFLAGS) $(COMMON_OBJS) $(SOCKET_OBJS) $(AF_XDP_OBJS) $(MAIN_OBJ) -o $@ $(LIBS) $(SOCKET_LIBS) $(AF_XDP_LIBS); \
	fi

# Compile Socket mode only (simplified version, without DPDK)
SOCKET_STUB_OBJ = $(OBJ_DIR)/af_xdp/af_xdp_receiver_stub.o $(OBJ_DIR)/dpdk/dpdk_receiver_stub.o
socket: directories $(COMMON_OBJS) $(SOCKET_OBJS) $(MAIN_OBJ)
	@mkdir -p $(OBJ_DIR)/af_xdp $(OBJ_DIR)/dpdk
	@$(CC) $(CFLAGS) $(INCLUDES) -c src/af_xdp/af_xdp_receiver_stub.c -o $(OBJ_DIR)/af_xdp/af_xdp_receiver_stub.o 2>/dev/null || true
	@$(CC) $(CFLAGS) $(INCLUDES) -c src/dpdk/dpdk_receiver_stub.c -o $(OBJ_DIR)/dpdk/dpdk_receiver_stub.o 2>/dev/null || true
	$(CC) $(LDFLAGS) $(COMMON_OBJS) $(SOCKET_OBJS) $(SOCKET_STUB_OBJ) $(MAIN_OBJ) -o $(TARGET) $(LIBS) $(SOCKET_LIBS)

# Compile AF_XDP mode only
AF_XDP_STUB_OBJ = $(OBJ_DIR)/socket/socket_receiver_stub.o $(OBJ_DIR)/dpdk/dpdk_receiver_stub.o
af_xdp: directories $(COMMON_OBJS) $(AF_XDP_OBJS) $(MAIN_OBJ)
	@mkdir -p $(OBJ_DIR)/socket $(OBJ_DIR)/dpdk
	@$(CC) $(CFLAGS) $(INCLUDES) -c src/socket/socket_receiver_stub.c -o $(OBJ_DIR)/socket/socket_receiver_stub.o 2>/dev/null || true
	@$(CC) $(CFLAGS) $(INCLUDES) -c src/dpdk/dpdk_receiver_stub.c -o $(OBJ_DIR)/dpdk/dpdk_receiver_stub.o 2>/dev/null || true
	$(CC) $(LDFLAGS) $(COMMON_OBJS) $(AF_XDP_OBJS) $(AF_XDP_STUB_OBJ) $(MAIN_OBJ) -o $(TARGET) $(LIBS) $(AF_XDP_LIBS)

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

