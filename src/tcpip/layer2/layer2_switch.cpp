// layer2_switch.cpp

#include <arpa/inet.h>
#include "layer3/layer3.h"
#include "layer2.h"
#include "graph.h"
#include "mac_table.h"
#include "phy.h"
#include "pcap.h"
#include "ether_hdr.h"
#include "vlan_tag.h"

// Forward declarations

int layer2_switch_flood_frame_bytes(node_t *n, interface_t *ignored, uint8_t *frame, uint32_t framelen);
int layer2_switch_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);

#pragma mark -

// Ingress

int layer2_switch_recv_frame_bytes(node_t *n, interface_t *iintf, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node param", -1);
  EXPECT_RETURN_VAL(iintf != nullptr, "Empty ingress interface param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty frame ptr param", -1);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  // Every time we see a frame, we want to update said table
  bool status = mac_table_process_reply(n->netprop.mac_table, ether_hdr, iintf);
#pragma unused(status)
  // First, handle broadcast frames
  mac_addr_t src_mac = ether_hdr_read_src_mac(ether_hdr);
  if (MAC_ADDR_IS_BROADCAST(src_mac)) {
    return layer2_switch_flood_frame_bytes(n, iintf, frame, framelen);
  }
  // Else, if we see a destination mac address, we will read the table to find the interface
  mac_entry_t *mac_entry = nullptr;
  // If found, we will send packet off using that interfaces
  mac_addr_t dst_mac = ether_hdr_read_dst_mac(ether_hdr);
  if (mac_table_lookup(n->netprop.mac_table, &dst_mac, &mac_entry)) {
    // Found entry in MAC table
    interface_t *ointf = node_get_interface_by_name(n, (const char *)mac_entry->oif_name);
    EXPECT_RETURN_VAL(ointf != nullptr, "node_get_interface_by_name failed", -1);
    if (INTF_MODE(ointf) == INTF_MODE_L3_SVI) {
      INTF_NETPROP(ointf).delegate = iintf;
      bool resp = layer2_switch_send_frame_bytes(n, ointf, frame, framelen);
      INTF_NETPROP(ointf).delegate = nullptr;
      return resp;
    }
    else {
      return layer2_switch_send_frame_bytes(n, ointf, frame, framelen);
    }
  }
  // If not, we will flood all interfaces like we already do below.
  return layer2_switch_flood_frame_bytes(n, iintf, frame, framelen);
}

#pragma mark -

// Egress

bool layer2_switch_qualify_send_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(ethhdr != nullptr, "Empty ethernet header param", false);
  if (INTF_MODE(intf) == INTF_MODE_L3) {
    printf("Reject: L3 mode (%s)\n", intf->if_name);
    return false;
  }
  if (!ETHER_HDR_VLAN_TAGGED(ethhdr)) {
    // Every packet being sent out must be tagged until this point.
    printf("Reject: Untagged (%s)\n", intf->if_name);
    return false;
  }
  vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
  uint16_t vlan_id = vlan_tag_read_vlan_id(tag);
  if (!interface_test_vlan_membership(intf, vlan_id)) {
    // Not part of the intended VLAN. Enforce separation.
    // Note that for TRUNK interfaces, this checks against
    // all registered VLAN memberships.
    printf("Reject: VLAN (%s, %u vs %u)\n", intf->if_name, vlan_id, INTF_NETPROP(intf).l2.vlan_memberships[0]);
    return false;
  }
  return true;
}

int layer2_switch_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(intf != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty packet ptr param", -1);
  if (!layer2_switch_qualify_send_frame_on_interface(intf, (ether_hdr_t *)frame)) {
    return 0;
  }
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  if (!ETHER_HDR_VLAN_TAGGED(ether_hdr)) {
    return 0;
  }
  if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS) {
    // Strip VLAN tag before egress from ACCESS interfaces
    uint32_t untagged_framelen = 0;
    ether_hdr_t *untagged_hdr = ether_hdr_untag_vlan(ether_hdr, framelen, &untagged_framelen);
    EXPECT_RETURN_VAL(untagged_hdr != nullptr, "ether_hdr_untag_vlan failed", -1);
    int resp = NODE_NETSTACK(n).phy.send(n, intf, (uint8_t *)untagged_hdr, untagged_framelen); 
    EXPECT_CONTINUE(resp == untagged_framelen, "NODE_NETSTACK(n).phy.send failed");
    return resp;
  }
  else if (INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
    // Forward tagged frames out of TRUNK interface
    int resp = NODE_NETSTACK(n).phy.send(n, intf, (uint8_t *)ether_hdr, framelen); 
    EXPECT_CONTINUE(resp == framelen, "NODE_NETSTACK(n).phy.send failed");
    return resp;
  }
  else if (INTF_MODE(intf) == INTF_MODE_L3_SVI) {
    // Strip VLAN tag before egress from L3_SVI interfaces
    uint32_t untagged_framelen = 0;
    ether_hdr_t *untagged_hdr = ether_hdr_untag_vlan(ether_hdr, framelen, &untagged_framelen);
    EXPECT_RETURN_VAL(untagged_hdr != nullptr, "ether_hdr_untag_vlan failed", -1);
    // Promote untagged frame to layer2 (will handle ARP broadcast + l3 promotion)
    int resp = NODE_NETSTACK(n).l2.promote(n, intf, untagged_hdr, untagged_framelen);
    EXPECT_CONTINUE(resp == untagged_framelen, "NODE_NETSTACK(n).l2.promote failed");
    return resp;
  }
  return -1;
}

int layer2_switch_flood_frame_bytes(node_t *n, interface_t *ignored, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty packet ptr param", -1);
  int acc = 0;
  // Prepare untagged version of input frame to send out via ACCESS interfaces
  ether_hdr_t *tagged_hdr = (ether_hdr_t *)frame;
  EXPECT_RETURN_VAL(ETHER_HDR_VLAN_TAGGED(tagged_hdr) == true, "Untagged frame param", -1);
  // Untag frame in advance. Untagging is destructive so make a temporary copy.
  uint8_t temp[1500] = {0};
  memcpy(temp, frame, framelen);
  ether_hdr_t *untagged_hdr = (ether_hdr_t *)temp;
  uint32_t untagged_framelen = framelen;
  untagged_hdr = ether_hdr_untag_vlan(untagged_hdr, framelen, &untagged_framelen);
  EXPECT_RETURN_VAL(untagged_hdr != nullptr, "ether_hdr_untag_vlan failed", -1);
  // Flood (selectively)
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!n->intf[i]) { continue; }
    interface_t *intf = n->intf[i];
    if (intf == ignored) { continue; } // ignored interface
    if (!layer2_switch_qualify_send_frame_on_interface(intf, tagged_hdr)) {
      continue;
    }
    if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS) {
      // Strip VLAN tag before egress
      int resp = NODE_NETSTACK(n).phy.send(n, intf, (uint8_t *)untagged_hdr, untagged_framelen); 
      EXPECT_CONTINUE(resp == untagged_framelen, "NODE_NETSTACK(n).phy.send failed");
      acc += resp;
    }
    else if (INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
      int resp = NODE_NETSTACK(n).phy.send(n, intf, (uint8_t *)tagged_hdr, framelen); 
      EXPECT_CONTINUE(resp == framelen, "NODE_NETSTACK(n).phy.send failed");
      acc += resp;
    }
    else if (INTF_MODE(intf) == INTF_MODE_L3_SVI) {
      // Promote untagged frame to layer2 (will handle ARP broadcast + l3 promotion)
      INTF_NETPROP(intf).delegate = ignored;
      int resp = NODE_NETSTACK(n).l2.promote(n, intf, untagged_hdr, untagged_framelen);
      INTF_NETPROP(intf).delegate = nullptr;
      EXPECT_CONTINUE(resp == untagged_framelen, "NODE_NETSTACK(n).l2.promote failed");
      acc += resp;
    }
  }
  return acc; // Number of bytes sent
}

