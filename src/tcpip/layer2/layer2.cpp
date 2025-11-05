// layer2.cpp

#include <arpa/inet.h>
#include "layer2.h"
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

#pragma mark -

// Layer 2 processing

bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(ethhdr != nullptr, "Empty ethernet header param", false);
  if (INTF_IS_L3_MODE(intf)) {
    // We will only respond if the frame is specially intended for this
    // interface (based on the dest MAC) or it's a broadcast MAC address.
    mac_addr_t dst_mac = ether_hdr_read_dst_mac(ethhdr);
    if (MAC_ADDR_IS_EQUAL(dst_mac, intf->netprop.mac_addr) || MAC_ADDR_IS_BROADCAST(dst_mac)) {
      return true;
    }
  }
  else if (INTF_IS_L2_MODE(intf)) {
    // This interface is operating in L2 mode
    return true; // Accept all frames
  }
  return false; // Rejected
}

#pragma mark -

// Layer 2 I/O

int layer2_node_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  // Entry point into our TCP/IP stack
  EXPECT_RETURN_VAL(n != nullptr, "Empty node param", -1);
  EXPECT_RETURN_VAL(intf != nullptr, "Empty interface param", -1);
  EXPECT_RETURN_VAL(frame != nullptr, "Empty frame ptr param", -1);
  // First check if we should even consider this frame
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  if (!layer2_qualify_recv_frame_on_interface(intf, ether_hdr)) {
    // Drop the frame
    return framelen;
  }
  if (INTF_IS_L3_MODE(intf)) { 
    // Interface is configured in L3 mode
    uint16_t hdr_type = ether_hdr_read_type(ether_hdr);
    // Check if it's an ARP message
    if (hdr_type == ETHER_TYPE_ARP) {
      arp_hdr_t *arp_hdr = (arp_hdr_t *)(ether_hdr + 1);
      uint16_t arp_op_code = arp_hdr_read_op_code(arp_hdr);
      switch (arp_op_code) {
        case ARP_OP_CODE_REQUEST: {
          bool resp = node_arp_recv_broadcast_request_frame(n, intf, ether_hdr);
          EXPECT_RETURN_VAL(resp == true, "node_arp_recv_broadcast_request_frame failed", -1);
          return framelen;
        }
        case ARP_OP_CODE_REPLY: {
          bool resp = node_arp_recv_reply_frame(n, intf, ether_hdr);
          EXPECT_RETURN_VAL(resp == true, "node_arp_recv_reply_frame failed", -1);
          return framelen;
        }
        default: {
          printf("Unknown ARP op code received! Ignoring...\n");
          return -1;
        }
      }
    }
    else {
      // TODO: We need to delegate processing of this frame to L3
    }
  }
  else if (INTF_IS_L2_MODE(intf)) {
    // TODO: This interface is configured in L2 mode. 
    // Go ahead and act like the good little L2 switch that you are.
    return layer2_switch_recv_frame_bytes(n, intf, frame, framelen);
  }
  // Interface is not in a functional state. 
  // Silently drop igress frames.
  // TODO: Probably add a hw counter to signal dropped packets (?)
  return 0;
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
    return phy_node_flood_frame_bytes(n, iintf, frame, framelen);
  }
  // Else, if we see a destination mac address, we will read the table to find the interface
  mac_entry_t *mac_entry = nullptr;
  // If found, we will send packet off using that interfaces
  mac_addr_t dst_mac = ether_hdr_read_dst_mac(ether_hdr);
  if (mac_table_lookup(n->netprop.mac_table, &dst_mac, &mac_entry)) {
    // Found entry in MAC table
    interface_t *ointf = node_get_interface_by_name(n, (const char *)mac_entry->oif_name);
    EXPECT_RETURN_VAL(ointf != nullptr, "node_get_interface_by_name failed", -1);
    return phy_node_send_frame_bytes(n, ointf, frame, framelen);
  }
  // If not, we will flood all interfaces like we already do below.
  return phy_node_flood_frame_bytes(n, iintf, frame, framelen);
}
