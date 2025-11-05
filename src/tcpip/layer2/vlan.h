// vlan.h

#pragma once

#include "utils.h"

typedef struct vlan_tag_t vlan_tag_t;

struct __PACK__ vlan_tag_t {
  uint16_t tpi;
  uint16_t tci;
};

#pragma mark -

// Accessor functions for `vlan_tag_t`

static inline uint16_t vlan_read_tpi(vlan_tag_t *t) {
  return ntohs(t->tpi);
}

static inline void vlan_set_tpi(vlan_tag_t *t, uint16_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tpi = htons(val);
}

static inline uint8_t vlan_read_pcp(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return (tci >> 13) & 0x07; // 0b111 = 3 bits
}

static inline void vlan_set_pcp(vlan_tag_t *t, uint8_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= ((val & 0x7) << 13);
}

static inline uint8_t vlan_read_dei(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return (tci >> 12) & 0x01; // 1 bit
}

static inline void vlan_set_dei(vlan_tag_t *t, uint8_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= ((val & 0x1) << 11);
}

static inline uint16_t vlan_read_vlan_id(vlan_tag_t *t) {
  uint16_t tci = ntohs(t->tci);
  return tci & 0xFFF; // 12 bits
}

static inline void vlan_set_vlan_id(vlan_tag_t *t, uint16_t val) {
  EXPECT_RETURN(t != nullptr, "Empty tag ptr param");
  t->tci |= htons(val & 0x0FFF);
}


