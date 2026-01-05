#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>
#include "../../include/common.h"
#include "../../include/packet_receiver.h"

typedef struct {
    pcap_t *handle;
    int socket_fd;
} socket_private_t;

static int socket_init(packet_receiver_t *receiver, const config_t *config) {
    socket_private_t *priv = (socket_private_t *)receiver->private_data;
    char errbuf[PCAP_ERRBUF_SIZE];
    
    if (!priv) {
        priv = calloc(1, sizeof(socket_private_t));
        if (!priv) return -1;
        receiver->private_data = priv;
    }
    
    // Open interface using libpcap
    priv->handle = pcap_open_live(config->interface, 
                                   config->buffer_size,
                                   config->promiscuous ? 1 : 0,
                                   config->timeout_ms,
                                   errbuf);
    
    if (!priv->handle) {
        fprintf(stderr, "Error: Failed to open interface %s: %s\n", config->interface, errbuf);
        return -1;
    }
    
    // Set filter (optional, receive all packets)
    struct bpf_program fp;
    if (pcap_compile(priv->handle, &fp, "", 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Warning: Failed to compile filter\n");
    } else {
        if (pcap_setfilter(priv->handle, &fp) == -1) {
            fprintf(stderr, "Warning: Failed to set filter\n");
        }
        pcap_freecode(&fp);
    }
    
    printf("Socket mode initialized successfully, interface: %s\n", config->interface);
    return 0;
}

static int socket_start(packet_receiver_t *receiver) {
    socket_private_t *priv = (socket_private_t *)receiver->private_data;
    struct pcap_pkthdr *header;
    const u_char *packet_data;
    int res;
    
    if (!priv || !priv->handle) return -1;
    
    receiver->running = true;
    printf("Starting packet reception...\n");
    
    while (receiver->running) {
        res = pcap_next_ex(priv->handle, &header, &packet_data);
        
        if (res == 1) {
            // Successfully received packet
            // In Socket mode, packet is copied from kernel space to user space (1 copy)
            stats_update(&receiver->stats, header->caplen);
            stats_update_copy(&receiver->stats, 1);
            
            if (receiver->config.verbose) {
                printf("Packet received: %d bytes\n", header->caplen);
            }
        } else if (res == 0) {
            // Timeout
            continue;
        } else if (res == -1) {
            fprintf(stderr, "Error: Failed to read packet\n");
            break;
        } else if (res == -2) {
            // End of reading
            break;
        }
        
        // Check if runtime duration is reached
        if (receiver->config.duration_sec > 0) {
            uint64_t elapsed = get_time_ns() - receiver->stats.start_time;
            if (elapsed / 1e9 >= receiver->config.duration_sec) {
                break;
            }
        }
    }
    
    return 0;
}

static int socket_stop(packet_receiver_t *receiver) {
    if (receiver) {
        receiver->running = false;
    }
    return 0;
}

static void socket_cleanup(packet_receiver_t *receiver) {
    socket_private_t *priv = (socket_private_t *)receiver->private_data;
    
    if (priv) {
        if (priv->handle) {
            pcap_close(priv->handle);
            priv->handle = NULL;
        }
        free(priv);
        receiver->private_data = NULL;
    }
}

static stats_t* socket_get_stats(packet_receiver_t *receiver) {
    return receiver ? &receiver->stats : NULL;
}

// Create Socket receiver
packet_receiver_t* socket_receiver_create(void) {
    packet_receiver_t *receiver = calloc(1, sizeof(packet_receiver_t));
    if (!receiver) return NULL;
    
    receiver->mode = MODE_SOCKET;
    receiver->ops.init = socket_init;
    receiver->ops.start = socket_start;
    receiver->ops.stop = socket_stop;
    receiver->ops.cleanup = socket_cleanup;
    receiver->ops.get_stats = socket_get_stats;
    
    stats_init(&receiver->stats);
    
    return receiver;
}

