// layer2.h

#pragma once

#include "net.h"

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
  EXPECT_RETURN_VAL(pkt != nullptr, "Empty pkt param", nullptr);
  uint8_t *start = pkt - __builtin_offsetof(ether_hdr_t, payload);
  ether_hdr_t *resp = (ether_hdr_t *)start;
  resp->src_mac = {0};
  resp->dst_mac = {0};
  resp->type = 0;
  resp->fcs = 0;
  return resp;
}

