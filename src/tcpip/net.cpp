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
  mac_table_init(&prop->mac_table);
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
  interface_assign_ip_address(candidate, addr, mask);
  return true;
}

bool node_unset_interface_ipv4_address(node_t *n, const char *intf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  ipv4_addr_t addr = {0};
  candidate->netprop.ip = {
    .configured = false,
    .addr = addr,
    .mask = 0
  };
  candidate->netprop.mode = INTF_MODE_UNKNOWN;
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

void node_interface_enable_l2_mode(node_t *n, const char *intf_name, interface_mode_t mode) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  EXPECT_RETURN(intf_name != nullptr, "Empty interface name param");
  interface_t *intf = node_get_interface_by_name(n, intf_name);
  EXPECT_RETURN(intf != nullptr, "node_get_interface_by_name failed");
  interface_enable_l2_mode(intf, mode);
}

bool node_interface_add_l2_vlan_membership(node_t *n, const char *intf_name, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf_name != nullptr, "Empty interface name param", false);
  interface_t *intf = node_get_interface_by_name(n, intf_name);
  EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_by_name failed", false);
  return interface_add_l2_vlan_membership(intf, vlan_id);
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

bool interface_assign_mac_address(interface_t *intf, const char *addrstr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  int resp = mac_addr_try_parse(addrstr, &intf->netprop.mac_addr);
  EXPECT_RETURN_BOOL(resp == true, "mac_addr_try_parse failed", false);
  return true;
}

void interface_assign_ip_address(interface_t *intf, ipv4_addr_t addr, uint8_t mask) {
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  interface_enable_l2_mode(intf, INTF_MODE_UNKNOWN); // Clears L2 related fields
  intf->netprop.ip = {
    .configured = true,
    .addr = addr,
    .mask = mask
  };
}

void interface_dump_netprop(interface_t *intf) {
  dump_line_indentation_guard_t guard0;
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  dump_line("MAC: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(intf->netprop.mac_addr));
  dump_line("Mode: ");
  switch (INTF_MODE(intf)) {
    case INTF_MODE_L2_ACCESS: {
      printf("ACCESS\n");
      dump_line_indentation_guard_t guard1;
      dump_line_indentation_add(1);
      uint16_t vlan = intf->netprop.vlan_memberships[0];
      dump_line("VLAN Membership: ");
      if (vlan == 0) {
        printf("NONE\n");
      }
      else {
        printf("%u\n", vlan);
      }
      break;
    }
    case INTF_MODE_L2_TRUNK: {
      printf("TRUNK\n"); 
      dump_line_indentation_guard_t guard1;
      dump_line_indentation_add(1);
      uint16_t vlan0 = intf->netprop.vlan_memberships[0];
      dump_line("VLAN Memberships: ");
      if (vlan0 == 0) {
        printf("None\n");
      }
      else {
        for (int i = 0; i < CONFIG_MAX_VLAN_PER_INTF; i++) {
          uint16_t vlan = intf->netprop.vlan_memberships[i];
          if (vlan != 0) {
            printf("%u ", vlan);
          }
        }
        printf("\n");
      }
      break;
    }
    case INTF_MODE_UNKNOWN:   
      printf("UNKNOWN\n"); 
      break;
  }
  dump_line("IP:\n");
  dump_line_indentation_add(1);
  dump_line("Configured?: %s\n", intf->netprop.ip.configured ? "true" : "false");
  if (intf->netprop.ip.configured) {
    dump_line("Address: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(intf->netprop.ip.addr));
    dump_line("Mask: %u\n", intf->netprop.ip.mask);
  }
}

void interface_enable_l2_mode(interface_t *intf, interface_mode_t mode) {
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  if (INTF_MODE(intf) == mode) {
    return;
  } 
  intf->netprop.mode = mode;
  // Disable L3 mode if mode != INTF_MODE_UNKNOWN
  if (mode != INTF_MODE_UNKNOWN && INTF_IS_L3_MODE(intf)) {
    intf->netprop.ip.configured = false;
  }
  // Update VLAN memberships
  switch (mode) {
    case INTF_MODE_UNKNOWN: {
      // Remove all memberships
      interface_clear_l2_vlan_memberships(intf);
      break;
    }
    case INTF_MODE_L2_ACCESS: {
      // Remove all but the first membership
      uint16_t saved = intf->netprop.vlan_memberships[0];
      interface_clear_l2_vlan_memberships(intf);
      interface_add_l2_vlan_membership(intf, saved);
      break;
    }
    case INTF_MODE_L2_TRUNK: {
      // Leave as is
      break;
    }
  }
}

bool interface_add_l2_vlan_membership(interface_t *intf, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(vlan_id != 0, "Invalid VLAN ID (0)", false); // 0 = reserved
  if (INTF_IS_L3_MODE(intf) || INTF_MODE(intf) == INTF_MODE_UNKNOWN) {
    return false;
  }
  switch (INTF_MODE(intf)) {
    case INTF_MODE_L2_ACCESS: {
      if (intf->netprop.vlan_memberships[0] != 0) {
        // Max 1 VLAN membership in access mode
        return false;
      }
      intf->netprop.vlan_memberships[0] = vlan_id;
      return true;
    }
    case INTF_MODE_L2_TRUNK: {
      for (int j = 0; j < CONFIG_MAX_VLAN_PER_INTF; j++) {
        if (intf->netprop.vlan_memberships[j] == 0) {
          intf->netprop.vlan_memberships[j] = vlan_id;
          return true;
        }
      }
      return false;
    }
  }
  // Should never reach this point!
  LOG_ERR("Interface in invalid mode!"); 
  return false;
}

void interface_clear_l2_vlan_memberships(interface_t *intf) {
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  memset((void *)intf->netprop.vlan_memberships, 0, sizeof(uint8_t) * CONFIG_MAX_VLAN_PER_INTF);
}

bool interface_test_vlan_membership(interface_t *intf, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  if (INTF_IS_L3_MODE(intf)) {
    return false;
  }
  switch (INTF_MODE(intf)) {
    case INTF_MODE_UNKNOWN: {
      return false;
    }
    case INTF_MODE_L2_ACCESS: {
      return intf->netprop.vlan_memberships[0] == vlan_id;
    }
    case INTF_MODE_L2_TRUNK: {
      for (int i = 0; i < CONFIG_MAX_VLAN_PER_INTF; i++) {
        if (intf->netprop.vlan_memberships[i] == vlan_id) {
          return true;
        }
      }
    }
  }
  return false;
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
