// layer5tests.cpp

#include "catch2.hpp"
#include "layer3/layer3.h"
#include "graph.h"
#include "topo.h"

TEST_CASE("Ping", "[layer5][ping]") {
  graph_t *topo = graph_create_three_node_linear_topology();
  node_t *R1 = graph_find_node_by_name(topo, "R1");
  // Tests
  SECTION("Self-ping to node loopback address should work") {
    bool invoked = false;
    NODE_NETSTACK(R1).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *addr, uint32_t prot) {
      invoked = true;
    };
    layer5_perform_ping(R1, &R1->netprop.loopback.addr);
    REQUIRE(invoked == true);
  }
  SECTION("Self-ping to a local interface should work") {
    bool invoked = false;
    NODE_NETSTACK(R1).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *addr, uint32_t prot) {
      invoked = true;
    };
    layer5_perform_ping(R1, &INTF_NETPROP(R1->intf[0]).l3.addr);
    REQUIRE(invoked == true);
  }
  SECTION("Ping to local subnet should work") {
    // TODO
  }
  SECTION("Ping to remote subnet should work") {
    // TODO
  }
  SECTION("Ping to remote subnet should work") {
    // TODO
  }
}
