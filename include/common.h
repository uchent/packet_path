#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

// Packet reception mode
typedef enum {
    MODE_SOCKET = 0,
    MODE_AF_XDP,
    MODE_DPDK
} packet_mode_t;

// Statistics structure
typedef struct {
    uint64_t packets_received;      // Total packets received
    uint64_t bytes_received;         // Total bytes received
    uint64_t packets_dropped;        // Packets dropped
    uint64_t copy_count;             // Memory copy count
    uint64_t first_update_time;      // First update time (nanoseconds)
    uint64_t last_update_time;       // Last update time (nanoseconds)
    
    // Rate statistics
    double pps;                      // Packets per second
    double bps;                      // Bits per second
    double avg_latency_ns;           // Average latency (nanoseconds)
    
    pthread_mutex_t mutex;           // Mutex lock
} stats_t;

// Configuration structure
typedef struct {
    packet_mode_t mode;              // Reception mode
    char interface[16];              // Network interface name
    bool promiscuous;                // Promiscuous mode
    uint32_t buffer_size;            // Buffer size
    uint32_t timeout_ms;              // Timeout (milliseconds)
    bool verbose;                    // Verbose output
    uint32_t duration_sec;           // Runtime duration (seconds, 0 means infinite)
} config_t;

// Function declarations
void stats_init(stats_t *stats);
void stats_update(stats_t *stats, uint32_t packet_size);
void stats_update_dropped(stats_t *stats, uint32_t dropped_count);
void stats_summarize(stats_t *stats);
void stats_print(stats_t *stats);
void stats_cleanup(stats_t *stats);

uint64_t get_time_ns(void);
void print_banner(void);
int parse_args(int argc, char *argv[], config_t *config);

#endif // COMMON_H

