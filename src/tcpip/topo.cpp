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
  node_t *H0 = graph_add_node(topo, "H0");
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H2 = graph_add_node(topo, "H2");
  // Link nodes
  link_nodes(H0, H1, "eth0/1", "eth0/2", 1);
  link_nodes(H1, H2, "eth0/3", "eth0/4", 1);
  // H0
  node_set_loopback_address(H0, "122.1.1.1");
  node_set_interface_ipv4_address(H0, "eth0/1", "10.1.1.1", 24);
  // H1
  node_set_loopback_address(H1, "122.1.1.2");
  node_set_interface_ipv4_address(H1, "eth0/2", "10.1.1.2", 24);
  node_set_interface_ipv4_address(H1, "eth0/3", "20.1.1.2", 24);
  // H2
  node_set_loopback_address(H2, "122.1.1.3");
  node_set_interface_ipv4_address(H2, "eth0/4", "20.1.1.1", 24);
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
