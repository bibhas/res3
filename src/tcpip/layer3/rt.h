// rt.h

#pragma once

#include "utils.h"
#include "config.h"
#include "glthread.h"

typedef struct rt_t rt_t;
typedef struct rt_entry_t rt_entry_t;
typedef struct interface_t interface_t;

// Routing Table

void rt_init(rt_t **t);
bool rt_add_direct_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask);
bool rt_add_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask, ipv4_addr_t *gw_addr, interface_t *ointf, bool is_direct = false);
bool rt_lookup(rt_t *t, ipv4_addr_t *addr, rt_entry_t **entry);
bool rt_lookup_exact(rt_t *t, ipv4_addr_t *addr, uint8_t mask, rt_entry_t **resp);
bool rt_delete_entry(rt_t *t, ipv4_addr_t *addr, uint8_t mask);
bool rt_clear(rt_t *t);
void rt_dump(rt_t *t);

// Routing Table entry

ipv4_addr_t* rt_entry_get_prefix_ip(rt_entry_t *entry);
uint8_t rt_entry_get_prefix_mask(rt_entry_t *entry);
bool rt_entry_is_direct(rt_entry_t *entry);
bool rt_entry_oif_is_configured(rt_entry_t *entry);
const char* rt_entry_get_oif_name(rt_entry_t *entry);
bool rt_entry_gw_is_configured(rt_entry_t *entry);
ipv4_addr_t* rt_entry_get_gw_ip(rt_entry_t *entry);
