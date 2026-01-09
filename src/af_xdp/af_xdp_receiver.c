#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h>
// #include <bpf/xsk.h>
#include <xdp/xsk.h>
#include <xdp/libxdp.h>
#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include "../../include/common.h"
#include "../../include/packet_receiver.h"

#define NUM_FRAMES 4096
#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define BATCH_SIZE 64

#define XDP_PROG_NAME "obj/af_xdp/xdp_kern.o"

#define TEST_QUEUE_INDEX 1

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

typedef struct {
    struct xsk_umem_info *umem_info;
    struct xsk_socket_info *xsk_info;
    void *bufs;
    
    uint32_t f_idx; // Fill Ring index
    
    struct xdp_program *prog; // XDP program
    struct bpf_object *obj;
    unsigned int attach_mode; // XDP attach mode
} af_xdp_private_t;

static int load_xdp_program(af_xdp_private_t *priv, const config_t *config) {
    if (!priv) return -1;
    
    int ret;
    priv->prog = xdp_program__open_file(XDP_PROG_NAME, "xdp", NULL);
    if (!priv->prog) {
        fprintf(stderr, "Error: Failed to load xdp program\n");
        return 1;
    }

    priv->obj = xdp_program__bpf_obj(priv->prog);

    priv->attach_mode = XDP_MODE_NATIVE; // Native mode
    ret = xdp_program__attach(priv->prog, if_nametoindex(config->interface), priv->attach_mode, 0);
    if (ret) {
        fprintf(stderr, "Error: Failed to attach xdp program: %s\n", strerror(-ret));
        return 1;
    }

    printf("XDP program attached to interface %s\n", config->interface);
    return ret;
}

static void detach_xdp_program(af_xdp_private_t *priv, const config_t *config) {
    if (!priv) return;
    int ret;

    if (priv->prog) {
        ret = xdp_program__detach(priv->prog, if_nametoindex(config->interface), priv->attach_mode, 0);
        if (ret) {
            fprintf(stderr, "Error: Failed to detach xdp program: %s\n", strerror(-ret));
            return;
        }
        xdp_program__close(priv->prog);
        priv->prog = NULL;
        printf("XDP program detached from interface %s\n", config->interface);
    }
}

static int af_xdp_init(packet_receiver_t *receiver, const config_t *config) {
    af_xdp_private_t *priv = (af_xdp_private_t *)receiver->private_data;
    
    if (!priv) {
        priv = calloc(1, sizeof(af_xdp_private_t));
        if (!priv) return -1;
        receiver->private_data = priv;
    }

    int ret;

    // load xdp program
    ret = load_xdp_program(priv, config);
    if (ret) {
        exit(1);
    }
    
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
    struct xsk_umem_config umem_cfg = {
        .fill_size = XSK_RING_PROD__DEFAULT_NUM_DESCS,
        .comp_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
        .frame_size = FRAME_SIZE,
        .frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
        .flags = 0
    };
    ret = xsk_umem__create(&priv->umem_info->umem, priv->bufs, NUM_FRAMES * FRAME_SIZE,
                           &priv->umem_info->fq, &priv->umem_info->cq, &umem_cfg);
    if (ret) {
        fprintf(stderr, "xsk_umem__create: %s (errno: %d)\n", strerror(-ret), -ret);
        exit(1);
    };

    // fill memory blocks to Fill Ring
    ret = xsk_ring_prod__reserve(&priv->umem_info->fq, XSK_RING_PROD__DEFAULT_NUM_DESCS, &priv->f_idx);
    for (int i = 0; i < XSK_RING_PROD__DEFAULT_NUM_DESCS; i++) {
        *xsk_ring_prod__fill_addr(&priv->umem_info->fq, priv-> f_idx++) = i * FRAME_SIZE;
    }
    xsk_ring_prod__submit(&priv->umem_info->fq, XSK_RING_PROD__DEFAULT_NUM_DESCS);

    // create AF_XDP Socket (XSK)
    priv->xsk_info = calloc(1, sizeof(*priv->xsk_info));
    if (!priv->xsk_info) {
        fprintf(stderr, "Error: Failed to allocate xsk_info\n");
        exit(1);
    }
    
    struct xsk_socket_config socket_cfg = {
        .rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
        .tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS,
        .libbpf_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD, // Load the XDP program manually
        .xdp_flags = XDP_FLAGS_DRV_MODE, // Zero-copy mode -> XDP native mode
        .bind_flags = XDP_USE_NEED_WAKEUP,
    };

    ret = xsk_socket__create(&priv->xsk_info->xsk, config->interface, TEST_QUEUE_INDEX, priv->umem_info->umem,
                             &priv->xsk_info->rx, &priv->xsk_info->tx, &socket_cfg);
    if (ret) {
        fprintf(stderr, "xsk_socket__create: %s (errno: %d)\n", strerror(-ret), -ret);
        exit(1);
    };

    // bind XSK to xsks_map
    int map_fd = bpf_object__find_map_fd_by_name(priv->obj, "xsks_map");
    if (map_fd < 0) {
		fprintf(stderr, "ERROR: xsks_map not found!\n");
        exit(1);
    }
    ret = xsk_socket__update_xskmap(priv->xsk_info->xsk, map_fd);
    if (ret) {
        fprintf(stderr, "xsk_socket__update_xskmap: %s (errno: %d)\n", strerror(-ret), -ret);
        exit(1);
    }
    printf("XSK bound to xsks_map\n");

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

        if (!rcvd) {
            // If no packets, enter poll to save CPU
            struct pollfd pfd = { .fd = xsk_socket__fd(priv->xsk_info->xsk), .events = POLLIN };
            poll(&pfd, 1, 1000);
            continue;
        }

        for (unsigned int i = 0; i < rcvd; i++) {
            const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&priv->xsk_info->rx, r_idx + i);
            uint64_t addr = desc->addr;
            uint32_t len = desc->len;

            // receive packet pointer
            unsigned char *pkt = xsk_umem__get_data(priv->bufs, addr);
            // process packet here

            stats_update(&receiver->stats, len);

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
        detach_xdp_program(priv, &receiver->config);
        free(priv);
        receiver->private_data = NULL;
    }
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
    
    stats_init(&receiver->stats);
    
    return receiver;
}

