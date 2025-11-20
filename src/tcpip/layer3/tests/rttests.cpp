// rt_trie_tests.cpp (Written by Claude)
//
// Comprehensive tests for routing table implementations.
// These tests are designed to catch bugs in:
// - Binary tries
// - Path-compressed tries (Patricia/Radix)
// - Multi-bit stride tries
// - Hybrid implementations (e.g., DIR-24-8)
//
// The tests are implementation-agnostic and test against the rt_* interface.

#include "catch2.hpp"
#include "layer3/rt.h"
#include "graph.h"
#include "utils.h"

// Helper to create an interface for routes that need one
static interface_t *make_test_interface(const char *name) {
  static interface_t iface;
  memset(&iface, 0, sizeof(iface));
  strncpy(iface.if_name, name, CONFIG_IF_NAME_SIZE - 1);
  return &iface;
}

// Helper to parse IP and add route
static bool add_route(rt_t *t, const char *prefix_str, uint8_t mask, 
                      const char *gw_str = nullptr, const char *oif_name = "eth0") {
  ipv4_addr_t prefix, gw;
  ipv4_addr_try_parse(prefix_str, &prefix);
  
  if (gw_str) {
    ipv4_addr_try_parse(gw_str, &gw);
    return rt_add_route(t, &prefix, mask, &gw, make_test_interface(oif_name), false);
  } else {
    return rt_add_direct_route(t, &prefix, mask);
  }
}

// Helper to lookup and verify result
static bool lookup_expects(rt_t *t, const char *addr_str, 
                           const char *expected_prefix, uint8_t expected_mask) {
  ipv4_addr_t addr, expected;
  ipv4_addr_try_parse(addr_str, &addr);
  ipv4_addr_try_parse(expected_prefix, &expected);
  ipv4_addr_apply_mask(&expected, expected_mask, &expected);
  
  rt_entry_t *entry = nullptr;
  if (!rt_lookup(t, &addr, &entry)) {
    return false;
  }
  
  return IPV4_ADDR_IS_EQUAL(entry->prefix.ip, expected) && 
         entry->prefix.mask == expected_mask;
}

// Helper to verify lookup fails
static bool lookup_fails(rt_t *t, const char *addr_str) {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  ipv4_addr_t addr;
  ipv4_addr_try_parse(addr_str, &addr);
  rt_entry_t *entry = nullptr;
  return !rt_lookup(t, &addr, &entry);
}

#pragma mark - Basic LPM Correctness

TEST_CASE("RT: Basic longest prefix match", "[rt][lpm][basic]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Single route lookup") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    REQUIRE(lookup_expects(t, "10.1.2.3", "10.0.0.0", 8));
    REQUIRE(lookup_expects(t, "10.255.255.255", "10.0.0.0", 8));
  }
  
  SECTION("Longer prefix wins") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.1.2.3", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.3.4", "10.0.0.0", 8));
  }
  
  SECTION("Three levels of nesting") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.1.5", "10.0.0.0", 8));
  }
  
  SECTION("No match returns failure") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    REQUIRE(lookup_fails(t, "11.0.0.1"));
    REQUIRE(lookup_fails(t, "192.168.1.1"));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Path Compression Edge Cases

TEST_CASE("RT: Path compression - split scenarios", "[rt][compression][split]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Insert causes split at beginning of edge") {
    // Insert long prefix first, then shorter one that shares beginning
    add_route(t, "10.1.1.0", 24, "1.1.1.1");
    add_route(t, "10.0.0.0", 8, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.2.3.4", "10.0.0.0", 8));
  }
  
  SECTION("Insert causes split in middle of edge") {
    add_route(t, "10.1.1.0", 24, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.1.0.0", 16));
  }
  
  SECTION("Insert diverges creating sibling branch") {
    add_route(t, "10.0.0.0", 24, "1.1.1.1");
    add_route(t, "10.0.1.0", 24, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.0.5", "10.0.0.0", 24));
    REQUIRE(lookup_expects(t, "10.0.1.5", "10.0.1.0", 24));
  }
  
  SECTION("Multiple splits on same path") {
    add_route(t, "10.1.1.128", 25, "1.1.1.1");
    add_route(t, "10.1.1.0", 24, "2.2.2.2");
    add_route(t, "10.1.0.0", 16, "3.3.3.3");
    add_route(t, "10.0.0.0", 8, "4.4.4.4");
    
    REQUIRE(lookup_expects(t, "10.1.1.200", "10.1.1.128", 25));
    REQUIRE(lookup_expects(t, "10.1.1.50", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.1", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.0.1", "10.0.0.0", 8));
  }
  
  rt_clear(t);
  free(t);
}

TEST_CASE("RT: Path compression - merge scenarios on delete", "[rt][compression][merge]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Delete middle prefix should maintain others") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.1.0.0", &del_addr);
    rt_delete_entry(t, &del_addr, 16);
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.0.0.0", 8));
    REQUIRE(lookup_expects(t, "10.2.0.1", "10.0.0.0", 8));
  }
  
  SECTION("Delete leaf should allow merge") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.1.0.0", &del_addr);
    rt_delete_entry(t, &del_addr, 16);
    
    REQUIRE(lookup_expects(t, "10.1.2.3", "10.0.0.0", 8));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Multi-bit Stride Edge Cases

TEST_CASE("RT: Stride boundary alignment", "[rt][stride][boundary]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Prefixes exactly on stride-8 boundaries") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.0", 24));
  }
  
  SECTION("Prefixes NOT on stride-8 boundaries - prefix expansion test") {
    // /20 ends mid-stride for stride-8 (bits 17-24)
    add_route(t, "10.1.0.0", 20, "1.1.1.1");
    
    // All these should match the /20
    REQUIRE(lookup_expects(t, "10.1.0.1", "10.1.0.0", 20));
    REQUIRE(lookup_expects(t, "10.1.15.255", "10.1.0.0", 20));
    
    // This should NOT match (10.1.16.0 is outside /20)
    REQUIRE(lookup_fails(t, "10.1.16.1"));
  }
  
  SECTION("Multiple non-aligned prefixes with expansion overlap") {
    add_route(t, "10.1.0.0", 20, "1.1.1.1");   // 10.1.0.0 - 10.1.15.255
    add_route(t, "10.1.8.0", 21, "2.2.2.2");   // 10.1.8.0 - 10.1.15.255 (subset)
    
    REQUIRE(lookup_expects(t, "10.1.5.1", "10.1.0.0", 20));
    REQUIRE(lookup_expects(t, "10.1.10.1", "10.1.8.0", 21));
  }
  
  SECTION("/12 prefix - spans stride boundary") {
    add_route(t, "172.16.0.0", 12, "1.1.1.1");
    
    REQUIRE(lookup_expects(t, "172.16.0.1", "172.16.0.0", 12));
    REQUIRE(lookup_expects(t, "172.31.255.254", "172.16.0.0", 12));
    REQUIRE(lookup_fails(t, "172.32.0.1"));
  }
  
  rt_clear(t);
  free(t);
}

TEST_CASE("RT: Prefix expansion overwrite rules", "[rt][stride][expansion]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Longer prefix should not be overwritten by shorter") {
    // Insert /24 first
    add_route(t, "10.1.1.0", 24, "1.1.1.1");
    // Insert /20 which would expand over the /24's slot
    add_route(t, "10.1.0.0", 20, "2.2.2.2");
    
    // /24 should still win for its range
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    // /20 wins elsewhere
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.1.0.0", 20));
  }
  
  SECTION("Insertion order should not matter") {
    // Insert shorter first
    add_route(t, "10.1.0.0", 20, "2.2.2.2");
    // Then longer
    add_route(t, "10.1.1.0", 24, "1.1.1.1");
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.1.0.0", 20));
  }
  
  SECTION("Many overlapping non-aligned prefixes") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.0.0.0", 12, "2.2.2.2");
    add_route(t, "10.0.0.0", 16, "3.3.3.3");
    add_route(t, "10.0.0.0", 20, "4.4.4.4");
    add_route(t, "10.0.0.0", 24, "5.5.5.5");
    add_route(t, "10.0.0.0", 28, "6.6.6.6");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 28));
    REQUIRE(lookup_expects(t, "10.0.0.17", "10.0.0.0", 24));
    REQUIRE(lookup_expects(t, "10.0.1.1", "10.0.0.0", 20));
    REQUIRE(lookup_expects(t, "10.0.16.1", "10.0.0.0", 16));
    REQUIRE(lookup_expects(t, "10.1.0.1", "10.0.0.0", 12));
    REQUIRE(lookup_expects(t, "10.16.0.1", "10.0.0.0", 8));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Bit Pattern Edge Cases

TEST_CASE("RT: Critical bit patterns", "[rt][bits][edge]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("All zeros vs all ones in same position") {
    add_route(t, "10.0.0.0", 24, "1.1.1.1");
    add_route(t, "10.0.255.0", 24, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 24));
    REQUIRE(lookup_expects(t, "10.0.255.1", "10.0.255.0", 24));
  }
  
  SECTION("Adjacent prefixes differ by 1 bit") {
    add_route(t, "10.0.0.0", 25, "1.1.1.1");    // .0 - .127
    add_route(t, "10.0.0.128", 25, "2.2.2.2");  // .128 - .255
    
    REQUIRE(lookup_expects(t, "10.0.0.127", "10.0.0.0", 25));
    REQUIRE(lookup_expects(t, "10.0.0.128", "10.0.0.128", 25));
  }
  
  SECTION("Prefixes differing at various bit positions") {
    // Differ at bit 8
    add_route(t, "10.0.0.0", 16, "1.1.1.1");
    add_route(t, "10.128.0.0", 16, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.1.1", "10.0.0.0", 16));
    REQUIRE(lookup_expects(t, "10.128.1.1", "10.128.0.0", 16));
  }
  
  SECTION("Maximum and minimum addresses") {
    add_route(t, "0.0.0.0", 8, "1.1.1.1");
    add_route(t, "255.0.0.0", 8, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "0.1.2.3", "0.0.0.0", 8));
    REQUIRE(lookup_expects(t, "255.254.253.252", "255.0.0.0", 8));
  }
  
  SECTION("Alternating bit patterns") {
    // 10101010 = 170, 01010101 = 85
    add_route(t, "170.85.170.0", 24, "1.1.1.1");
    add_route(t, "85.170.85.0", 24, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "170.85.170.1", "170.85.170.0", 24));
    REQUIRE(lookup_expects(t, "85.170.85.1", "85.170.85.0", 24));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Host Routes (/32)

TEST_CASE("RT: Host routes /32", "[rt][host][32]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Single host route") {
    add_route(t, "10.0.0.1", 32, "1.1.1.1");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.1", 32));
    REQUIRE(lookup_fails(t, "10.0.0.2"));
  }
  
  SECTION("Host route with covering prefix") {
    add_route(t, "10.0.0.0", 24, "1.1.1.1");
    add_route(t, "10.0.0.1", 32, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.1", 32));
    REQUIRE(lookup_expects(t, "10.0.0.2", "10.0.0.0", 24));
  }
  
  SECTION("Multiple adjacent host routes") {
    add_route(t, "10.0.0.1", 32, "1.1.1.1");
    add_route(t, "10.0.0.2", 32, "2.2.2.2");
    add_route(t, "10.0.0.3", 32, "3.3.3.3");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.1", 32));
    REQUIRE(lookup_expects(t, "10.0.0.2", "10.0.0.2", 32));
    REQUIRE(lookup_expects(t, "10.0.0.3", "10.0.0.3", 32));
    REQUIRE(lookup_fails(t, "10.0.0.4"));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Default Route

TEST_CASE("RT: Default route /0", "[rt][default][0]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Default route matches everything") {
    add_route(t, "0.0.0.0", 0, "1.1.1.1");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "0.0.0.0", 0));
    REQUIRE(lookup_expects(t, "192.168.1.1", "0.0.0.0", 0));
    REQUIRE(lookup_expects(t, "255.255.255.255", "0.0.0.0", 0));
  }
  
  SECTION("More specific routes override default") {
    add_route(t, "0.0.0.0", 0, "1.1.1.1");
    add_route(t, "10.0.0.0", 8, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 8));
    REQUIRE(lookup_expects(t, "11.0.0.1", "0.0.0.0", 0));
  }
  
  SECTION("Full hierarchy from /0 to /32") {
    add_route(t, "0.0.0.0", 0, "1.1.1.1");
    add_route(t, "10.0.0.0", 8, "2.2.2.2");
    add_route(t, "10.1.0.0", 16, "3.3.3.3");
    add_route(t, "10.1.1.0", 24, "4.4.4.4");
    add_route(t, "10.1.1.1", 32, "5.5.5.5");
    
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.1", 32));
    REQUIRE(lookup_expects(t, "10.1.1.2", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.1", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.0.1", "10.0.0.0", 8));
    REQUIRE(lookup_expects(t, "11.0.0.1", "0.0.0.0", 0));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Deletion Edge Cases

TEST_CASE("RT: Deletion correctness", "[rt][delete]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Delete and re-add same prefix") {
    add_route(t, "10.0.0.0", 24, "1.1.1.1");
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.0.0.0", &del_addr);
    REQUIRE(rt_delete_entry(t, &del_addr, 24));
    REQUIRE(lookup_fails(t, "10.0.0.1"));
    
    add_route(t, "10.0.0.0", 24, "2.2.2.2");
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 24));
  }
  
  SECTION("Delete non-existent prefix") {
    add_route(t, "10.0.0.0", 24, "1.1.1.1");
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.0.1.0", &del_addr);
    REQUIRE_FALSE(rt_delete_entry(t, &del_addr, 24));
  }
  
  SECTION("Delete restores shorter prefix visibility") {
    add_route(t, "10.0.0.0", 16, "1.1.1.1");
    add_route(t, "10.0.0.0", 24, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 24));
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.0.0.0", &del_addr);
    REQUIRE(rt_delete_entry(t, &del_addr, 24));
    
    // Now /16 should be visible
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 16));
  }
  
  SECTION("Delete all routes leaves table empty") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "192.168.0.0", 16, "2.2.2.2");
    
    ipv4_addr_t del_addr;
    ipv4_addr_try_parse("10.0.0.0", &del_addr);
    rt_delete_entry(t, &del_addr, 8);
    ipv4_addr_try_parse("192.168.0.0", &del_addr);
    rt_delete_entry(t, &del_addr, 16);
    
    REQUIRE(lookup_fails(t, "10.0.0.1"));
    REQUIRE(lookup_fails(t, "192.168.1.1"));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Scale and Stress Tests

TEST_CASE("RT: Realistic routing table patterns", "[rt][scale][realistic]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Common prefix lengths distribution") {
    // Simulate common BGP prefix length distribution
    // /24 is most common, then /16, /8, etc.
    
    // Several /24s
    add_route(t, "192.168.1.0", 24, "1.1.1.1");
    add_route(t, "192.168.2.0", 24, "1.1.1.2");
    add_route(t, "192.168.3.0", 24, "1.1.1.3");
    add_route(t, "10.0.1.0", 24, "2.2.2.1");
    add_route(t, "10.0.2.0", 24, "2.2.2.2");
    
    // Some /16s
    add_route(t, "172.16.0.0", 16, "3.3.3.1");
    add_route(t, "172.17.0.0", 16, "3.3.3.2");
    
    // A /8
    add_route(t, "11.0.0.0", 8, "4.4.4.1");
    
    // Default
    add_route(t, "0.0.0.0", 0, "5.5.5.5");
    
    REQUIRE(lookup_expects(t, "192.168.1.100", "192.168.1.0", 24));
    REQUIRE(lookup_expects(t, "172.16.50.1", "172.16.0.0", 16));
    REQUIRE(lookup_expects(t, "11.22.33.44", "11.0.0.0", 8));
    REQUIRE(lookup_expects(t, "8.8.8.8", "0.0.0.0", 0));
  }
  
  SECTION("Dense /24 allocation in same /16") {
    // All /24s within 10.1.x.0
    for (int i = 0; i < 256; i++) {
      char prefix[20];
      char gw[20];
      snprintf(prefix, sizeof(prefix), "10.1.%d.0", i);
      snprintf(gw, sizeof(gw), "1.1.1.%d", i % 256);
      add_route(t, prefix, 24, gw);
    }
    
    // Verify a sampling
    REQUIRE(lookup_expects(t, "10.1.0.50", "10.1.0.0", 24));
    REQUIRE(lookup_expects(t, "10.1.128.50", "10.1.128.0", 24));
    REQUIRE(lookup_expects(t, "10.1.255.50", "10.1.255.0", 24));
  }
  
  rt_clear(t);
  free(t);
}

TEST_CASE("RT: Degenerate cases", "[rt][stress][degenerate]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Linear chain of nested prefixes") {
    // This creates a worst-case for binary tries - a long chain
    add_route(t, "128.0.0.0", 1, "1.1.1.1");
    add_route(t, "128.0.0.0", 2, "1.1.1.2");
    add_route(t, "128.0.0.0", 3, "1.1.1.3");
    add_route(t, "128.0.0.0", 4, "1.1.1.4");
    add_route(t, "128.0.0.0", 5, "1.1.1.5");
    add_route(t, "128.0.0.0", 6, "1.1.1.6");
    add_route(t, "128.0.0.0", 7, "1.1.1.7");
    add_route(t, "128.0.0.0", 8, "1.1.1.8");
    
    REQUIRE(lookup_expects(t, "128.1.2.3", "128.0.0.0", 8));
    REQUIRE(lookup_expects(t, "129.0.0.1", "128.0.0.0", 7));
  }
  
  SECTION("All /32s in a /24 - maximum density") {
    for (int i = 0; i < 256; i++) {
      char prefix[20];
      snprintf(prefix, sizeof(prefix), "10.0.0.%d", i);
      add_route(t, prefix, 32, "1.1.1.1");
    }
    
    REQUIRE(lookup_expects(t, "10.0.0.0", "10.0.0.0", 32));
    REQUIRE(lookup_expects(t, "10.0.0.255", "10.0.0.255", 32));
    REQUIRE(lookup_fails(t, "10.0.1.0"));
  }
  
  SECTION("Sparse prefixes across address space") {
    add_route(t, "1.0.0.0", 24, "1.1.1.1");
    add_route(t, "64.0.0.0", 24, "2.2.2.2");
    add_route(t, "128.0.0.0", 24, "3.3.3.3");
    add_route(t, "192.0.0.0", 24, "4.4.4.4");
    add_route(t, "255.0.0.0", 24, "5.5.5.5");
    
    REQUIRE(lookup_expects(t, "1.0.0.1", "1.0.0.0", 24));
    REQUIRE(lookup_expects(t, "64.0.0.1", "64.0.0.0", 24));
    REQUIRE(lookup_expects(t, "128.0.0.1", "128.0.0.0", 24));
    REQUIRE(lookup_expects(t, "192.0.0.1", "192.0.0.0", 24));
    REQUIRE(lookup_expects(t, "255.0.0.1", "255.0.0.0", 24));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - RFC 1918 Private Address Ranges

TEST_CASE("RT: RFC 1918 private ranges", "[rt][rfc1918]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("All three private ranges with default") {
    add_route(t, "0.0.0.0", 0, "1.1.1.1");      // Default
    add_route(t, "10.0.0.0", 8, "2.2.2.2");     // Class A private
    add_route(t, "172.16.0.0", 12, "3.3.3.3");  // Class B private
    add_route(t, "192.168.0.0", 16, "4.4.4.4"); // Class C private
    
    REQUIRE(lookup_expects(t, "10.255.255.255", "10.0.0.0", 8));
    REQUIRE(lookup_expects(t, "172.16.0.1", "172.16.0.0", 12));
    REQUIRE(lookup_expects(t, "172.31.255.254", "172.16.0.0", 12));
    REQUIRE(lookup_expects(t, "172.32.0.1", "0.0.0.0", 0)); // Outside /12
    REQUIRE(lookup_expects(t, "192.168.255.255", "192.168.0.0", 16));
    REQUIRE(lookup_expects(t, "8.8.8.8", "0.0.0.0", 0));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Duplicate Handling

TEST_CASE("RT: Duplicate prefix handling", "[rt][duplicate]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Insert same prefix twice should fail or update") {
    bool first = add_route(t, "10.0.0.0", 24, "1.1.1.1");
    REQUIRE(first == true);
    
    // Second insert of same prefix - behavior depends on implementation
    // Most implementations either reject or update
    bool second = add_route(t, "10.0.0.0", 24, "2.2.2.2");
    // At minimum, lookup should still work
    rt_entry_t *entry = nullptr;
    ipv4_addr_t addr;
    ipv4_addr_try_parse("10.0.0.1", &addr);
    REQUIRE(rt_lookup(t, &addr, &entry));
  }
  
  SECTION("Same IP, different masks are different prefixes") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.0.0.0", 16, "2.2.2.2");
    add_route(t, "10.0.0.0", 24, "3.3.3.3");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 24));
    REQUIRE(lookup_expects(t, "10.0.1.1", "10.0.0.0", 16));
    REQUIRE(lookup_expects(t, "10.1.0.1", "10.0.0.0", 8));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Special Addresses

TEST_CASE("RT: Special and edge addresses", "[rt][special]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Loopback range") {
    add_route(t, "127.0.0.0", 8, "1.1.1.1");
    REQUIRE(lookup_expects(t, "127.0.0.1", "127.0.0.0", 8));
    REQUIRE(lookup_expects(t, "127.255.255.255", "127.0.0.0", 8));
  }
  
  SECTION("Link-local range") {
    add_route(t, "169.254.0.0", 16, "1.1.1.1");
    REQUIRE(lookup_expects(t, "169.254.1.1", "169.254.0.0", 16));
  }
  
  SECTION("Multicast range") {
    add_route(t, "224.0.0.0", 4, "1.1.1.1");
    REQUIRE(lookup_expects(t, "224.0.0.1", "224.0.0.0", 4));
    REQUIRE(lookup_expects(t, "239.255.255.255", "224.0.0.0", 4));
  }
  
  SECTION("Broadcast address") {
    add_route(t, "255.255.255.255", 32, "1.1.1.1");
    REQUIRE(lookup_expects(t, "255.255.255.255", "255.255.255.255", 32));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Complex Overlap Scenarios

TEST_CASE("RT: Complex overlapping prefixes", "[rt][overlap][complex]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  SECTION("Overlapping prefixes with gaps") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    // Skip /16, go directly to /24
    add_route(t, "10.1.1.0", 24, "2.2.2.2");
    
    REQUIRE(lookup_expects(t, "10.1.1.5", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.5", "10.0.0.0", 8));
  }
  
  SECTION("Multiple branches at same level") {
    add_route(t, "10.0.0.0", 16, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.2.0.0", 16, "3.3.3.3");
    add_route(t, "10.3.0.0", 16, "4.4.4.4");
    
    REQUIRE(lookup_expects(t, "10.0.5.5", "10.0.0.0", 16));
    REQUIRE(lookup_expects(t, "10.1.5.5", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.5.5", "10.2.0.0", 16));
    REQUIRE(lookup_expects(t, "10.3.5.5", "10.3.0.0", 16));
  }
  
  SECTION("Deep nesting with siblings") {
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.0.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.0.0", 16, "3.3.3.3");
    add_route(t, "10.0.0.0", 24, "4.4.4.4");
    add_route(t, "10.0.1.0", 24, "5.5.5.5");
    add_route(t, "10.1.0.0", 24, "6.6.6.6");
    add_route(t, "10.1.1.0", 24, "7.7.7.7");
    
    REQUIRE(lookup_expects(t, "10.0.0.1", "10.0.0.0", 24));
    REQUIRE(lookup_expects(t, "10.0.1.1", "10.0.1.0", 24));
    REQUIRE(lookup_expects(t, "10.0.2.1", "10.0.0.0", 16));
    REQUIRE(lookup_expects(t, "10.1.0.1", "10.1.0.0", 24));
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.0", 24));
    REQUIRE(lookup_expects(t, "10.1.2.1", "10.1.0.0", 16));
    REQUIRE(lookup_expects(t, "10.2.0.1", "10.0.0.0", 8));
  }
  
  rt_clear(t);
  free(t);
}

#pragma mark - Insertion Order Variations

TEST_CASE("RT: Insertion order independence", "[rt][order]") {
  rt_t *t = nullptr;
  
  SECTION("Shortest to longest") {
    rt_init(&t);
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    add_route(t, "10.1.1.1", 32, "4.4.4.4");
    
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.1", 32));
    REQUIRE(lookup_expects(t, "10.1.1.2", "10.1.1.0", 24));
    rt_clear(t);
    free(t);
  }
  
  SECTION("Longest to shortest") {
    rt_init(&t);
    add_route(t, "10.1.1.1", 32, "4.4.4.4");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.1", 32));
    REQUIRE(lookup_expects(t, "10.1.1.2", "10.1.1.0", 24));
    rt_clear(t);
    free(t);
  }
  
  SECTION("Random order") {
    rt_init(&t);
    add_route(t, "10.1.0.0", 16, "2.2.2.2");
    add_route(t, "10.1.1.1", 32, "4.4.4.4");
    add_route(t, "10.0.0.0", 8, "1.1.1.1");
    add_route(t, "10.1.1.0", 24, "3.3.3.3");
    
    REQUIRE(lookup_expects(t, "10.1.1.1", "10.1.1.1", 32));
    REQUIRE(lookup_expects(t, "10.1.1.2", "10.1.1.0", 24));
    rt_clear(t);
    free(t);
  }
}

#pragma mark - Exact Match vs LPM

TEST_CASE("RT: Exact match search", "[rt][exact]") {
  rt_t *t = nullptr;
  rt_init(&t);
  
  add_route(t, "10.0.0.0", 8, "1.1.1.1");
  add_route(t, "10.1.0.0", 16, "2.2.2.2");
  add_route(t, "10.1.1.0", 24, "3.3.3.3");
  
  SECTION("Exact match finds correct prefix") {
    ipv4_addr_t addr;
    rt_entry_t *entry = nullptr;
    
    ipv4_addr_try_parse("10.1.0.0", &addr);
    REQUIRE(rt_lookup_exact(t, &addr, 16, &entry));
    REQUIRE(entry != nullptr);
    REQUIRE(entry->prefix.mask == 16);
  }
  
  SECTION("Exact match fails for non-existent prefix length") {
    ipv4_addr_t addr;
    rt_entry_t *entry = nullptr;
    
    ipv4_addr_try_parse("10.1.0.0", &addr);
    REQUIRE_FALSE(rt_lookup_exact(t, &addr, 20, &entry));
  }
  
  SECTION("Exact match fails for wrong address") {
    ipv4_addr_t addr;
    rt_entry_t *entry = nullptr;
    
    ipv4_addr_try_parse("10.2.0.0", &addr);
    REQUIRE_FALSE(rt_lookup_exact(t, &addr, 16, &entry));
  }
  
  rt_clear(t);
  free(t);
}
