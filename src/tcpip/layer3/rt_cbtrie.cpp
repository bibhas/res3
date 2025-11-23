// rt_cbtrie.cpp
// Routing table implementation using compressed binary trie

#include "rt.h"
#include "graph.h"
#include "glthread.h"
#include "utils.h"

#define RT_RADIX 2

typedef struct rt_node_t rt_node_t;
typedef struct rt_t rt_t;
typedef struct rt_entry_t rt_entry_t;

#pragma mark -

// Structs

struct rt_t {
  rt_node_t *root_node = nullptr;
  glthread_t entries;
};

struct rt_node_t {
  uint32_t prefix;
  uint32_t prefixlen;
  rt_entry_t *entry = nullptr;
  rt_node_t *child_nodes[RT_RADIX];
};

struct rt_entry_t {
  struct {
    ipv4_addr_t addr;
    uint8_t mask;
  } prefix;
  struct {
    ipv4_addr_t addr;
    bool configured;
  } gw;
  struct {
    char name[CONFIG_IF_NAME_SIZE];
    bool configured;
  } oif;
  bool is_direct;
  glthread_t rt_glue;
};

DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  rt_entry_ptr_from_rt_glue,      // fn name
  rt_entry_t,                     // return type
  rt_glue                         // glthread_t field in rt_entry_t
);

#pragma mark -

// Functions

void rt_init(rt_t **t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
  auto resp = (rt_t *)calloc(1, sizeof(rt_t));
  EXPECT_RETURN(resp != nullptr, "calloc failed");
  resp->root_node = nullptr;
  glthread_init(&resp->entries);
  *t = resp;
}

static inline rt_node_t* rt_node_allocate(uint32_t prefix, uint8_t mask, rt_entry_t *entry = nullptr) {
  auto resp = (rt_node_t *)calloc(1, sizeof(rt_node_t));
  resp->prefix = prefix;
  resp->prefixlen = mask;
  resp->entry = entry;
  return resp;
}

static inline uint32_t rt_node_count_children(rt_node_t *n, rt_node_t **last_child = nullptr) {
  uint32_t resp = 0;
  for (int i = 0; i < RT_RADIX; i++) {
    if (n->child_nodes[i] != nullptr) {
      resp++;
      if (last_child != nullptr) {
        *last_child = n->child_nodes[i];
      }
    }
  }
  return resp;
}

static inline bool rt_insert_entry(rt_t *t, rt_entry_t *entry) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(entry != nullptr, "Empty entry param", false);
  // First, 
  return false;
}

bool rt_add_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask, ipv4_addr_t *gw, interface_t *ointf, bool is_direct) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty addr param", false);
  auto entry = (rt_entry_t *)calloc(1, sizeof(rt_entry_t));
  ipv4_addr_apply_mask(addr, mask, &entry->prefix.addr);
  entry->prefix.mask = mask;
  entry->is_direct = is_direct;
  if (gw != nullptr) {
    entry->gw.addr.value = gw->value;
    entry->gw.configured = true;
  }
  if (ointf != nullptr) {
    strncpy((char *)entry->oif.name, (char *)ointf->if_name, CONFIG_IF_NAME_SIZE);
    entry->oif.configured = true;
  }
  return rt_insert_entry(t, entry);
}

bool rt_add_direct_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask) {
  return rt_add_route(t, addr, mask, nullptr, nullptr, true);
}

bool rt_delete_entry(rt_t *t, ipv4_addr_t *addr, uint8_t mask) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty destination ip address param", false);
  // ...
  return false;
}

bool rt_clear(rt_t *t) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  // ...
  return false;
}

bool rt_lookup_exact(rt_t *t, ipv4_addr_t *addr, uint8_t mask, rt_entry_t **resp) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address param", false);
  EXPECT_RETURN_BOOL(resp != nullptr, "Empty resp ptr param", false);
  // ...
  return false;
}

bool rt_lookup(rt_t *t, ipv4_addr_t *addr, rt_entry_t **resp) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address param", false);
  EXPECT_RETURN_BOOL(resp != nullptr, "Empty resp entry ptr ptr param", false);
  // ...
  return false;
}


void rt_dump(rt_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
  // ...
}

#pragma mark -

// Accessor functions for rt_entry_t

ipv4_addr_t* rt_entry_get_prefix_ip(rt_entry_t *entry) {
  return &entry->prefix.addr;
}

uint8_t rt_entry_get_prefix_mask(rt_entry_t *entry) {
  return entry->prefix.mask;
}

bool rt_entry_is_direct(rt_entry_t *entry) {
  return entry->is_direct;
}

bool rt_entry_oif_is_configured(rt_entry_t *entry) {
  return entry->oif.configured;
}

const char* rt_entry_get_oif_name(rt_entry_t *entry) {
  return (const char*)entry->oif.name;
}

bool rt_entry_gw_is_configured(rt_entry_t *entry) {
  return entry->gw.configured;
}

ipv4_addr_t* rt_entry_get_gw_ip(rt_entry_t *entry) {
  return &entry->gw.addr;
}
