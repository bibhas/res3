// phy.h

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

// Forward declarations

typedef struct graph_t graph_t;
typedef struct node_t node_t;
typedef struct interface_t interface_t;

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

// Utils

bool phy_frame_buffer_shift_right(uint8_t **frameptr, uint32_t framelen, uint32_t buflen);

#pragma mark -

// Frame I/O

using phy_send_frame_fn_t = std::function<int(node_t*,interface_t*,uint8_t*,uint32_t)>;

int __phy_node_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen);
