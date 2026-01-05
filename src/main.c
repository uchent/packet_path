#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../include/common.h"
#include "../include/packet_receiver.h"

// External receiver creation functions
#include "socket/socket_receiver.h"
#include "af_xdp/af_xdp_receiver.h"
#include "dpdk/dpdk_receiver.h"

static packet_receiver_t *g_receiver = NULL;

// Signal handling
static void signal_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    if (g_receiver) {
        printf("\nInterrupt signal received, stopping...\n");
        packet_receiver_stop(g_receiver);
    }
}

int main(int argc, char *argv[]) {
    config_t config;
    packet_receiver_t *receiver = NULL;
    
    print_banner();
    
    // Parse command line arguments
    if (parse_args(argc, argv, &config) != 0) {
        return 0;  // Exit after showing help
    }
    
    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create receiver based on mode
    switch (config.mode) {
        case MODE_SOCKET:
            receiver = socket_receiver_create();
            break;
        case MODE_AF_XDP:
            receiver = af_xdp_receiver_create();
            break;
        case MODE_DPDK:
            receiver = dpdk_receiver_create();
            break;
        default:
            fprintf(stderr, "Error: Unknown receive mode\n");
            return 1;
    }
    
    if (!receiver) {
        fprintf(stderr, "Error: Failed to create receiver\n");
        return 1;
    }
    
    g_receiver = receiver;
    receiver->config = config;
    
    // Initialize receiver
    if (receiver->ops.init(receiver, &config) != 0) {
        fprintf(stderr, "Error: Receiver initialization failed\n");
        packet_receiver_cleanup(receiver);
        return 1;
    }
    
    // Start reception
    if (receiver->ops.start(receiver) != 0) {
        fprintf(stderr, "Error: Receiver start failed\n");
        packet_receiver_cleanup(receiver);
        return 1;
    }
    
    // Display statistics
    stats_print(receiver->ops.get_stats(receiver));
    
    // Cleanup
    packet_receiver_cleanup(receiver);
    
    printf("Program terminated\n");
    return 0;
}

// Wrapper functions
packet_receiver_t* packet_receiver_create(packet_mode_t mode) {
    switch (mode) {
        case MODE_SOCKET: return socket_receiver_create();
        case MODE_AF_XDP: return af_xdp_receiver_create();
        case MODE_DPDK: return dpdk_receiver_create();
        default: return NULL;
    }
}

void packet_receiver_destroy(packet_receiver_t *receiver) {
    if (receiver) {
        packet_receiver_cleanup(receiver);
        free(receiver);
    }
}

int packet_receiver_init(packet_receiver_t *receiver, const config_t *config) {
    if (!receiver || !receiver->ops.init) return -1;
    return receiver->ops.init(receiver, config);
}

int packet_receiver_start(packet_receiver_t *receiver) {
    if (!receiver || !receiver->ops.start) return -1;
    return receiver->ops.start(receiver);
}

int packet_receiver_stop(packet_receiver_t *receiver) {
    if (!receiver || !receiver->ops.stop) return -1;
    return receiver->ops.stop(receiver);
}

void packet_receiver_cleanup(packet_receiver_t *receiver) {
    if (receiver && receiver->ops.cleanup) {
        receiver->ops.cleanup(receiver);
    }
}

stats_t* packet_receiver_get_stats(packet_receiver_t *receiver) {
    if (!receiver || !receiver->ops.get_stats) return NULL;
    return receiver->ops.get_stats(receiver);
}

