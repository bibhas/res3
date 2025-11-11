// rt.cpp

#include "rt.h"
#include "graph.h"
#include "utils.h"

void rt_init(rt_t **t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
  auto resp = (rt_t *)calloc(1, sizeof(rt_t));
  EXPECT_RETURN(resp != nullptr, "calloc failed");
  glthread_init(&resp->rt_entries);
  *t = resp;
}

bool rt_add_direct_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty addr param", false);
  auto entry = (rt_entry_t *)calloc(1, sizeof(rt_entry_t));
  bool resp = ipv4_addr_apply_mask(addr, mask, &entry->prefix.ip);
  entry->prefix.mask = mask;
  entry->is_direct = true;
  // Other fields are zeroed thanks to `calloc`
  glthread_init(&entry->rt_glue);
  glthread_add_next(&t->rt_entries, &entry->rt_glue);
  return true;
}

bool rt_add_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask, ipv4_addr_t *gw_ip, interface_t *ointf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty addr param", false);
  EXPECT_RETURN_BOOL(gw_ip != nullptr, "Empty gateway address param", false);
  EXPECT_RETURN_BOOL(ointf != nullptr, "Empty out interface ptr param", false);
  auto entry = (rt_entry_t *)calloc(1, sizeof(rt_entry_t));
  bool resp = ipv4_addr_apply_mask(addr, mask, &entry->prefix.ip);
  entry->prefix.mask = mask;
  entry->is_direct = false;
  entry->gw_ip = {.value = gw_ip->value};
  strncpy((char *)entry->oif_name, (char *)ointf->if_name, CONFIG_IF_NAME_SIZE);
  glthread_init(&entry->rt_glue);
  glthread_add_next(&t->rt_entries, &entry->rt_glue);
  return true;
}

bool rt_lookup(rt_t *t, ipv4_addr_t *addr, rt_entry_t **resp) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address param", false);
  EXPECT_RETURN_BOOL(resp != nullptr, "Empty resp entry ptr ptr param", false);
  *resp = nullptr;
  // TODO: Use a Trie to implement efficient searching
  uint8_t max_mask = 0;
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->rt_entries, curr) {
    rt_entry_t *entry = rt_entry_ptr_from_rt_glue(curr);
    ipv4_addr_t candidate;
    if (!ipv4_addr_apply_mask(addr, entry->prefix.mask, &candidate)) {
      continue; // Something happened, but we'll just ignore it
    }
    if (IPV4_ADDR_IS_EQUAL(candidate, entry->prefix.ip)) {
      if (max_mask < entry->prefix.mask) {
        max_mask = entry->prefix.mask;
        *resp = entry;
      }
    }
  }
  GLTHREAD_FOREACH_END();
  EXPECT_RETURN_BOOL(*resp != nullptr, "No entry found", false);
  return true;
}

bool rt_delete_entry(rt_t *t, ipv4_addr_t *addr, uint8_t mask) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty destination ip address param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->rt_entries, curr) {
    rt_entry_t *entry = rt_entry_ptr_from_rt_glue(curr);
    if (!IPV4_ADDR_PTR_IS_EQUAL(&entry->prefix.ip, addr)) {
      continue;
    }
    if (entry->prefix.mask != mask) {
      continue;
    }
    // This is ok to do since curr is never the head of the thread (the
    // glthread_t instance held by the owner).
    bool resp = glthread_remove(curr);
    EXPECT_RETURN_BOOL(resp == true, "glthread_remove failed", false);
    free(entry);
    return true;
  }
  GLTHREAD_FOREACH_END();
  return false; // Didn't find entry
}

bool rt_clear(rt_t *t) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->rt_entries, curr) {
    rt_entry_t *entry = rt_entry_ptr_from_rt_glue(curr); 
    // This is ok to do since curr is never the head of the thread (the
    // glthread_t instance held by the owner).
    bool resp = glthread_remove(curr);
    EXPECT_RETURN_BOOL(resp == true, "glthread_remove failed", false);
    free(entry);
  }
  GLTHREAD_FOREACH_END();
  return true; // All entries deleted
}

void rt_dump(rt_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->rt_entries, curr) {
    rt_entry_t *entry = rt_entry_ptr_from_rt_glue(curr);
    if (entry->is_direct) {
      dump_line(
        "Dest: " IPV4_ADDR_FMT "/%u, Direct: true, Gateway: -, OIF: -\n",
        IPV4_ADDR_BYTES_BE(entry->prefix.ip), entry->prefix.mask
      );
    } else {
      dump_line(
        "Dest: " IPV4_ADDR_FMT "/%u, Direct: false, Gateway: " IPV4_ADDR_FMT ", OIF: %s\n",
        IPV4_ADDR_BYTES_BE(entry->prefix.ip), entry->prefix.mask,
        IPV4_ADDR_BYTES_BE(entry->gw_ip),
        entry->oif_name
      );
    }
  }
  GLTHREAD_FOREACH_END();
}
