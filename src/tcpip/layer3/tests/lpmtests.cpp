// lpmtests.cpp

#include "catch2.hpp"
#include "layer3/layer3.h"
#include "graph.h"

TEST_CASE("LPM test for four entry routing table", "[layer3][rt][lpm]") {
  rt_t *rt = nullptr;
  rt_init(&rt);
  // Add /8 entry
  ipv4_addr_t addr8 {.bytes={10, 0, 0, 0}};
  ipv4_addr_t gw8 {.bytes={80, 0, 0, 1}};
  interface_t ointf8 = {0};
  strncpy((char *)ointf8.if_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  rt_add_route(rt, &addr8, 8, &gw8, &ointf8);
  // Add /16 entry
  ipv4_addr_t addr16 {.bytes={10, 1, 0, 0}};
  ipv4_addr_t gw16 {.bytes={80, 0, 0, 2}};
  interface_t ointf16 = {0};
  strncpy((char *)ointf16.if_name, "eth1/0", CONFIG_IF_NAME_SIZE);
  rt_add_route(rt, &addr16, 16, &gw16, &ointf16);
  // Add /24 entry
  ipv4_addr_t addr24 {.bytes={10, 1, 1, 0}};
  ipv4_addr_t gw24 {.bytes={80, 0, 0, 3}};
  interface_t ointf24 = {0};
  strncpy((char *)ointf24.if_name, "eth2/0", CONFIG_IF_NAME_SIZE);
  rt_add_route(rt, &addr24, 24, &gw24, &ointf24);
  // Add /32
  ipv4_addr_t addr32 {.bytes={10, 1, 1, 1}};
  ipv4_addr_t gw32 {.bytes={80, 0, 0, 4}};
  interface_t ointf32 = {0};
  strncpy((char *)ointf32.if_name, "eth3/0", CONFIG_IF_NAME_SIZE);
  rt_add_route(rt, &addr32, 32, &gw32, &ointf32);
  // Run tests
  SECTION("Query 10.1.1.2 should match 10.1.1.0/24") {
    ipv4_addr_t query {.bytes={10, 1, 1, 2}};
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &query, &result);
    REQUIRE(found == true);
    REQUIRE(result != nullptr);
    REQUIRE(result->prefix.ip.bytes[0] == 10);
    REQUIRE(result->prefix.ip.bytes[1] == 1);
    REQUIRE(result->prefix.ip.bytes[2] == 1);
    REQUIRE(result->prefix.ip.bytes[3] == 0);
    REQUIRE(result->prefix.mask == 24);
    REQUIRE(result->gw_ip.bytes[0] == 80);
    REQUIRE(result->gw_ip.bytes[1] == 0);
    REQUIRE(result->gw_ip.bytes[2] == 0);
    REQUIRE(result->gw_ip.bytes[3] == 3);
    REQUIRE(strncmp(result->oif_name, "eth2/0", CONFIG_IF_NAME_SIZE) == 0);
  }
}
