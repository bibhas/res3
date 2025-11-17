// vlan.h

#pragma once

#include <functional>
#include <arpa/inet.h>
#include "utils.h"

typedef struct vlan_tag_t vlan_tag_t;

/*
 * We're skipping TPID because the type field in ether_hdr_t "aliases" the TPID
 * field when the frame is VLAN tagged. What's more, we consider the trailing
 * `ether_type` field as part of the vlan tag, which is technically part of the
 * ethernet header. This setup makes field access easier and works even if the
 * frame is double tagged.
 * 
 * Idea borrowed from DPDK.
 */
struct __PACK__ vlan_tag_t {
  uint16_t tci;
  uint16_t ether_type; // left-over from ether_hdr_t
};

static inline void vlan_tag_init(vlan_tag_t *t) {
  memset((void *)t, 0, sizeof(vlan_tag_t));
}
static inline uint8_t vlan_tag_read_pcp(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return (tci >> 13) & 0x07; // 0b111 = 3 bits
}

static inline void vlan_tag_set_pcp(vlan_tag_t *t, uint8_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= ((val & 0x7) << 13);
}

static inline uint8_t vlan_tag_read_dei(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return (tci >> 12) & 0x01; // 1 bit
}

static inline void vlan_tag_set_dei(vlan_tag_t *t, uint8_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= ((val & 0x1) << 11);
}

static inline uint16_t vlan_tag_read_vlan_id(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return tci & 0xFFF; // 12 bits
}

static inline void vlan_tag_set_vlan_id(vlan_tag_t *t, uint16_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= htons(val & 0x0FFF);
}

static inline uint16_t vlan_tag_read_ether_type(vlan_tag_t *t) {
  return ntohs(t->ether_type);
}

static inline void vlan_tag_set_ether_type(vlan_tag_t *t, uint16_t val) {
  t->ether_type = htons(val);
}
