#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h>
#include <bpf/xsk.h>
// #include <xdp/xsk.h>
#include <bpf/bpf.h>
#include <linux/if_link.h>
#include "../../include/common.h"
#include "../../include/packet_receiver.h"

#define NUM_FRAMES 4096
#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define BATCH_SIZE 64

struct xsk_umem_info {
    struct xsk_ring_prod fq; // Fill Ring
    struct xsk_ring_cons cq; // Completion Ring
    struct xsk_umem *umem;
    void *buffer;
};

struct xsk_socket_info {
    struct xsk_ring_cons rx; // RX Ring
    struct xsk_ring_prod tx; // TX Ring
    struct xsk_socket *xsk;
};

// AF_XDP private data structure
typedef struct {
    struct xsk_umem_info *umem_info;
    struct xsk_socket_info *xsk_info;
    void *bufs;

    uint32_t f_idx; // Fill Ring index
} af_xdp_private_t;

// Simplified AF_XDP implementation (requires full libbpf support)
static int af_xdp_init(packet_receiver_t *receiver, const config_t *config) {
    af_xdp_private_t *priv = (af_xdp_private_t *)receiver->private_data;
    
    if (!priv) {
        priv = calloc(1, sizeof(af_xdp_private_t));
        if (!priv) return -1;
        receiver->private_data = priv;
    }

    int ret;
    
    // allocate memory
    ret = posix_memalign(&priv->bufs, getpagesize(), NUM_FRAMES * FRAME_SIZE);
    if (ret) {
        fprintf(stderr, "Error: Failed to allocate bufs\n");
        exit(1);
    }

    priv->umem_info = calloc(1, sizeof(*priv->umem_info));
    if (!priv->umem_info) {
        fprintf(stderr, "Error: Failed to allocate umem_info\n");
        exit(1);
    }

    // create UMEM
    ret = xsk_umem__create(&priv->umem_info->umem, priv->bufs, NUM_FRAMES * FRAME_SIZE,
                           &priv->umem_info->fq, &priv->umem_info->cq, NULL);
    if (ret) {
        fprintf(stderr, "xsk_umem__create: %s (errno: %d)\n", strerror(-ret), -ret);
        exit(1);
    };

    // create AF_XDP Socket (XSK)
    priv->xsk_info = calloc(1, sizeof(*priv->xsk_info));
    if (!priv->xsk_info) {
        fprintf(stderr, "Error: Failed to allocate xsk_info\n");
        exit(1);
    }
    
    struct xsk_socket_config cfg = {
        .rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
        .tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS,
        .libbpf_flags = 0,
        .xdp_flags = XDP_ZEROCOPY,
        .bind_flags = XDP_USE_NEED_WAKEUP,
    };

    ret = xsk_socket__create(&priv->xsk_info->xsk, config->interface, 0, priv->umem_info->umem,
                             &priv->xsk_info->rx, &priv->xsk_info->tx, &cfg);
    if (ret) {
        fprintf(stderr, "xsk_socket__create: %s (errno: %d)\n", strerror(-ret), -ret);
        exit(1);
    };

    // fill memory blocks to Fill Ring
    ret = xsk_ring_prod__reserve(&priv->umem_info->fq, BATCH_SIZE, &priv->f_idx);
    for (int i = 0; i < BATCH_SIZE; i++) {
        *xsk_ring_prod__fill_addr(&priv->umem_info->fq, priv-> f_idx++) = i * FRAME_SIZE;
    }
    xsk_ring_prod__submit(&priv->umem_info->fq, BATCH_SIZE);
    return 0;
}

static int af_xdp_start(packet_receiver_t *receiver) {
    af_xdp_private_t *priv = (af_xdp_private_t *)receiver->private_data;
    
    if (!priv) return -1;
    
    receiver->running = true;
    printf("Starting packet reception (AF_XDP zero-copy mode)...\n");
    
    while (receiver->running) {
        uint32_t r_idx; // Receive Ring index
        unsigned int rcvd = xsk_ring_cons__peek(&priv->xsk_info->rx, BATCH_SIZE, &r_idx);

        if (rcvd == 0) continue;

        for (unsigned int i = 0; i < rcvd; i++) {
            const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&priv->xsk_info->rx, r_idx + i);
            uint64_t addr = desc->addr;
            uint32_t len = desc->len;

            // receive packet pointer
            // unsigned char *pkt = xsk_umem__get_data(priv->bufs, addr);
            
            stats_update(&receiver->stats, len);
            stats_update_copy(&receiver->stats, 0);  // Zero copy

            if (receiver->config.verbose) {
                printf("Packet received: %d bytes (zero-copy)\n", len);
            }
        }

        xsk_ring_cons__release(&priv->xsk_info->rx, rcvd);

        // refill memory blocks to Fill Ring
        xsk_ring_prod__reserve(&priv->umem_info->fq, rcvd, &priv->f_idx);
        for (unsigned int i = 0; i < rcvd; i++) {
            *xsk_ring_prod__fill_addr(&priv->umem_info->fq, priv->f_idx++) =
                xsk_ring_cons__rx_desc(&priv->xsk_info->rx, r_idx + i)->addr;
        }
        xsk_ring_prod__submit(&priv->umem_info->fq, rcvd);
        
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

static int af_xdp_stop(packet_receiver_t *receiver) {
    if (receiver) {
        receiver->running = false;
    }
    return 0;
}

static void af_xdp_cleanup(packet_receiver_t *receiver) {
    af_xdp_private_t *priv = (af_xdp_private_t *)receiver->private_data;
    
    if (priv) {
        if (priv->umem_info) {
            xsk_umem__delete(priv->umem_info->umem);
            free(priv->umem_info);
            priv->umem_info = NULL;
        }
        if (priv->xsk_info) {
            xsk_socket__delete(priv->xsk_info->xsk);
            free(priv->xsk_info);
            priv->xsk_info = NULL;
        }
        if (priv->bufs) {
            free(priv->bufs);
            priv->bufs = NULL;
        }
        free(priv);
        receiver->private_data = NULL;
    }
}

static stats_t* af_xdp_get_stats(packet_receiver_t *receiver) {
    return receiver ? &receiver->stats : NULL;
}

// Create AF_XDP receiver
packet_receiver_t* af_xdp_receiver_create(void) {
    packet_receiver_t *receiver = calloc(1, sizeof(packet_receiver_t));
    if (!receiver) return NULL;
    
    receiver->mode = MODE_AF_XDP;
    receiver->ops.init = af_xdp_init;
    receiver->ops.start = af_xdp_start;
    receiver->ops.stop = af_xdp_stop;
    receiver->ops.cleanup = af_xdp_cleanup;
    receiver->ops.get_stats = af_xdp_get_stats;
    
    stats_init(&receiver->stats);
    
    return receiver;
}

