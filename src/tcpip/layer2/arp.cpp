// arp.cpp

#include <arpa/inet.h>
#include "graph.h"
#include "net.h"
#include "arp.h"
#include "layer2.h"
#include "phy.h"

bool arp_table_add_entry(arp_table_t *t, arp_entry_t *entry, glthread_t **pending_lookups);

#pragma mark -

// I/O

bool node_arp_send_broadcast_request(node_t *n, interface_t *ointf, ipv4_addr_t *ip_addr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(ointf != nullptr, "Empty output interface param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  printf(
    "Sending broadcast request using interface: %s, for ip: " IPV4_ADDR_FMT "\n", 
    ointf->if_name, IPV4_ADDR_BYTES_BE(*ip_addr)
  );
  // Create unresolved arp table entry (when we get a reply, we'll fill it up)
  arp_entry_t *__entry = nullptr;
  arp_table_t *t = n->netprop.arp_table;
  if (!arp_table_lookup(t, ip_addr, &__entry)) {
    bool resp = arp_table_add_unresolved_entry(t, ip_addr, &__entry);
    EXPECT_RETURN_BOOL(resp == true, "arp_table_add_unresolved_entry failed", false);
  }
  // Allocate Ethernet frame wide enough to fit the ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + sizeof(arp_hdr_t);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)calloc(1, framelen);
  // Fill out Ethernet header fields
  ether_hdr_set_src_mac(ether_hdr, INTF_MAC_PTR(ointf)); // our intf MAC
  mac_addr_t broadcast_mac = {0};
  mac_addr_fill_broadcast(&broadcast_mac);           // broadcast MAC address
  ether_hdr_set_dst_mac(ether_hdr, &broadcast_mac);
  ether_hdr_set_type(ether_hdr, ETHER_TYPE_ARP);
  // Fill out ARP header fields
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(ether_hdr + 1);
  arp_hdr_set_hw_type(arp_hdr, ARP_HW_TYPE_ETHERNET);
  arp_hdr_set_proto_type(arp_hdr, ETHER_TYPE_IPV4);  // PTYPE shares field values with ETHER_TYPE
  arp_hdr_set_hw_addr_len(arp_hdr, 6);               // 6 byte MAC address
  arp_hdr_set_proto_addr_len(arp_hdr, 4);            // 4 byte IP address
  arp_hdr_set_op_code(arp_hdr, ARP_OP_CODE_REQUEST);
  arp_hdr_set_src_mac(arp_hdr, INTF_MAC_PTR(ointf));
  arp_hdr_set_src_ip(arp_hdr, INTF_IP_PTR(ointf)->value);
  arp_hdr_set_dst_mac(arp_hdr, MAC_ADDR_PTR_ZEROED);
  arp_hdr_set_dst_ip(arp_hdr, ip_addr->value); // <- The IPv4 address for which we want to know the MAC address
  // Pass frame to layer 1
  int resp = NODE_NETSTACK(n).phy.send(n, ointf, (uint8_t *)ether_hdr, framelen);
  EXPECT_RETURN_BOOL((uint32_t)resp == framelen, "NODE_NETSTACK(n).phy.send failed", false);
  free(ether_hdr);
  return true;
}

bool node_arp_recv_broadcast_request_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(iintf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty ethernet header param", false);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(hdr + 1); // payload, beyond the ethernet header
  ipv4_addr_t target_ip = arp_hdr_read_dst_ip(arp_hdr);
  if (IPV4_ADDR_PTR_IS_EQUAL(INTF_IP_PTR(iintf), &target_ip)) { 
    if (arp_hdr_read_op_code(arp_hdr) == ARP_OP_CODE_REQUEST) {
      return node_arp_send_reply_frame(n, iintf, hdr);
    }
  }
  // Ignore packet
  // Note, this is not strictly an error, which is why we return a true return value.
  return true; // TODO: Add log / counter to record dropped frame statistics
}

bool node_arp_send_reply_frame(node_t *n, interface_t *ointf, ether_hdr_t *in_ether_hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(ointf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(in_ether_hdr != nullptr, "Empty ethernet header param", false);
  arp_hdr_t *in_arp_hdr = (arp_hdr_t *)(in_ether_hdr + 1);
  // Allocate frame
  uint32_t out_framelen = sizeof(ether_hdr_t) + sizeof(arp_hdr_t);
  ether_hdr_t *out_ether_hdr = (ether_hdr_t *)calloc(1, out_framelen);
  // Fill up Ethernet header fields (note: be mindful of host/net. byte order)
  mac_addr_t in_src_mac = ether_hdr_read_src_mac(in_ether_hdr);
  ether_hdr_set_dst_mac(out_ether_hdr, &in_src_mac);
  ether_hdr_set_src_mac(out_ether_hdr, INTF_MAC_PTR(ointf));
  ether_hdr_set_type(out_ether_hdr, ETHER_TYPE_ARP);
  // Fill up ARP header fields (note: be mindful of host/net. byte order)
  arp_hdr_t *out_arp_hdr = (arp_hdr_t *)(out_ether_hdr + 1);
  arp_hdr_set_hw_type(out_arp_hdr, ARP_HW_TYPE_ETHERNET);
  arp_hdr_set_proto_type(out_arp_hdr, ETHER_TYPE_IPV4);
  arp_hdr_set_hw_addr_len(out_arp_hdr, 6);
  arp_hdr_set_proto_addr_len(out_arp_hdr, 4);
  arp_hdr_set_op_code(out_arp_hdr, ARP_OP_CODE_REPLY);
  mac_addr_t dst_mac = arp_hdr_read_src_mac(in_arp_hdr);
  arp_hdr_set_dst_mac(out_arp_hdr, &dst_mac);
  arp_hdr_set_dst_ip(out_arp_hdr, arp_hdr_read_src_ip(in_arp_hdr).value);
  arp_hdr_set_src_mac(out_arp_hdr, INTF_MAC_PTR(ointf));
  arp_hdr_set_src_ip(out_arp_hdr, INTF_IP_PTR(ointf)->value);
  // Send out packet
  int resp = NODE_NETSTACK(n).phy.send(n, ointf, (uint8_t *)out_ether_hdr, out_framelen);
  EXPECT_RETURN_BOOL(resp == (int)out_framelen, "NODE_NETSTACK(n).phy.send failed", false);
  free(out_ether_hdr);
  return true;
}

bool node_arp_recv_reply_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(iintf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty ethernet header param", false);
  printf("[%s] Received ARP reply!\n", n->node_name);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(hdr + 1);
  return arp_table_process_reply(n->netprop.arp_table, arp_hdr, iintf);
}

#pragma mark -

// ARP table

bool arp_table_process_reply(arp_table_t *t, arp_hdr_t *hdr, interface_t *intf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty table param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty header param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  arp_entry_t *arp_entry = nullptr;
  ipv4_addr_t ip_addr = arp_hdr_read_src_ip(hdr);
  if (!arp_table_lookup(t, &ip_addr, &arp_entry)) {
    // We got an arp reply for request we didn't put out
    return false;
  }
  EXPECT_RETURN_BOOL(arp_entry_is_resolved(arp_entry) == false, "Got reply for resolved entry", false);
  // Fil out entry with new new info in the reply
  arp_entry->mac_addr = arp_hdr_read_src_mac(hdr);
  glthread_t *curr = nullptr;
  strncpy((char *)arp_entry->oif_name, (char *)intf->if_name, CONFIG_IF_NAME_SIZE);
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

bool arp_entry_add_pending_lookup(arp_entry_t *e, uint8_t *pay, uint32_t paylen, arp_lookup_processing_fn cb) {
  EXPECT_RETURN_BOOL(e != nullptr, "Empty entry param", false);
  EXPECT_RETURN_BOOL(pay != nullptr, "Empty payload ptr param", false);
  uint32_t lookuplen = paylen + sizeof(arp_lookup_t);
  auto lookup = (arp_lookup_t *)calloc(1, lookuplen);
  lookup->cb = cb;
  lookup->bufflen = paylen;
  memcpy((uint8_t *)(lookup + 1), pay, paylen);
  glthread_init(&lookup->arp_entry_glue);
  glthread_add_next(&e->aod.pending_lookups, &lookup->arp_entry_glue);
  return true;
}
