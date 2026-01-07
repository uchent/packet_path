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
#include <sys/ioctl.h>
#include <pthread.h>
#include "../../include/common.h"
#include "../../include/packet_receiver.h"

typedef struct {
    int socket_fd;
} socket_private_t;

static int socket_init(packet_receiver_t *receiver, const config_t *config) {
    socket_private_t *priv = (socket_private_t *)receiver->private_data;
    
    if (!priv) {
        priv = calloc(1, sizeof(socket_private_t));
        if (!priv) return -1;
        receiver->private_data = priv;
    }
    
    // create raw socket
    priv->socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (priv->socket_fd < 0) {
        fprintf(stderr,"socket: %s\n", strerror(errno));
        return 1;
    }

    // get interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, config->interface, IFNAMSIZ - 1);

    if (ioctl(priv->socket_fd, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr,"SIOCGIFINDEX: %s\n", strerror(errno));
        close(priv->socket_fd);
        exit(1);
    }

    // bind socket to interface
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(priv->socket_fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        fprintf(stderr,"bind: %s\n", strerror(errno));
        close(priv->socket_fd);
        exit(1);
    }

    // set socket to promiscuous mode
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type    = PACKET_MR_PROMISC;

    if (setsockopt(priv->socket_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                   &mr, sizeof(mr)) < 0) {
        fprintf(stderr,"setsockopt PROMISC: %s\n", strerror(errno));
        close(priv->socket_fd);
        exit(1);
    }

    printf("Socket mode initialized successfully, interface: %s\n", config->interface);
    return 0;
}

static int socket_start(packet_receiver_t *receiver) {
    socket_private_t *priv = (socket_private_t *)receiver->private_data;
    if (!priv || priv->socket_fd < 0) return -1;

    const u_char *packet_data;
    u_char buf[2048];
    
    receiver->running = true;
    printf("Starting packet reception...\n");
    
    while (receiver->running) {
        ssize_t len = recvfrom(priv->socket_fd, buf, sizeof(buf), 0, NULL, NULL);
        
        if (len > 0) {
            stats_update(&receiver->stats, len);
            if (receiver->config.verbose) {
                printf("Raw packet received: %ld bytes\n", len);
            }
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
        if (priv->socket_fd >= 0) {
            close(priv->socket_fd);
            priv->socket_fd = -1;
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

