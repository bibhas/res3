// net.h

#pragma once

#include <cstdint>
#include "utils.h"
#include "layer2/arp.h"
#include "layer2/mac.h"
#include "layer3/rt.h"

// Forward declarations

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct graph_t graph_t;
typedef struct arp_table_t arp_table_t;
typedef struct mac_table_t mac_table_t;

#pragma mark -

// Node Network Properties

struct node_netprop_t {
  // L2 properties
  arp_table_t *arp_table = nullptr;
  mac_table_t *mac_table = nullptr;
  // L3 properties 
  rt_t *r_table = nullptr;
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
bool node_is_local_address(node_t *node, ipv4_addr_t *addr);



#pragma mark -

// Interface Network Properties

enum interface_mode_t {
  INTF_MODE_UNKNOWN = 0,
  INTF_MODE_L2_ACCESS = 1,
  INTF_MODE_L2_TRUNK = 2
};

struct interface_netprop_t {
  interface_mode_t mode = INTF_MODE_UNKNOWN;
  // L2 properties
  mac_addr_t mac_addr;
  uint16_t vlan_memberships[CONFIG_MAX_VLAN_PER_INTF] = {0};
  // L3 properties
  struct {
    bool configured = false;
    ipv4_addr_t addr;
    uint8_t mask;
  } ip;
};

typedef struct interface_netprop_t interface_netprop_t;

#define INTF_MAC(INTFPTR) (&((INTFPTR)->netprop.mac_addr))
#define INTF_IP(INTFPTR) (&((INTFPTR)->netprop.ip.addr))
#define INTF_IP_SUBNET_MASK(INTFPTR) (&((INTFPTR)->netprop.ip.mask))
#define INTF_IS_L3_MODE(INTFPTR) ((INTFPTR)->netprop.ip.configured)
#define INTF_IS_L2_MODE(INTFPTR) (!INTF_IS_L3_MODE(INTFPTR) && \
  (INTFPTR)->netprop.mode != INTF_MODE_UNKNOWN)
#define INTF_MODE(INTFPTR) ((INTFPTR)->netprop.mode)

void interface_netprop_init(interface_netprop_t *prop);
bool interface_assign_mac_address(interface_t *i, const char *addrstr);
void interface_assign_ip_address(interface_t *i, ipv4_addr_t addr, uint8_t mask);
void interface_dump_netprop(interface_t *i);
void interface_enable_l2_mode(interface_t *i, interface_mode_t mode);
bool interface_add_l2_vlan_membership(interface_t *i, uint16_t vlan_id);
void interface_clear_l2_vlan_memberships(interface_t *i);
bool interface_test_vlan_membership(interface_t *i, uint16_t vlan_id);

#pragma mark -

// Graph properties

void graph_dump_netprop(graph_t *g);

#pragma mark -

// Interface utils

void node_interface_enable_l2_mode(node_t *n, const char *intf_name, interface_mode_t mode);
bool node_interface_add_l2_vlan_membership(node_t *n, const char *intf_name, uint16_t vlan_id);
