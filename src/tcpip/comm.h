// comm.h

#pragma once

#include <atomic>
#include <cstdint>
#include "graph.h"

#pragma mark -

// General

/*
 * Upon receiving a packet, the thread will pass on the parsed packet to
 * `layer2_frame_recv()` function. See `layer2/layer2.cpp`
 */
void comm_pkt_receiver_thread_main(graph_t *topo);
bool comm_pkt_receiver_thread_ready(); // Thread safe
bool comm_setup_udp_socket(uint32_t *port, int *fd);

#pragma mark -

// Packet I/O

int comm_interface_send_pkt_bytes(interface_t *intf, uint8_t *pkt, uint32_t pktlen);
int comm_node_flood_pkt_bytes(node_t *n, interface_t *ign_intf, uint8_t *pkt, uint32_t pktlen);

