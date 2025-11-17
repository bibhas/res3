// arp_table.cpp

#include <arpa/inet.h>
#include "graph.h"
#include "net.h"
#include "arp_table.h"
#include "layer2.h"
#include "phy.h"

// ARP table

bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty header param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  arp_entry_t *arp_entry = nullptr;
  ipv4_addr_t ip_addr = arp_hdr_read_src_ip(hdr);
  if (!arp_table_lookup(t, &ip_addr, &arp_entry)) {
    // We got an arp reply for request we didn't put out
    // This is normal in switched networks (flooded ARP replies)
    return true;
  }
  EXPECT_RETURN_BOOL(arp_entry_is_resolved(arp_entry) == false, "Got reply for resolved entry", false);
  // Fil out entry with new new info in the reply
  arp_entry->mac_addr = arp_hdr_read_src_mac(hdr);
  glthread_t *curr = nullptr;
  strncpy((char *)arp_entry->oif_name, (char *)intf->if_name, CONFIG_IF_NAME_SIZE);
  //printf("[%s] arp_table_process_reply got for intf: %s\n", intf->att_node->node_name, intf->if_name);
  // Process all pending lookups
  GLTHREAD_FOREACH_BEGIN(&arp_entry->aod.pending_lookups, curr) {
    arp_lookup_t *lookup = arp_lookup_ptr_from_arp_entry_glue(curr);
    lookup->cb(arp_entry, lookup);
    glthread_remove(&lookup->arp_entry_glue);
    free(lookup);
  }
  GLTHREAD_FOREACH_END();
  arp_entry->aod.is_resolved = true;
  return true;
}

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
      // Just update it
      __entry->mac_addr = entry->mac_addr;
      return true;
    }
  }
  auto owned_entry = (arp_entry_t *)calloc(1, sizeof(arp_entry_t));
  memcpy((void *)owned_entry, (void *)entry, sizeof(arp_entry_t));
  glthread_init(&owned_entry->arp_table_glue);
  glthread_add_next(&t->arp_entries, &owned_entry->arp_table_glue);
  return true;
}

bool arp_table_add_unresolved_entry(arp_table_t *t, ipv4_addr_t *addr, arp_entry_t **entry) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address param", false);
  EXPECT_RETURN_BOOL(entry != nullptr, "Empty entry ptr ptr param", false);
  arp_entry_t *__entry = nullptr;
  if (arp_table_lookup(t, addr, &__entry)) {
    return false;
  }
  auto owned_entry = (arp_entry_t *)calloc(1, sizeof(arp_entry_t));
  owned_entry->ip_addr = {.value = addr->value};
  owned_entry->aod.is_resolved = false;
  glthread_init(&owned_entry->aod.pending_lookups);
  glthread_init(&owned_entry->arp_table_glue);
  glthread_add_next(&t->arp_entries, &owned_entry->arp_table_glue);
  *entry = owned_entry;
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
      "IP: " IPV4_ADDR_FMT ", Resolved?: %s, MAC: " MAC_ADDR_FMT ", OIF: %s\n",
      IPV4_ADDR_BYTES_BE(entry->ip_addr), (entry->aod.is_resolved ? "true" : "false"),
      MAC_ADDR_BYTES_BE(entry->mac_addr), entry->oif_name
    );
  }
  GLTHREAD_FOREACH_END();
}

bool arp_entry_add_pending_lookup(arp_entry_t *e, uint8_t *pay, uint32_t paylen, arp_lookup_processing_fn cb, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(e != nullptr, "Empty entry param", false);
  EXPECT_RETURN_BOOL(pay != nullptr, "Empty payload ptr param", false);
  uint32_t lookuplen = paylen + sizeof(arp_lookup_t);
  auto lookup = (arp_lookup_t *)calloc(1, lookuplen);
  lookup->cb = cb;
  lookup->bufflen = paylen;
  lookup->vlan_id = vlan_id;
  memcpy((uint8_t *)(lookup + 1), pay, paylen);
  glthread_init(&lookup->arp_entry_glue);
  glthread_add_next(&e->aod.pending_lookups, &lookup->arp_entry_glue);
  return true;
}
