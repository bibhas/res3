// layer2_arp.cpp

#include "layer2.h"
#include "arp_table.h"
#include "arp_hdr.h"
#include "net.h"
#include "graph.h"
#include "ether_hdr.h"
#include "vlan_tag.h"

void layer2_send_with_resolved_arp(node_t *n, arp_entry_t *entry, ether_hdr_t *hdr, uint32_t framelen, uint16_t ethertype, uint16_t vlan_id) {
  // Get outgoing interface
  interface_t *ointf = node_get_interface_by_name(n, entry->oif_name);
  EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
  // Set ethernet header fields
  mac_addr_t *dst_mac = &entry->mac_addr;
  EXPECT_RETURN(dst_mac != nullptr, "Missing dst mac");
  mac_addr_t *src_mac = INTF_MAC_PTR(ointf);
  EXPECT_RETURN(src_mac != nullptr, "Missing src mac");
  ether_hdr_set_src_mac(hdr, src_mac);
  ether_hdr_set_dst_mac(hdr, dst_mac);
  ether_hdr_set_type(hdr, ethertype);
  // Tag frame if sending over trunk interface
  uint8_t *frame = (uint8_t *)hdr;
  uint32_t actual_framelen = framelen;
  if (INTF_MODE(ointf) == INTF_MODE_L2_TRUNK && vlan_id != 0) {
    // Need to tag the frame
    ether_hdr_t *tagged_hdr = ether_hdr_tag_vlan(hdr, framelen, vlan_id, &actual_framelen);
    EXPECT_RETURN(tagged_hdr != nullptr, "ether_hdr_tag_vlan failed");
    frame = (uint8_t *)tagged_hdr;
  }
  // Send off the packet
  int sentlen = NODE_NETSTACK(n).phy.send(n, ointf, frame, actual_framelen);
  EXPECT_RETURN(sentlen == actual_framelen, "NODE_NETSTACK(n).phy.send failed");
}

#pragma mark -

// I/O

bool node_arp_send_broadcast_request(node_t *n, interface_t *intf, ipv4_addr_t *ip_addr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty output interface param", false);
  EXPECT_RETURN_BOOL(ip_addr != nullptr, "Empty ip address param", false);
  // Create unresolved arp table entry (when we get a reply, we'll fill it up)
  arp_entry_t *__entry = nullptr;
  arp_table_t *t = n->netprop.arp_table;
  if (!arp_table_lookup(t, ip_addr, &__entry)) {
    bool resp = arp_table_add_unresolved_entry(t, ip_addr, &__entry);
    EXPECT_RETURN_BOOL(resp == true, "arp_table_add_unresolved_entry failed", false);
  }
  // Allocate Ethernet frame wide enough to fit the ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + sizeof(arp_hdr_t);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)calloc(1, framelen + sizeof(vlan_tag_t)); // Extra room for possible tag
  // Send function (with SVIs, we will need to forward frames via multiple interfaces in the VLAN)
  auto send_fn = [&, intf](interface_t *ointf, uint16_t vlan_id) -> bool {
    // Fill out Ethernet header fields
    ether_hdr_set_src_mac(ether_hdr, INTF_MAC_PTR(ointf)); // our intf MAC
    mac_addr_t broadcast_mac = {0};
    mac_addr_fill_broadcast(&broadcast_mac);           // broadcast MAC address
    ether_hdr_set_dst_mac(ether_hdr, &broadcast_mac);
    ether_hdr_set_type(ether_hdr, ETHER_TYPE_ARP);
    arp_hdr_t *arp_hdr = (arp_hdr_t *)(ether_hdr + 1);
    uint32_t actual_framelen = framelen;
    // In case of L2 TRUNK interface, we need to tag the frame
    if (INTF_MODE(ointf) == INTF_MODE_L2_TRUNK) {
      printf("Tagging TRUNK frame with VLAN: %u\n", vlan_id);
      vlan_tag_t *tag = (vlan_tag_t *)(ether_hdr + 1);
      vlan_tag_set_vlan_id(tag, vlan_id);
      // Copy ether_type to type
      vlan_tag_set_ether_type(tag, ETHER_TYPE_ARP);
      // Set frame type to VLAN
      ether_hdr_set_type(ether_hdr, ETHER_TYPE_VLAN);
      // Update arp_hdr offset
      arp_hdr = (arp_hdr_t *)(tag + 1);
      // Update frame length to include VLAN tag
      actual_framelen += sizeof(vlan_tag_t);
    }
    // Fill out ARP header fields
    arp_hdr_set_hw_type(arp_hdr, ARP_HW_TYPE_ETHERNET);
    arp_hdr_set_proto_type(arp_hdr, ETHER_TYPE_IPV4);  // PTYPE shares field values with ETHER_TYPE
    arp_hdr_set_hw_addr_len(arp_hdr, 6);               // 6 byte MAC address
    arp_hdr_set_proto_addr_len(arp_hdr, 4);            // 4 byte IP address
    arp_hdr_set_op_code(arp_hdr, ARP_OP_CODE_REQUEST);
    arp_hdr_set_src_mac(arp_hdr, INTF_MAC_PTR(ointf));
    // Use SVI's IP if broadcasting from SVI, otherwise use interface's own IP
    ipv4_addr_t *src_ip = (INTF_MODE(intf) == INTF_MODE_L3_SVI) ? INTF_IP_PTR(intf) : INTF_IP_PTR(ointf);
    arp_hdr_set_src_ip(arp_hdr, src_ip->value);
    arp_hdr_set_dst_mac(arp_hdr, MAC_ADDR_PTR_ZEROED);
    arp_hdr_set_dst_ip(arp_hdr, ip_addr->value); // <- The IPv4 address for which we want to know the MAC address
    // Pass frame to layer 1
    int resp = NODE_NETSTACK(n).phy.send(n, ointf, (uint8_t *)ether_hdr, actual_framelen);
    EXPECT_RETURN_BOOL((uint32_t)resp == actual_framelen, "NODE_NETSTACK(n).phy.send failed", false);
    return true;
  };
  if (INTF_MODE(intf) == INTF_MODE_L3_SVI) {
    for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
      if (!n->intf[i]) { continue; }
      interface_t *candidate = n->intf[i];
      if (candidate == intf) { continue; } // Ignore self
      uint16_t vlan_id = INTF_NETPROP(intf).l2.vlan_memberships[0];
      if (!interface_test_vlan_membership(candidate, vlan_id)) { continue; } // Not in VLAN
      if (!INTF_IN_L2_MODE(candidate)) { continue; }
      if (INTF_MODE(candidate) == INTF_MODE_L3_SVI) { continue; } // This isn't possible, still, just for sanity
      bool resp = send_fn(candidate, vlan_id);
      EXPECT_RETURN_BOOL(resp == true, "senf_fn failed", false);
    }
  }
  else {
    bool resp = send_fn(intf, 0);
    EXPECT_RETURN_BOOL(resp == true, "senf_fn failed", false);
  }
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
  printf("[%s] ARP broadcast request ignored (%s)\n", n->node_name, iintf->if_name);
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
  if (INTF_MODE(ointf) == INTF_MODE_L3_SVI && INTF_NETPROP(ointf).delegate != nullptr) {
    // If the outgoing interface is a logical SVI, then we need to reply using its delegate interface
    int resp = NODE_NETSTACK(n).phy.send(n, INTF_NETPROP(ointf).delegate, (uint8_t *)out_ether_hdr, out_framelen);
    EXPECT_RETURN_BOOL(resp == (int)out_framelen, "NODE_NETSTACK(n).phy.send failed", false);
  }
  else {
    int resp = NODE_NETSTACK(n).phy.send(n, ointf, (uint8_t *)out_ether_hdr, out_framelen);
    EXPECT_RETURN_BOOL(resp == (int)out_framelen, "NODE_NETSTACK(n).phy.send failed", false);
  }
  free(out_ether_hdr);
  return true;
}

bool node_arp_recv_reply_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(iintf != nullptr, "Empty input interface param", false);
  EXPECT_RETURN_BOOL(hdr != nullptr, "Empty ethernet header param", false);
  printf("[%s] Received ARP reply!\n", n->node_name);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(hdr + 1);
  if (INTF_MODE(iintf) == INTF_MODE_L3_SVI) {
    return arp_table_process_reply(n->netprop.arp_table, arp_hdr, INTF_NETPROP(iintf).delegate);
  }
  return arp_table_process_reply(n->netprop.arp_table, arp_hdr, iintf);
}

