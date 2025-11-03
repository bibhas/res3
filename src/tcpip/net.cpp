// net.cpp

#include "net.h"
#include "graph.h"
#include "utils.h"

#pragma mark -

// Node

void node_netprop_init(node_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty prop");
  prop->loopback.configured = false;
  prop->loopback.addr.value = 0;
  arp_table_init(&prop->arp_table);
}

bool node_set_loopback_address(node_t *n, const char *addrstr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  ipv4_addr_t addr = {0};
  bool resp = ipv4_addr_try_parse(addrstr, &addr);
  EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_try_parse failed", false);
  n->netprop.loopback.configured = true;
  // We could've passed addr directly to the parsing function, but didn't, for
  // the sake of readability.
  n->netprop.loopback.addr = addr;
  return true;
}

bool node_set_interface_ipv4_address(node_t *n, const char *intf, const char *addrstr, uint8_t mask) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  // Parse address string
  ipv4_addr_t addr = {0};
  bool resp = ipv4_addr_try_parse(addrstr, &addr);
  EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_try_parse failed", false);
  candidate->netprop.ip = {
    .configured = true,
    .addr = addr,
    .mask = mask
  };
  candidate->netprop.mode = INTF_MODE_UNKNOWN;
  return true;
}

bool node_unset_interface_ipv4_address(node_t *n, const char *intf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  // Parse address string
  ipv4_addr_t addr = {0};
  candidate->netprop.ip = {
    .configured = false,
    .addr = addr,
    .mask = 0
  };
  candidate->netprop.mode = INTF_MODE_L2_ACCESS;
  return true;
}

void node_dump_netprop(node_t *n) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  dump_line_indentation_guard_t guard;
  dump_line("Loopback?: %s\n", n->netprop.loopback.configured ? "true" : "false");
  if (!n->netprop.loopback.configured) {
    return;
  }
  dump_line("Loopback address: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(n->netprop.loopback.addr));
}

bool node_get_interface_matching_subnet(node_t *n, ipv4_addr_t *addr, interface_t **out) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty subnet address param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out interface param", false);
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!n->intf[i]) { continue; }
    interface_t *intf = n->intf[i];
    if (!INTF_IS_L3_MODE(intf)) { continue; }
    ipv4_addr_t prefix0;
    bool resp = ipv4_addr_apply_mask(INTF_IP(intf), *INTF_IP_SUBNET_MASK(intf), &prefix0);
    EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_apply_mask failed", false);
    ipv4_addr_t prefix1;
    resp = ipv4_addr_apply_mask(addr, *INTF_IP_SUBNET_MASK(intf), &prefix1);
    EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_apply_mask failed", false);
    if (prefix0.value == prefix1.value) {
      *out = intf;
      return true;
    }
  }
  return false; // Couldn't find
}

#pragma mark -

// Interface

void interface_netprop_init(interface_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty interface property param");
  prop->mac_addr.value = 0;
  prop->ip.configured = false;
  prop->ip.addr.value = 0;
  prop->mode = INTF_MODE_L2_ACCESS; // Default
}

bool interface_assign_mac_address(interface_t *interface, const char *addrstr) {
  EXPECT_RETURN_BOOL(interface != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  int resp = mac_addr_try_parse(addrstr, &interface->netprop.mac_addr);
  EXPECT_RETURN_BOOL(resp == true, "mac_addr_try_parse failed", false);
  return true;
}

void interface_dump_netprop(interface_t *i) {
  dump_line_indentation_guard_t guard;
  EXPECT_RETURN(i != nullptr, "Empty interface param");
  dump_line("MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(i->netprop.mac_addr));
  dump_line("Mode: ");
  switch (i->netprop.mode) {
    case INTF_MODE_L2_ACCESS: printf("ACCESS\n"); break;
    case INTF_MODE_L2_TRUNK:  printf("TRUNK\n"); break;
    case INTF_MODE_UNKNOWN:   printf("UNKNOWN\n"); break;
  }
  dump_line("IP:\n");
  dump_line_indentation_add(1);
  dump_line("Configured?: %s\n", i->netprop.ip.configured ? "true" : "false");
  if (i->netprop.ip.configured) {
    dump_line("Address: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(i->netprop.ip.addr));
    dump_line("Mask: %u\n", i->netprop.ip.mask);
  }
}

#pragma mark -

// Graph

void graph_dump_netprop(graph_t *g) {
  EXPECT_RETURN(g != nullptr, "Empty graph param");
  dump_line_indentation_guard_t guard0;
  glthread_t *curr;
  dump_line("Graph network properties:\n");
  dump_line_indentation_add(1);
  GLTHREAD_FOREACH_BEGIN(&g->node_list, curr) {
    dump_line_indentation_guard_t guard1;
    node_t *n = node_ptr_from_graph_glue(curr);
    dump_line("Node:\n");
    dump_line_indentation_add(1);
    node_dump_netprop(n); 
    for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
      if (!n->intf[i]) { continue; }
      dump_line_indentation_guard_t guard2;
      dump_line("Interface:\n");
      dump_line_indentation_add(1);
      interface_dump_netprop(n->intf[i]);
    }
  }
  GLTHREAD_FOREACH_END();
}
