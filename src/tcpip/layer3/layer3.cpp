// layer3.cpp

#include "layer3.h"
#include "layer2/layer2.h"
#include "layer5/layer5.h"
#include "graph.h"
#include "phy.h"

void __layer3_demote(node_t *n, uint8_t *payload, uint32_t paylen, uint8_t prot, ipv4_addr_t *dst_addr) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(payload != nullptr, "Empty payload param");
  EXPECT_RETURN(dst_addr != nullptr, "Empty destination address param");
  // Decide on next hop address and outgoing interface
  ipv4_addr_t *next_hop_addr = nullptr;
  interface_t *ointf = nullptr;
  bool resp = layer3_resolve_next_hop(n, dst_addr, &next_hop_addr, &ointf);
  EXPECT_RETURN(resp == true, "layer3_resolve_next_hop failed");
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
    ipv4_hdr_set_src_addr(hdr, &INTF_NETPROP(ointf).l3.addr);
  }
  ipv4_hdr_set_dst_addr(hdr, dst_addr); // <= This is NOT next hop address
  // Next, find the start of payload and copy provided pkt
  uint8_t *pkt_payload = (uint8_t *)(hdr + 1);
  memcpy(pkt_payload, payload, paylen);
  // Next, shift the contents to the right
  uint8_t *ptr = buffer;
  resp = phy_frame_buffer_shift_right(&ptr, pktlen, CONFIG_MAX_PACKET_BUFFER_SIZE);
  EXPECT_RETURN(resp == true, "phy_frame_buffer_shift_right failed");
  // Finally, hand over the packet to Layer2
  NODE_NETSTACK(n).l2.demote(n, next_hop_addr, ointf, ptr, pktlen, ETHER_TYPE_IPV4);
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
    EXPECT_RETURN(rt_entry->oif.configured, "Missing OIF name in RT entry");
    EXPECT_RETURN(rt_entry->gw.configured, "Missing GW IP address in RT entry");
    interface_t *ointf = node_get_interface_by_name(n, (const char *)rt_entry->oif.name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
    ipv4_hdr_set_ttl(hdr, ipv4_hdr_read_ttl(hdr) - 1);
    if (ipv4_hdr_read_ttl(hdr) == 0) {
      printf("TTL == 0\n");
      return; // drop 
    }
    NODE_NETSTACK(n).l2.demote(n, &rt_entry->gw.ip, ointf, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
    return;
  }
  else if (rt_entry->is_direct && rt_entry->oif.configured) {
    // SVI routes are direct but have a specified outgoing interface
    interface_t *ointf = node_get_interface_by_name(n, (const char *)rt_entry->oif.name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
    EXPECT_RETURN(INTF_MODE(ointf) == INTF_MODE_L3_SVI, "Encountered non-SVI local interface!");
    ipv4_hdr_set_ttl(hdr, ipv4_hdr_read_ttl(hdr) - 1);
    if (ipv4_hdr_read_ttl(hdr) == 0) {
      printf("TTL == 0\n");
      return; // drop 
    }
    if (rt_entry->gw.configured) {
      // A GW address has been configured for this SVI (use that as the next hop)
      NODE_NETSTACK(n).l2.demote(n, &rt_entry->gw.ip, ointf, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
    }
    else {
      // No GW address has been configured for this SVI. In this we expect the destination to be within the
      // broadcast domain. Use that as the next hop address.
      NODE_NETSTACK(n).l2.demote(n, &dst_addr, ointf, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
    }
    return;
  }
  // Local address?
  ipv4_addr_t dest_addr = ipv4_hdr_read_dst_addr(hdr);
  ipv4_addr_t src_addr = ipv4_hdr_read_src_addr(hdr);
  if (node_is_local_address(n, &dst_addr)) {
    uint16_t prot = ipv4_hdr_read_protocol(hdr);
    uint8_t *payload = (uint8_t *)(hdr + 1);
    uint32_t payloadsize = ipv4_hdr_read_total_length(hdr) - (ipv4_hdr_read_ihl(hdr) * 4);
    if (prot == PROT_IPIP) {
      // Handle IP-in-IP tunneling
      // The packet has reached the ERO destination. We now need to strip the outer IPv4 header
      // and send it off to its destination. 
      bool resp = layer3_forward_ipnp(n, payload, payloadsize);
      EXPECT_RETURN(resp == true, "layer3_forward_ipnp failed");
      return;  
    }
    NODE_NETSTACK(n).l5.promote(n, intf, payload, payloadsize, &src_addr, prot);
    return;
  }
  // Local subnet
  NODE_NETSTACK(n).l2.demote(n, &dest_addr, nullptr, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
}

void __layer3_promote(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen, uint16_t ether_type) {
  if (ether_type != ETHER_TYPE_IPV4) {
    // We only accept IPV4 packets
    return;
  } 
  return layer3_recv_ipv4_pkt(n, intf, (ipv4_hdr_t *)pkt, pktlen);
}

#pragma mark -

// Utils

bool layer3_resolve_next_hop(node_t *n, ipv4_addr_t *dst_addr, ipv4_addr_t **hop_addr, interface_t **ointf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(dst_addr != nullptr, "Empty dest addrress param", false);
  EXPECT_RETURN_BOOL(hop_addr != nullptr, "Empty out next hop address param", false);
  EXPECT_RETURN_BOOL(ointf != nullptr, "Empty out interface param", false);
  rt_entry_t *entry = nullptr;
  if (!rt_lookup(n->netprop.r_table, dst_addr, &entry)) {
    // We couldn't find any matching prefix in the routing table. Drop.
    return false;
  }
  ipv4_addr_t *next_hop_addr = nullptr;
  interface_t *intf = nullptr;
  if (entry->is_direct) {
    // self-ping or direct delivery
    next_hop_addr = dst_addr;
    if (node_is_local_address(n, dst_addr)) {
      intf = nullptr; // self-ping
    }
    else {
      bool resp = node_get_interface_matching_subnet(n, dst_addr, &intf);
      EXPECT_RETURN_BOOL(resp == true, "node_get_interface_matching_subnet failed", false);
      EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_matching_subnet failed", false);
    }
  }
  else {
    next_hop_addr = &entry->gw.ip;
    intf = node_get_interface_by_name(n, entry->oif.name);
    EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_by_name failed", false);
  }
  *ointf = intf;
  *hop_addr = next_hop_addr;
  return true;
}

bool layer3_resolve_src_for_dst(node_t *n, ipv4_addr_t *dst_addr, ipv4_addr_t **src_addr, interface_t **ointf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(dst_addr != nullptr, "Empty addr param", false);
  EXPECT_RETURN_BOOL(src_addr != nullptr, "Empty return src addr ptr param", false);
  rt_entry_t *entry = nullptr;
  if (!rt_lookup(n->netprop.r_table, dst_addr, &entry)) {
    // We couldn't find any matching prefix in the routing table.
    *src_addr = nullptr;
    if (ointf) {
      *ointf = nullptr;
    }
    return false;
  }
  if (!entry->is_direct) {
    interface_t *intf = node_get_interface_by_name(n, entry->oif.name);
    if (ointf) {
      *ointf = intf;
    }
    *src_addr = &INTF_NETPROP(intf).l3.addr;
    if (ointf) {
      EXPECT_RETURN_BOOL(*ointf != nullptr, "node_get_interface_by_name failed", false);
    }
    return true;
  }
  else {
    if (node_is_local_address(n, dst_addr)) {
      *src_addr = &n->netprop.loopback.addr;
      if (ointf) {
        *ointf = nullptr; // No outgoing interface if dst is a local address
      }
      return true;
    }
    else {
      interface_t *intf = nullptr;
      bool resp = node_get_interface_matching_subnet(n, dst_addr, &intf);
      EXPECT_RETURN_BOOL(resp == true, "node_get_interface_matching_subnet failed", false);
      EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_matching_subnet failed", false);
      *src_addr = &INTF_NETPROP(intf).l3.addr;
      if (ointf) {
        *ointf = intf;
      }
      return true;
    }
  }
  return false;
}

#pragma mark -

// IP-in-IP Encapsulation 

void layer3_demote_ipnip(node_t *n, uint8_t *pkt, uint32_t pktlen, uint8_t prot, ipv4_addr_t *dst_addr, ipv4_addr_t *ero_addr) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(pkt != nullptr, "Empty pkt param");
  EXPECT_RETURN(dst_addr != nullptr, "Empty destination address param");
  EXPECT_RETURN(ero_addr != nullptr, "Empty ERO address param");
  EXPECT_RETURN(pktlen < CONFIG_MAX_PACKET_BUFFER_SIZE - sizeof(ipv4_hdr_t), "Invalid pktlen param");
  // First, resolve the source address and/or outgoing interface for the
  // provided ERO address. Note how we're not using the final destination
  // address (even though the inner header we prepare here embeds that
  // address). That's because, regardless of the final destination, the
  // encapsulated packet still needs to be routed to the ERO first. Also, the
  // ERO address could be a local address, in which case, the outgoing
  // interface would be nullptr.
  interface_t *ointf = nullptr;
  ipv4_addr_t *src_addr = nullptr;
  bool resp = layer3_resolve_src_for_dst(n, ero_addr, &src_addr, &ointf);
  EXPECT_RETURN(resp == true, "layer3_resolve_src_for_dst failed");
  EXPECT_RETURN(src_addr != nullptr, "layer3_resolve_src_for_dst failed");
  // Append an IPv4 header with the actual destination address and PROTOCOL type.
  // Then, demote it to layer3 using the ERO address as the dest.
  uint8_t *buffer = (uint8_t *)calloc(1, CONFIG_MAX_PACKET_BUFFER_SIZE);
  ipv4_hdr_t *hdr = (ipv4_hdr_t *)buffer;
  ipv4_hdr_set_version(hdr, 4);
  ipv4_hdr_set_ihl(hdr, 5);
  uint32_t bufflen = (5 * 4) + pktlen;
  ipv4_hdr_set_total_length(hdr, bufflen);
  ipv4_hdr_set_flags(hdr, 0b010); // Don't Fragment flag => 1
  ipv4_hdr_set_ttl(hdr, 10); // TODO: TTL default??
  ipv4_hdr_set_protocol(hdr, prot);
  ipv4_hdr_set_checksum(hdr, 0);
  ipv4_hdr_set_src_addr(hdr, src_addr);
  ipv4_hdr_set_dst_addr(hdr, dst_addr); // <= This is NOT ERO address
  // Next, find the start of payload and copy provided pkt
  uint8_t *encap_payload = (uint8_t *)(hdr + 1);
  memcpy(encap_payload, pkt, pktlen);
  // Demote the packet (in the usual fashion) to layer3.
  NODE_NETSTACK(n).l3.demote(n, buffer, sizeof(ipv4_hdr_t) + bufflen, PROT_IPIP, ero_addr);
  // Finally, free the buffer (layer3 will copy it out to its own buffer)
  free(buffer);
}

/*
 * Things to do:
 *    - Fill in the src address for the inner hdr
 *    - Find the next hop address / interface
 *    - Demote to layer2 (if the address is local, it'll work like self-ping)
 */
bool layer3_forward_ipnp(node_t *n, uint8_t *payload, uint32_t paylen) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(payload != nullptr, "Empty payload param", false);
  ipv4_hdr_t *hdr = (ipv4_hdr_t *)payload;
  // Read final destination address
  ipv4_addr_t dst_addr = ipv4_hdr_read_dst_addr(hdr);
  // Resolve source address to use
  ipv4_addr_t *src_addr = nullptr;
  bool resp = layer3_resolve_src_for_dst(n, &dst_addr, &src_addr);
  EXPECT_RETURN_BOOL(resp == true, "layer3_resolve_src_for_dst failed", false);
  EXPECT_RETURN_BOOL(src_addr != nullptr, "layer3_resolve_src_for_dst failed", false);
  // Resolve next hop and outgoing interface
  ipv4_addr_t *hop_addr = nullptr;
  interface_t *ointf = nullptr;
  resp = layer3_resolve_next_hop(n, &dst_addr, &hop_addr, &ointf);
  // Update IPv4 header fields
  ipv4_hdr_set_src_addr(hdr, src_addr);
  // Demote to layer2 using next hop address and interface
  NODE_NETSTACK(n).l2.demote(n, hop_addr, ointf, payload, paylen, ETHER_TYPE_IPV4);
  return true;
}
