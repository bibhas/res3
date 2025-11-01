// tcpip.cpp

#include <thread>
#include <chrono>
#include <iostream>
#include <string.h>
#include "cli.h"
#include "topo.h"
#include "comm.h"

int main(int argc, const char **argv) {
  graph_t *topo = graph_create_three_node_ring_topology();
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

