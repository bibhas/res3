// pcap.h

#pragma once

#include <cstdint>

void pcap_pkt_dump(uint8_t *frame, uint32_t framelen);
void pcap_pkt_dump_ethernet(uint8_t *frame, uint32_t framelen);
void pcap_pkt_dump_vlan(uint8_t *hdr, uint32_t len);
void pcap_pkt_dump_arp(uint8_t *hdr, uint32_t len);
void pcap_pkt_dump_ipv4(uint8_t *hdr, uint32_t len);
