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

#define ETHER_HDR_VLAN_TAGGED(HDR) (ether_hdr_read_type((HDR)) == ETHER_TYPE_VLAN)

// Precondition: Enough headroom in front of `pkt` to fit a `ether_hdr_t`
ether_hdr_t* ether_hdr_alloc_with_payload(uint8_t *pkt, uint32_t pktlen);
// Precondition: Enough headroom in front of `hdr` to fit a `vlan_tag_t`
ether_hdr_t* ether_hdr_tag_vlan(ether_hdr_t *hdr, uint32_t len, uint16_t vlanid, uint32_t *newlen=nullptr);
ether_hdr_t* ether_hdr_untag_vlan(ether_hdr_t *hdr, uint32_t len, uint32_t *newlen=nullptr);

#pragma mark -

// Layer 2 processing

bool layer2_qualify_recv_frame_on_interface(interface_t *i, ether_hdr_t *hdr, uint16_t *vlan_id);
bool layer2_qualify_send_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr);
int layer2_node_recv_frame_bytes(node_t *n, interface_t *i, uint8_t *frame, uint32_t framelen);

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

#pragma mark -

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
