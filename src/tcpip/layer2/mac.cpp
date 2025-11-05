// mac.cpp

#include <arpa/inet.h>
#include "graph.h"
#include "net.h"
#include "mac.h"
#include "layer2.h"
#include "phy.h"

#pragma mark -

// MAC table

void mac_table_init(mac_table_t **t) {
  EXPECT_RETURN(t != nullptr, "Empty table ptr param");
  auto resp = (mac_table_t *)calloc(1, sizeof(mac_table_t));
  glthread_init(&resp->mac_entries);
  *t = resp;
}

bool mac_table_lookup(mac_table_t *t, mac_addr_t *addr, mac_entry_t **out) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty mac address param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->mac_entries, curr) {
    mac_entry_t *e = mac_entry_ptr_from_mac_table_glue(curr);
    if (MAC_ADDR_PTR_IS_EQUAL(&e->mac_addr, addr)) {
      *out = e;
      return true;
    }
  }
  GLTHREAD_FOREACH_END();
  *out = nullptr;
  return false;
}

bool mac_table_add_entry(mac_table_t *t, mac_entry_t *entry) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(entry != nullptr, "Empty mac param", false);
  mac_entry_t *__entry = nullptr;
  if (mac_table_lookup(t, &entry->mac_addr, &__entry)) {
    if (MAC_ENTRY_PTR_KEYS_ARE_EQUAL(entry, __entry)) {
      // Table already contains entry with the same mac_addr primary key
      // Just update it
      strncpy(__entry->oif_name, entry->oif_name, CONFIG_IF_NAME_SIZE);
      return true;
    }
  }
  auto owned_entry = (mac_entry_t *)calloc(1, sizeof(mac_entry_t));
  memcpy((void *)owned_entry, (void *)entry, sizeof(mac_entry_t));
  glthread_init(&owned_entry->mac_table_glue);
  glthread_add_next(&t->mac_entries, &owned_entry->mac_table_glue);
  return true;
}

bool mac_table_delete_entry(mac_table_t *t, mac_addr_t *addr) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty mac address param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->mac_entries, curr) {
    mac_entry_t *entry = mac_entry_ptr_from_mac_table_glue(curr);
    if (!MAC_ADDR_PTR_IS_EQUAL(&entry->mac_addr, addr)) {
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

bool mac_table_clear(mac_table_t *t) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->mac_entries, curr) {
    mac_entry_t *entry = mac_entry_ptr_from_mac_table_glue(curr); 
    // This is ok to do since curr is never the head of the thread (the
    // glthread_t instance held by the owner).
    bool resp = glthread_remove(curr);
    EXPECT_RETURN_BOOL(resp == true, "glthread_remove failed", false);
    free(entry);
  }
  GLTHREAD_FOREACH_END();
  return true; // All entries deleted
}

void mac_table_dump(mac_table_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty table param");
  glthread_t *curr = nullptr;
  GLTHREAD_FOREACH_BEGIN(&t->mac_entries, curr) {
    mac_entry_t *entry = mac_entry_ptr_from_mac_table_glue(curr); 
    dump_line(
      "MAC: " MAC_ADDR_FMT ", OIF: %s\n",
      MAC_ADDR_BYTES_BE(entry->mac_addr),
      entry->oif_name
    );
  }
  GLTHREAD_FOREACH_END();
}

bool mac_table_process_reply(mac_table_t *t, ether_hdr_t *ether_hdr, interface_t *intf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(ether_hdr != nullptr, "Empty header param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Fill out entry fields
  mac_entry_t entry = {0};
  entry.mac_addr = ether_hdr_read_src_mac(ether_hdr);
  strncpy((char *)entry.oif_name, (char *)intf->if_name, CONFIG_IF_NAME_SIZE);
  glthread_init(&entry.mac_table_glue);
  // Add entry to thread (function will memcpy entry fields)
  bool resp = mac_table_add_entry(t, &entry);
  EXPECT_RETURN_BOOL(resp == true, "mac_table_add_entry failed", false);
  return true;
}
