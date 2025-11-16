// nettests.cpp

#include "catch2.hpp"
#include "net.h"
#include "graph.h"
#include "topo.h"

TEST_CASE("node_get_interface_matching_subnet - basic match", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link (which creates interfaces)
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/1", 1);
  // Configure IP address on node's interface (192.168.1.1/24)
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  // Test: Look for an IP in the same subnet (192.168.1.0/24)
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 100}};
  interface_t *found_intf = nullptr;
  result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  REQUIRE(result == true);
  REQUIRE(found_intf != nullptr);
  REQUIRE(strcmp((const char*)found_intf->if_name, "eth0/0") == 0);
}

TEST_CASE("node_get_interface_matching_subnet - no match different subnet", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  // Configure IP address on node's interface (192.168.1.1/24)
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  // Test: Look for an IP in a different subnet (10.0.0.0/8)
  ipv4_addr_t test_ip = {.bytes = {10, 0, 0, 1}};
  interface_t *found_intf = nullptr;
  result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  REQUIRE(result == false);
  REQUIRE(found_intf == nullptr);
}

TEST_CASE("node_get_interface_matching_subnet - multiple interfaces", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add nodes
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  node_t *node3 = graph_add_node(graph, "R3");
  REQUIRE(node3 != nullptr);
  // Create multiple links
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  link_nodes(node, node3, "eth0/1", "eth0/0", 1);
  // Configure IP addresses on both interfaces
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  node_interface_set_mode(node, "eth0/1", INTF_MODE_L3);
  result = node_interface_set_ipv4_address(node, "eth0/1", "10.1.1.1", 24);
  REQUIRE(result == true);
  SECTION("Find interface in first subnet") {
    ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 50}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
    REQUIRE(result == true);
    REQUIRE(found_intf != nullptr);
    REQUIRE(strcmp((const char*)found_intf->if_name, "eth0/0") == 0);
  }
  SECTION("Find interface in second subnet") {
    ipv4_addr_t test_ip = {.bytes = {10, 1, 1, 200}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
    REQUIRE(result == true);
    REQUIRE(found_intf != nullptr);
    REQUIRE(strcmp((const char*)found_intf->if_name, "eth0/1") == 0);
  }
  SECTION("No match in either subnet") {
    ipv4_addr_t test_ip = {.bytes = {172, 16, 0, 1}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
    REQUIRE(result == false);
    REQUIRE(found_intf == nullptr);
  }
}

TEST_CASE("node_get_interface_matching_subnet - different subnet masks", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  SECTION("Test /16 subnet mask") {
    // Configure with /16 mask (172.16.0.1/16)
    node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
    bool result = node_interface_set_ipv4_address(node, "eth0/0", "172.16.0.1", 16);
    REQUIRE(result == true);
    // Should match 172.16.x.x
    ipv4_addr_t test_ip1 = {.bytes = {172, 16, 100, 200}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip1, &found_intf);
    REQUIRE(result == true);
    REQUIRE(found_intf != nullptr);
    // Should not match 172.17.x.x
    ipv4_addr_t test_ip2 = {.bytes = {172, 17, 0, 1}};
    found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip2, &found_intf);
    REQUIRE(result == false);
  }
  SECTION("Test /8 subnet mask") {
    // Configure with /8 mask (10.0.0.1/8)
    node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
    bool result = node_interface_set_ipv4_address(node, "eth0/0", "10.0.0.1", 8);
    REQUIRE(result == true);
    // Should match 10.x.x.x
    ipv4_addr_t test_ip1 = {.bytes = {10, 200, 100, 50}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip1, &found_intf);
    REQUIRE(result == true);
    REQUIRE(found_intf != nullptr);
    // Should not match 11.x.x.x
    ipv4_addr_t test_ip2 = {.bytes = {11, 0, 0, 1}};
    found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip2, &found_intf);
    REQUIRE(result == false);
  }
  SECTION("Test /30 subnet mask") {
    // Configure with /30 mask (192.168.1.1/30)
    // This gives us a subnet with 4 IPs: .0, .1, .2, .3
    node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
    bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 30);
    REQUIRE(result == true);
    // Should match 192.168.1.2 (in range .0-.3)
    ipv4_addr_t test_ip1 = {.bytes = {192, 168, 1, 2}};
    interface_t *found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip1, &found_intf);
    REQUIRE(result == true);
    REQUIRE(found_intf != nullptr);
    // Should not match 192.168.1.4 (outside range)
    ipv4_addr_t test_ip2 = {.bytes = {192, 168, 1, 4}};
    found_intf = nullptr;
    result = node_get_interface_matching_subnet(node, &test_ip2, &found_intf);
    REQUIRE(result == false);
  }
}

TEST_CASE("node_get_interface_matching_subnet - interface without IP configured", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link but don't configure IP
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  // The interface exists but has no IP configured (not in L3 mode)
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 1}};
  interface_t *found_intf = nullptr;
  bool result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  // Should return false since interface is not in L3 mode
  REQUIRE(result == false);
  REQUIRE(found_intf == nullptr);
}

TEST_CASE("node_get_interface_matching_subnet - node with no interfaces", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node with no interfaces
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Try to find a matching interface
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 1}};
  interface_t *found_intf = nullptr;
  bool result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  // Should return false since there are no interfaces
  REQUIRE(result == false);
  REQUIRE(found_intf == nullptr);
}

TEST_CASE("node_get_interface_matching_subnet - null parameter handling", "[net][subnet][error]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Create a valid setup for testing
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 1}};
  interface_t *found_intf = nullptr;
  SECTION("Null node parameter") {
    bool result = node_get_interface_matching_subnet(nullptr, &test_ip, &found_intf);
    REQUIRE(result == false);
  }
  SECTION("Null address parameter") {
    bool result = node_get_interface_matching_subnet(node, nullptr, &found_intf);
    REQUIRE(result == false);
  }
  SECTION("Null output parameter") {
    bool result = node_get_interface_matching_subnet(node, &test_ip, nullptr);
    REQUIRE(result == false);
  }
}

TEST_CASE("node_get_interface_matching_subnet - exact IP match", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  // Configure IP address
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  // Test: Look for the exact same IP as configured on the interface
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 1}};
  interface_t *found_intf = nullptr;
  result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  // Should still match since it's in the same subnet
  REQUIRE(result == true);
  REQUIRE(found_intf != nullptr);
}

TEST_CASE("node_get_interface_matching_subnet - broadcast address", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  // Configure IP address (192.168.1.1/24)
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  // Test: Look for the broadcast address (192.168.1.255)
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 255}};
  interface_t *found_intf = nullptr;
  result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  // Should match since broadcast is in the subnet
  REQUIRE(result == true);
  REQUIRE(found_intf != nullptr);
}

TEST_CASE("node_get_interface_matching_subnet - network address", "[net][subnet]") {
  // Create a topology
  graph_t *graph = graph_init("test_topology");
  REQUIRE(graph != nullptr);
  // Add a node
  node_t *node = graph_add_node(graph, "R1");
  REQUIRE(node != nullptr);
  // Create a link
  node_t *node2 = graph_add_node(graph, "R2");
  REQUIRE(node2 != nullptr);
  link_nodes(node, node2, "eth0/0", "eth0/0", 1);
  // Configure IP address (192.168.1.1/24)
  node_interface_set_mode(node, "eth0/0", INTF_MODE_L3);
  bool result = node_interface_set_ipv4_address(node, "eth0/0", "192.168.1.1", 24);
  REQUIRE(result == true);
  // Test: Look for the network address (192.168.1.0)
  ipv4_addr_t test_ip = {.bytes = {192, 168, 1, 0}};
  interface_t *found_intf = nullptr;
  result = node_get_interface_matching_subnet(node, &test_ip, &found_intf);
  // Should match since it's in the subnet
  REQUIRE(result == true);
  REQUIRE(found_intf != nullptr);
}

TEST_CASE("node_is_local_address", "[net][node]") {
  graph_t *topo = graph_create_three_node_ring_topology();
  node_t *H0 = graph_find_node_by_name(topo, "H0");
  // Run tests
  SECTION("Using neighbor's address should return false") {
    ipv4_addr_t query {.bytes={30, 1, 1, 2}};
    bool resp = node_is_local_address(H0, &query); 
    REQUIRE(resp == false);
  }
  SECTION("Using local interface address should return true") {
    ipv4_addr_t query {.bytes={20, 1, 1, 1}};
    bool resp = node_is_local_address(H0, &query); 
    REQUIRE(resp == true);
  }
  SECTION("Using a wrong address from local interface subnet should return false") {
    ipv4_addr_t query {.bytes={20, 1, 1, 2}};
    bool resp = node_is_local_address(H0, &query); 
    REQUIRE(resp == false);
  }
  SECTION("Using loopback address should return true") {
    ipv4_addr_t query {.bytes={122, 1, 1, 0}};
    bool resp = node_is_local_address(H0, &query); 
    REQUIRE(resp == true);
  }
}
