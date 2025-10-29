// arp.h

#pragma once

#include "glthread.h"
#include "graph.h" // IF_NAME_SIZE
#include "utils.h"
#include "config.h"

#pragma mark -

// ARP

typedef struct arp_hdr_t arp_hdr_t;
typedef struct arp_entry_t arp_entry_t;
typedef struct arp_table_t arp_table_t;

struct __attribute__((packed)) arp_hdr_t {
  uint16_t hw_type;       // 1 = Ethernet
  uint16_t proto_type;    // 0x800 = IPv4
  uint8_t hw_addr_len;    // 6 = sizeof(mac_addr_t)
  uint8_t proto_addr_len; // 4 = sizeof(ipv4_addr_t)
  uint16_t op_code;       // 1 = request, 2 = reply
  mac_addr_t src_mac;     
  uint32_t src_ip;
  mac_addr_t dst_mac;     
  uint32_t dst_ip;        // Used only for request
};

struct arp_entry_t {
  ipv4_addr_t ip_addr;
  mac_addr_t mac_addr;
  char oif_name[CONFIG_IF_NAME_SIZE];
  glthread_t arp_glue;
};

struct arp_table_t {
  glthread_t arp_entries;
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  arp_entry_ptr_from_arp_glue,   // fn name
  arp_entry_t,                   // return type
  arp_glue                       // glthread_t* field in arp_entry_t
);

void arp_table_init(arp_table_t **t);

