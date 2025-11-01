// phy.h

#pragma once

#include <atomic>
#include <cstdint>
#include "graph.h"

#pragma mark -

// General

/*
 * Upon receiving a frame, the thread will pass on the parsed frame to
 * `layer2_frame_recv()` function. See `layer2/layer2.cpp`
 */
void phy_receiver_thread_main(graph_t *topo);
bool phy_receiver_thread_ready(); // Thread safe
bool phy_setup_udp_socket(uint32_t *port, int *fd);

#pragma mark -

// Frame I/O

int phy_node_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);
int phy_node_flood_frame_bytes(node_t *n, interface_t *ignored, uint8_t *frame, uint32_t framelen);

