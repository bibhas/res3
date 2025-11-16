// layer2.cpp

#include <arpa/inet.h>
#include "layer3/layer3.h"
#include "layer2.h"
#include "graph.h"
#include "arp.h"
#include "mac.h"
#include "phy.h"

int layer2_switch_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);

#pragma mark -

// Ethernet

ether_hdr_t* ether_hdr_alloc_with_payload(uint8_t *pkt, uint32_t pktlen) {
  /* 
   * [ free space ][ pkt ][free space]
   *         ↑ ← ← ↑
   *        ret   pkt
   */
  EXPECT_RETURN_VAL(pkt != nullptr, "Empty pkt param", nullptr);
  uint8_t *start = pkt - sizeof(ether_hdr_t);
  ether_hdr_t *resp = (ether_hdr_t *)start;
  ether_hdr_set_src_mac(resp, MAC_ADDR_PTR_ZEROED);
  ether_hdr_set_dst_mac(resp, MAC_ADDR_PTR_ZEROED);
  ether_hdr_set_type(resp, 0);
  return resp;
}

ether_hdr_t* ether_hdr_tag_vlan(ether_hdr_t *hdr, uint32_t len, uint16_t vlanid, uint32_t *newlen) {
  EXPECT_RETURN_VAL(hdr != nullptr, "Empty header ptr param", nullptr);
  // First, check if frame is tagged
  if (ether_hdr_read_type(hdr) == ETHER_TYPE_VLAN) {
    // No need to do anything
    if (newlen) {
      *newlen = len;
    }
    return hdr;
  }
  // Allocate a dedicated static temp buffer for vlan tagging
  static ether_hdr_t *temp_hdr = nullptr;
  if (temp_hdr == nullptr) {
    temp_hdr = (ether_hdr_t *)malloc(sizeof(ether_hdr_t));
  }
  // Copy hdr to temp
  memcpy(temp_hdr, hdr, sizeof(ether_hdr_t));
  // Find start of payload
  uint8_t *payload = (uint8_t *)(hdr + 1);
  // Locate vlan header start ptr
  uint8_t *_tag = (payload - sizeof(vlan_tag_t));
  uint8_t *_new_hdr = (_tag - sizeof(ether_hdr_t));
  vlan_tag_t *tag = (vlan_tag_t *)_tag;
  vlan_tag_init(tag);
  ether_hdr_t *new_hdr = (ether_hdr_t *)_new_hdr;
  ether_hdr_init(new_hdr);
  // Copy over temp to new header
  memcpy(new_hdr, temp_hdr, sizeof(ether_hdr_t));
  // Fill in vland id in the tag
  vlan_tag_set_vlan_id(tag, vlanid);
  // Copy ether_type to type
  vlan_tag_set_ether_type(tag, ether_hdr_read_type(temp_hdr));
  // Set frame type to VLAN
  ether_hdr_set_type(new_hdr, ETHER_TYPE_VLAN);
  // And, we're done
  if (newlen) {
    *newlen = len + sizeof(vlan_tag_t);
  }
  return new_hdr;
}

ether_hdr_t* ether_hdr_untag_vlan(ether_hdr_t *hdr, uint32_t len, uint32_t *newlen) {
  EXPECT_RETURN_VAL(hdr != nullptr, "Empty header ptr param", nullptr);
  // First, check if frame is tagged at all
  if (ether_hdr_read_type(hdr) != ETHER_TYPE_VLAN) {
    // No need to do anything
    *newlen = len;
    return hdr;
  }
  // Allocate a dedicated static temp buffer for vlan untagging
  static ether_hdr_t *temp_hdr = nullptr;
  if (temp_hdr == nullptr) {
    temp_hdr = (ether_hdr_t *)malloc(sizeof(ether_hdr_t));
  }
  // Copy header to temp -> zero out memory
  memcpy((void *)temp_hdr, (void *)hdr, sizeof(ether_hdr_t));
  memset((void *)hdr, 0, sizeof(ether_hdr_t));
  // Retrieve ethertype of VLAN
  vlan_tag_t *tag = (vlan_tag_t *)(hdr + 1);
  uint16_t orig_type = vlan_tag_read_ether_type(tag);
  // Then, zero out vlan 
  memset((void *)tag, 0, sizeof(vlan_tag_t));
  // Locate new header ptr
  uint8_t *payload = (uint8_t *)(tag + 1);
  uint8_t *_new_hdr = payload - sizeof(ether_hdr_t);
  ether_hdr_t *new_hdr = (ether_hdr_t *)_new_hdr;
  // Copy over header from temp
  memcpy((void *)new_hdr, (void *)temp_hdr, sizeof(ether_hdr_t));
  // Update ether type
  ether_hdr_set_type(new_hdr, orig_type);
  // And, we're done
  *newlen = len - sizeof(vlan_tag_t);
  return new_hdr;
}

#pragma mark -

// Layer 2 processing

bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr, uint16_t *vlan_id) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(ethhdr != nullptr, "Empty ethernet header param", false);
  EXPECT_RETURN_BOOL(vlan_id != nullptr, "Empty VLAN ID ptr param", false);
  if (INTF_MODE(intf) == INTF_MODE_L3) {
    if (ETHER_HDR_VLAN_TAGGED(ethhdr)) {
      // We won't accept any VLAN tagged frames in L3 mode.
      // This is to separate the broadcast domain from spilling over.
      return false; 
    }
    // We will only respond if the frame is specially intended for this
    // interface (based on the dest MAC) or it's a broadcast MAC address.
    mac_addr_t dst_mac = ether_hdr_read_dst_mac(ethhdr);
    if (MAC_ADDR_IS_EQUAL(dst_mac, INTF_NETPROP(intf).l2.mac_addr) || MAC_ADDR_IS_BROADCAST(dst_mac)) {
      *vlan_id = 0;
      return true;
    }
  }
  else if (INTF_IN_L2_MODE(intf) && INTF_MODE(intf) != INTF_MODE_L3_SVI) {
    if (INTF_NETPROP(intf).l2.vlan_memberships[0] == 0) {
      // No assigned VLAN memberships for this interface: drop frame
      return false;
    }
    if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS) {
      if (ETHER_HDR_VLAN_TAGGED(ethhdr)) {
        // We're dealing with a tagged frame
        vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
        if (!interface_test_vlan_membership(intf, vlan_tag_read_vlan_id(tag))) {
          return false; // tag VLAN id does not match interface's VLAN
        }
        *vlan_id = vlan_tag_read_vlan_id(tag);
        return true;
      }
      else {
        // We're dealing with an untagged frame.
        // Caller needs to be tag and L2 switch this frame.
        *vlan_id = INTF_NETPROP(intf).l2.vlan_memberships[0];
        return true;
      }
    }
    else if (INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
      if (!ETHER_HDR_VLAN_TAGGED(ethhdr)) {
        // TRUNK interfaces must reject all untagged frames
        // regardless of interface VLAN membership(s)
        return false;
      }
      // We're dealing with a tagged frame
      vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
      if (!interface_test_vlan_membership(intf, vlan_tag_read_vlan_id(tag))) {
        return false; // tag VLAN id does not match interface's VLAN(s)
      }
      *vlan_id = vlan_tag_read_vlan_id(tag);
      return true;
    }
    return false; // unreachable
  }
  // We have received frames on an SVI, which is illegal.
  return false; 
}

#pragma mark -

// Layer 2 I/O

int __layer2_promote(node_t *n, interface_t *iintf, ether_hdr_t *ether_hdr, uint32_t framelen) {
  uint16_t hdr_type = ether_hdr_read_type(ether_hdr);
  // Check if it's an ARP message
  if (hdr_type == ETHER_TYPE_ARP) {
    arp_hdr_t *arp_hdr = (arp_hdr_t *)(ether_hdr + 1);
    uint16_t arp_op_code = arp_hdr_read_op_code(arp_hdr);
    switch (arp_op_code) {
      case ARP_OP_CODE_REQUEST: {
        bool resp = node_arp_recv_broadcast_request_frame(n, iintf, ether_hdr);
        EXPECT_RETURN_VAL(resp == true, "node_arp_recv_broadcast_request_frame failed", -1);
        return framelen;
      }
      case ARP_OP_CODE_REPLY: {
        bool resp = node_arp_recv_reply_frame(n, iintf, ether_hdr);
        EXPECT_RETURN_VAL(resp == true, "node_arp_recv_reply_frame failed", -1);
        return framelen;
      }
      default: {
        printf("Unknown ARP op code received! Ignoring...\n");
        return -1;
      }
    }
  }
  else if (hdr_type == ETHER_TYPE_IPV4) {
    // We need to delegate processing of this packet to L3
    uint8_t *pkt = (uint8_t *)(ether_hdr + 1);
    uint32_t pktlen = framelen - sizeof(ether_hdr_t); // We ignore FCS
    NODE_NETSTACK(n).l3.promote(n, iintf, pkt, pktlen, hdr_type);
    return framelen;
  }
  else {
    ; // Discard
  }
  return -1;
}

void __layer2_demote(node_t *n, ipv4_addr_t *nxt_hop_addr, interface_t *ointf, uint8_t *payload, uint32_t paylen, uint16_t ethertype) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(nxt_hop_addr != nullptr, "Empty next hop address param");
  // We will to handle ointf == nullptr case manually
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(payload != nullptr, "Empty payload ptr param");
  if (!ointf && node_is_local_address(n, nxt_hop_addr)) {
    // Self-ping case
    NODE_NETSTACK(n).l3.promote(n, nullptr, payload, paylen, ethertype);
    return;
  }
  if (!ointf) {
    // Direct delivery case
    bool resp = node_get_interface_matching_subnet(n, nxt_hop_addr, &ointf); // <-- Overwrites ointf
    EXPECT_RETURN(resp == true, "node_get_interface_matching_subnet failed");
  }
  // Resolve src and dst mac addresses
  auto pending_lookup_processing_cb = [n, ethertype](arp_entry_t *entry, arp_lookup_t *pending) {
    uint8_t *payload = (uint8_t *)(pending + 1);
    uint32_t paylen = pending->bufflen;
    ether_hdr_t *hdr = (ether_hdr_t *)payload;
    // Get outgoing interface
    interface_t *ointf = node_get_interface_by_name(n, entry->oif_name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
    // Set src MAC field
    mac_addr_t *dst_mac = &entry->mac_addr;
    EXPECT_RETURN(dst_mac != nullptr, "Missing dst mac");
    ether_hdr_set_dst_mac(hdr, dst_mac);
    // Set dst MAC field
    mac_addr_t *src_mac = INTF_MAC_PTR(ointf);
    EXPECT_RETURN(src_mac != nullptr, "Missing src mac");
    ether_hdr_set_src_mac(hdr, src_mac);
    // Set ethertype field
    ether_hdr_set_type(hdr, ethertype);
    // Finally, send off the packet
    int sentlen = NODE_NETSTACK(n).phy.send(n, ointf, payload, paylen);
    EXPECT_RETURN(sentlen == paylen, "NODE_NETSTACK(n).phy.send failed");
    // No need to free arp_lookup_t (will be done internally)
  };
  arp_table_t *t = n->netprop.arp_table;
  arp_entry_t *arp_entry = nullptr;
  if (!arp_table_lookup(t, nxt_hop_addr, &arp_entry)) {
    // No entry found. Create unresolved entry with pending lookups and send out ARP broadcast.
    bool resp = arp_table_add_unresolved_entry(t, nxt_hop_addr, &arp_entry);
    EXPECT_RETURN(resp == true, "arp_table_add_unresolved_entry failed");
    EXPECT_RETURN(arp_entry_is_resolved(arp_entry) == false, "arp_table_add_unresolved_entry failed");
    uint8_t *hdr_payload = payload - sizeof(ether_hdr_t);
    uint32_t hdr_paylen = paylen + sizeof(ether_hdr_t);
    resp = arp_entry_add_pending_lookup(arp_entry, hdr_payload, hdr_paylen, pending_lookup_processing_cb);
    EXPECT_RETURN(resp == true, "arp_entry_add_pending_lookup failed");
    resp = node_arp_send_broadcast_request(n, ointf, nxt_hop_addr);
    EXPECT_RETURN(resp == true, "node_arp_send_broadcast_request failed");
  }
  else if (!arp_entry_is_resolved(arp_entry)) {
    // Entry found, but it is pending
    uint8_t *hdr_payload = payload - sizeof(ether_hdr_t);
    uint32_t hdr_paylen = paylen + sizeof(ether_hdr_t);
    bool resp = arp_entry_add_pending_lookup(arp_entry, hdr_payload, hdr_paylen, pending_lookup_processing_cb);
    EXPECT_RETURN(resp == true, "arp_entry_add_pending_lookup failed");
  }
  else {
    // Found resolved entry
    mac_addr_t *dst_mac = &arp_entry->mac_addr;
    mac_addr_t *src_mac = INTF_MAC_PTR(ointf);
    EXPECT_RETURN(src_mac != nullptr, "Missing src mac");
    // Tack on ethernet header (precondition: should be enough headroom)
    ether_hdr_t *hdr = (ether_hdr_t *)(payload - sizeof(ether_hdr_t));
    ether_hdr_set_src_mac(hdr, src_mac);
    ether_hdr_set_dst_mac(hdr, dst_mac);
    ether_hdr_set_type(hdr, ethertype);
    // Send packet via phy
    uint32_t pktlen = sizeof(ether_hdr_t) + paylen;
    uint8_t *pkt = (uint8_t *)hdr;
    int sentlen = NODE_NETSTACK(n).phy.send(n, ointf, pkt, pktlen);
    EXPECT_RETURN(sentlen == pktlen, "NODE_NETSTACK(n).phy.send failed");
  }
}

int layer2_node_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  // Entry point into our TCP/IP stack
  EXPECT_RETURN_VAL(n != nullptr, "Empty node param", -1);
  EXPECT_RETURN_VAL(intf != nullptr, "Empty interface param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty frame ptr param", -1);
  // First check if we should even consider this frame
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  uint16_t vlan_id = 0; // <- Overwritten by qualify fn below
  if (!layer2_qualify_recv_frame_on_interface(intf, ether_hdr, &vlan_id)) {
    // Drop the frame
    return framelen;
  }
  if (INTF_IN_L2_MODE(intf)) {
    // TODO: This interface is configured in L2 mode. 
    // Go ahead and act like the good little L2 switch that you are.
    if (!ETHER_HDR_VLAN_TAGGED(ether_hdr)) {
      // If not tagged, we need to tag an ingress frame (w/ `vlan_id`)
      uint32_t new_framelen = 0; // <- Updated by fn below
      ether_hdr_t *new_ether_hdr = ether_hdr_tag_vlan(ether_hdr, framelen, vlan_id, &new_framelen);
      EXPECT_RETURN_VAL(new_ether_hdr != nullptr, "ether_hdr_tag_vlan failed", -1);
      EXPECT_RETURN_VAL(new_framelen > framelen, "new_framelen too small", -1);
      frame = (uint8_t *)new_ether_hdr;
      framelen = new_framelen;
    }
    return layer2_switch_recv_frame_bytes(n, intf, frame, framelen);
  }
  else if (INTF_IN_L3_MODE(intf)) { 
    // Interface is configured in L3 mode
    return NODE_NETSTACK(n).l2.promote(n, intf, ether_hdr, framelen);
  }
  // Interface is not in a functional state. 
  // Silently drop ingress frames.
  // TODO: Probably add a hw counter to signal dropped packets (?)
  return 0;
}

bool layer2_qualify_send_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(ethhdr != nullptr, "Empty ethernet header param", false);
  if (INTF_MODE(intf) == INTF_MODE_L3) {
    return false;
  }
  if (!ETHER_HDR_VLAN_TAGGED(ethhdr)) {
    // Every packet being sent out must be tagged until this point.
    return false;
  }
  vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
  uint16_t vlan_id = vlan_tag_read_vlan_id(tag);
  if (!interface_test_vlan_membership(intf, vlan_id)) {
    // Not part of the intended VLAN. Enforce separation.
    // Note that for TRUNK interfaces, this checks against
    // all registered VLAN memberships.
    return false;
  }
  return true;
}

int layer2_switch_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(intf != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty packet ptr param", -1);
  if (!layer2_qualify_send_frame_on_interface(intf, (ether_hdr_t *)frame)) {
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
    if (!layer2_qualify_send_frame_on_interface(intf, tagged_hdr)) {
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
      int resp = NODE_NETSTACK(n).l2.promote(n, intf, untagged_hdr, untagged_framelen);
      EXPECT_CONTINUE(resp == untagged_framelen, "NODE_NETSTACK(n).l2.promote failed");
      acc += resp;
    }
  }
  return acc; // Number of bytes sent
}

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
    return layer2_switch_send_frame_bytes(n, ointf, frame, framelen);
  }
  // If not, we will flood all interfaces like we already do below.
  return layer2_switch_flood_frame_bytes(n, iintf, frame, framelen);
}
