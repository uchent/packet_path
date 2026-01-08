#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64); 
    __type(key, int);
    __type(value, int);
} xsks_map SEC(".maps");

SEC("xdp")
int xdp_sock_prog(struct xdp_md *ctx) {
    /* ctx->rx_queue_index is the hardware queue index of the current packet.
     * We try to redirect the packet to the AF_XDP Socket corresponding to that queue.
     */
     __u32 rx_queue_index = ctx->rx_queue_index;

     /* Check if the Socket is bound to the index in the Map.
      * If yes, the packet will be directly sent to the user space.
      * If no, return XDP_PASS, let the packet go through the normal Linux network process.
      */
     if (bpf_map_lookup_elem(&xsks_map, &rx_queue_index)) {
         return bpf_redirect_map(&xsks_map, rx_queue_index, 0);
     }
 
     return XDP_PASS;
}

char _license[] SEC("license") = "GPL";