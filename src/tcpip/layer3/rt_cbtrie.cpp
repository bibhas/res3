// rt_cbtrie.cpp
// Routing table implementation using compressed binary trie

#include <algorithm>
#include "rt.h"
#include "graph.h"
#include "glthread.h"
#include "utils.h"

#define RT_RADIX 2
#define RT_NODE_DESCENT_LEFT 0
#define RT_NODE_DESCENT_RIGHT 1

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

static inline rt_node_t* rt_node_allocate(rt_t *t, uint32_t prefix, uint8_t mask, rt_entry_t *entry = nullptr) {
  auto resp = (rt_node_t *)calloc(1, sizeof(rt_node_t));
  resp->prefix = prefix;
  resp->prefixlen = mask;
  resp->entry = entry;
  if (entry != nullptr) {
    // Registering entries here is ok because once a node is marked as key, it
    // cannot be demoted to an intermediary node UNLESS it is deleted (which
    // will automatically take care of unregistering said entry).
    glthread_init(&entry->rt_glue);
    glthread_add_next(&t->entries, &entry->rt_glue);
  }
  return resp;
}

static inline void rt_node_entry_deallocate(rt_t *t, rt_entry_t **entry_ptr) {
  glthread_remove(&((*entry_ptr)->rt_glue));
  free(*entry_ptr);
  *entry_ptr = nullptr;
}

static inline void rt_node_deallocate(rt_t *t, rt_node_t **node_ptr) {
  if (!node_ptr || !*node_ptr) {
    return;
  }
  if ((*node_ptr)->entry != nullptr) {
    rt_node_entry_deallocate(t, &((*node_ptr)->entry));
  }
  free(*node_ptr);
  *node_ptr = nullptr;
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
  uint32_t _prefix = htonl(entry->prefix.addr.value);
  uint32_t __prefix = ntohl(entry->prefix.addr.value);
  uint32_t entry_prefix = UINT32_MASK(htonl(entry->prefix.addr.value), entry->prefix.mask);
  uint32_t entry_mask = entry->prefix.mask;
  if (t->root_node == nullptr) {
    t->root_node = rt_node_allocate(t, entry_prefix, entry_mask, entry);
    return true;
  }
  rt_node_t *curr_node = t->root_node;
  uint8_t curr_node_descent = RT_NODE_DESCENT_LEFT;
  rt_node_t *parent_node = nullptr;
  for (;;) {
    int i = 0;
    for (; i < std::min(curr_node->prefixlen, entry_mask); i++) {
      if (UINT32_READ_BIT(curr_node->prefix, i) != UINT32_READ_BIT(entry_prefix, i)) {
        break; // divergence detected at `i`
      }
    }
    // We have three cases to consider:
    //  - case 1 : divergence happens in [0, curr_node->prefixlen)
    //  - case 2 : or, entry_prefix == curr_node->prefix
    //  - case 3 : or, divergence happens in [curr_node->prefixlen, ...)
    if (i < curr_node->prefixlen) {
      // Perform splitting
      bool splits_to_key_node = (i == entry_mask);
      uint32_t node_prefix = UINT32_MASK(entry_prefix, i);
      rt_node_t *split_node = rt_node_allocate(t, node_prefix, i, (splits_to_key_node ? entry : nullptr));
      uint32_t diverged_bitval = UINT32_READ_BIT(curr_node->prefix, i);
      split_node->child_nodes[diverged_bitval] = curr_node;
      if (i < entry_mask) {
        split_node->child_nodes[!diverged_bitval] = rt_node_allocate(t, entry_prefix, entry_mask, entry);
      }
      curr_node = split_node;
      if (parent_node) {
        parent_node->child_nodes[curr_node_descent] = split_node;
      }
      else {
        t->root_node = split_node;
      }
      return true;
    }
    else if (i == curr_node->prefixlen && i == entry_mask) {
      curr_node->entry = entry;
      // Normally, `rt_node_allocate` indirectly registers entries, but here we
      // do so manually since we're reusing an existing node.
      glthread_init(&entry->rt_glue);
      glthread_add_next(&t->entries, &entry->rt_glue);
      return true;
    }
    else {
      uint32_t next_bitval = UINT32_READ_BIT(entry_prefix, i);
      if (curr_node->child_nodes[next_bitval] == nullptr) {
        curr_node->child_nodes[next_bitval] = rt_node_allocate(t, entry_prefix, entry_mask, entry);
        return true;
      }
      parent_node = curr_node;
      curr_node = curr_node->child_nodes[next_bitval];
      curr_node_descent = next_bitval;
    }
  }
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

bool rt_delete_entry(rt_t *t, ipv4_addr_t *entry_addr, uint8_t entry_mask) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(entry_addr != nullptr, "Empty destination ip address param", false);
  if (!t->root_node) { return false; }
  rt_node_t *curr_node = t->root_node;
  rt_node_t **parent_node_ptr_stack[33] = {0};
  int32_t parent_node_ptr_stack_depth = 0;
  parent_node_ptr_stack[parent_node_ptr_stack_depth++] = &t->root_node;
  uint32_t entry_prefix = UINT32_MASK(htonl(entry_addr->value), (uint32_t)entry_mask);
  for (;;) {
    for (int i = 0; i < std::min(curr_node->prefixlen, (uint32_t)entry_mask); i++) {
      if (UINT32_READ_BIT(curr_node->prefix, i) != UINT32_READ_BIT(entry_prefix, i)) {
        return false;
      }
    }
    if (curr_node->prefix == entry_prefix && curr_node->prefixlen == entry_mask) {
      if (curr_node->entry == nullptr) {
        return true;
      }
      if (rt_node_count_children(curr_node) == 0) {
        rt_node_t **parent_ptr = parent_node_ptr_stack[parent_node_ptr_stack_depth - 1];
        *parent_ptr = nullptr;
        rt_node_deallocate(t, &curr_node);
      }
      else if (rt_node_count_children(curr_node) > 1) {
        if (curr_node->entry != nullptr) {
          rt_node_entry_deallocate(t, &curr_node->entry);
        }
        return true;
      }
      else {
        rt_node_entry_deallocate(t, &curr_node->entry);
      }
      // Compress
      while (parent_node_ptr_stack_depth > 0) {
        rt_node_t **parent_ptr = parent_node_ptr_stack[--parent_node_ptr_stack_depth];
        rt_node_t *parent_node = *parent_ptr;
        if (parent_node == nullptr) { continue; }
        if (parent_node->entry != nullptr) { continue; }
        rt_node_t *child_node = nullptr;
        if (rt_node_count_children(parent_node, &child_node) == 1) {
          *parent_ptr = child_node;
          rt_node_deallocate(t, &parent_node);
        }
      }
      return true;
    }
    if (entry_mask < curr_node->prefixlen) {
      return false;
    }
    uint32_t diverged_bitval = UINT32_READ_BIT(entry_prefix, curr_node->prefixlen);
    if (curr_node->child_nodes[diverged_bitval] != nullptr) {
      parent_node_ptr_stack[parent_node_ptr_stack_depth++] = &curr_node->child_nodes[diverged_bitval];
      curr_node = curr_node->child_nodes[diverged_bitval];
    }
    else {
      return false; // Avoids looping forever (if we can't find the right node)
    }
  }
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
  uint32_t query_prefix = UINT32_MASK(htonl(addr->value), mask);
  uint32_t query_mask = mask;
  if (t->root_node == nullptr) {
    return false;
  }
  rt_node_t *curr_node = t->root_node;
  for (;;) {
    int i = 0;
    for (; i < std::min(curr_node->prefixlen, query_mask); i++) {
      if (UINT32_READ_BIT(curr_node->prefix, i) != UINT32_READ_BIT(query_prefix, i)) {
        return false;
      }
    }
    if (curr_node->prefix == query_prefix && curr_node->prefixlen == query_mask) {
      if (curr_node->entry == nullptr) {
        return false; // Intermediary node
      }
      *resp = curr_node->entry;
      return true;
    }
 
      uint32_t diverged_bitval = UINT32_READ_BIT(query_prefix, i);
      curr_node = curr_node->child_nodes[diverged_bitval];
      if (!curr_node) {
        return false;
      }

  } 
}

bool rt_lookup(rt_t *t, ipv4_addr_t *addr, rt_entry_t **resp) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address param", false);
  EXPECT_RETURN_BOOL(resp != nullptr, "Empty resp entry ptr ptr param", false);
  uint32_t query_prefix = htonl(addr->value);
  if (t->root_node == nullptr) {
    return false;
  }
  rt_node_t *curr_node = t->root_node;
  rt_node_t *candidate_stack[33] = {0};
  int16_t candidate_stack_depth = -1;
  for (;;) {
    int i = 0;
    for (; i < curr_node->prefixlen; i++) {
      if (UINT32_READ_BIT(curr_node->prefix, i) != UINT32_READ_BIT(query_prefix, i)) {
        goto ret_false;
      }
    }
    if (curr_node->entry != nullptr) {
      candidate_stack[++candidate_stack_depth] = curr_node;
    }
    uint32_t diverged_bitval = UINT32_READ_BIT(query_prefix, i);
    curr_node = curr_node->child_nodes[diverged_bitval];
    if (!curr_node) {
      goto ret_false;
    }
  } 
ret_false:
  if (candidate_stack_depth > -1) {
    *resp = candidate_stack[candidate_stack_depth]->entry;
    return true;
  }
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
