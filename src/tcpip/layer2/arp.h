// arp.h

#pragma once

#include <arpa/inet.h>
#include "glthread.h"
#include "utils.h"
#include "config.h"

#define ARP_HW_TYPE_ETHERNET 1
#define ARP_OP_CODE_REQUEST 1
#define ARP_OP_CODE_REPLY 2

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct ether_hdr_t ether_hdr_t;

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

bool node_arp_send_broadcast_request(node_t *n, interface_t *ointf, ipv4_addr_t *ip_addr);
bool node_arp_recv_broadcast_request_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr);
bool node_arp_send_reply_frame(node_t *n, interface_t *oif, ether_hdr_t *hdr);
bool node_arp_recv_reply_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr);

#pragma mark -

// ARP table

typedef struct arp_entry_t arp_entry_t;
typedef struct arp_table_t arp_table_t;

struct arp_table_t {
  glthread_t arp_entries;
};

struct arp_entry_t {
  ipv4_addr_t ip_addr;
  mac_addr_t mac_addr;
  char oif_name[CONFIG_IF_NAME_SIZE];
  glthread_t arp_table_glue;
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  arp_entry_ptr_from_arp_table_glue,    // fn name
  arp_entry_t,                          // return type
  arp_table_glue                        // glthread_t field in arp_entry_t
);

#define ARP_ENTRY_PTR_KEYS_ARE_EQUAL(ARP0, ARP1) \
  ((ARP0)->ip_addr.value == (ARP1)->ip_addr.value) && \
  (strncmp((char *)(ARP0)->oif_name, (char *)(ARP1)->oif_name, CONFIG_IF_NAME_SIZE) == 0)

#define ARP_ENTRY_PTRS_ARE_EQUAL(ARP0, ARP1) \
  ARP_ENTRY_PTR_KEYS_ARE_EQUAL(ARP0, ARP1) && \
  ((ARP0)->mac_addr.value == (ARP1)->mac_addr.value)

void arp_table_init(arp_table_t **t);
bool arp_table_lookup(arp_table_t *t, ipv4_addr_t *ip_addr, arp_entry_t **out);
bool arp_table_add_entry(arp_table_t *t, arp_entry_t *entry);
bool arp_table_delete_entry(arp_table_t *t, ipv4_addr_t *ip_addr);
bool arp_table_clear(arp_table_t *t);
void arp_table_dump(arp_table_t *t);
bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf);

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
