// graph.cpp
#include "graph.h"
#include "utils.h"
#include "net.h"
#include "phy.h"

#pragma mark -

// Interface

node_t* interface_get_neighbor_node(interface_t *interface) {
  // Check ptr is valid
  if (interface == NULL) { return NULL; }
  // Make sure we have a link connected to the interface
  if (interface->link == NULL) { return NULL; }
  // We're not sure which one of the interfaces in the link we're
  // attached to. Figure out and return the other node. That's our
  // neighbor.
  if (strcmp(interface->if_name, interface->link->intf1.if_name) == 0) {
    return interface->link->intf2.att_node;
  }
  if (strcmp(interface->if_name, interface->link->intf2.if_name) == 0) {
    return interface->link->intf1.att_node;
  }
  // Neither of the interfaces on the link matches the provided
  // interface. This should signal and error. TODO.
  return NULL;
}

void interface_dump(interface_t *interface) {
  if (!interface) { return; }
  dump_line_indentation_guard_t guard0;
  node_t *nbr_node = interface_get_neighbor_node(interface);
  dump_line("Attached node: %s\n", interface->att_node ? interface->att_node->node_name : "x");
  dump_line("Neighbor node: %s\n", nbr_node ? nbr_node->node_name : "x");
  dump_line("Cost: %u\n", interface->link ? interface->link->cost : 0);
  dump_line("Network Properties:\n");
  dump_line_indentation_add(1);
  interface_dump_netprop(interface);
}

#pragma mark -

static inline bool next_mac_str(char *resp) {
  static int counter = 1;
  EXPECT_RETURN_BOOL(counter < 256, "Too many calls (>255) to next_mac", false);
  snprintf(resp, 18, "aa:bb:cc:dd:ee:%02x", counter++);
  return true;
}

// Link

void link_nodes(node_t *n1, node_t *n2, const char *name1, const char *name2, uint32_t cost) {
  // Sanity check
  if (!n1 || !n2 || !name1 || !name2) { return; }
  // Allocate link
  link_t *new_link = (link_t *)calloc(1, sizeof(link_t));
  // Setup interface 1
  COPY_STRING_TO(new_link->intf1.if_name, name1, CONFIG_IF_NAME_SIZE);
  new_link->intf1.link = new_link;
  new_link->intf1.att_node = n1;
  interface_netprop_init(&new_link->intf1.netprop);
  char addr_buf[18] = {0};
  bool status = next_mac_str(addr_buf);
  EXPECT_RETURN(status == true, "next_mac_str failed");
  status = interface_assign_mac_address(&new_link->intf1, addr_buf);
  EXPECT_RETURN(status == true, "interface_assign_mac_address intf1 failed");
  // Setup interface 2
  COPY_STRING_TO(new_link->intf2.if_name, name2, CONFIG_IF_NAME_SIZE);
  new_link->intf2.link = new_link;
  new_link->intf2.att_node = n2;
  interface_netprop_init(&new_link->intf2.netprop);
  status = next_mac_str(addr_buf);
  EXPECT_RETURN(status == true, "next_mac_str failed");
  status = interface_assign_mac_address(&new_link->intf2, addr_buf);
  EXPECT_RETURN(status == true, "interface_assign_mac_address intf2 failed");
  // Fill out the cost
  new_link->cost = cost;
  // Find and populate an empty interface in n1
  int slot_index = node_get_usable_interface_index(n1);
  n1->intf[slot_index] = &new_link->intf1;
  // Find and populate an empty interface in n2
  slot_index = node_get_usable_interface_index(n2);
  n2->intf[slot_index] = &new_link->intf2;
}

bool link_get_other_interface(link_t *l, interface_t *intf, interface_t **otherptr) {
  EXPECT_RETURN_BOOL(l != nullptr, "Empty link param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty intf param", false);
  EXPECT_RETURN_BOOL(otherptr != nullptr, "Empty out intf ptr param", false);
  if (&l->intf1 == intf) {  // TODO: replace ptr comparision with a better alternative
    *otherptr = &l->intf2; 
    return true; 
  }
  else if (&l->intf2 == intf) { 
    *otherptr = &l->intf1; 
    return true; 
  }
  else { 
    *otherptr = nullptr; 
    return false; 
  }
}

#pragma mark -

// Node

int node_get_usable_interface_index(node_t *node) {
  // Sanity check
  if (!node) { return -1; }
  // Iterate over all the interfaces and find the first
  // one with an empty link slot.
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (node->intf[i]) { continue; }
    // Empty link slot
    return i;
  }
  return -1;
}

interface_t* node_get_interface_by_name(node_t *node, const char *if_name) {
  if (!node || !if_name) { return NULL; }
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    interface_t *candidate = node->intf[i];
    if (strncmp(candidate->if_name, if_name, CONFIG_IF_NAME_SIZE) == 0) {
      return candidate;
    }
  }
  return NULL; // Not found
}

void node_dump(node_t *node) {
  if (!node) { return; }
  dump_line_indentation_guard_t guard0;
  // Node name
  dump_line("Node name: %s\n", node->node_name);
  dump_line_indentation_add(1);
  // UDP port
  dump_line("UDP port: %d\n", node->udp.port);
  // Network properties
  dump_line_indentation_push();
  dump_line("Network Properties:\n");
  dump_line_indentation_add(1);
  node_dump_netprop(node);
  dump_line_indentation_pop();
  // Interfaces
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    dump_line_indentation_guard_t guard1;
    dump_line("Interface: %s\n", node->intf[i]->if_name);
    dump_line_indentation_add(1);
    interface_dump(node->intf[i]);
  }
}

#pragma mark -

// Graph

graph_t* graph_init(const char *topology_name) {
  if (!topology_name) { return NULL; } // Should we give it a sane default?
  // Allocate memory
  graph_t *resp = (graph_t *)calloc(1, sizeof(graph_t));
  // Set name
  COPY_STRING_TO(resp->topology_name, topology_name, CONFIG_GRAPH_NAME_SIZE);
  // Initialize node_list
  glthread_init(&resp->node_list);
  // And, we're done.
  return resp;
}

void graph_dump(graph_t *graph) {
  dump_line("Topology name: %s\n", graph->topology_name);
  dump_line_indentation_guard_t guard0;
  dump_line_indentation_add(1);
  // Iterate over nodes
  node_t *node;
  glthread_t *curr;
  GLTHREAD_FOREACH_BEGIN(&graph->node_list, curr) {
    dump_line_indentation_guard_t guard1;
    node = node_ptr_from_graph_glue(curr);
    node_dump(node);
  } 
  GLTHREAD_FOREACH_END();
}

node_t *graph_add_node(graph_t *graph, const char *node_name) {
  if (!node_name) { return NULL; }
  // Allocate node
  node_t *resp = (node_t *)calloc(1, sizeof(node_t));
  // Set name
  COPY_STRING_TO(resp->node_name, node_name, CONFIG_NODE_NAME_SIZE);
  // Initialize thread
  glthread_init(&resp->graph_glue);
  glthread_add_next(&graph->node_list, &resp->graph_glue);
  // Initialize network properties
  node_netprop_init(&resp->netprop);
  // Start udp socket
  bool status = phy_setup_udp_socket(&resp->udp.port, &resp->udp.fd);
  EXPECT_RETURN_VAL(status == true, "node_setup_udp_socket failed", nullptr);
  return resp;
}

node_t* graph_find_node_by_name(graph_t *g, const char *node_name) {
  glthread_t *curr = NULL;
  GLTHREAD_FOREACH_BEGIN(&g->node_list, curr) {
    node_t *curr_node = node_ptr_from_graph_glue(curr);
    if (strcmp(curr_node->node_name, node_name) == 0) {
      return curr_node;
    }
  } GLTHREAD_FOREACH_END();
  return NULL;
}

