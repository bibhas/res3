// arptests.cpp

#include "catch2.hpp"
#include "arp.h"
#include "net.h"
#include "graph.h"
#include <arpa/inet.h>

TEST_CASE("ARP table initialization", "[arp][init]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  free(table);
}

TEST_CASE("ARP table add single entry", "[arp][add]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Create test entry
  arp_entry_t entry = {0};
  entry.ip_addr = {.bytes = {192, 168, 1, 100}};
  entry.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
  strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  // Add an entry
  bool result = arp_table_add_entry(table, &entry);
  REQUIRE(result == true);
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table add multiple entries", "[arp][add]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add 3 different entries
  for (int i = 0; i < 3; i++) {
    arp_entry_t entry = {0};
    entry.ip_addr = {.bytes = {192, 168, 1, (uint8_t)(100 + i)}};
    entry.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, (uint8_t)(0x00 + i)}};
    strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
    bool result = arp_table_add_entry(table, &entry);
    REQUIRE(result == true);
  }
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table lookup", "[arp][lookup]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add entry
  arp_entry_t entry = {0};
  entry.ip_addr = {.bytes = {10, 0, 0, 1}};
  entry.mac_addr = {.bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};
  strncpy(entry.oif_name, "eth0/1", CONFIG_IF_NAME_SIZE);
  arp_table_add_entry(table, &entry);
  // Perform lookups
  SECTION("Lookup existing entry") {
    // Lookup entry
    ipv4_addr_t lookup_ip = {.bytes = {10, 0, 0, 1}};
    arp_entry_t *found = nullptr;
    bool result = arp_table_lookup(table, &lookup_ip, &found);
    REQUIRE(result == true);
    REQUIRE(found != nullptr);
    REQUIRE(found->ip_addr.value == entry.ip_addr.value);
    REQUIRE(found->mac_addr.value == entry.mac_addr.value);
  }
  SECTION("Lookup non-existing entry") {
    // Try to lookup without adding any entries
    ipv4_addr_t lookup_ip = {.bytes = {192, 168, 1, 50}};
    arp_entry_t *found = nullptr;
    bool result = arp_table_lookup(table, &lookup_ip, &found);
    REQUIRE(result == false);
    REQUIRE(found == nullptr);
  }
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table add duplicate entry", "[arp][add][duplicate]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add first entry
  arp_entry_t entry = {0};
  entry.ip_addr = {.bytes = {172, 16, 0, 10}};
  entry.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
  strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  bool result1 = arp_table_add_entry(table, &entry);
  REQUIRE(result1 == true);
  // Try to add duplicate (same IP and interface)
  bool result2 = arp_table_add_entry(table, &entry);
  REQUIRE(result2 == false);
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table same IP with different interfaces", "[arp][add][interface]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add first entry on eth0/0
  arp_entry_t entry1 = {0};
  entry1.ip_addr = {.bytes = {192, 168, 100, 1}};
  entry1.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}};
  strncpy(entry1.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  bool result1 = arp_table_add_entry(table, &entry1);
  REQUIRE(result1 == true);
  // Add second entry with same IP but different interface
  arp_entry_t entry2 = {0};
  entry2.ip_addr = {.bytes = {192, 168, 100, 1}};  // Same IP
  entry2.mac_addr = {.bytes = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0x02}};
  strncpy(entry2.oif_name, "eth0/1", CONFIG_IF_NAME_SIZE);  // Different interface
  bool result2 = arp_table_add_entry(table, &entry2);
  REQUIRE(result2 == true);
  // Verify by looking up - should find one of them
  ipv4_addr_t lookup_ip = {.bytes = {192, 168, 100, 1}};
  arp_entry_t *found = nullptr;
  bool lookup_result = arp_table_lookup(table, &lookup_ip, &found);
  REQUIRE(lookup_result == true);
  REQUIRE(found != nullptr);
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table delete operations", "[arp][delete]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add entry
  arp_entry_t entry = {0};
  entry.ip_addr = {.bytes = {10, 10, 10, 10}};
  entry.mac_addr = {.bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
  strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  arp_table_add_entry(table, &entry);
  // Delete entries
  SECTION("Delete existing entry") {
    // Delete entry
    bool result = arp_table_delete_entry(table, &entry.ip_addr);
    REQUIRE(result == true);
    // Verify deletion by trying to lookup
    arp_entry_t *found = nullptr;
    bool lookup_result = arp_table_lookup(table, &entry.ip_addr, &found);
    REQUIRE(lookup_result == false);
    REQUIRE(found == nullptr);
  }
  SECTION("Delete non-existing entry") {
    // Try to delete without adding any entries
    ipv4_addr_t ip = {.bytes = {1, 2, 3, 4}};
    bool result = arp_table_delete_entry(table, &ip);
    REQUIRE(result == false);
  }
  // Cleanup
  free(table);
}

TEST_CASE("ARP table clear", "[arp][clear]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add multiple entries
  for (int i = 0; i < 5; i++) {
    arp_entry_t entry = {0};
    entry.ip_addr = {.bytes = {192, 168, 0, (uint8_t)(1 + i)}};
    entry.mac_addr = {.bytes = {0x00, 0x11, 0x22, 0x33, 0x44, (uint8_t)(0x50 + i)}};
    strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
    arp_table_add_entry(table, &entry);
  }
  // Clear table
  bool clear_result = arp_table_clear(table);
  REQUIRE(clear_result == true);
  // Verify table is empty by trying to lookup
  ipv4_addr_t test_ip = {.bytes = {192, 168, 0, 1}};
  arp_entry_t *found = nullptr;
  bool lookup_result = arp_table_lookup(table, &test_ip, &found);
  REQUIRE(lookup_result == false);
  // Cleanup
  free(table);
}

TEST_CASE("ARP table dump", "[arp][dump]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Add a few entries
  for (int i = 0; i < 3; i++) {
    arp_entry_t entry = {0};
    entry.ip_addr = {.bytes = {10, 0, 0, (uint8_t)(1 + i)}};
    entry.mac_addr = {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, (uint8_t)(0x00 + i)}};
    strncpy(entry.oif_name, "eth0/0", CONFIG_IF_NAME_SIZE);
    arp_table_add_entry(table, &entry);
  }
  // This is a visual test - just verify it doesn't crash
  arp_table_dump(table);
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table process reply", "[arp][reply]") {
  arp_table_t *table = nullptr;
  arp_table_init(&table);
  REQUIRE(table != nullptr);
  // Create a mock ARP header
  arp_hdr_t hdr = {0};
  hdr.hw_type = htons(1); // Ethernet
  hdr.proto_type = htons(0x0800); // IPv4
  hdr.hw_addr_len = 6;
  hdr.proto_addr_len = 4;
  hdr.op_code = htons(2); // Reply
  // Source (sender) info
  hdr.src_mac[0] = 0xDE;
  hdr.src_mac[1] = 0xAD;
  hdr.src_mac[2] = 0xBE;
  hdr.src_mac[3] = 0xEF;
  hdr.src_mac[4] = 0x00;
  hdr.src_mac[5] = 0x01;
  // IP in network byte order
  uint8_t ip_bytes[] = {192, 168, 100, 50};
  memcpy(&hdr.src_ip, ip_bytes, 4);
  // Create a mock interface
  interface_t intf = {0};
  strncpy((char *)intf.if_name, "eth0/0", CONFIG_IF_NAME_SIZE);
  // Process the reply
  bool result = arp_table_process_reply(table, &hdr, &intf);
  REQUIRE(result == true);
  // Verify the entry was added
  ipv4_addr_t lookup_ip = {0};
  memcpy(&lookup_ip.value, &hdr.src_ip, 4);
  arp_entry_t *found = nullptr;
  bool lookup_result = arp_table_lookup(table, &lookup_ip, &found);
  REQUIRE(lookup_result == true);
  REQUIRE(found != nullptr);
  // Cleanup
  arp_table_clear(table);
  free(table);
}

TEST_CASE("ARP table null parameter handling", "[arp][error]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Test lookup with null table
  ipv4_addr_t ip = {0};
  arp_entry_t *out = nullptr;
  bool result = arp_table_lookup(nullptr, &ip, &out);
  REQUIRE(result == false);
  // Test add with null table
  arp_entry_t entry = {0};
  result = arp_table_add_entry(nullptr, &entry);
  REQUIRE(result == false);
  // Test delete with null table
  result = arp_table_delete_entry(nullptr, &ip);
  REQUIRE(result == false);
  // Test clear with null table
  result = arp_table_clear(nullptr);
  REQUIRE(result == false);
}
