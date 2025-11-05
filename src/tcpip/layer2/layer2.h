// layer2.h

#pragma once

#include "net.h"
#include "graph.h"
#include "utils.h"

#pragma mark -

// Ethernet

#define ETHER_TYPE_ARP  0x0806
#define ETHER_TYPE_IPV4 0x0800
#define ETHER_TYPE_VLAN 0x8100

#define ETHER_FCS_PTR(HDRPTR, PAY_SZ) (ether_hdr_t *)(HDRPTR + sizeof(ether_hdr_t) + PAY_SZ)

typedef struct ether_hdr_t ether_hdr_t;

struct __PACK__ ether_hdr_t {
  mac_addr_t src_mac;
  mac_addr_t dst_mac;
  uint16_t type;
};

ether_hdr_t* ether_hdr_alloc_with_payload(uint8_t *pkt, uint32_t pktlen);

#pragma mark -

// Layer 2 processing

bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr);
int layer2_node_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);

#pragma mark -

// Accessors for `ether_hdr_t`

static inline mac_addr_t ether_hdr_read_src_mac(const ether_hdr_t *hdr) {
  return hdr->src_mac;
}

static inline void ether_hdr_set_src_mac(ether_hdr_t *hdr, const mac_addr_t *mac) {
  hdr->src_mac = *mac;
}

static inline mac_addr_t ether_hdr_read_dst_mac(const ether_hdr_t *hdr) {
  return hdr->dst_mac;
}

static inline void ether_hdr_set_dst_mac(ether_hdr_t *hdr, const mac_addr_t *mac) {
  hdr->dst_mac = *mac;
}

static inline uint16_t ether_hdr_read_type(ether_hdr_t *hdr) {
  return ntohs(hdr->type);
}

static inline void ether_hdr_set_type(ether_hdr_t *hdr, uint16_t type) {
  hdr->type = htons(type);
}
