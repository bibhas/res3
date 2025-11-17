// layer2.h

#pragma once

#include <functional>
#include <arpa/inet.h>
#include "utils.h"

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct ether_hdr_t ether_hdr_t;

#pragma mark -

// Layer 2 processing

using layer2_promote_fn_t = std::function<int(node_t*,interface_t*,ether_hdr_t*,uint32_t)>;
using layer2_demote_fn_t = std::function<void(node_t*,ipv4_addr_t*,interface_t*,uint8_t*,uint32_t,uint16_t)>;

int layer2_promote(node_t *n, interface_t *iintf, ether_hdr_t *ether_hdr, uint32_t framelen);
void layer2_demote(node_t *n, ipv4_addr_t *nxt_hop_addr, interface_t *ointf, uint8_t *payload, uint32_t paylen, uint16_t ethertype);

bool node_arp_send_broadcast_request(node_t *n, interface_t *intf, ipv4_addr_t *ip_addr);
bool node_arp_recv_broadcast_request_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr);
bool node_arp_recv_reply_frame(node_t *n, interface_t *iintf, ether_hdr_t *hdr);
bool node_arp_send_reply_frame(node_t *n, interface_t *ointf, ether_hdr_t *in_ether_hdr);

ether_hdr_t* ether_hdr_tag_vlan(ether_hdr_t *hdr, uint32_t len, uint16_t vlanid, uint32_t *newlen);
ether_hdr_t* ether_hdr_untag_vlan(ether_hdr_t *hdr, uint32_t len, uint32_t *newlen);

bool layer2_qualify_recv_frame_on_interface(interface_t *intf, ether_hdr_t *ethhdr, uint16_t *vlan_id);
int layer2_node_recv_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);
