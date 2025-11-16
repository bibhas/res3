// net.cpp

#include "net.h"
#include "graph.h"
#include "utils.h"
#include "layer2/arp.h"
#include "layer2/mac.h"

#pragma mark -

// VLAN

vlan_t* node_vlan_create(node_t *n, uint16_t vlanid, const char *svi_name, const char *svi_addr_str, uint8_t svi_mask) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node param", nullptr);
  EXPECT_RETURN_VAL(svi_addr_str != nullptr, "Empty svi name param", nullptr);
  EXPECT_RETURN_VAL(vlanid != 0, "Invalid VLAN ID param", nullptr);
  // Make sure that this VLAN doesn't already exist. Admittedly a rather clumsy
  // way to go about it, but we do this by checking if an SVI for the given
  // VLAN exists in the node's list of interfaces. We could have explicitly 
  // maintained a list of in-use VLAN IDs but alas we don't.
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!n->intf[i]) { continue; }
    interface_t *candidate = n->intf[i];
    if (INTF_MODE(candidate) != INTF_MODE_L3_SVI) { continue; }
    bool svi_exists = (INTF_NETPROP(candidate).l2.vlan_memberships[0] == vlanid);
    EXPECT_RETURN_VAL(svi_exists == false, "Existing VLAN ID!", nullptr);
  }
  // Parse SVI IP address
  ipv4_addr_t svi_addr = {0};
  bool resp = ipv4_addr_try_parse(svi_addr_str, &svi_addr);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", nullptr);
  // Allocate VLAN
  vlan_t *vlan = (vlan_t *)calloc(1, sizeof(vlan_t));
  vlan->id = vlanid;
  // Setup SVI
  interface_t *svi = &vlan->svi;
  svi->att_node = n;
  svi->link = nullptr;
  COPY_STRING_TO(svi->if_name, svi_name, CONFIG_IF_NAME_SIZE);
  // Setup SVI's network properties
  interface_netprop_init(&INTF_NETPROP(svi));
  INTF_NETPROP(svi).mode = INTF_MODE_L3_SVI;
  // Set L2 properties
  uint8_t vlanid_pre = vlanid & 0x00FF; // flipped
  uint8_t vlanid_pos = vlanid & 0xFF00;
  static uint8_t counter = 0;
  counter++;
  INTF_NETPROP(svi).l2.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, vlanid_pre, vlanid_pos, counter}};
  resp = interface_add_vlan_membership(svi, vlanid);
  EXPECT_RETURN_VAL(resp == true, "interface_add_vlan_membership failed", nullptr); // TODO: Leaks `svi`
  // Set L3 properties
  resp = interface_assign_ip_address(svi, svi_addr, svi_mask);
  EXPECT_RETURN_VAL(resp == true, "interface_add_vlan_membership failed", nullptr); // TODO: Leaks `svi`
  // Attach SVI to node
  int slot_index = node_get_usable_interface_index(n);
  EXPECT_RETURN_VAL(slot_index >= 0, "node_get_usable_interface_index failed", nullptr); // TODO: Leaks `svi`
  n->intf[slot_index] = svi;
  // Add SVI to routing table
  ipv4_addr_t svi_prefix;
  resp = ipv4_addr_apply_mask(&svi_addr, svi_mask, &svi_prefix);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_apply_mask failed", nullptr); // TODO: Leaks `svi`
  resp = rt_add_route(n->netprop.r_table, &svi_prefix, svi_mask, &svi_addr, svi, true);
  EXPECT_RETURN_VAL(resp == true, "rt_add_direct_route failed", nullptr); // TODO: Leaks `svi`
  // And, we're done
  return vlan;
}


#pragma mark -

// Interface Network Properties

void interface_netprop_init(interface_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty interface property param");
  prop->l2.mac_addr.value = 0;
  prop->l3.configured = false;
  prop->l3.addr.value = 0;
  prop->mode = INTF_MODE_L2_ACCESS; // Default
}

bool interface_set_mode(interface_t *intf, interface_mode_t mode) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  if (INTF_MODE(intf) == mode) { return true; } 
  INTF_MODE(intf) = mode;
  // Update VLAN memberships
  switch (mode) {
    case INTF_MODE_L3: {
      // Remove all memberships (we're calling memset manually because
      // interface_clear_vlan_memberships cannot be called for a non-L2
      // interface.
      memset((void *)INTF_NETPROP(intf).l2.vlan_memberships, 0, sizeof(uint8_t) * CONFIG_MAX_VLAN_PER_INTF);
      return true;
    }
    case INTF_MODE_L3_SVI:
    case INTF_MODE_L2_ACCESS: {
      // Remove all but the first membership
      uint16_t saved = INTF_NETPROP(intf).l2.vlan_memberships[0];
      bool resp = interface_clear_vlan_memberships(intf);
      EXPECT_RETURN_BOOL(resp == true, "interface_clear_vlan_memberships failed", false);
      resp = interface_add_vlan_membership(intf, saved);
      EXPECT_RETURN_BOOL(resp == true, "interface_add_vlan_membership failed", false);
      return true;
    }
    default: {
      // Leave as is
      return true;
    }
  }
}

bool interface_assign_mac_address(interface_t *intf, const char *addrstr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  int resp = mac_addr_try_parse(addrstr, INTF_MAC_PTR(intf)); // <-- Assigned here, fyi
  EXPECT_RETURN_BOOL(resp == true, "mac_addr_try_parse failed", false);
  return true;
}

bool interface_assign_mac_address(interface_t *intf, mac_addr_t *addr) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty address string param", false);
  INTF_NETPROP(intf).l2.mac_addr.value = addr->value;
  return true;
}

bool interface_assign_ip_address(interface_t *intf, ipv4_addr_t addr, uint8_t mask) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(INTF_IN_L3_MODE(intf), "Interface not in L3 mode", false);
  INTF_NETPROP(intf).l3 = {
    .configured = true,
    .addr = addr,
    .mask = mask
  };
  return true;
}

bool interface_add_vlan_membership(interface_t *intf, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  if (!INTF_IN_L2_MODE(intf)) {   // Note: SVIs are in L2 mode too
    return false;
  }
  switch (INTF_MODE(intf)) {
    case INTF_MODE_L3_SVI:
    case INTF_MODE_L2_ACCESS: {
      if (INTF_NETPROP(intf).l2.vlan_memberships[0] != 0) {
        // Max 1 VLAN membership in L2_ACCESS/L3_SVI mode
        return false;
      }
      INTF_NETPROP(intf).l2.vlan_memberships[0] = vlan_id;
      return true;
    }
    case INTF_MODE_L2_TRUNK: {
      for (int j = 0; j < CONFIG_MAX_VLAN_PER_INTF; j++) {
        if (INTF_NETPROP(intf).l2.vlan_memberships[j] == 0) {
          INTF_NETPROP(intf).l2.vlan_memberships[j] = vlan_id;
          return true;
        }
      }
      return false;
    }
  }
  // Should never reach this point!
  LOG_ERR("Interface found in ?? (%u) mode!", (uint32_t)INTF_MODE(intf)); 
  return false;
}

bool interface_clear_vlan_memberships(interface_t *intf) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(INTF_IN_L2_MODE(intf), "Interface not in L2 mode", false);
  memset((void *)INTF_NETPROP(intf).l2.vlan_memberships, 0, sizeof(uint8_t) * CONFIG_MAX_VLAN_PER_INTF);
  return true;
}

bool interface_test_vlan_membership(interface_t *intf, uint16_t vlan_id) {
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(INTF_IN_L2_MODE(intf), "Interface not in L2 mode", false);
  switch (INTF_MODE(intf)) {
    case INTF_MODE_L3_SVI: 
    case INTF_MODE_L2_ACCESS: {
      return INTF_NETPROP(intf).l2.vlan_memberships[0] == vlan_id;
    }
    case INTF_MODE_L2_TRUNK: {
      for (int i = 0; i < CONFIG_MAX_VLAN_PER_INTF; i++) {
        if (INTF_NETPROP(intf).l2.vlan_memberships[i] == vlan_id) {
          return true;
        }
      }
    }
  }
  return false;
}

void interface_dump_netprop(interface_t *intf) {
  dump_line_indentation_guard_t guard0;
  EXPECT_RETURN(intf != nullptr, "Empty interface param");
  dump_line_indentation_add(1);
  switch (INTF_MODE(intf)) {
    case INTF_MODE_L2_ACCESS: {
      printf("L2_ACCESS ");
      uint16_t vlan = INTF_NETPROP(intf).l2.vlan_memberships[0];
      printf("VLAN-");
      if (vlan == 0) {
        printf("x ");
      }
      else {
        printf("%u ", vlan);
      }
      break;
    }
    case INTF_MODE_L2_TRUNK: {
      printf("L2_TRUNK "); 
      uint16_t vlan0 = INTF_NETPROP(intf).l2.vlan_memberships[0];
      printf("VLAN-");
      if (vlan0 == 0) {
        printf("x ");
      }
      else {
        for (int i = 0; i < CONFIG_MAX_VLAN_PER_INTF; i++) {
          uint16_t vlan = INTF_NETPROP(intf).l2.vlan_memberships[i];
          if (i != 0 && vlan != 0) {
            printf(",");
          }
          if (vlan != 0) {
            printf("%u", vlan);
          }
        }
        printf(" ");
      }
      break;
    }
    case INTF_MODE_L3: {
      printf("L3 ");
      dump_line_indentation_guard_t guard1;
      dump_line_indentation_add(1);
      if (INTF_NETPROP(intf).l3.configured) {
        printf(IPV4_ADDR_FMT "/%u ", IPV4_ADDR_BYTES_BE(INTF_IP(intf)), INTF_IP_SUBNET_MASK(intf));
      }
      else {
        printf("None");
      }
      break;
    }
    case INTF_MODE_L3_SVI: {
      printf("L3_SVI ");
      uint16_t vlan = INTF_NETPROP(intf).l2.vlan_memberships[0];
      printf("VLAN-");
      if (vlan == 0) {
        printf("-x ");
      }
      else {
        printf("%u ", vlan);
      }
      if (INTF_NETPROP(intf).l3.configured) {
        printf("" IPV4_ADDR_FMT "/%u", IPV4_ADDR_BYTES_BE(INTF_IP(intf)), INTF_IP_SUBNET_MASK(intf));
      }
      else {
        printf(" None");
      }
      printf(" ");
      break;
    }
  }
  printf(MAC_ADDR_FMT " ", MAC_ADDR_BYTES_BE(INTF_NETPROP(intf).l2.mac_addr));
}

#pragma mark -

// Node

void node_netprop_init(node_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty prop");
  prop->loopback.configured = false;
  prop->loopback.addr.value = 0;
  arp_table_init(&prop->arp_table);
  mac_table_init(&prop->mac_table);
  rt_init(&prop->r_table);
  prop->netstack = node_netstack_t();
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
  // Update rt
  resp = rt_add_direct_route(n->netprop.r_table, &addr, 32);
  EXPECT_RETURN_BOOL(resp == true, "rt_add_direct_route failed", false);
  return true;
}

bool node_get_interface_matching_subnet(node_t *n, ipv4_addr_t *addr, interface_t **out) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty subnet address param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out interface param", false);
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!n->intf[i]) { continue; }
    interface_t *intf = n->intf[i];
    if (!INTF_IN_L3_MODE(intf)) { continue; }
    ipv4_addr_t prefix0;
    bool resp = ipv4_addr_apply_mask(INTF_IP_PTR(intf), INTF_IP_SUBNET_MASK(intf), &prefix0);
    EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_apply_mask failed", false);
    ipv4_addr_t prefix1;
    resp = ipv4_addr_apply_mask(addr, INTF_IP_SUBNET_MASK(intf), &prefix1);
    EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_apply_mask failed", false);
    if (prefix0.value == prefix1.value) {
      *out = intf;
      return true;
    }
  }
  return false; // Couldn't find
}

bool node_is_local_address(node_t *node, ipv4_addr_t *addr) {
  glthread_t *curr = nullptr;
  // See if any of the local interfaces have this address
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    interface_t *candidate = node->intf[i];
    if (!INTF_IN_L3_MODE(candidate)) { continue; }
    if (IPV4_ADDR_PTR_IS_EQUAL(INTF_IP_PTR(candidate), addr)) {
      return true;
    }
  }
  // If not, check node loopback address
  if (node->netprop.loopback.configured) {
    ipv4_addr_t *lb_addr = &node->netprop.loopback.addr;
    if (IPV4_ADDR_PTR_IS_EQUAL(lb_addr, addr)) {
      return true;
    }
  } 
  return false;
}

void node_dump_netprop(node_t *n) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  dump_line_indentation_guard_t guard;
  if (n->netprop.loopback.configured) {
    printf("" IPV4_ADDR_FMT "/32\n", IPV4_ADDR_BYTES_BE(n->netprop.loopback.addr));
  }
  else {
    printf("\n");
  }
}

bool node_interface_set_ipv4_address(node_t *n, const char *intf, const char *addrstr, uint8_t mask) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  EXPECT_RETURN_BOOL(INTF_IN_L3_MODE(candidate), "Interface not in L3 mode", false);
  // Parse address string
  ipv4_addr_t addr = {0};
  bool resp = ipv4_addr_try_parse(addrstr, &addr);
  EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_try_parse failed", false);
  interface_assign_ip_address(candidate, addr, mask);
  // Update rt
  resp = rt_add_direct_route(n->netprop.r_table, &addr, mask);
  return true;
}

bool node_interface_unset_ipv4_address(node_t *n, const char *intf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  ipv4_addr_t addr = {0};
  INTF_NETPROP(candidate).l3 = {
    .configured = false,
    .addr = addr,
    .mask = 0
  };
  return true;
}

bool node_interface_set_mode(node_t *n, const char *intf_name, interface_mode_t mode) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf_name != nullptr, "Empty interface name param", false);
  interface_t *intf = node_get_interface_by_name(n, intf_name);
  EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_by_name failed", false);
  bool resp = interface_set_mode(intf, mode);
  EXPECT_RETURN_BOOL(resp == true, "interface_set_mode failed", false);
  return true;
}

bool node_interface_add_vlan_membership(node_t *n, const char *intf_name, vlan_t *vlan) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf_name != nullptr, "Empty interface name param", false);
  EXPECT_RETURN_BOOL(vlan != nullptr, "Empty VLAN param", false);
  interface_t *intf = node_get_interface_by_name(n, intf_name);
  EXPECT_RETURN_BOOL(intf != nullptr, "node_get_interface_by_name failed", false);
  return interface_add_vlan_membership(intf, vlan->id);
}

#pragma mark -

// Graph properties

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

