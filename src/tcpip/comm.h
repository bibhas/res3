// comm.h

#pragma once

#include <atomic>
#include <cstdint>
#include "graph.h"

int comm_pkt_send_bytes(uint8_t *pkt, uint32_t pktlen, interface_t *intf);
int comm_pkt_receive_bytes(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen);
int comm_pkt_send_flood_bytes(node_t *n, interface_t *ign_intf, uint8_t *pkt, uint32_t pktlen);

bool comm_udp_socket_setup(uint32_t *port, int *fd);
void comm_pkt_receiver_thread_main(graph_t *topo);
bool comm_pkt_receiver_thread_ready(); // Thread safe

bool comm_pkt_buffer_shift_right(uint8_t **pktptr, uint32_t pktlen, uint32_t buflen);
