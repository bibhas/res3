// ether.h

#pragma once

#include <functional>
#include <arpa/inet.h>
#include "utils.h"

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

#define ETHER_HDR_VLAN_TAGGED(HDR) (ether_hdr_read_type((HDR)) == ETHER_TYPE_VLAN)

#pragma mark -

// Accessors for `ether_hdr_t`

static inline void ether_hdr_init(const ether_hdr_t *hdr) {
  memset((void *)hdr, 0, sizeof(ether_hdr_t));
}

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

