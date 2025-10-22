// graph.h

#pragma once

#include <cmath>
#include "glthread.h"

#define IF_NAME_SIZE 16
#define NODE_NAME_SIZE 16
#define MAX_INTF_PER_NODE 10

struct node_;
struct link_;

typedef struct interface_ {
  char if_name[IF_NAME_SIZE];
  struct node_ *att_node;
  struct link_ *link;
} interface_t;

typedef struct link_ {
  interface_t intf1;
  interface_t intf2;
  unsigned int cost; /* unused */
} link_t;

typedef struct node_ {
  char node_name[NODE_NAME_SIZE];
  interface_t *intf[MAX_INTF_PER_NODE];
  glthread_t graph_glue;
} node_t;

typedef struct graph_ {
  char topology_name[32];
  glthread_t node_list;
} graph_t;

static inline graph_t* create_new_graph(char *topology_name) {
  if (!topology_name) { return NULL; } // Should we give it a sane default?
  // Allocate memory
  graph_t *resp = (graph_t *)malloc(sizeof(graph_t));
  // Set name
  int name_len = std::max((int)strlen(topology_name), (int)NODE_NAME_SIZE - 1);
  strncpy(resp->topology_name, topology_name, name_len);
  *(resp->topology_name + name_len - 1) = '\0';
  // Initialize node_list
  init_glthread(&resp->node_list);
  // And, we're done.
  return resp;
}

static inline node_t *create_graph_node(graph_t *graph, char *node_name) {
  return NULL;
}

static void insert_link_between_two_nodes(node_t *node1, node_t *node2, char *from_if_name, char *to_if_name, unsigned int cost) {

}

static inline node_t* get_nbr_node(interface_t *interface) {
  // Check ptr is valid
  if (interface == NULL) { return NULL; }
  // Make sure we have a link connected to the interface
  if (interface->link == NULL) { return NULL; }
  // We're not sure which one of the interfaces in the link we're
  // attached to. Figure out and return the other node. That's our
  // neighbor.
  if (strcmp(interface->if_name, interface->link->intf1.if_name)) {
    return interface->link->intf2.att_node;
  }
  else if (strcmp(interface->if_name, interface->link->intf2.if_name)) {
    return interface->link->intf1.att_node;
  }
  // Neither of the interfaces on the link matches the provided
  // interface. This should signal and error. TODO.
  return NULL;
}

static inline int get_node_intf_available_slot(node_t *node) {
  // Sanity check
  if (!node) { return -1; }
  // Iterate over all the interfaces and find the first
  // one with an empty link slot.
  for (int i = 0; i < MAX_INTF_PER_NODE; i++) {
    if (!node->intf[i]) { continue; }
    if (!node->intf[i]->link) {
      // Empty link slot
      return i;
    }
  }
}

