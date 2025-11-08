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
