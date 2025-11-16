// rt.h

#pragma once

#include "utils.h"
#include "config.h"
#include "glthread.h"

typedef struct rt_t rt_t;
typedef struct rt_entry_t rt_entry_t;
typedef struct interface_t interface_t;

struct rt_t {
  glthread_t rt_entries;
};

struct rt_entry_t {
  struct {
    ipv4_addr_t ip;
    uint8_t mask;
  } prefix;
  bool is_direct;
  struct {
    ipv4_addr_t ip;
    bool configured;
  } gw;
  struct {
    char name[CONFIG_IF_NAME_SIZE];
    bool configured;
  } oif;
  glthread_t rt_glue;
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  rt_entry_ptr_from_rt_glue,      // fn name
  rt_entry_t,                     // return type
  rt_glue                         // glthread_t field in rt_t
);

void rt_init(rt_t **t);
bool rt_add_direct_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask);
bool rt_add_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask, ipv4_addr_t *gw_addr, interface_t *ointf, bool is_direct = false);
bool rt_lookup(rt_t *t, ipv4_addr_t *addr, rt_entry_t **entry);
bool rt_lookup_exact(rt_t *t, ipv4_addr_t *addr, uint8_t mask, rt_entry_t **resp);
bool rt_delete_entry(rt_t *t, ipv4_addr_t *addr, uint8_t mask);
bool rt_clear(rt_t *t);
void rt_dump(rt_t *t);



