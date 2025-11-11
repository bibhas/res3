// layer3.cpp

#include "layer3.h"
#include "layer2/layer2.h"
#include "layer5/layer5.h"

void layer3_demote(node_t *n, uint8_t *pkt, uint32_t pktlen, uint8_t prot, ipv4_addr_t *dst_addr) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(pkt != nullptr, "Empty pkt param");
  EXPECT_RETURN(dst_addr != nullptr, "Empty destination address param");
  // Need to add IPv4 header, shift right and demote to l2
}

void layer3_recv_ipv4_pkt(node_t *n, interface_t *intf, ipv4_hdr_t *hdr, uint32_t pktlen) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  EXPECT_RETURN(hdr != nullptr, "Empty pkt header param");
  // Check if we can find an entry for the destination address in the routing table
  ipv4_addr_t dst_addr = ipv4_hdr_read_dst_addr(hdr);
  rt_entry_t *rt_entry = nullptr;
  if (!rt_lookup(n->netprop.r_table, &dst_addr, &rt_entry)) {
    return; // Discard packet since no route was found
  }
  // Direct route?
  if (!rt_entry->is_direct) {
    // Update dst ip (to gateway ip) and hand it over to L2 for forwarding
    interface_t *ointf = node_get_interface_by_name(n, (const char *)rt_entry->oif_name);
    EXPECT_RETURN(ointf != nullptr, "node_get_interface_by_name failed");
    ipv4_hdr_set_dst_addr(hdr, &rt_entry->gw_ip);
    ipv4_hdr_set_ttl(hdr, ipv4_hdr_read_ttl(hdr) - 1);
    if (ipv4_hdr_read_ttl(hdr) == 0) {
      return; // drop 
    }
    layer2_demote(n, ointf, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
    return;
  }
  // Local address?
  if (node_is_local_address(n, &dst_addr)) {
    uint16_t prot = ipv4_hdr_read_protocol(hdr);
    uint8_t *payload = (uint8_t *)(hdr + 1);
    uint32_t payloadsize = ipv4_hdr_read_total_length(hdr) - (ipv4_hdr_read_ihl(hdr) * 4);
    layer5_promote(n, intf, payload, payloadsize, prot);
    return;
  }
  // Local subnet
  layer2_demote(n, nullptr, (uint8_t *)hdr, pktlen, ETHER_TYPE_IPV4);
}

void layer3_promote(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen, uint16_t ether_type) {
  if (ether_type != ETHER_TYPE_IPV4) {
    // We only accept IPV4 packets
    return;
  } 
  return layer3_recv_ipv4_pkt(n, intf, (ipv4_hdr_t *)pkt, pktlen);
}

