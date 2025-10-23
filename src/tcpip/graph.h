// graph.h

#pragma once

#include <cmath>
#include <cstring>
#include "glthread.h"
#include "net.h"

#define IF_NAME_SIZE 16
#define GRAPH_NAME_SIZE 32
#define NODE_NAME_SIZE 16
#define MAX_INTF_PER_NODE 10

#define COPY_STRING_TO(DST, LITERAL, MAXLEN) \
  do { \
    int name_len = std::max((int)strlen((LITERAL)), MAXLEN); \
    strncpy((DST), (LITERAL), name_len); \
    *((DST) + name_len - 1) = '\0'; \
  } \
  while (false) 

// Forward declarations

typedef struct node_t node_t;
typedef struct link_t link_t;
typedef struct graph_t graph_t;
typedef struct interface_t interface_t;

#pragma mark -

// Struct definitions

struct interface_t {
  char if_name[IF_NAME_SIZE];
  struct node_t *att_node;
  struct link_t *link;
};

struct link_t {
  interface_t intf1;
  interface_t intf2;
  unsigned int cost; /* unused */
};

struct node_t {
  char node_name[NODE_NAME_SIZE];
  interface_t *intf[MAX_INTF_PER_NODE];
  glthread_t graph_glue;
};

struct graph_t {
  char topology_name[GRAPH_NAME_SIZE];
  glthread_t node_list;
};

#pragma mark -

// Inline functions

static inline node_t* interface_get_neighbor_node(interface_t *interface) {
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

// Uses __builtin_offsetof to get node_t* from glthread_t *
DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  node_ptr_from_graph_glue,   // fn name
  node_t,                     // return type
  graph_glue                  // glthread_t* field in node_t
);

static inline int node_get_usable_interface_index(node_t *node) {
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

static inline interface_t* node_get_interface_by_name(node_t *node, const char *if_name) {
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

static inline graph_t* graph_init(const char *topology_name) {
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

static inline node_t *graph_add_node(graph_t *graph, const char *node_name) {
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

static inline node_t* graph_find_node_by_name(graph_t *g, const char *node_name) {
  glthread_t *curr = NULL;
  GLTHREAD_FOREACH_BEGIN(&g->node_list, curr) {
    node_t *curr_node = node_ptr_from_graph_glue(curr);
    if (strcmp(curr_node->node_name, node_name) == 0) {
      return curr_node;
    }
  } GLTHREAD_FOREACH_END();
  return NULL;
}

static void link_nodes(node_t *node1, node_t *node2, const char *from_if_name, const char *to_if_name, unsigned int cost) {
  // Sanity check
  if (!node1 || !node2 || !from_if_name || !to_if_name) { return; }

  /*
   * Setup link
   */

  // Allocate link
  link_t *new_link = (link_t *)malloc(sizeof(link_t));
  // Setup interface 1
  COPY_STRING_TO(new_link->intf1.if_name, from_if_name, IF_NAME_SIZE);
  new_link->intf1.link = new_link;
  new_link->intf1.att_node = node1;
  // Setup interface 2
  COPY_STRING_TO(new_link->intf2.if_name, to_if_name, IF_NAME_SIZE);
  new_link->intf2.link = new_link;
  new_link->intf2.att_node = node2;
  // Fill out the cost
  new_link->cost = cost;

  /*
   * Connect nodes with link
   */

  // Find and populate an empty interface in node1
  int slot_index = node_get_usable_interface_index(node1);
  node1->intf[slot_index] = &new_link->intf1;
  // Find and populate an empty interface in node2
  slot_index = node_get_usable_interface_index(node2);
  node2->intf[slot_index] = &new_link->intf2;
}



