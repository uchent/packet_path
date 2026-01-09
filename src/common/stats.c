#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../../include/common.h"

void stats_init(stats_t *stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(stats_t));
    pthread_mutex_init(&stats->mutex, NULL);
}

void stats_update(stats_t *stats, uint32_t packet_size) {
    if (!stats) return;
    
    pthread_mutex_lock(&stats->mutex);
    
    stats->packets_received++;
    stats->bytes_received += packet_size;

    pthread_mutex_unlock(&stats->mutex);
}

void stats_update_dropped(stats_t *stats, uint32_t dropped_count) {
    if (!stats) return;
    
    pthread_mutex_lock(&stats->mutex);
    stats->packets_dropped += dropped_count;
    pthread_mutex_unlock(&stats->mutex);
}

void stats_summarize(stats_t *stats) {
    if (!stats) return;
    
    pthread_mutex_lock(&stats->mutex);
    
    uint64_t runtime_ns = stats->end_time_ns - stats->start_time_ns;
    double runtime_sec = runtime_ns / 1e9;
    if (runtime_sec > 0) {
        stats->pps = stats->packets_received / runtime_sec;
        stats->bps = (stats->bytes_received * 8.0) / runtime_sec;
    }
    
    printf("\n========== Statistics ==========\n");
    printf("Run time: %.2f seconds\n", runtime_sec);
    printf("Packets received: %lu\n", stats->packets_received);
    printf("Bytes received: %lu (%.2f MB)\n", 
           stats->bytes_received, stats->bytes_received / (1024.0 * 1024.0));
    printf("Packets dropped: %lu\n", stats->packets_dropped);
    printf("Packet rate: %.2f PPS\n", stats->pps);
    printf("Bit rate: %.2f Mbps\n", stats->bps / 1e6);
    printf("Memory copy count: %lu\n", stats->copy_count);
    if (stats->packets_received > 0) {
        printf("Average copies per packet: %.2f\n", 
               (double)stats->copy_count / stats->packets_received);
    }
    printf("=============================\n");
    
    pthread_mutex_unlock(&stats->mutex);
}

void stats_cleanup(stats_t *stats) {
    if (!stats) return;
    pthread_mutex_destroy(&stats->mutex);
}

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║   Packet Datapath - Performance Test Tool ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
}

int parse_args(int argc, char *argv[], config_t *config) {
    // Default configuration
    config->mode = MODE_SOCKET;
    strncpy(config->interface, "eth0", sizeof(config->interface) - 1);
    config->promiscuous = true;
    config->buffer_size = 65536;
    config->timeout_ms = 1000;
    config->verbose = false;
    config->duration_sec = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "socket") == 0) {
                config->mode = MODE_SOCKET;
            } else if (strcmp(argv[i + 1], "af_xdp") == 0) {
                config->mode = MODE_AF_XDP;
            } else if (strcmp(argv[i + 1], "dpdk") == 0) {
                config->mode = MODE_DPDK;
            }
            i++;
        } else if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            strncpy(config->interface, argv[i + 1], sizeof(config->interface) - 1);
            i++;
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            config->duration_sec = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            config->verbose = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mode <socket|af_xdp|dpdk>  Receive mode (default: socket)\n");
            printf("  --interface <name>           Network interface (default: eth0)\n");
            printf("  --duration <seconds>         Runtime duration (0=infinite, default: 0)\n");
            printf("  --verbose, -v                Verbose output\n");
            printf("  --help, -h                   Show this help\n");
            return 1;
        }
    }
    
    return 0;
}

