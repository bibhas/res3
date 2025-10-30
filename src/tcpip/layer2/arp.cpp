// arp.cpp

#include "graph.h"
#include "net.h"
#include "arp.h"

#pragma mark -

// ARP table

void arp_table_init(arp_table_t **t) {
  EXPECT_RETURN(t != nullptr, "Empty table ptr param");
  auto resp = (arp_table_t *)malloc(sizeof(arp_table_t));
  glthread_init(&resp->arp_entries);
  *t = resp;
}

bool arp_table_lookup(arp_table_t *t, ipv4_addr_t *ip_addr, arp_entry_t **out) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->arp_entries, curr) {
    arp_entry_t *e = arp_entry_ptr_from_arp_table_glue(curr);
    if (IPV4_ADDR_PTR_IS_EQUAL(&e->ip_addr, ip_addr)) {
      *out = e;
      return true;
    }
  }
  GLTHREAD_FOREACH_END();
  *out = nullptr;
  return false;
}

bool arp_table_add_entry(arp_table_t *t, arp_entry_t *entry) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(entry != nullptr, "Empty entry param", false);
  return false;
}

bool arp_table_delete_entry(arp_table_t *t, ipv4_addr_t *ip_addr) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  return false;
}

bool arp_table_clear(arp_table_t *t) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  return false;
}

void arp_table_dump(arp_table_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty table param");
  
}

bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty header param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  return false;
}
