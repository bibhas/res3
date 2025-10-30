// arp.h

#pragma once

#include "glthread.h"
#include "utils.h"
#include "config.h"

typedef struct interface_t interface_t;

#pragma mark -

// ARP header

typedef struct arp_hdr_t arp_hdr_t;

struct __PACK__ arp_hdr_t {
  uint16_t hw_type;       // 1 = Ethernet
  uint16_t proto_type;    // 0x800 = IPv4
  uint8_t hw_addr_len;    // 6 = sizeof(mac_addr_t)
  uint8_t proto_addr_len; // 4 = sizeof(ipv4_addr_t)
  uint16_t op_code;       // 1 = request, 2 = reply
  uint8_t src_mac[6];     
  uint32_t src_ip;
  uint8_t dst_mac[6];     
  uint32_t dst_ip;        // Used only for request
};

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

void arp_table_init(arp_table_t **t);
bool arp_table_lookup(arp_table_t *t, ipv4_addr_t *ip_addr, arp_entry_t **out);
bool arp_table_add_entry(arp_table_t *t, arp_entry_t *entry);
bool arp_table_delete_entry(arp_table_t *t, ipv4_addr_t *ip_addr);
bool arp_table_clear(arp_table_t *t);
void arp_table_dump(arp_table_t *t);
bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf);

