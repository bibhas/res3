// layer2_io.cpp

#include <arpa/inet.h>
#include "layer3/layer3.h"
#include "layer2.h"
#include "graph.h"
#include "arp_table.h"
#include "vlan_tag.h"
#include "ether_hdr.h"
#include "mac.h"
#include "phy.h"
#include "pcap.h"

int layer2_switch_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);
void layer2_send_with_resolved_arp(node_t *n, arp_entry_t *entry, ether_hdr_t *hdr, uint32_t framelen, uint16_t ethertype, uint16_t vlan_id);

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
      printf("Reject: L3 and tagged\n");
      return false;
    }
    // We will only respond if the frame is specially intended for this
    // interface (based on the dest MAC) or it's a broadcast MAC address.
    mac_addr_t dst_mac = ether_hdr_read_dst_mac(ethhdr);
    if (MAC_ADDR_IS_EQUAL(dst_mac, INTF_NETPROP(intf).l2.mac_addr) || MAC_ADDR_IS_BROADCAST(dst_mac)) {
      *vlan_id = 0;
      return true;
    }
    // Frame not addressed to us - reject it
    printf("Reject: L3 MAC mismatch\n");
    return false;
  }
  else if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS || INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
    if (INTF_NETPROP(intf).l2.vlan_memberships[0] == 0) {
      // No assigned VLAN memberships for this interface: drop frame
      printf("Reject: NO interface VLAN membership\n");
      return false;
    }
    if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS) {
      if (ETHER_HDR_VLAN_TAGGED(ethhdr)) {
        // We're dealing with a tagged frame
        vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
        if (!interface_test_vlan_membership(intf, vlan_tag_read_vlan_id(tag))) {
          printf("Reject: L2 ACCESS got tagged frame\n");
          return false; // tag VLAN id does not match interface's VLAN
        }
        *vlan_id = vlan_tag_read_vlan_id(tag);
        return true;
      }
      else {
        // We're dealing with an untagged frame.
        // Caller needs to tag and L2 switch this frame.
        *vlan_id = INTF_NETPROP(intf).l2.vlan_memberships[0];
        return true;
      }
    }
    else if (INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
      if (!ETHER_HDR_VLAN_TAGGED(ethhdr)) {
        // TRUNK interfaces must reject all untagged frames
        // regardless of interface VLAN membership(s)
        printf("Reject: TRUNK got untagged frame\n");
        return false;
      }
      // We're dealing with a tagged frame
      vlan_tag_t *tag = (vlan_tag_t *)(ethhdr + 1);
      if (!interface_test_vlan_membership(intf, vlan_tag_read_vlan_id(tag))) {
        printf("Reject: Trunk got foreign VLAN frame\n");
        return false; // tag VLAN id does not match interface's VLAN(s)
      }
      *vlan_id = vlan_tag_read_vlan_id(tag);
      return true;
    }
    printf("Reject: unreachable\n");
    return false; // unreachable
  }
  // We have received frames on an SVI, which is illegal.
  printf("Reject: SVI??\n");
  return true; // TODO: Change this
}

#pragma mark -

// Layer 2 I/O

void layer2_demote(node_t *n, ipv4_addr_t *nxt_hop_addr, interface_t *ointf, uint8_t *payload, uint32_t paylen, uint16_t ethertype) {
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
  // Capture VLAN ID if routing from an SVI
  uint16_t vlan_id = 0;
  if (INTF_MODE(ointf) == INTF_MODE_L3_SVI) {
    vlan_id = INTF_NETPROP(ointf).l2.vlan_memberships[0];
  }
  // Resolve src and dst mac addresses
  auto pending_lookup_processing_cb = [n, ethertype](arp_entry_t *entry, arp_lookup_t *pending) {
    uint8_t *payload = (uint8_t *)(pending + 1);
    uint32_t framelen = pending->bufflen;
    ether_hdr_t *hdr = (ether_hdr_t *)payload;
    layer2_send_with_resolved_arp(n, entry, hdr, framelen, ethertype, pending->vlan_id);
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
    resp = arp_entry_add_pending_lookup(arp_entry, hdr_payload, hdr_paylen, pending_lookup_processing_cb, vlan_id);
    EXPECT_RETURN(resp == true, "arp_entry_add_pending_lookup failed");
    resp = node_arp_send_broadcast_request(n, ointf, nxt_hop_addr);
    EXPECT_RETURN(resp == true, "node_arp_send_broadcast_request failed");
  }
  else if (!arp_entry_is_resolved(arp_entry)) {
    // Entry found, but it is pending
    uint8_t *hdr_payload = payload - sizeof(ether_hdr_t);
    uint32_t hdr_paylen = paylen + sizeof(ether_hdr_t);
    bool resp = arp_entry_add_pending_lookup(arp_entry, hdr_payload, hdr_paylen, pending_lookup_processing_cb, vlan_id);
    EXPECT_RETURN(resp == true, "arp_entry_add_pending_lookup failed");
  }
  else {
    // Found resolved entry - send immediately
    ether_hdr_t *hdr = (ether_hdr_t *)(payload - sizeof(ether_hdr_t));
    uint32_t framelen = sizeof(ether_hdr_t) + paylen;
    layer2_send_with_resolved_arp(n, arp_entry, hdr, framelen, ethertype, vlan_id);
  }
}

int layer2_promote(node_t *n, interface_t *iintf, ether_hdr_t *ether_hdr, uint32_t framelen) {
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
  mac_addr_t dst_mac = ether_hdr_read_dst_mac(ether_hdr);
  if (INTF_MODE(iintf) == INTF_MODE_L3_SVI && !MAC_ADDR_IS_EQUAL(dst_mac, INTF_NETPROP(iintf).l2.mac_addr)) {
    printf(
      "Droped because " MAC_ADDR_FMT " != " MAC_ADDR_FMT "\n", 
      MAC_ADDR_BYTES_BE(dst_mac), MAC_ADDR_BYTES_BE(INTF_NETPROP(iintf).l2.mac_addr)
    );
    return framelen; // We can't process any frames not intended for us is this is an SVI
  }
  if (hdr_type == ETHER_TYPE_IPV4) {
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
  if (INTF_MODE(intf) == INTF_MODE_L2_ACCESS || INTF_MODE(intf) == INTF_MODE_L2_TRUNK) {
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
  else if (INTF_MODE(intf) == INTF_MODE_L3) { 
    // Interface is configured in L3 mode
    return NODE_NETSTACK(n).l2.promote(n, intf, ether_hdr, framelen);
  }
  else if (INTF_MODE(intf) == INTF_MODE_L3_SVI) {
    // Not possible (frames cannot flow in through virtual interfaces)
    LOG_ERR("[%s] L2 received frame on SVI interface! (%s)\n", n->node_name, intf->if_name);
    return 0;
  }
  // Interface is not in a functional state. 
  // Silently drop ingress frames.
  // TODO: Probably add a hw counter to signal dropped packets (?)
  return 0;
}

