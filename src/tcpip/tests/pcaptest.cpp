// pcaptest.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "pcap.h"
#include "layer2/layer2.h"
#include "layer2/arp.h"
#include "utils.h"

void test_pcap_dump_untagged_arp_request() {
  // Allocate frame: Ethernet header + ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + sizeof(arp_hdr_t);
  uint8_t *frame = (uint8_t *)calloc(1, framelen);
  // Fill Ethernet header
  ether_hdr_t *eth_hdr = (ether_hdr_t *)frame;
  mac_addr_t src_mac = {.bytes = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
  mac_addr_t dst_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // Broadcast
  ether_hdr_set_src_mac(eth_hdr, &src_mac);
  ether_hdr_set_dst_mac(eth_hdr, &dst_mac);
  ether_hdr_set_type(eth_hdr, ETHER_TYPE_ARP);
  // Fill ARP header
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(eth_hdr + 1);
  arp_hdr_set_hw_type(arp_hdr, ARP_HW_TYPE_ETHERNET);
  arp_hdr_set_proto_type(arp_hdr, ETHER_TYPE_IPV4);
  arp_hdr_set_hw_addr_len(arp_hdr, 6);
  arp_hdr_set_proto_addr_len(arp_hdr, 4);
  arp_hdr_set_op_code(arp_hdr, ARP_OP_CODE_REQUEST);
  // Source (sender) info
  mac_addr_t arp_src_mac = {.bytes = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
  ipv4_addr_t arp_src_ip = {.bytes = {192, 168, 1, 10}};
  arp_hdr_set_src_mac(arp_hdr, &arp_src_mac);
  arp_hdr_set_src_ip(arp_hdr, &arp_src_ip);
  // Target (destination) info
  mac_addr_t arp_dst_mac = {.bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; // Unknown
  ipv4_addr_t arp_dst_ip = {.bytes = {192, 168, 1, 1}}; // Gateway IP
  arp_hdr_set_dst_mac(arp_hdr, &arp_dst_mac);
  arp_hdr_set_dst_ip(arp_hdr, &arp_dst_ip);
  // Dump the packet
  printf("\n=== UNTAGGED ARP REQUEST ===\n");
  pcap_pkt_dump(frame, framelen);
  // Cleanup
  free(frame);
}

void test_pcap_dump_single_vlan_tagged_arp_request() {
  // Allocate frame: Ethernet header + VLAN tag + ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + sizeof(vlan_tag_t) + sizeof(arp_hdr_t);
  uint8_t *frame = (uint8_t *)calloc(1, framelen);
  // Fill Ethernet header
  ether_hdr_t *eth_hdr = (ether_hdr_t *)frame;
  mac_addr_t src_mac = {.bytes = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33}};
  mac_addr_t dst_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // Broadcast
  ether_hdr_set_src_mac(eth_hdr, &src_mac);
  ether_hdr_set_dst_mac(eth_hdr, &dst_mac);
  ether_hdr_set_type(eth_hdr, ETHER_TYPE_VLAN); // VLAN tag follows
  // Fill VLAN tag
  vlan_tag_t *vlan_tag = (vlan_tag_t *)(eth_hdr + 1);
  vlan_tag_set_pcp(vlan_tag, 3);        // Priority: 3
  vlan_tag_set_dei(vlan_tag, 0);        // Drop Eligible: 0
  vlan_tag_set_vlan_id(vlan_tag, 100);  // VLAN ID: 100
  vlan_tag_set_ether_type(vlan_tag, ETHER_TYPE_ARP);
  // Fill ARP header
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(vlan_tag + 1);
  arp_hdr_set_hw_type(arp_hdr, ARP_HW_TYPE_ETHERNET);
  arp_hdr_set_proto_type(arp_hdr, ETHER_TYPE_IPV4);
  arp_hdr_set_hw_addr_len(arp_hdr, 6);
  arp_hdr_set_proto_addr_len(arp_hdr, 4);
  arp_hdr_set_op_code(arp_hdr, ARP_OP_CODE_REQUEST);
  // Source (sender) info
  mac_addr_t arp_src_mac = {.bytes = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33}};
  ipv4_addr_t arp_src_ip = {.bytes = {10, 0, 0, 50}};
  arp_hdr_set_src_mac(arp_hdr, &arp_src_mac);
  arp_hdr_set_src_ip(arp_hdr, &arp_src_ip);
  // Target (destination) info
  mac_addr_t arp_dst_mac = {.bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; // Unknown
  ipv4_addr_t arp_dst_ip = {.bytes = {10, 0, 0, 1}}; // Gateway IP
  arp_hdr_set_dst_mac(arp_hdr, &arp_dst_mac);
  arp_hdr_set_dst_ip(arp_hdr, &arp_dst_ip);
  // Dump the packet
  printf("\n=== SINGLE VLAN-TAGGED ARP REQUEST (VLAN 100) ===\n");
  pcap_pkt_dump(frame, framelen);
  // Cleanup
  free(frame);
}

void test_pcap_dump_double_vlan_tagged_arp_request() {
  // Allocate frame: Ethernet header + VLAN tag + VLAN tag + ARP header
  uint32_t framelen = sizeof(ether_hdr_t) + 2 * sizeof(vlan_tag_t) + sizeof(arp_hdr_t);
  uint8_t *frame = (uint8_t *)calloc(1, framelen);
  // Fill Ethernet header
  ether_hdr_t *eth_hdr = (ether_hdr_t *)frame;
  mac_addr_t src_mac = {.bytes = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE}};
  mac_addr_t dst_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // Broadcast
  ether_hdr_set_src_mac(eth_hdr, &src_mac);
  ether_hdr_set_dst_mac(eth_hdr, &dst_mac);
  ether_hdr_set_type(eth_hdr, ETHER_TYPE_VLAN); // Outer VLAN tag follows
  // Fill outer VLAN tag
  vlan_tag_t *outer_vlan = (vlan_tag_t *)(eth_hdr + 1);
  vlan_tag_set_pcp(outer_vlan, 5);        // Priority: 5
  vlan_tag_set_dei(outer_vlan, 0);        // Drop Eligible: 0
  vlan_tag_set_vlan_id(outer_vlan, 200);  // Outer VLAN ID: 200
  vlan_tag_set_ether_type(outer_vlan, ETHER_TYPE_VLAN); // Inner VLAN tag follows
  // Fill inner VLAN tag
  vlan_tag_t *inner_vlan = (vlan_tag_t *)(outer_vlan + 1);
  vlan_tag_set_pcp(inner_vlan, 2);        // Priority: 2
  vlan_tag_set_dei(inner_vlan, 0);        // Drop Eligible: 0
  vlan_tag_set_vlan_id(inner_vlan, 150);  // Inner VLAN ID: 150
  vlan_tag_set_ether_type(inner_vlan, ETHER_TYPE_ARP);
  // Fill ARP header
  arp_hdr_t *arp_hdr = (arp_hdr_t *)(inner_vlan + 1);
  arp_hdr_set_hw_type(arp_hdr, ARP_HW_TYPE_ETHERNET);
  arp_hdr_set_proto_type(arp_hdr, ETHER_TYPE_IPV4);
  arp_hdr_set_hw_addr_len(arp_hdr, 6);
  arp_hdr_set_proto_addr_len(arp_hdr, 4);
  arp_hdr_set_op_code(arp_hdr, ARP_OP_CODE_REQUEST);
  // Source 
  mac_addr_t arp_src_mac = {.bytes = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE}};
  ipv4_addr_t arp_src_ip = {.bytes = {172, 16, 10, 5}};
  arp_hdr_set_src_mac(arp_hdr, &arp_src_mac);
  arp_hdr_set_src_ip(arp_hdr, &arp_src_ip);
  // Target
  mac_addr_t arp_dst_mac = {.bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; // Unknown
  ipv4_addr_t arp_dst_ip = {.bytes = {172, 16, 10, 1}}; // Gateway IP
  arp_hdr_set_dst_mac(arp_hdr, &arp_dst_mac);
  arp_hdr_set_dst_ip(arp_hdr, &arp_dst_ip);
  // Dump the packet
  printf("\n=== DOUBLE VLAN-TAGGED ARP REQUEST (Outer VLAN 200, Inner VLAN 150) ===\n");
  pcap_pkt_dump(frame, framelen);
  // Cleanup
  free(frame);
}

int main(int argc, char *argv[]) {
  // Manual tests
  test_pcap_dump_untagged_arp_request();
  test_pcap_dump_single_vlan_tagged_arp_request();
  test_pcap_dump_double_vlan_tagged_arp_request();
  return 0;
}
