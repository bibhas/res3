// layer5.h

#pragma once

#include <functional>
#include "utils.h"

// Forward declarations

typedef struct node_t node_t;
typedef struct interface_t interface_t;

#pragma mark -

// Layer 5 I/0

using layer5_promote_fn_t = std::function<void(node_t*,interface_t*,uint8_t*,uint32_t,ipv4_addr_t*,uint32_t)>;

void __layer5_promote(node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *addr, uint32_t prot);

#pragma mark -

// Layer 5 services

bool layer5_perform_ping(node_t *n, ipv4_addr_t *addr);
