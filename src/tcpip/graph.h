// graph.h

#pragma once

#include <cmath>
#include <cstring>
#include <iostream>
#include "glthread.h"
#include "net.h"
#include "config.h"

// Forward declarations

typedef struct node_t node_t;
typedef struct link_t link_t;
typedef struct graph_t graph_t;
typedef struct interface_t interface_t;

#pragma mark -

// Interface

struct interface_t {
  char if_name[CONFIG_IF_NAME_SIZE];
  struct node_t *att_node;
  struct link_t *link;
  interface_netprop_t netprop;
};

node_t* interface_get_neighbor_node(interface_t *interface);
void interface_dump(interface_t *interface);

#define INTF_MAC(INTFPTR) &((INTFPTR)->netprop.mac_addr)
#define INTF_IP(INTFPTR) &((INTFPTR)->netprop.ip.addr)
#define INTF_IP_SUBNET_MASK(INTFPTR) &((INTFPTR)->netprop.ip.mask)
#define INTF_IS_L3_MODE(INTFPTR) (INTFPTR)->netprop.ip.configured

#pragma mark -

// Link

struct link_t {
  interface_t intf1;
  interface_t intf2;
  unsigned int cost; /* unused */
};

void link_nodes(node_t *n0, node_t *n1, const char *name0, const char *name1, uint32_t cost);
bool link_get_other_interface(link_t *l, interface_t *intf, interface_t **otherptr);

#pragma mark -

// Node

struct node_t {
  char node_name[CONFIG_NODE_NAME_SIZE];
  interface_t *intf[CONFIG_MAX_INTF_PER_NODE];
  glthread_t graph_glue;
  struct {
    uint32_t port;
    int fd;
  } udp;
  node_netprop_t netprop;
};

int node_get_usable_interface_index(node_t *node);
interface_t* node_get_interface_by_name(node_t *node, const char *if_name);
void node_dump(node_t *node);

// Uses __builtin_offsetof to get node_t* from glthread_t *
DEFINE_GLTHREAD_TO_STRUCT_FUNC(
  node_ptr_from_graph_glue,   // fn name
  node_t,                     // return type
  graph_glue                  // glthread_t* field in node_t
);

#define NODE_LO_ADDR(NODEPTR) &((NODEPTR)->netprop.loopback.addr)

#pragma mark -

// Graph

struct graph_t {
  char topology_name[CONFIG_GRAPH_NAME_SIZE];
  glthread_t node_list;
};

graph_t* graph_init(const char *topology_name);
void graph_dump(graph_t *graph);
node_t *graph_add_node(graph_t *graph, const char *node_name);
node_t* graph_find_node_by_name(graph_t *g, const char *node_name);


