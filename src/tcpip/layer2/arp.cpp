// arp.cpp

#include <arpa/inet.h>
#include "graph.h"
#include "net.h"
#include "arp.h"
#include "layer2.h"
#include "phy.h"

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
  // Allocate Ethernet frame wide enough to fit the ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + sizeof(arp_hdr_t);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)calloc(1, framelen);
  // Fill out Ethernet header fields
  memcpy((void *)ether_hdr->src_mac.bytes, (void *)INTF_MAC(ointf)->bytes, 6);    // our intf MAC
  mac_addr_fill_broadcast(&ether_hdr->dst_mac);                                   // broadcast MAC address
  ether_hdr->type = htons(ETHER_TYPE_ARP);
  // Fill out ARP header fields
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(ether_hdr + 1);
  arp_hdr->hw_type = htons(ARP_HW_TYPE_ETHERNET);
  arp_hdr->proto_type = htons(ETHER_TYPE_IPV4); // PTYPE shares field values with ETHER_TYPE
  arp_hdr->hw_addr_len = 6; // 6 byte MAC address
  arp_hdr->proto_addr_len = 4; // 4 byte IP address
  arp_hdr->op_code = htons(ARP_OP_CODE_REQUEST); // 1 = request, 2 = reply
  memcpy((void *)arp_hdr->src_mac, (void *)INTF_MAC(ointf)->bytes, 6);
  arp_hdr->src_ip = htonl(INTF_IP(ointf)->value);
  memset(arp_hdr->dst_mac, 0, 6);
  arp_hdr->dst_ip = htonl(ip_addr->value); // <- The IPv4 address for which we want to know the MAC address
  // Pass frame to layer 1
  int resp = phy_node_send_frame_bytes(n, ointf, (uint8_t *)ether_hdr, framelen);
  EXPECT_RETURN_BOOL((uint32_t)resp == framelen, "phy_node_send_frame_bytes failed", false);
  free(ether_hdr);
  return true;
}

bool node_arp_recv_broadcast_request_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(iintf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty ethernet header param", false);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(hdr + 1); // payload, beyond the ethernet header
  ipv4_addr_t target_ip = {.value = ntohl(arp_hdr->dst_ip)};
  if (IPV4_ADDR_PTR_IS_EQUAL(INTF_IP(iintf), &target_ip) && ntohs(arp_hdr->op_code) == ARP_OP_CODE_REQUEST) {
    return node_arp_send_reply_frame(n, iintf, hdr);
  }
  // Ignore packet
  // Note, this is not an error, so we return a true return value.
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
  memcpy((void *)&out_ether_hdr->dst_mac, (void *)&in_ether_hdr->src_mac, 6);
  memcpy((void *)&out_ether_hdr->src_mac, (void *)&INTF_MAC(ointf)->bytes, 6);
  out_ether_hdr->type = htons(ETHER_TYPE_ARP);
  // Fill up ARP header fields (note: be mindful of host/net. byte order)
  arp_hdr_t *out_arp_hdr = (arp_hdr_t *)(out_ether_hdr + 1);
  out_arp_hdr->hw_type = htons(ARP_HW_TYPE_ETHERNET);
  out_arp_hdr->proto_type = htons(ETHER_TYPE_IPV4);
  out_arp_hdr->hw_addr_len = 6;
  out_arp_hdr->proto_addr_len = 4;
  out_arp_hdr->op_code = htons(ARP_OP_CODE_REPLY);
  memcpy((void *)&out_arp_hdr->dst_mac, (void *)&in_ether_hdr->src_mac, 6);
  out_arp_hdr->dst_ip = in_arp_hdr->src_ip; // already in network byte order
  memcpy((void *)&out_arp_hdr->src_mac, (void *)&INTF_MAC(ointf)->bytes, 6);
  out_arp_hdr->src_ip = htonl(INTF_IP(ointf)->value);
  // Send out packet
  int resp = phy_node_send_frame_bytes(n, ointf, (uint8_t *)out_ether_hdr, out_framelen);
  EXPECT_RETURN_BOOL(resp == (int)out_framelen, "phy_node_send_frame_bytes failed", false);
  free(out_ether_hdr);
  return true;
}

bool node_arp_recv_reply_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(iintf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty ethernet header param", false);
  printf("Received ARP reply!\n");
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(hdr + 1);
  return arp_table_process_reply(n->netprop.arp_table, arp_hdr, iintf);
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
  entry.ip_addr = {.value = ntohl(hdr->src_ip)};    // network byte order
  entry.mac_addr = {.bytes = {BYTES_FROM_BYTEARRAY_BE(hdr->src_mac)}};
  strncpy((char *)entry.oif_name, (char *)intf->if_name, CONFIG_IF_NAME_SIZE);
  glthread_init(&entry.arp_table_glue);
  // Add entry to thread (function will memcpy entry fields)
  bool resp = arp_table_add_entry(t, &entry);
  EXPECT_RETURN_BOOL(resp == true, "arp_table_add_entry failed", false);
  return true;
}
