// ipv4.h

#pragma once

#include <cstdint>
#include <arpa/inet.h>
#include <functional>
#include "utils.h"
#include "rt.h"

typedef struct node_t node_t;
typedef struct interface_t interface_t;

#pragma mark -

// IPv4

typedef struct ipv4_hdr_t ipv4_hdr_t;

struct __PACK__ ipv4_hdr_t {
  uint8_t   version_ihl;
  uint8_t   dscp_ecn;
  uint16_t  total_length;
  uint16_t  packet_id;
  uint16_t  fragment_offset;
  uint8_t   ttl;
  uint8_t   protocol;
  uint16_t  checksum;
  uint32_t  src_addr;
  uint32_t  dst_addr;
};

#define IPV4_HDR_LEN_BYTES(HDRPTR) (ipv4_hdr_read_ihl(HDRPTR) * 4)
#define IPV4_HDR_PAYLOAD_SIZE(HDRPTR) (HDRPTR) \
  ipv4_hdr_read_total_length(HDRPTR) - IPV4_HDR_LEN_BYTES(HDRPTR) 

#pragma mark -

// Layer 3

#define PROT_ICMP   1
#define PROT_IPIP   4
#define PROT_UDP    17

using layer3_promote_fn_t = std::function<void(node_t*,interface_t*,uint8_t*,uint32_t,uint16_t)>;
using layer3_demote_fn_t = std::function<void(node_t*,uint8_t*,uint32_t,uint8_t,ipv4_addr_t*)>;

// Not to be called directly (use node's netstack function pointers instead)

void __layer3_promote(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen, uint16_t ether_type);
void __layer3_demote(node_t *n, uint8_t *pkt, uint32_t pktlen, uint8_t prot, ipv4_addr_t *dst_addr);

// Utils

bool layer3_resolve_next_hop(node_t *n, ipv4_addr_t *dst_addr, ipv4_addr_t **hop_addr, interface_t **ointf);
bool layer3_resolve_src_for_dst(node_t *n, ipv4_addr_t *dst_addr, ipv4_addr_t **src_addr, interface_t **ointf = nullptr);

#pragma mark -

// IP-in-IP encapsulation

void layer3_demote_ipnip(node_t *n, uint8_t *pkt, uint32_t pktlen, uint8_t prot, ipv4_addr_t *dst_addr, ipv4_addr_t *ero_addr);
bool layer3_forward_ipnp(node_t *n, uint8_t *payload, uint32_t paylen);

#pragma mark -

// Accessors for ipv4_hdr_t

static inline uint8_t ipv4_hdr_read_version(ipv4_hdr_t *hdr) {
  return (hdr->version_ihl >> 4) & 0xF;
}

static inline void ipv4_hdr_set_version(ipv4_hdr_t *hdr, uint8_t val) {
  hdr->version_ihl = (hdr->version_ihl & 0x0F) | ((val & 0x0F) << 4);
}

static inline uint8_t ipv4_hdr_read_ihl(ipv4_hdr_t *hdr) {
  return hdr->version_ihl & 0xF;
}

static inline void ipv4_hdr_set_ihl(ipv4_hdr_t *hdr, uint8_t val) {
  hdr->version_ihl = (hdr->version_ihl & 0xF0) | (val & 0x0F);
}

static inline uint16_t ipv4_hdr_read_total_length(ipv4_hdr_t *hdr) {
  return ntohs(hdr->total_length);
}

static inline void ipv4_hdr_set_total_length(ipv4_hdr_t *hdr, uint16_t val) {
  hdr->total_length = htons(val);
}

static inline uint16_t ipv4_hdr_read_packet_id(ipv4_hdr_t *hdr) {
  return htons(hdr->packet_id);
}

static inline void ipv4_hdr_set_packet_id(ipv4_hdr_t *hdr, uint16_t val) {
  hdr->packet_id = htons(val);
}

static inline uint8_t ipv4_hdr_read_flags(ipv4_hdr_t *hdr) {
  return (ntohs(hdr->fragment_offset) >> 13) & 0x7;
}

static inline void ipv4_hdr_set_flags(ipv4_hdr_t *hdr, uint8_t val) {
  hdr->fragment_offset = htons((ntohs(hdr->fragment_offset) & 0x1FFF) | ((val & 0x07) << 13));
}

static inline uint16_t ipv4_hdr_read_fragment_offset(ipv4_hdr_t *hdr) {
  return ntohs(hdr->fragment_offset) & 0x1FFF;
}

static inline void ipv4_hdr_set_fragment_offset(ipv4_hdr_t *hdr, uint16_t val) {
  hdr->fragment_offset = htons((ntohs(hdr->fragment_offset) & 0xE000) | (val & 0x1FFF));
}

static inline uint8_t ipv4_hdr_read_ttl(ipv4_hdr_t *hdr) {
  return hdr->ttl;
}

static inline void ipv4_hdr_set_ttl(ipv4_hdr_t *hdr, uint8_t val) {
  hdr->ttl = val;
}

static inline uint8_t ipv4_hdr_read_protocol(ipv4_hdr_t *hdr) {
  return hdr->protocol;
}

static inline void ipv4_hdr_set_protocol(ipv4_hdr_t *hdr, uint8_t val) {
  hdr->protocol = val;
}

static inline uint16_t ipv4_hdr_read_checksum(ipv4_hdr_t *hdr) {
  return ntohs(hdr->checksum);
}

static inline void ipv4_hdr_set_checksum(ipv4_hdr_t *hdr, uint16_t val) {
  hdr->checksum = htons(val);
}

static inline ipv4_addr_t ipv4_hdr_read_src_addr(ipv4_hdr_t *hdr) {
  return (ipv4_addr_t){.value = ntohl(hdr->src_addr)};
}

static inline void ipv4_hdr_set_src_addr(ipv4_hdr_t *hdr, uint32_t addr) {
  hdr->src_addr = htonl(addr);
}

static inline void ipv4_hdr_set_src_addr(ipv4_hdr_t *hdr, ipv4_addr_t *addr) {
  hdr->src_addr = htonl(addr->value);
}

static inline ipv4_addr_t ipv4_hdr_read_dst_addr(ipv4_hdr_t *hdr) {
  return (ipv4_addr_t){.value = ntohl(hdr->dst_addr)};
}

static inline void ipv4_hdr_set_dst_addr(ipv4_hdr_t *hdr, uint32_t addr) {
  hdr->dst_addr = htonl(addr);
}

static inline void ipv4_hdr_set_dst_addr(ipv4_hdr_t *hdr, ipv4_addr_t *addr) {
  hdr->dst_addr = htonl(addr->value);
}

