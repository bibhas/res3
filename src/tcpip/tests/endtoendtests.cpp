// endtoendtests.cpp

#include "catch2.hpp"
#include "utils.h"
#include "graph.h"
#include "topo.h"
#include "layer2/layer2.h"
#include "layer3/layer3.h"
#include "layer5/layer5.h"

TEST_CASE("End-to-end tests", "[e2e]") {
  // Create topology (dual switch topology)
  graph_t *topo = graph_create_dual_switch_topology();
  REQUIRE(topo != nullptr);
  // Get nodes from the topology
  node_t *H1 = graph_find_node_by_name(topo, "H1");
  node_t *H2 = graph_find_node_by_name(topo, "H2");
  node_t *H3 = graph_find_node_by_name(topo, "H3");
  node_t *H4 = graph_find_node_by_name(topo, "H4");
  node_t *H5 = graph_find_node_by_name(topo, "H5");
  node_t *H6 = graph_find_node_by_name(topo, "H6");
  node_t *SW1 = graph_find_node_by_name(topo, "SW1");
  node_t *SW2 = graph_find_node_by_name(topo, "SW2");
  // Write physical layer stubs to make everything synchronous
  // Helper lambda to create synchronous phy.send
  auto create_sync_phy_send = []() {
    // Creating a lambda for each node like so allow us to allocate memory in the stack
    // for every invocation
    return [](node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) -> int {
      // Get the neighbor node via the link
      link_t *link = intf->link;
      // If there's no link (e.g., SVI), just return success
      // SVIs are virtual and don't actually send physical frames
      if (!link) {
        return framelen;
      }
      interface_t *neighbor_intf = &link->intf1 == intf ? &link->intf2 : &link->intf1;
      node_t *neighbor_node = neighbor_intf->att_node;
      EXPECT_RETURN_VAL(neighbor_node != nullptr, "No neighbor node", -1);
      // Make a copy of the frame to avoid memory issues during recursive processing
      uint8_t frame_copy[2048];
      if (framelen > sizeof(frame_copy)) {
        printf("failing...\n");
        return -1;
      }
      memcpy(frame_copy, frame, framelen);
      // Synchronously deliver the frame
       int resp = layer2_node_recv_frame_bytes(neighbor_node, neighbor_intf, frame_copy, framelen);
       #pragma unused(resp)
       return framelen;
    };
  };
  // Override phy.send for all nodes to make packet delivery synchronous
  NODE_NETSTACK(H1).phy.send = create_sync_phy_send();
  NODE_NETSTACK(H2).phy.send = create_sync_phy_send();
  NODE_NETSTACK(H3).phy.send = create_sync_phy_send();
  NODE_NETSTACK(H4).phy.send = create_sync_phy_send();
  NODE_NETSTACK(H5).phy.send = create_sync_phy_send();
  NODE_NETSTACK(H6).phy.send = create_sync_phy_send();
  NODE_NETSTACK(SW1).phy.send = create_sync_phy_send();
  NODE_NETSTACK(SW2).phy.send = create_sync_phy_send();
  // Add route to H1 (equiv of: config node H1 11.0.0.0 24 10.0.0.8 eth0/1)
  // This allows H1 to reach the 11.0.0.0/24 network via SW1's SVI
  ipv4_addr_t dst_addr_h1 {.bytes = {11, 0, 0, 0}};
  ipv4_addr_t gw_addr_h1 {.bytes = {10, 0, 0, 8}}; // SW1's SVI for VLAN 10
  interface_t *h1_eth0_1 = node_get_interface_by_name(H1, "eth0/1");
  REQUIRE(h1_eth0_1 != nullptr);
  bool route_added_h1 = rt_add_route(H1->netprop.r_table, &dst_addr_h1, 24, &gw_addr_h1, h1_eth0_1);
  REQUIRE(route_added_h1 == true);
  // Run tests
  SECTION("Pinging H1 -> H2 should work (same VLAN, same switch)") {
    // Intercept layer5 promote in netstack to see if it got the ICMP message
    bool l5_callback_invoked = false;
    NODE_NETSTACK(H2).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *src_addr, uint32_t prot) {
      l5_callback_invoked = true;
    };
    // Send ping from machine H1 to 10.0.0.2 (H2)
    ipv4_addr_t ping_target {.bytes = {10, 0, 0, 2}};
    bool ping_sent = layer5_perform_ping(H1, &ping_target);
    REQUIRE(ping_sent == true);
    // Verify the ICMP response was received
    REQUIRE(l5_callback_invoked == true);
  }
  SECTION("Pinging H1 -> H5 should work (same VLAN, different switches across trunk interfaces)") {
    // Intercept layer5 promote in netstack to see if it got the ICMP message
    bool l5_callback_invoked = false;
    NODE_NETSTACK(H5).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *src_addr, uint32_t prot) {
      l5_callback_invoked = true;
    };
    // Send ping from machine H1 to 10.0.0.5 (H5)
    ipv4_addr_t ping_target {.bytes = {10, 0, 0, 5}};
    bool ping_sent = layer5_perform_ping(H1, &ping_target);
    REQUIRE(ping_sent == true);
    // Verify the ICMP response was received
    REQUIRE(l5_callback_invoked == true);
  }
  SECTION("Pinging H1 -> H4 should work (despite needing Inter-VLAN routing)") {
    // Intercept layer5 promote in netstack to see if it got the ICMP message
    bool l5_callback_invoked = false;
    NODE_NETSTACK(H4).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *src_addr, uint32_t prot) {
      l5_callback_invoked = true;
    };
    // Send ping from machine H1 to 11.0.0.4 (H4)
    ipv4_addr_t ping_target {.bytes = {11, 0, 0, 4}};
    bool ping_sent = layer5_perform_ping(H1, &ping_target);
    REQUIRE(ping_sent == true);
    // Verify the ICMP response was received
    REQUIRE(l5_callback_invoked == true);
  }
  SECTION("Pinging H1 -> H3 -> H4 should work (despite needing Inter-VLAN routing + ERO)") {
    // Intercept layer5 promote in netstack to see if it got the ICMP message
    bool l5_callback_invoked = false;
    NODE_NETSTACK(H4).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *src_addr, uint32_t prot) {
      l5_callback_invoked = true;
    };
    // Send ping from machine H1 to 11.0.0.4 (H4)
    ipv4_addr_t ping_target {.bytes = {11, 0, 0, 4}};
    ipv4_addr_t ero_target {.bytes = {11, 0, 0, 3}};
    bool ping_sent = layer5_perform_ping(H1, &ping_target, &ero_target);
    REQUIRE(ping_sent == true);
    // Verify the ICMP response was received
    REQUIRE(l5_callback_invoked == true);
  }
  SECTION("Pinging H1 -> H2 -> H4 should work (despite needing Inter-VLAN routing + ERO)") {
    // Intercept layer5 promote in netstack to see if it got the ICMP message
    bool l5_callback_invoked = false;
    NODE_NETSTACK(H4).l5.promote = [&](node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *src_addr, uint32_t prot) {
      l5_callback_invoked = true;
    };
    // For this one, we need to add static route to H2 (for 11.0.0.0/24)
    ipv4_addr_t dst_addr_h2 {.bytes = {11, 0, 0, 0}};
    ipv4_addr_t gw_addr_h2 {.bytes = {10, 0, 0, 8}}; // SW1's SVI for VLAN 10
    interface_t *h1_eth0_3 = node_get_interface_by_name(H2, "eth0/3");
    REQUIRE(h1_eth0_3 != nullptr);
    bool route_added_h2 = rt_add_route(H2->netprop.r_table, &dst_addr_h2, 24, &gw_addr_h2, h1_eth0_3);
    REQUIRE(route_added_h2 == true);
    // Send ping from machine H1 to 11.0.0.4 (H4)
    ipv4_addr_t ping_target {.bytes = {11, 0, 0, 4}};
    ipv4_addr_t ero_target {.bytes = {10, 0, 0, 2}};
    bool ping_sent = layer5_perform_ping(H1, &ping_target, &ero_target);
    REQUIRE(ping_sent == true);
    // Verify the ICMP response was received
    REQUIRE(l5_callback_invoked == true);
  }
}

