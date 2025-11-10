// topo.cpp

#include "topo.h"

graph_t* graph_create_three_node_ring_topology() {
  graph_t *topo = graph_init("3-node ring topology");
  // Add nodes
  node_t *H0 = graph_add_node(topo, "H0");
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H2 = graph_add_node(topo, "H2");
  // link nodes
  link_nodes(H0, H1, "eth0/0", "eth0/1", 1);
  link_nodes(H1, H2, "eth0/2", "eth0/3", 1);
  link_nodes(H0, H2, "eth0/4", "eth0/5", 1);
  // H0  
  node_set_loopback_address(H0, "122.1.1.0");
  node_set_interface_ipv4_address(H0, "eth0/4", "40.1.1.1", 24);
  node_set_interface_ipv4_address(H0, "eth0/0", "20.1.1.1", 24);
  // H1
  node_set_loopback_address(H1, "122.1.1.1");
  node_set_interface_ipv4_address(H1, "eth0/1", "20.1.1.2", 24);
  node_set_interface_ipv4_address(H1, "eth0/2", "30.1.1.1", 24);
  // H2
  node_set_loopback_address(H2, "122.1.1.2");
  node_set_interface_ipv4_address(H2, "eth0/3", "30.1.1.2", 24);
  node_set_interface_ipv4_address(H2, "eth0/5", "40.1.1.2", 24);
  return topo;
}

graph_t* graph_create_two_node_linear_topology() {
  graph_t *topo = graph_init("2-node linear topology");
  // Add nodes
  node_t *H0 = graph_add_node(topo, "H0");
  node_t *H1 = graph_add_node(topo, "H1");
  // Link nodes
  link_nodes(H0, H1, "eth0/1", "eth0/2", 1);
  // H0
  node_set_loopback_address(H0, "122.1.1.0");
  node_set_interface_ipv4_address(H0, "eth0/1", "10.1.1.1", 24);
  // H1
  node_set_loopback_address(H1, "122.1.1.1");
  node_set_interface_ipv4_address(H1, "eth0/2", "10.1.1.2", 24);
  return topo;
}

graph_t* graph_create_three_node_linear_topology() {
  graph_t *topo = graph_init("3-node linear topology");
  // Add nodes
  node_t *R1 = graph_add_node(topo, "R1");
  node_t *R2 = graph_add_node(topo, "R2");
  node_t *R3 = graph_add_node(topo, "R3");
  // Link nodes
  link_nodes(R1, R2, "eth0/1", "eth0/2", 1);
  link_nodes(R2, R3, "eth0/3", "eth0/4", 1);
  // R1
  node_set_loopback_address(R1, "122.1.1.1");
  node_set_interface_ipv4_address(R1, "eth0/1", "10.1.1.1", 24);
  // R2
  node_set_loopback_address(R2, "122.1.1.2");
  node_set_interface_ipv4_address(R2, "eth0/2", "10.1.1.2", 24);
  node_set_interface_ipv4_address(R2, "eth0/3", "11.1.1.2", 24);
  // R3
  node_set_loopback_address(R3, "122.1.1.3");
  node_set_interface_ipv4_address(R3, "eth0/4", "11.1.1.1", 24);
  return topo;
}

graph_t* graph_create_four_node_cross_topology() {
  graph_t *topo = graph_init("4-node cross topology");
  // Add nodes
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H2 = graph_add_node(topo, "H2");
  node_t *H3 = graph_add_node(topo, "H3");
  node_t *H4 = graph_add_node(topo, "H4");
  node_t *SW = graph_add_node(topo, "SW"); // L2 switch
  // Link nodes
  link_nodes(H1, SW, "eth0/5", "eth0/4", 1);
  link_nodes(H2, SW, "eth0/8", "eth0/3", 1);
  link_nodes(H3, SW, "eth0/6", "eth0/2", 1);
  link_nodes(H4, SW, "eth0/7", "eth0/1", 1);
  // H1
  node_set_loopback_address(H1, "122.1.1.1");
  node_set_interface_ipv4_address(H1, "eth0/5", "10.1.1.2", 24);
  // H2
  node_set_loopback_address(H2, "122.1.1.2");
  node_set_interface_ipv4_address(H2, "eth0/8", "10.1.1.4", 24);
  // H3
  node_set_loopback_address(H3, "122.1.1.3");
  node_set_interface_ipv4_address(H3, "eth0/6", "10.1.1.1", 24);
  // H4
  node_set_loopback_address(H4, "122.1.1.4");
  node_set_interface_ipv4_address(H4, "eth0/7", "10.1.1.3", 24);
  // No IPv4 address to set in all 4 switch interfaces 
  // because they're running (by definition) in L2 mode.
  return topo;
}

graph_t* graph_create_dual_switch_topology() {
  graph_t *topo = graph_init("Dual-switch topology");
  // Add nodes
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H2 = graph_add_node(topo, "H2");
  node_t *H3 = graph_add_node(topo, "H3");
  node_t *H4 = graph_add_node(topo, "H4");
  node_t *H5 = graph_add_node(topo, "H5");
  node_t *H6 = graph_add_node(topo, "H6");
  node_t *SW1 = graph_add_node(topo, "SW1");
  node_t *SW2 = graph_add_node(topo, "SW2");
  // Link Nodes
  link_nodes(H1, SW1, "eth0/1", "eth0/2", 1);
  link_nodes(H2, SW1, "eth0/3", "eth0/7", 1);
  link_nodes(H3, SW1, "eth0/4", "eth0/6", 1);
  link_nodes(SW1, SW2, "eth0/5", "eth0/7", 1);
  link_nodes(H5, SW2, "eth0/8", "eth0/9", 1);
  link_nodes(H4, SW2, "eth0/13", "eth0/12", 1);
  link_nodes(H6, SW2, "eth0/11", "eth0/10", 1);
  // H1
  node_set_loopback_address(H1, "122.1.1.1");
  node_set_interface_ipv4_address(H1, "eth0/1", "10.1.1.1", 24);
  // H2
  node_set_loopback_address(H2, "122.1.1.2");
  node_set_interface_ipv4_address(H2, "eth0/3", "10.1.1.2", 24);
  // H3
  node_set_loopback_address(H3, "122.1.1.3");
  node_set_interface_ipv4_address(H3, "eth0/4", "10.1.1.3", 24);
  // H4
  node_set_loopback_address(H4, "122.1.1.4");
  node_set_interface_ipv4_address(H4, "eth0/13", "10.1.1.4", 24);
  // H5
  node_set_loopback_address(H5, "122.1.1.5");
  node_set_interface_ipv4_address(H5, "eth0/8", "10.1.1.5", 24); 
  // H6
  node_set_loopback_address(H6, "122.1.1.6");
  node_set_interface_ipv4_address(H6, "eth0/11", "10.1.1.6", 24);
  // Setup VLAN 10 (Access Interfaces)
  node_interface_enable_l2_mode(SW1, "eth0/2", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW1, "eth0/2", 10);
  node_interface_enable_l2_mode(SW1, "eth0/7", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW1, "eth0/7", 10);
  node_interface_enable_l2_mode(SW2, "eth0/9", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW2, "eth0/9", 10);
  node_interface_enable_l2_mode(SW2, "eth0/10", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW2, "eth0/10", 10);
  // Setup VLAN 11 (ACCESSES Interfaces)
  node_interface_enable_l2_mode(SW1, "eth0/6", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW1, "eth0/6", 11);
  node_interface_enable_l2_mode(SW2, "eth0/12", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW2, "eth0/12", 11);
  // Setup TRUNK interfaces 
  node_interface_enable_l2_mode(SW1, "eth0/5", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW1, "eth0/5", 10);
  node_interface_add_l2_vlan_membership(SW1, "eth0/5", 11);
  node_interface_enable_l2_mode(SW2, "eth0/7", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW2, "eth0/7", 10);
  node_interface_add_l2_vlan_membership(SW2, "eth0/7", 11);
  // And, we're done.
  return topo;
}

graph_t *graph_create_quad_switch_loop_topology() {
  graph_t *topo = graph_init("Dual-switch topology");
  // Add nodes
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H6 = graph_add_node(topo, "H6");
  node_t *SW1 = graph_add_node(topo, "SW1");
  node_t *SW2 = graph_add_node(topo, "SW2");
  node_t *SW3 = graph_add_node(topo, "SW3");
  node_t *SW4 = graph_add_node(topo, "SW4");
  // Link Nodes
  link_nodes(H1, SW1, "eth0/1", "eth0/2", 1);
  link_nodes(SW4, SW1, "eth0/3", "eth0/7", 1);
  link_nodes(SW1, SW2, "eth0/5", "eth0/7", 1);
  link_nodes(SW3, SW2, "eth0/8", "eth0/9", 1);
  link_nodes(H6, SW2, "eth0/11", "eth0/10", 1);
  link_nodes(SW3, SW4, "eth0/2", "eth0/5", 1);
  // H1
  node_set_loopback_address(H1, "122.1.1.1");
  node_set_interface_ipv4_address(H1, "eth0/1", "10.1.1.1", 24);
  // H6
  node_set_loopback_address(H6, "122.1.1.6");
  node_set_interface_ipv4_address(H6, "eth0/11", "10.1.1.6", 24);
  // SW1: ACCESS
  node_interface_enable_l2_mode(SW1, "eth0/2", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW1, "eth0/2", 10);
  // SW1: TRUNK
  node_interface_enable_l2_mode(SW1, "eth0/5", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW1, "eth0/5", 10);
  node_interface_enable_l2_mode(SW1, "eth0/7", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW1, "eth0/7", 10);
  // SW2: ACCESS
  node_interface_enable_l2_mode(SW2, "eth0/10", INTF_MODE_L2_ACCESS);
  node_interface_add_l2_vlan_membership(SW2, "eth0/10", 10);
  // SW2: TRUNK
  node_interface_enable_l2_mode(SW2, "eth0/7", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW2, "eth0/7", 10);
  node_interface_enable_l2_mode(SW2, "eth0/9", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW2, "eth0/9", 10);
  // SW3: TRUNK
  node_interface_enable_l2_mode(SW3, "eth0/8", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW3, "eth0/8", 10);
  node_interface_enable_l2_mode(SW3, "eth0/2", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW3, "eth0/2", 10);
  // SW4: TRUNK
  node_interface_enable_l2_mode(SW4, "eth0/5", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW4, "eth0/5", 10);
  node_interface_enable_l2_mode(SW4, "eth0/3", INTF_MODE_L2_TRUNK);
  node_interface_add_l2_vlan_membership(SW4, "eth0/3", 10);
  // And, we're done.
  return topo;
}
