// arp_hdr.h

#pragma once

#include <arpa/inet.h>
#include "glthread.h"
#include "utils.h"
#include "config.h"

#define ARP_HW_TYPE_ETHERNET 1
#define ARP_OP_CODE_REQUEST 1
#define ARP_OP_CODE_REPLY 2

#pragma mark -

// ARP header

typedef struct arp_hdr_t arp_hdr_t;

struct __PACK__ arp_hdr_t {
  uint16_t  hw_type;        // 1 = Ethernet
  uint16_t  proto_type;     // 0x800 = IPv4
  uint8_t   hw_addr_len;    // 6 = sizeof(mac_addr_t)
  uint8_t   proto_addr_len; // 4 = sizeof(ipv4_addr_t)
  uint16_t  op_code;        // 1 = request, 2 = reply
  uint8_t   src_mac[6];     
  uint32_t  src_ip;
  uint8_t   dst_mac[6];     
  uint32_t  dst_ip;         // Used only for request
};

#pragma mark -

static inline uint16_t arp_hdr_read_hw_type(const arp_hdr_t *hdr) {
  return ntohs(hdr->hw_type);
}

static inline void arp_hdr_set_hw_type(arp_hdr_t *hdr, uint16_t val) {
  hdr->hw_type = htons(val);
}

static inline uint16_t arp_hdr_read_proto_type(const arp_hdr_t *hdr) {
  return ntohs(hdr->proto_type);
}

static inline void arp_hdr_set_proto_type(arp_hdr_t *hdr, uint16_t val) {
  hdr->proto_type = htons(val);
}

static inline uint8_t arp_hdr_read_hw_addr_len(const arp_hdr_t *hdr) {
  return hdr->hw_addr_len;
}

static inline void arp_hdr_set_hw_addr_len(arp_hdr_t *hdr, uint8_t val) {
  hdr->hw_addr_len = val;
}

static inline uint8_t arp_hdr_read_proto_addr_len(const arp_hdr_t *hdr) {
  return hdr->proto_addr_len;
}

static inline void arp_hdr_set_proto_addr_len(arp_hdr_t *hdr, uint8_t val) {
  hdr->proto_addr_len = val;
}

static inline uint16_t arp_hdr_read_op_code(const arp_hdr_t *hdr) {
  return ntohs(hdr->op_code);
}

static inline void arp_hdr_set_op_code(arp_hdr_t *hdr, uint16_t val) {
  hdr->op_code = htons(val);
}

static inline mac_addr_t arp_hdr_read_src_mac(const arp_hdr_t *hdr) {
  mac_addr_t mac = {0};
  memcpy(mac.bytes, hdr->src_mac, 6);
  return mac;
}

static inline void arp_hdr_set_src_mac(arp_hdr_t *hdr, const mac_addr_t *mac) {
  memcpy(hdr->src_mac, mac->bytes, 6);
}

static inline mac_addr_t arp_hdr_read_dst_mac(const arp_hdr_t *hdr) {
  mac_addr_t mac = {0};
  memcpy(mac.bytes, hdr->dst_mac, 6);
  return mac;
}

static inline void arp_hdr_set_dst_mac(arp_hdr_t *hdr, const mac_addr_t *mac) {
  memcpy(hdr->dst_mac, mac->bytes, 6);
}

static inline ipv4_addr_t arp_hdr_read_src_ip(const arp_hdr_t *hdr) {
  return (ipv4_addr_t){.value = ntohl(hdr->src_ip)};
};

static inline void arp_hdr_set_src_ip(arp_hdr_t *hdr, uint32_t ip) {
  hdr->src_ip = htonl(ip);
}

static inline void arp_hdr_set_src_ip(arp_hdr_t *hdr, ipv4_addr_t *ip) {
  hdr->src_ip = htonl(ip->value);
}

static inline ipv4_addr_t arp_hdr_read_dst_ip(const arp_hdr_t *hdr) {
  return (ipv4_addr_t){.value = ntohl(hdr->dst_ip)};
}

static inline void arp_hdr_set_dst_ip(arp_hdr_t *hdr, uint32_t ip) {
  hdr->dst_ip = htonl(ip);
}

static inline void arp_hdr_set_dst_ip(arp_hdr_t *hdr, ipv4_addr_t *ip) {
  hdr->dst_ip = htonl(ip->value);
}
