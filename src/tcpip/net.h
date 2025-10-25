// net.h

#pragma once

#include <cstdint>
#include "utils.h"

// Forward declarations

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct graph_t graph_t;

#pragma mark -

// Node Network Properties

struct node_netprop_t {
  // L3 properties 
  struct {
    bool configured;
    ipv4_addr_t addr;
  } loopback;
};

typedef struct node_netprop_t node_netprop_t;

void node_netprop_init(node_netprop_t *prop);
bool node_set_loopback_address(node_t *n, const char *addrstr);
bool node_set_interface_ipv4_address(node_t *n, const char *intf, const char *addrstr, uint8_t mask);
bool node_unset_interface_ipv4_address(node_t *n, const char *intf);
void node_dump_netprop(node_t *n);
bool node_get_interface_matching_subnet(node_t *n, ipv4_addr_t *addr, interface_t **out);

#pragma mark -

// Interface Network Properties

struct interface_netprop_t {
  // L2 properties
  mac_addr_t mac_addr;
  // L3 properties
  struct {
    bool configured;
    ipv4_addr_t addr;
    uint8_t mask;
  } ip;
};

typedef struct interface_netprop_t interface_netprop_t;

void interface_netprop_init(interface_netprop_t *prop);
bool interface_assign_mac_address(interface_t *interface, const char *addrstr);
void interface_dump_netprop(interface_t *i);

#pragma mark -

// Graph properties

void graph_dump_netprop(graph_t *g);
