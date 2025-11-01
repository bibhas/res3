// layer2.h

#pragma once

#include "net.h"
#include "graph.h"
#include "utils.h"

#pragma mark -

// Ethernet

#define ETHER_TYPE_ARP 0x0806
#define ETHER_HDR_SIZE_EXCL_PAYLOAD (sizeof(ether_hdt_t) - sizeof(uint8_t *))
#define ETHER_FCS_PTR(HDRPTR, PAY_SZ) (ether_hdt_t *)(HDRPTR + sizeof(ether_hdr_t) + PAY_SZ)

typedef struct ether_hdr_t ether_hdr_t;

struct __PACK__ ether_hdr_t {
  mac_addr_t src_mac;
  mac_addr_t dst_mac;
  uint16_t type;
  uint8_t payload[];
};

ether_hdr_t* ether_hdr_alloc_with_payload(uint8_t *pkt, uint32_t pktlen);

#pragma mark -

// Layer 2 processing

bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr);
int layer2_node_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);

