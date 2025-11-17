// arp_table.h

#pragma once

#include <arpa/inet.h>
#include "glthread.h"
#include "utils.h"
#include "config.h"
#include "arp_hdr.h"

typedef struct arp_entry_t arp_entry_t;
typedef struct arp_table_t arp_table_t;
typedef struct arp_lookup_t arp_lookup_t;
typedef struct interface_t interface_t;

#pragma mark -

// ARP table

struct arp_table_t {
  glthread_t arp_entries;
};

struct arp_entry_t {
  ipv4_addr_t ip_addr;
  mac_addr_t mac_addr;
  char oif_name[CONFIG_IF_NAME_SIZE];
  glthread_t arp_table_glue;
  // ARP on Demand
  struct {
    bool is_resolved = true;
    glthread_t pending_lookups;
  } aod;
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

// ARP table

void arp_table_init(arp_table_t **t);
bool arp_table_lookup(arp_table_t *t, ipv4_addr_t *ip_addr, arp_entry_t **out);
bool arp_table_add_entry(arp_table_t *t, arp_entry_t *entry);
bool arp_table_add_unresolved_entry(arp_table_t *t, ipv4_addr_t *addr, arp_entry_t **entry);
bool arp_table_delete_entry(arp_table_t *t, ipv4_addr_t *ip_addr);
bool arp_table_clear(arp_table_t *t);
void arp_table_dump(arp_table_t *t);
bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf);

#pragma mark -

// Utils

static inline bool arp_entry_is_resolved(arp_entry_t *entry) {
  EXPECT_RETURN_BOOL(entry != nullptr, "Empty entry param", false);
  return entry->aod.is_resolved;
}

#pragma mark -

// ARP-on-demand

using arp_lookup_processing_fn = std::function<void(arp_entry_t*,arp_lookup_t*)>;

struct arp_lookup_t {
  glthread_t arp_entry_glue;
  arp_lookup_processing_fn cb;
  uint32_t bufflen;
  uint16_t vlan_id; // VLAN ID for tagging trunk frames (0 = no VLAN)
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  arp_lookup_ptr_from_arp_entry_glue,   // fn name
  arp_lookup_t,                         // return type
  arp_entry_glue                        // glthread_t field in arp_lookup_t
);

bool arp_entry_add_pending_lookup(arp_entry_t *e, uint8_t *pay, uint32_t paylen, arp_lookup_processing_fn cb, uint16_t vlan_id);

