// net.h

#pragma once

#include <cstdint>
#include "utils.h"
#include "layer2/layer2.h"
#include "layer3/layer3.h"
#include "layer5/layer5.h"
#include "phy.h"

// Forward declarations

typedef struct node_t node_t;
typedef struct interface_t interface_t;
typedef struct graph_t graph_t;
typedef struct arp_table_t arp_table_t;
typedef struct mac_table_t mac_table_t;
typedef struct vlan_t vlan_t;

#pragma mark -

// Network stack

struct node_netstack_t {
  struct {
    layer5_promote_fn_t promote   = &__layer5_promote;
  } l5;
  struct {
    layer3_promote_fn_t promote   = &__layer3_promote;
    layer3_demote_fn_t demote     = &__layer3_demote;
  } l3;
  struct {
    layer2_promote_fn_t promote   = &__layer2_promote;
    layer2_demote_fn_t demote     = &__layer2_demote;
  } l2;
  struct {
    phy_send_frame_fn_t send      = &__phy_node_send_frame_bytes;
  } phy;
};

#pragma mark -

vlan_t* node_vlan_create(node_t *n, uint16_t vlanid, const char *svi_name, const char *svi_addr_str, uint8_t svi_mask);

#pragma mark -

// Interface Network Properties

enum interface_mode_t {
  INTF_MODE_L2_ACCESS = 1,
  INTF_MODE_L2_TRUNK = 2,
  INTF_MODE_L3 = 3,
  INTF_MODE_L3_SVI = 4
};

struct interface_netprop_t {
  interface_mode_t mode = INTF_MODE_L2_ACCESS;
  // L2 properties
  struct {
    mac_addr_t mac_addr;
    uint16_t vlan_memberships[CONFIG_MAX_VLAN_PER_INTF] = {0};
  } l2;
  // L3 properties
  struct {
    bool configured = false;
    ipv4_addr_t addr;
    uint8_t mask;
  } l3;
};

typedef struct interface_netprop_t interface_netprop_t;

#define INTF_NETPROP(INTFPTR) (INTFPTR)->netprop
#define INTF_MODE(INTFPTR) INTF_NETPROP(INTFPTR).mode
#define INTF_MAC(INTFPTR) INTF_NETPROP(INTFPTR).l2.mac_addr)
#define INTF_MAC_PTR(INTFPTR) (&(INTF_NETPROP(INTFPTR).l2.mac_addr))
#define INTF_IP(INTFPTR) INTF_NETPROP(INTFPTR).l3.addr
#define INTF_IP_PTR(INTFPTR) (&(INTF_NETPROP(INTFPTR).l3.addr))
#define INTF_IP_CONFIGURED(INTFPTR) INTF_NETPROP(INTFPTR).l3.configured
#define INTF_IP_SUBNET_MASK(INTFPTR) INTF_NETPROP(INTFPTR).l3.mask

#define INTF_IN_L3_MODE(INTFPTR) \
  (INTF_MODE(INTFPTR) == INTF_MODE_L3 || \
  INTF_MODE(INTFPTR) == INTF_MODE_L3_SVI)
#define INTF_IN_L2_MODE(INTFPTR) \
  (INTF_MODE(INTFPTR) == INTF_MODE_L2_ACCESS || \
  INTF_MODE(INTFPTR) == INTF_MODE_L2_TRUNK || \
  INTF_MODE(INTFPTR) == INTF_MODE_L3_SVI) 

// An interface is by default configured for L2 ACCESS mode
void interface_netprop_init(interface_netprop_t *prop);
bool interface_set_mode(interface_t *i, interface_mode_t mode);
bool interface_assign_mac_address(interface_t *i, const char *addrstr);
bool interface_assign_mac_address(interface_t *i, mac_addr_t *addr);
bool interface_assign_ip_address(interface_t *i, ipv4_addr_t addr, uint8_t mask);
bool interface_add_vlan_membership(interface_t *i, uint16_t vlan_id);
bool interface_clear_vlan_memberships(interface_t *i);
bool interface_test_vlan_membership(interface_t *i, uint16_t vlan_id);
void interface_dump_netprop(interface_t *i);

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
  node_netstack_t netstack;
};

typedef struct node_netprop_t node_netprop_t;

void node_netprop_init(node_netprop_t *prop);

bool node_set_loopback_address(node_t *n, const char *addrstr);
bool node_get_interface_matching_subnet(node_t *n, ipv4_addr_t *addr, interface_t **out);
bool node_is_local_address(node_t *node, ipv4_addr_t *addr);
void node_dump_netprop(node_t *n);

bool node_interface_set_mode(node_t *n, const char *intf_name, interface_mode_t mode);
bool node_interface_set_ipv4_address(node_t *n, const char *intf, const char *addrstr, uint8_t mask);
bool node_interface_unset_ipv4_address(node_t *n, const char *intf);
bool node_interface_add_vlan_membership(node_t *n, const char *intf_name, vlan_t *vlan);

#pragma mark -

// Graph properties

void graph_dump_netprop(graph_t *g);

