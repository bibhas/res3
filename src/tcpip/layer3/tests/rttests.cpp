// rttests.cpp

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
  SECTION("Query 10.1.1.1 should match 10.1.1.1/32") {
    ipv4_addr_t query {.bytes={10, 1, 1, 1}};
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &query, &result);
    REQUIRE(found == true);
    REQUIRE(result != nullptr);
    REQUIRE(result->prefix.ip.bytes[0] == 10);
    REQUIRE(result->prefix.ip.bytes[1] == 1);
    REQUIRE(result->prefix.ip.bytes[2] == 1);
    REQUIRE(result->prefix.ip.bytes[3] == 1);
    REQUIRE(result->prefix.mask == 32);
    REQUIRE(result->gw_ip.bytes[0] == 80);
    REQUIRE(result->gw_ip.bytes[1] == 0);
    REQUIRE(result->gw_ip.bytes[2] == 0);
    REQUIRE(result->gw_ip.bytes[3] == 4);
    REQUIRE(strncmp(result->oif_name, "eth3/0", CONFIG_IF_NAME_SIZE) == 0);
  }
  SECTION("Query 10.1.2.1 should match 10.1.0.0/16") {
    ipv4_addr_t query {.bytes={10, 1, 2, 1}};
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &query, &result);
    REQUIRE(found == true);
    REQUIRE(result != nullptr);
    REQUIRE(result->prefix.ip.bytes[0] == 10);
    REQUIRE(result->prefix.ip.bytes[1] == 1);
    REQUIRE(result->prefix.ip.bytes[2] == 0);
    REQUIRE(result->prefix.ip.bytes[3] == 0);
    REQUIRE(result->prefix.mask == 16);
    REQUIRE(result->gw_ip.bytes[0] == 80);
    REQUIRE(result->gw_ip.bytes[1] == 0);
    REQUIRE(result->gw_ip.bytes[2] == 0);
    REQUIRE(result->gw_ip.bytes[3] == 2);
    REQUIRE(strncmp(result->oif_name, "eth1/0", CONFIG_IF_NAME_SIZE) == 0);
  }
  SECTION("Query 10.2.0.1 should match 10.0.0.0/8") {
    ipv4_addr_t query {.bytes={10, 2, 0, 1}};
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &query, &result);
    REQUIRE(found == true);
    REQUIRE(result != nullptr);
    REQUIRE(result->prefix.ip.bytes[0] == 10);
    REQUIRE(result->prefix.ip.bytes[1] == 0);
    REQUIRE(result->prefix.ip.bytes[2] == 0);
    REQUIRE(result->prefix.ip.bytes[3] == 0);
    REQUIRE(result->prefix.mask == 8);
    REQUIRE(result->gw_ip.bytes[0] == 80);
    REQUIRE(result->gw_ip.bytes[1] == 0);
    REQUIRE(result->gw_ip.bytes[2] == 0);
    REQUIRE(result->gw_ip.bytes[3] == 1);
    REQUIRE(strncmp(result->oif_name, "eth0/0", CONFIG_IF_NAME_SIZE) == 0);
  }
  SECTION("Query 11.0.0.1 should not match any entry") {
    err_logging_disable_guard_t guard; // We expect errors, so silence err logging
    ipv4_addr_t query {.bytes={11, 0, 0, 1}};
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &query, &result);
    REQUIRE(found == false);
  }
}

TEST_CASE("Duplicate entry in routing table", "[layer3][rt][duplicate]") {
  rt_t *rt = nullptr;
  rt_init(&rt);
  // Add initial route: 10.0.0.0/24 via 192.168.1.1 on eth0/0
  ipv4_addr_t addr {.bytes={10, 0, 0, 0}};
  ipv4_addr_t gw1 {.bytes={192, 168, 1, 1}};
  interface_t ointf1 = {0};
  strncpy((char *)ointf1.if_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  bool success1 = rt_add_route(rt, &addr, 24, &gw1, &ointf1);
  REQUIRE(success1 == true);
  // Run tests
  SECTION("Adding duplicate route should fail and preserve original entry") {
    // Count initial entries
    int count1 = 0;
    glthread_t *ptr = nullptr;
    GLTHREAD_FOREACH_BEGIN(&rt->rt_entries, ptr) {
      count1++;
    }
    GLTHREAD_FOREACH_END();
    // Attempt to add duplicate route with different gateway and interface:
    // 10.0.0.0/24 via 192.168.2.1 on eth1/0. This should fail since the route
    // already exists
    err_logging_disable_guard_t guard; // We expect errors, so silence err logging
    ipv4_addr_t gw2 {.bytes={192, 168, 2, 1}};
    interface_t ointf2 = {0};
    strncpy((char *)ointf2.if_name, "eth1/0", CONFIG_IF_NAME_SIZE);
    bool success2 = rt_add_route(rt, &addr, 24, &gw2, &ointf2);
    // Verify that the duplicate add failed
    REQUIRE(success2 == false);
    // Count entries after duplicate add attempt
    int count2 = 0;
    GLTHREAD_FOREACH_BEGIN(&rt->rt_entries, ptr) {
      count2++;
    } GLTHREAD_FOREACH_END();
    // Verify that no new entry was created
    REQUIRE(count1 == 1);
    REQUIRE(count2 == 1);
    // Verify that the route still has the ORIGINAL gateway and interface
    rt_entry_t *result = nullptr;
    bool found = rt_lookup(rt, &addr, &result);
    REQUIRE(found == true);
    REQUIRE(result != nullptr);
    REQUIRE(result->prefix.ip.bytes[0] == 10);
    REQUIRE(result->prefix.ip.bytes[1] == 0);
    REQUIRE(result->prefix.ip.bytes[2] == 0);
    REQUIRE(result->prefix.ip.bytes[3] == 0);
    REQUIRE(result->prefix.mask == 24);
    // Check that it still has the ORIGINAL gateway (gw1), not the new one (gw2)
    REQUIRE(result->gw_ip.bytes[0] == 192);
    REQUIRE(result->gw_ip.bytes[1] == 168);
    REQUIRE(result->gw_ip.bytes[2] == 1);
    REQUIRE(result->gw_ip.bytes[3] == 1);
    // Check that it still has the ORIGINAL interface (eth0/0), not the new one (eth1/0)
    REQUIRE(strncmp(result->oif_name, "eth0/0", CONFIG_IF_NAME_SIZE) == 0);
  }
}
