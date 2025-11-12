// layer3.cpp

#include "layer3.h"
#include "layer2/layer2.h"
#include "layer5/layer5.h"
#include "phy.h"

void layer3_demote(node_t *n, uint8_t *payload, uint32_t paylen, uint8_t prot, ipv4_addr_t *dst_addr) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(payload != nullptr, "Empty payload param");
  EXPECT_RETURN(dst_addr != nullptr, "Empty destination address param");
  // Decide on next hop address and outgoing interface
  rt_entry_t *entry = nullptr;
  if (!rt_lookup(n->netprop.r_table, dst_addr, &entry)) {
    // We couldn't find any matching prefix in the routing table. Drop.
    return;
  }
  ipv4_addr_t *next_hop_addr = nullptr;
  interface_t *ointf = nullptr;
  if (entry->is_direct) {
    // self-ping or direct delivery
    next_hop_addr = dst_addr;
    if (node_is_local_address(n, dst_addr)) {
      ointf = nullptr; // self-ping
    }
    else {
      bool resp = node_get_interface_matching_subnet(n, dst_addr, &ointf);
      EXPECT_RETURN(resp == true, "node_get_interface_matching_subnet failed");
      EXPECT_RETURN(ointf != nullptr, "node_get_interface_matching_subnet failed");
    }
  }
  else {
    next_hop_addr = &entry->gw_ip;
    ointf = node_get_interface_by_name(n, entry->oif_name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
  }
  // Prepare ipv4 packet
  uint8_t *buffer = (uint8_t *)malloc(CONFIG_MAX_PACKET_BUFFER_SIZE);
  memset(buffer, 0, CONFIG_MAX_PACKET_BUFFER_SIZE);
  // Setup IPv4 header
  ipv4_hdr_t *hdr = (ipv4_hdr_t *)buffer;
  ipv4_hdr_set_version(hdr, 4);
  ipv4_hdr_set_ihl(hdr, 5);
  uint32_t pktlen = (5 * 4) + paylen;
  ipv4_hdr_set_total_length(hdr, pktlen);
  ipv4_hdr_set_flags(hdr, 0b010); // Don't Fragment flag => 1
  ipv4_hdr_set_ttl(hdr, 10); // TODO: TTL default??
  ipv4_hdr_set_protocol(hdr, prot);
  ipv4_hdr_set_checksum(hdr, 0);
  if (ointf != nullptr) {
    ipv4_hdr_set_src_addr(hdr, &ointf->netprop.ip.addr);
  }
  ipv4_hdr_set_dst_addr(hdr, dst_addr); // <= This is NOT next hop address
  // Next, find the start of payload and copy provided pkt
  uint8_t *pkt_payload = (uint8_t *)(hdr + 1);
  memcpy(pkt_payload, payload, paylen);
  // Next, shift the contents to the right
  uint8_t *ptr = buffer;
  bool resp = phy_frame_buffer_shift_right(&ptr, pktlen, CONFIG_MAX_PACKET_BUFFER_SIZE);
  EXPECT_RETURN(resp == true, "phy_frame_buffer_shift_right failed");
  // Finally, hand over the packet to Layer2
  layer2_demote(n, next_hop_addr, ointf, ptr, pktlen, ETHER_TYPE_IPV4);
  // Free memory
  free(buffer);
}

void layer3_recv_ipv4_pkt(node_t *n, interface_t *intf, ipv4_hdr_t *hdr, uint32_t pktlen) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  //EXPECT_RETURN(intf != nullptr, "Empty interface param");
  EXPECT_RETURN(hdr != nullptr, "Empty pkt header param");
  // Check if we can find an entry for the destination address in the routing table
  ipv4_addr_t dst_addr = ipv4_hdr_read_dst_addr(hdr);
  rt_entry_t *rt_entry = nullptr;
  if (!rt_lookup(n->netprop.r_table, &dst_addr, &rt_entry)) {
    return; // Discard packet since no route was found
  }
  // Not direct route?
  if (!rt_entry->is_direct) {
    // Update dst ip (to gateway ip) and hand it over to L2 for forwarding
    interface_t *ointf = node_get_interface_by_name(n, (const char *)rt_entry->oif_name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
    ipv4_hdr_set_dst_addr(hdr, &rt_entry->gw_ip);
    ipv4_hdr_set_ttl(hdr, ipv4_hdr_read_ttl(hdr) - 1);
    if (ipv4_hdr_read_ttl(hdr) == 0) {
      return; // drop 
    }
    layer2_demote(n, &rt_entry->gw_ip, ointf, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
    return;
  }
  // Local address?
  ipv4_addr_t dest_addr = ipv4_hdr_read_dst_addr(hdr);
  if (node_is_local_address(n, &dst_addr)) {
    uint16_t prot = ipv4_hdr_read_protocol(hdr);
    uint8_t *payload = (uint8_t *)(hdr + 1);
    uint32_t payloadsize = ipv4_hdr_read_total_length(hdr) - (ipv4_hdr_read_ihl(hdr) * 4);
    layer5_promote(n, intf, payload, payloadsize, &dest_addr, prot);
    return;
  }
  // Local subnet
  layer2_demote(n, &dest_addr, nullptr, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
}

void layer3_promote(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen, uint16_t ether_type) {
  if (ether_type != ETHER_TYPE_IPV4) {
    // We only accept IPV4 packets
    return;
  } 
  return layer3_recv_ipv4_pkt(n, intf, (ipv4_hdr_t *)pkt, pktlen);
}

