// tcpip.cpp

#include <thread>
#include <chrono>
#include <iostream>
#include <string.h>
#include "glthread.h"
#include "graph.h"
#include "utils.h"
#include "cli.h"
#include "comm.h"

graph_t *build_first_topo() {
  graph_t *topo = graph_init("Hello World Generic Graph");
  node_t *R0_re = graph_add_node(topo, "R0_re");
  node_t *R1_re = graph_add_node(topo, "R1_re");
  node_t *R2_re = graph_add_node(topo, "R2_re");

  link_nodes(R0_re, R1_re, "eth0/0", "eth0/1", 1);
  link_nodes(R1_re, R2_re, "eth0/2", "eth0/3", 1);
  link_nodes(R0_re, R2_re, "eth0/4", "eth0/5", 1);

  node_set_loopback_address(R0_re, "122.1.1.0");
  node_set_interface_ipv4_address(R0_re, "eth0/4", "40.1.1.1", 24);
  node_set_interface_ipv4_address(R0_re, "eth0/0", "20.1.1.1", 24);

  node_set_loopback_address(R1_re, "122.1.1.1");
  node_set_interface_ipv4_address(R1_re, "eth0/1", "20.1.1.2", 24);
  node_set_interface_ipv4_address(R1_re, "eth0/2", "30.1.1.1", 24);

  node_set_loopback_address(R2_re, "122.1.1.2");
  node_set_interface_ipv4_address(R2_re, "eth0/3", "30.1.1.2", 24);
  node_set_interface_ipv4_address(R2_re, "eth0/5", "40.1.1.2", 24);

  return topo;
}

graph_t *build_linear_topo() {
  graph_t *topo = graph_init("Linear Topo");
  node_t *H1 = graph_add_node(topo, "H1");
  node_t *H2 = graph_add_node(topo, "H2");
  node_t *H3 = graph_add_node(topo, "H3");

  link_nodes(H1, H2, "eth0/1", "eth0/2", 1);
  link_nodes(H2, H3, "eth0/3", "eth0/4", 1);

  node_set_loopback_address(H1, "122.1.1.1");
  node_set_loopback_address(H2, "122.1.1.2");
  node_set_loopback_address(H3, "122.1.1.3");

  node_set_interface_ipv4_address(H1, "eth0/1", "10.1.1.1", 24);
  node_set_interface_ipv4_address(H2, "eth0/2", "10.1.1.2", 24);
  node_set_interface_ipv4_address(H2, "eth0/3", "20.1.1.2", 24);
  node_set_interface_ipv4_address(H3, "eth0/4", "20.1.1.1", 24);

  return topo;
}

int main(int argc, const char **argv) {
  graph_t *topo = build_linear_topo();
  // Setup CLI
  cli_init();
  // Create graph
  cli_set_topology(topo);
  printf("Starting...\n");
  // Start receiver thread
  std::jthread receiver_thread([&] {
    comm_pkt_receiver_thread_main(topo);
  });
  printf("Waiting...\n");
  while (!comm_pkt_receiver_thread_ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  // Start cli runloop
  cli_runloop();
  return 0;
}

