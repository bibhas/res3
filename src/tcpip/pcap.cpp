// pcap.cpp

#include <arpa/inet.h>
#include "pcap.h"
#include "layer2/arp_hdr.h"
#include "layer2/ether_hdr.h"
#include "layer2/vlan_tag.h"
#include "layer3/layer3.h"
#include "utils.h"

void pcap_pkt_dump(uint8_t *frame, uint32_t framelen) {
  pcap_pkt_dump_ethernet(frame, framelen);
}

void pcap_pkt_dump_ethernet(uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN(frame != nullptr, "Empty frame ptr param");
  dump_line_indentation_guard_t guard0; 
  dump_line("==========================\n");
  dump_line("Ethernet Header\n");
  dump_line("==========================\n");
  dump_line_indentation_add(1);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  mac_addr_t src_mac = ether_hdr_read_src_mac(ether_hdr);
  dump_line("Src. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(src_mac));
  mac_addr_t dst_mac = ether_hdr_read_dst_mac(ether_hdr);
  dump_line("Dst. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(dst_mac));
  uint16_t ether_type = ether_hdr_read_type(ether_hdr);
  uint8_t *payload = (uint8_t *)(ether_hdr + 1);
  uint32_t len = framelen - sizeof(ether_hdr_t);
  switch (ether_type) {
    case ETHER_TYPE_VLAN: {
      pcap_pkt_dump_vlan(payload, len);
      break; 
    }
    case ETHER_TYPE_ARP: { 
      dump_line("Type: ARP\n"); 
      pcap_pkt_dump_arp(payload, len);
      break; 
    }
    case ETHER_TYPE_IPV4: {
      dump_line("Type: IPv4\n"); 
      pcap_pkt_dump_ipv4(payload, len);
      break; 
    }
    default: { 
      dump_line("Type: ?? (%u)\n", ether_type); 
      break; 
    }
  }
}

void pcap_pkt_dump_vlan(uint8_t *hdr, uint32_t framelen) {
  dump_line_indentation_push();
  dump_line("VLAN Tag:\n");
  dump_line_indentation_add(1);
  vlan_tag_t *vlan_tag = (vlan_tag_t *)hdr;
  dump_line("Priority Code Point: %u\n", vlan_tag_read_pcp(vlan_tag));
  dump_line("Drop Eligible Indicator: %u\n", vlan_tag_read_dei(vlan_tag));
  dump_line("VLAN ID: %u\n", vlan_tag_read_vlan_id(vlan_tag));
  dump_line_indentation_pop();
  uint16_t ether_type = vlan_tag_read_ether_type(vlan_tag);
  uint8_t *payload = (uint8_t *)(vlan_tag + 1);
  uint32_t len = framelen - sizeof(vlan_tag_t);
  switch (ether_type) {
    case ETHER_TYPE_VLAN: {
      pcap_pkt_dump_vlan(payload, len);
      break; 
    }
    case ETHER_TYPE_ARP: { 
      dump_line("Type: ARP\n");
      pcap_pkt_dump_arp(payload, len);
      break; 
    }
    case ETHER_TYPE_IPV4: {
      dump_line("Type: IPv4\n");
      pcap_pkt_dump_ipv4(payload, len);
      break; 
    }
    default: { 
      dump_line("Type: ?? (%u)\n", ether_type);
      break; 
    }
  }
}

void pcap_pkt_dump_arp(uint8_t *hdr, uint32_t len) {
  EXPECT_RETURN(hdr != nullptr, "Empty header ptr param"); 
  dump_line_indentation_guard_t guard0; 
  dump_line("==========================\n");
  dump_line("ARP Header\n");
  dump_line("==========================\n");
  dump_line_indentation_add(1);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)hdr;
  dump_line("Hardware Type: ");
  uint16_t hw_type = arp_hdr_read_hw_type(arp_hdr);
  switch (hw_type) {
    case 1 : printf("Ethernet\n"); break;
    default: printf("%u\n", hw_type);
  }
  dump_line("Protocol Type: ");
  uint16_t proto_type = arp_hdr_read_proto_type(arp_hdr);
  switch (proto_type) {
    case ETHER_TYPE_VLAN: printf("VLAN\n"); break;
    case ETHER_TYPE_ARP: printf("ARP\n"); break;
    case ETHER_TYPE_IPV4: printf("IPv4\n"); break;
    default: printf("?? (%u)\n", proto_type); break;
  }
  dump_line("Hardware Addr. Length: %u\n", (uint16_t)arp_hdr_read_hw_addr_len(arp_hdr));
  dump_line("Protocol Addr. Length: %u\n", (uint16_t)arp_hdr_read_proto_addr_len(arp_hdr));
  dump_line("Op. Code: %u\n", arp_hdr_read_op_code(arp_hdr));
  mac_addr_t src_mac = arp_hdr_read_src_mac(arp_hdr);
  dump_line("Src. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(src_mac));
  ipv4_addr_t src_ip = arp_hdr_read_src_ip(arp_hdr);
  dump_line("Src. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(src_ip));
  mac_addr_t dst_mac = arp_hdr_read_dst_mac(arp_hdr);
  dump_line("Dst. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(dst_mac));
  ipv4_addr_t dst_ip = arp_hdr_read_dst_ip(arp_hdr);
  dump_line("Dst. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(dst_ip));
}

void pcap_pkt_dump_ipv4(uint8_t *hdr, uint32_t len) {
  EXPECT_RETURN(hdr != nullptr, "Empty header ptr param");
  dump_line_indentation_guard_t guard0;
  dump_line("==========================\n");
  dump_line("IPv4 Header\n");
  dump_line("==========================\n");
  dump_line_indentation_add(1);
  ipv4_hdr_t *ipv4_hdr = (ipv4_hdr_t *)hdr;
  dump_line("Version: %u\n", ipv4_hdr_read_version(ipv4_hdr));
  dump_line("IHL: %u (%u bytes)\n", ipv4_hdr_read_ihl(ipv4_hdr), ipv4_hdr_read_ihl(ipv4_hdr) * 4);
  dump_line("Total Length: %u\n", ipv4_hdr_read_total_length(ipv4_hdr));
  dump_line("Packet ID: %u\n", ipv4_hdr_read_packet_id(ipv4_hdr));
  dump_line("Flags: 0x%X\n", ipv4_hdr_read_flags(ipv4_hdr));
  dump_line("Fragment Offset: %u\n", ipv4_hdr_read_fragment_offset(ipv4_hdr));
  dump_line("TTL: %u\n", ipv4_hdr_read_ttl(ipv4_hdr));
  dump_line("Protocol: ");
  uint8_t protocol = ipv4_hdr_read_protocol(ipv4_hdr);
  switch (protocol) {
    case PROT_ICMP: printf("ICMP\n"); break;
    case PROT_IPIP: printf("IP-in-IP\n"); break;
    case PROT_UDP: printf("UDP\n"); break;
    default: printf("%u\n", protocol); break;
  }
  dump_line("Checksum: 0x%04X\n", ipv4_hdr_read_checksum(ipv4_hdr));
  ipv4_addr_t src_addr = ipv4_hdr_read_src_addr(ipv4_hdr);
  dump_line("Src. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(src_addr));
  ipv4_addr_t dst_addr = ipv4_hdr_read_dst_addr(ipv4_hdr);
  dump_line("Dst. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(dst_addr));
}
