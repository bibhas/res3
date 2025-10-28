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

int main(int argc, const char **argv) {
  graph_t *topo = build_first_topo();
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
  // Do some packet processing
  node_t *sender = graph_find_node_by_name(topo, "R0_re");
  interface_t *sender_intf = node_get_interface_by_name(sender, "eth0/0");
  const char *msg = "Hello, how are you bibhas?\0";
  comm_pkt_send_bytes((uint8_t *)msg, strlen(msg), sender_intf);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // Start cli runloop
  cli_runloop();
  return 0;
}

