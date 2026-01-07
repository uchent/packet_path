#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_errno.h>
#include "../../include/common.h"
#include "../../include/packet_receiver.h"

// DPDK private data structure
typedef struct {
    uint16_t port_id;
    struct rte_mempool *mbuf_pool;
    bool initialized;
    bool eal_initialized;
} dpdk_private_t;

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

// Find port ID by interface name
static int find_port_id_by_name(const char *interface_name, uint16_t *port_id) {

    return 0;
    // uint16_t port_count = rte_eth_dev_count_avail();
    
    // for (uint16_t i = 0; i < port_count; i++) {
    //     struct rte_eth_dev_info dev_info;
    //     if (rte_eth_dev_info_get(i, &dev_info) != 0) {
    //         continue;
    //     }
        
    //     // Check if interface name matches
    //     // Note: DPDK port names are typically in format "0000:XX:XX.X"
    //     // We'll try to match by PCI address or use port index
    //     if (strstr(dev_info.device->name, interface_name) != NULL ||
    //         strcmp(interface_name, dev_info.device->name) == 0) {
    //         *port_id = i;
    //         return 0;
    //     }
    // }
    
    // // If not found by name, try to use port 0 as default
    // if (port_count > 0) {
    //     *port_id = 0;
    //     fprintf(stderr, "Warning: Interface '%s' not found, using port 0\n", interface_name);
    //     return 0;
    // }
    
    // return -1;
}

// DPDK initialization (requires full DPDK environment)
static int dpdk_init(packet_receiver_t *receiver, const config_t *config) {
    // dpdk_private_t *priv = (dpdk_private_t *)receiver->private_data;
    // int ret;
    
    // if (!priv) {
    //     priv = calloc(1, sizeof(dpdk_private_t));
    //     if (!priv) return -1;
    //     receiver->private_data = priv;
    // }
    
    // // Initialize EAL if not already done
    // if (!priv->eal_initialized) {
    //     // DPDK EAL arguments - minimal setup
    //     // Note: In production, these should come from command line
    //     const char *eal_args[] = {
    //         "packet_receiver",
    //         "-l", "0",  // Use core 0
    //         "--proc-type", "primary",
    //         NULL
    //     };
        
    //     int eal_argc = 4;
    //     char **eal_argv = (char **)eal_args;
        
    //     ret = rte_eal_init(eal_argc, eal_argv);
    //     if (ret < 0) {
    //         fprintf(stderr, "Error: DPDK EAL initialization failed: %s\n", 
    //                 rte_strerror(rte_errno));
    //         return -1;
    //     }
    //     priv->eal_initialized = true;
    //     printf("DPDK EAL initialized successfully\n");
    // }
    
    // // Check available ports
    // uint16_t port_count = rte_eth_dev_count_avail();
    // if (port_count == 0) {
    //     fprintf(stderr, "Error: No DPDK ports available\n");
    //     fprintf(stderr, "Note: Make sure network interfaces are bound to DPDK driver\n");
    //     fprintf(stderr, "      Use: dpdk-devbind.py --bind=igb_uio <PCI_ADDRESS>\n");
    //     return -1;
    // }
    
    // printf("Found %u DPDK port(s)\n", port_count);
    
    // // Find port ID by interface name
    // if (find_port_id_by_name(config->interface, &priv->port_id) != 0) {
    //     fprintf(stderr, "Error: Failed to find DPDK port for interface '%s'\n", 
    //             config->interface);
    //     return -1;
    // }
    
    // printf("Using DPDK port %u for interface '%s'\n", priv->port_id, config->interface);
    
    // // Create mbuf pool
    // char pool_name[32];
    // snprintf(pool_name, sizeof(pool_name), "mbuf_pool_%u", priv->port_id);
    
    // priv->mbuf_pool = rte_pktmbuf_pool_create(
    //     pool_name,
    //     NUM_MBUFS,
    //     MBUF_CACHE_SIZE,
    //     0,
    //     RTE_MBUF_DEFAULT_BUF_SIZE,
    //     rte_socket_id()
    // );
    
    // if (priv->mbuf_pool == NULL) {
    //     fprintf(stderr, "Error: Failed to create mbuf pool: %s\n", 
    //             rte_strerror(rte_errno));
    //     return -1;
    // }
    
    // printf("Created mbuf pool: %s\n", pool_name);
    
    // // Get port configuration
    // struct rte_eth_conf port_conf = {
    //     .rxmode = {
    //         .mtu = RTE_ETHER_MTU,
    //     },
    // };
    
    // struct rte_eth_dev_info dev_info;
    // ret = rte_eth_dev_info_get(priv->port_id, &dev_info);
    // if (ret != 0) {
    //     fprintf(stderr, "Error: Failed to get device info: %s\n", 
    //             rte_strerror(rte_errno));
    //     rte_mempool_free(priv->mbuf_pool);
    //     return -1;
    // }
    
    // // Configure port
    // ret = rte_eth_dev_configure(priv->port_id, 1, 0, &port_conf);
    // if (ret < 0) {
    //     fprintf(stderr, "Error: Failed to configure DPDK port %u: %s\n", 
    //             priv->port_id, rte_strerror(rte_errno));
    //     rte_mempool_free(priv->mbuf_pool);
    //     return -1;
    // }
    
    // // Setup RX queue
    // ret = rte_eth_rx_queue_setup(
    //     priv->port_id,
    //     0,
    //     RX_RING_SIZE,
    //     rte_eth_dev_socket_id(priv->port_id),
    //     NULL,
    //     priv->mbuf_pool
    // );
    
    // if (ret < 0) {
    //     fprintf(stderr, "Error: Failed to setup RX queue: %s\n", 
    //             rte_strerror(rte_errno));
    //     rte_mempool_free(priv->mbuf_pool);
    //     return -1;
    // }
    
    // // Start port
    // ret = rte_eth_dev_start(priv->port_id);
    // if (ret < 0) {
    //     fprintf(stderr, "Error: Failed to start DPDK port %u: %s\n", 
    //             priv->port_id, rte_strerror(rte_errno));
    //     rte_mempool_free(priv->mbuf_pool);
    //     return -1;
    // }
    
    // // Display port link status
    // struct rte_eth_link link;
    // ret = rte_eth_link_get(priv->port_id, &link);
    // if (ret == 0) {
    //     if (link.link_status) {
    //         printf("Port %u link up - speed %u Mbps - %s\n",
    //                priv->port_id, link.link_speed,
    //                (link.link_duplex == RTE_ETH_LINK_FULL_DUPLEX) ?
    //                "full-duplex" : "half-duplex");
    //     } else {
    //         printf("Port %u link down\n", priv->port_id);
    //     }
    // }
    
    // priv->initialized = true;
    // printf("DPDK mode initialized successfully\n");
    
    return 0;
}

static int dpdk_start(packet_receiver_t *receiver) {
    // dpdk_private_t *priv = (dpdk_private_t *)receiver->private_data;
    
    // if (!priv || !priv->initialized) {
    //     fprintf(stderr, "Error: DPDK receiver not initialized\n");
    //     return -1;
    // }
    
    // receiver->running = true;
    // printf("Starting packet reception (DPDK userspace mode)...\n");
    // printf("Receiving packets on port %u\n", priv->port_id);
    
    // // Main receive loop
    // while (receiver->running) {
    //     struct rte_mbuf *bufs[BURST_SIZE];
    //     uint16_t nb_rx = rte_eth_rx_burst(priv->port_id, 0, bufs, BURST_SIZE);
        
    //     if (nb_rx > 0) {
    //         for (uint16_t i = 0; i < nb_rx; i++) {
    //             uint32_t pkt_len = rte_pktmbuf_pkt_len(bufs[i]);
                
    //             // Update statistics
    //             stats_update(&receiver->stats, pkt_len);
                
    //             if (receiver->config.verbose) {
    //                 printf("Packet received: %u bytes (zero-copy)\n", pkt_len);
    //             }
                
    //             // Free mbuf back to pool
    //             rte_pktmbuf_free(bufs[i]);
    //         }
    //     }
        
    //     // Check if runtime duration is reached
    //     if (receiver->config.duration_sec > 0) {
    //         uint64_t elapsed = get_time_ns() - receiver->stats.start_time;
    //         if (elapsed / 1e9 >= receiver->config.duration_sec) {
    //             break;
    //         }
    //     }
    // }
    
    return 0;
}

static int dpdk_stop(packet_receiver_t *receiver) {
    // if (receiver) {
    //     receiver->running = false;
    // }
    return 0;
}

static void dpdk_cleanup(packet_receiver_t *receiver) {
    // dpdk_private_t *priv = (dpdk_private_t *)receiver->private_data;
    
    // if (priv) {
    //     if (priv->initialized) {
    //         // Stop port
    //         if (priv->port_id < RTE_MAX_ETHPORTS) {
    //             rte_eth_dev_stop(priv->port_id);
    //             rte_eth_dev_close(priv->port_id);
    //             printf("DPDK port %u stopped and closed\n", priv->port_id);
    //         }
            
    //         // Free mbuf pool
    //         if (priv->mbuf_pool) {
    //             rte_mempool_free(priv->mbuf_pool);
    //             priv->mbuf_pool = NULL;
    //         }
    //     }
        
    //     // Cleanup EAL if we initialized it
    //     if (priv->eal_initialized) {
    //         rte_eal_cleanup();
    //         priv->eal_initialized = false;
    //     }
        
    //     free(priv);
    //     receiver->private_data = NULL;
    // }
}

static stats_t* dpdk_get_stats(packet_receiver_t *receiver) {
    return receiver ? &receiver->stats : NULL;
}

// Create DPDK receiver
packet_receiver_t* dpdk_receiver_create(void) {
    packet_receiver_t *receiver = calloc(1, sizeof(packet_receiver_t));
    if (!receiver) return NULL;
    
    receiver->mode = MODE_DPDK;
    receiver->ops.init = dpdk_init;
    receiver->ops.start = dpdk_start;
    receiver->ops.stop = dpdk_stop;
    receiver->ops.cleanup = dpdk_cleanup;
    receiver->ops.get_stats = dpdk_get_stats;
    
    stats_init(&receiver->stats);
    
    return receiver;
}
