// pcap.cpp

#include <arpa/inet.h>
#include "pcap.h"
#include "layer2/layer2.h"
#include "layer2/arp.h"
#include "utils.h"

void pcap_pkt_dump(uint8_t *frame, uint32_t framelen) {
  pcap_pkt_dump_ethernet(frame, framelen);
}

void pcap_pkt_dump_ethernet(uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN(frame != nullptr, "Empty frame ptr param");
  dump_line_indentation_guard_t guard0; 
  dump_line("Ethernet Header\n");
  dump_line("==========================\n");
  dump_line_indentation_add(1);
  ether_hdr_t *ether_hdr = (ether_hdr_t *)frame;
  dump_line("Src. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(ether_hdr->src_mac));
  dump_line("Dst. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(ether_hdr->dst_mac));
  dump_line("Type: ");
  uint16_t ether_type = ntohs(ether_hdr->type);
  uint8_t *payload = (uint8_t *)(ether_hdr + 1);
  uint32_t len = framelen - sizeof(ether_hdr_t);
  switch (ether_type) {
    case ETHER_TYPE_VLAN: {
      printf("VLAN\n"); 
      pcap_pkt_dump_vlan(payload, len);
      break; 
    }
    case ETHER_TYPE_ARP: { 
      printf("ARP\n"); 
      pcap_pkt_dump_arp(payload, len);
      break; 
    }
    case ETHER_TYPE_IPV4: {
      printf("IPv4\n"); 
      pcap_pkt_dump_ipv4(payload, len);
      break; 
    }
    default: { 
      printf("?? (%u)\n", ether_type); 
      break; 
    }
  }
}

void pcap_pkt_dump_vlan(uint8_t *hdr, uint32_t len) {
  dump_line("VLAN Header:");
  dump_line("==========================\n");
}

void pcap_pkt_dump_arp(uint8_t *hdr, uint32_t len) {
  EXPECT_RETURN(hdr != nullptr, "Empty header ptr param"); 
  dump_line_indentation_guard_t guard0; 
  dump_line("ARP Header\n");
  dump_line("==========================\n");
  dump_line_indentation_add(1);
  arp_hdr_t *arp_hdr = (arp_hdr_t *)hdr;
  dump_line("Hardware Type: ");
  uint16_t hw_type = ntohs(arp_hdr->hw_type);
  switch (hw_type) {
    case 1 : printf("Ethernet\n"); break;
    default: printf("%u\n", hw_type);
  }
  dump_line("Protocol Type: ");
  uint16_t proto_type = ntohs(arp_hdr->proto_type);
  switch (proto_type) {
    case ETHER_TYPE_VLAN: printf("VLAN\n"); break; 
    case ETHER_TYPE_ARP: printf("ARP\n"); break; 
    case ETHER_TYPE_IPV4: printf("IPv4\n"); break; 
    default: printf("?? (%u)\n", proto_type); break; 
  }
  dump_line("Hardware Addr. Length: %u\n", (uint16_t)arp_hdr->hw_addr_len);
  dump_line("Protocol Addr. Length: %u\n", (uint16_t)arp_hdr->proto_addr_len);
  dump_line("Op. Code: %u\n", ntohs(arp_hdr->op_code));
  mac_addr_t src_mac;
  memcpy(src_mac.bytes, arp_hdr->src_mac, 6);
  dump_line("Src. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(src_mac));
  ipv4_addr_t src_ip {.value = ntohl(arp_hdr->src_ip)};
  dump_line("Src. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(src_ip));
  mac_addr_t dst_mac;
  memcpy(dst_mac.bytes, arp_hdr->dst_mac, 6);
  dump_line("Dst. MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(dst_mac));
  ipv4_addr_t dst_ip {.value = ntohl(arp_hdr->dst_ip)};
  dump_line("Dst. IPv4 Addr.: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(dst_ip));
}

void pcap_pkt_dump_ipv4(uint8_t *hdr, uint32_t len) {
  dump_line("IPv4 Header:");
  dump_line("==========================\n");
}
