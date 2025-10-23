// graph.cpp

#include "graph.h"
#include "utils.h"

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
  node_t *nbr_node = interface_get_neighbor_node(interface);
  printf("  Interface: %s\n", interface->if_name);
  printf("    Attached node: %s\n", interface->att_node ? interface->att_node->node_name : "x");
  printf("    Neighbor node: %s\n", nbr_node ? nbr_node->node_name : "x");
  printf("    Cost: %u\n", interface->link ? interface->link->cost : 0);
}

#pragma mark -

// Link

void link_nodes(node_t *n1, node_t *n2, const char *name1, const char *name2, uint32_t cost) {
  // Sanity check
  if (!n1 || !n2 || !name1 || !name2) { return; }
  // Allocate link
  link_t *new_link = (link_t *)malloc(sizeof(link_t));
  // Setup interface 1
  COPY_STRING_TO(new_link->intf1.if_name, name1, IF_NAME_SIZE);
  new_link->intf1.link = new_link;
  new_link->intf1.att_node = n1;
  // Setup interface 2
  COPY_STRING_TO(new_link->intf2.if_name, name2, IF_NAME_SIZE);
  new_link->intf2.link = new_link;
  new_link->intf2.att_node = n2;
  // Fill out the cost
  new_link->cost = cost;
  // Find and populate an empty interface in n1
  int slot_index = node_get_usable_interface_index(n1);
  n1->intf[slot_index] = &new_link->intf1;
  // Find and populate an empty interface in n2
  slot_index = node_get_usable_interface_index(n2);
  n2->intf[slot_index] = &new_link->intf2;
}

#pragma mark -

// Node

int node_get_usable_interface_index(node_t *node) {
  // Sanity check
  if (!node) { return -1; }
  // Iterate over all the interfaces and find the first
  // one with an empty link slot.
  for (int i = 0; i < MAX_INTF_PER_NODE; i++) {
    if (node->intf[i]) { continue; }
    // Empty link slot
    return i;
  }
  return -1;
}

interface_t* node_get_interface_by_name(node_t *node, const char *if_name) {
  if (!node || !if_name) { return NULL; }
  for (int i = 0; i < MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    interface_t *candidate = node->intf[i];
    if (strcmp(candidate->if_name, if_name) == 0) {
      return candidate;
    }
  }
  return NULL; // Not found
}

void node_dump(node_t *node) {
  if (!node) { return; }
  printf("Node name: %s\n", node->node_name);
  for (int i = 0; i < MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    interface_dump(node->intf[i]);
  }
}

#pragma mark -

// Graph

graph_t* graph_init(const char *topology_name) {
  if (!topology_name) { return NULL; } // Should we give it a sane default?
  // Allocate memory
  graph_t *resp = (graph_t *)malloc(sizeof(graph_t));
  // Set name
  COPY_STRING_TO(resp->topology_name, topology_name, GRAPH_NAME_SIZE);
  // Initialize node_list
  init_glthread(&resp->node_list);
  // And, we're done.
  return resp;
}

void graph_dump(graph_t *graph) {
  node_t *node;
  glthread_t *curr;
  printf("Topology name: %s\n", graph->topology_name);
  GLTHREAD_FOREACH_BEGIN(&graph->node_list, curr) {
    node = node_ptr_from_graph_glue(curr);
    node_dump(node);
  } 
  GLTHREAD_FOREACH_END();
}

node_t *graph_add_node(graph_t *graph, const char *node_name) {
  if (!node_name) { return NULL; }
  // Allocate node
  node_t *resp = (node_t *)malloc(sizeof(node_t));
  // Set name
  COPY_STRING_TO(resp->node_name, node_name, NODE_NAME_SIZE);
  // Initialize thread
  init_glthread(&resp->graph_glue);
  glthread_add_next(&graph->node_list, &resp->graph_glue);
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

