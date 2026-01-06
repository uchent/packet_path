// Stub implementation for DPDK receiver when DPDK is not available
#include "../../include/packet_receiver.h"

packet_receiver_t* dpdk_receiver_create(void) {
    return NULL;  // DPDK not available
}




