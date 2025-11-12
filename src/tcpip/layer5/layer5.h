// layer5.h

#pragma once

#include "graph.h"
#include "utils.h"

bool layer5_perform_ping(node_t *n, ipv4_addr_t *addr);

void layer5_promote(node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *addr, uint32_t prot);
