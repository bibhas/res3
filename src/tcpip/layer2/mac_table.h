// mac.h

#pragma once

#include "glthread.h"
#include "utils.h"
#include "config.h"

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct ether_hdr_t ether_hdr_t;

#pragma mark -

// mac table

typedef struct mac_entry_t mac_entry_t;
typedef struct mac_table_t mac_table_t;

struct mac_table_t {
  glthread_t mac_entries;
};

struct mac_entry_t {
  mac_addr_t mac_addr;
  char oif_name[CONFIG_IF_NAME_SIZE];
  glthread_t mac_table_glue;
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  mac_entry_ptr_from_mac_table_glue,    // fn name
  mac_entry_t,                          // return type
  mac_table_glue                        // glthread_t field in mac_entry_t
);

#define MAC_ENTRY_PTR_KEYS_ARE_EQUAL(MAC0, MAC1) \
  ((MAC0)->mac_addr.value == (MAC1)->mac_addr.value)

#define MAC_ENTRY_PTRS_ARE_EQUAL(MAC0, MAC1) \
  MAC_ENTRY_PTR_KEYS_ARE_EQUAL(MAC0, MAC1) && \
  (strncmp((char *)(MAC0)->oif_name, (char *)(MAC1)->oif_name, CONFIG_IF_NAME_SIZE) == 0)

void mac_table_init(mac_table_t **t);
bool mac_table_lookup(mac_table_t *t, mac_addr_t *addr, mac_entry_t **out);
bool mac_table_add_entry(mac_table_t *t, mac_entry_t *entry);
bool mac_table_delete_entry(mac_table_t *t, mac_addr_t *addr);
bool mac_table_clear(mac_table_t *t);
void mac_table_dump(mac_table_t *t);
bool mac_table_process_reply(mac_table_t *t, ether_hdr_t *ether_hdr, interface_t *intf);

