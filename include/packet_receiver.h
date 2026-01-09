#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include "common.h"

// Packet receiver interface
typedef struct packet_receiver packet_receiver_t;

// Receiver operation function pointers
typedef struct {
    int (*init)(packet_receiver_t *receiver, const config_t *config);
    int (*start)(packet_receiver_t *receiver);
    int (*stop)(packet_receiver_t *receiver);
    void (*cleanup)(packet_receiver_t *receiver);
} receiver_ops_t;

// Receiver structure
struct packet_receiver {
    packet_mode_t mode;
    config_t config;
    stats_t stats;
    receiver_ops_t ops;
    void *private_data;  // Mode-specific private data
    bool running;
};

// Function declarations
packet_receiver_t* packet_receiver_create(packet_mode_t mode);
void packet_receiver_destroy(packet_receiver_t *receiver);
int packet_receiver_init(packet_receiver_t *receiver, const config_t *config);
int packet_receiver_start(packet_receiver_t *receiver);
int packet_receiver_stop(packet_receiver_t *receiver);
void packet_receiver_cleanup(packet_receiver_t *receiver);

#endif // PACKET_RECEIVER_H

