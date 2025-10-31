// arp.cpp

#include "graph.h"
#include "net.h"
#include "arp.h"

#pragma mark -

bool arp_send_broadcast_request(node_t *n, interface_t *ointf, ipv4_addr_t *ip_addr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(ointf != nullptr, "Empty output interface param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  // ...
  return false;
}

#pragma mark -

// ARP table

void arp_table_init(arp_table_t **t) {
  EXPECT_RETURN(t != nullptr, "Empty table ptr param");
  auto resp = (arp_table_t *)calloc(1, sizeof(arp_table_t));
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
  arp_entry_t *__entry = nullptr;
  if (arp_table_lookup(t, &entry->ip_addr, &__entry)) {
    if (ARP_ENTRY_PTR_KEYS_ARE_EQUAL(entry, __entry)) {
      // Table already contains entry with the same (ip, intf) primary key
      return false;
    }
  }
  auto owned_entry = (arp_entry_t *)calloc(1, sizeof(arp_entry_t));
  memcpy((void *)owned_entry, (void *)entry, sizeof(arp_entry_t));
  glthread_init(&owned_entry->arp_table_glue);
  glthread_add_next(&t->arp_entries, &owned_entry->arp_table_glue);
  return true;
}

bool arp_table_delete_entry(arp_table_t *t, ipv4_addr_t *ip_addr) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->arp_entries, curr) {
    arp_entry_t *entry = arp_entry_ptr_from_arp_table_glue(curr);
    if (!IPV4_ADDR_PTR_IS_EQUAL(&entry->ip_addr, ip_addr)) {
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

bool arp_table_clear(arp_table_t *t) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->arp_entries, curr) {
    arp_entry_t *entry = arp_entry_ptr_from_arp_table_glue(curr); 
    // This is ok to do since curr is never the head of the thread (the
    // glthread_t instance held by the owner).
    bool resp = glthread_remove(curr);
    EXPECT_RETURN_BOOL(resp == true, "glthread_remove failed", false);
    free(entry);
  }
  GLTHREAD_FOREACH_END();
  return true; // All entries deleted
}

void arp_table_dump(arp_table_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty table param");
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->arp_entries, curr) {
    arp_entry_t *entry = arp_entry_ptr_from_arp_table_glue(curr); 
    dump_line(
      "IP: " IPV4_ADDR_FMT ", MAC: " MAC_ADDR_FMT ", OIF: %s\n",
      IPV4_ADDR_BYTES_BE(entry->ip_addr), MAC_ADDR_BYTES_BE(entry->mac_addr),
      entry->oif_name
    );
  }
  GLTHREAD_FOREACH_END();
}

bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty header param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Fill out entry fields
  arp_entry_t entry = {0};
  entry.ip_addr = {.value = hdr->src_ip};    // network byte order
  entry.mac_addr = {.bytes = {BYTES_FROM_BYTEARRAY_BE(hdr->src_mac)}};
  strncpy((char *)entry.oif_name, (char *)intf->if_name, CONFIG_IF_NAME_SIZE);
  glthread_init(&entry.arp_table_glue);
  // Add entry to thread (function will memcpy entry fields)
  bool resp = arp_table_add_entry(t, &entry);
  EXPECT_RETURN_BOOL(resp == true, "arp_table_add_entry failed", false);
  return true;
}
