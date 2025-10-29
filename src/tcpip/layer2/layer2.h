// layer2.h

#pragma once

#include "net.h"
#include "graph.h"

#define ETHER_MTU 1500

struct __attribute__((packed)) ether_hdr_t {
  mac_addr_t src_mac;
  mac_addr_t dst_mac;
  uint16_t type;
  uint8_t payload[ETHER_MTU];
  uint32_t fcs;
};

typedef struct ether_hdr_t ether_hdr_t;

#define ETHER_HDR_SIZE_EXCL_PAYLOAD (sizeof(ether_hdt_t) - ETHER_MTU)
#define ETHER_FCS(ETHER_HDR_PTR, PAY_SZ) (ether_hdt_t *)(ETHER_HDR_PTR)->fcs

static inline ether_hdr_t *ether_hdr_alloc_with_payload(uint8_t *pkt, uint32_t pktlen) {
  /* 
   * [ free space ][ pkt ][free space]
   *         ↑ ← ← ↑
   *        ret   pkt
   */
  EXPECT_RETURN_VAL(pkt != nullptr, "Empty pkt param", nullptr);
  uint8_t *start = pkt - __builtin_offsetof(ether_hdr_t, payload);
  ether_hdr_t *resp = (ether_hdr_t *)start;
  resp->src_mac = {0};
  resp->dst_mac = {0};
  resp->type = 0;
  resp->fcs = 0;
  return resp;
}
static inline bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(ethhdr != nullptr, "Empty ethernet header param", false);
  if (!INTF_IS_L3_MODE(intf)) { 
    return false; 
  }
  if (INTF_IS_L3_MODE(intf) && MAC_ADDR_IS_EQUAL(ethhdr->dst_mac, intf->netprop.mac_addr)) {
    return true;
  }
  if (INTF_IS_L3_MODE(intf) && MAC_ADDR_IS_BROADCAST(ethhdr->dst_mac)) {
    return true;
  }
  return false;
}

static inline int layer2_frame_recv(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen) {
  // Entry point into our TCP/IP stack
  // TODO
  return 0;
}
