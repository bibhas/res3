// utiltests.cpp

#include "catch2.hpp"
#include "utils.h"

#pragma mark - IPv4 Address Parsing Tests

TEST_CASE("IPv4 address parsing - valid addresses", "[ipv4][parse]") {
  ipv4_addr_t addr = {0};
  SECTION("Parse standard address 192.168.1.1") {
    bool result = ipv4_addr_try_parse("192.168.1.1", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.bytes[0] == 192);
    REQUIRE(addr.bytes[1] == 168);
    REQUIRE(addr.bytes[2] == 1);
    REQUIRE(addr.bytes[3] == 1);
  }
  SECTION("Parse loopback address 127.0.0.1") {
    bool result = ipv4_addr_try_parse("127.0.0.1", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.bytes[0] == 127);
    REQUIRE(addr.bytes[1] == 0);
    REQUIRE(addr.bytes[2] == 0);
    REQUIRE(addr.bytes[3] == 1);
  }
  SECTION("Parse address with all zeros 0.0.0.0") {
    bool result = ipv4_addr_try_parse("0.0.0.0", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.value == 0);
  }
  SECTION("Parse address with max values 255.255.255.255") {
    bool result = ipv4_addr_try_parse("255.255.255.255", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.bytes[0] == 255);
    REQUIRE(addr.bytes[1] == 255);
    REQUIRE(addr.bytes[2] == 255);
    REQUIRE(addr.bytes[3] == 255);
  }
  SECTION("Parse 10.0.0.1") {
    bool result = ipv4_addr_try_parse("10.0.0.1", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.bytes[0] == 10);
    REQUIRE(addr.bytes[1] == 0);
    REQUIRE(addr.bytes[2] == 0);
    REQUIRE(addr.bytes[3] == 1);
  }
  SECTION("Parse 172.16.254.1") {
    bool result = ipv4_addr_try_parse("172.16.254.1", &addr);
    REQUIRE(result == true);
    REQUIRE(addr.bytes[0] == 172);
    REQUIRE(addr.bytes[1] == 16);
    REQUIRE(addr.bytes[2] == 254);
    REQUIRE(addr.bytes[3] == 1);
  }
}

TEST_CASE("IPv4 address parsing - invalid addresses", "[ipv4][parse][error]") {
  err_logging_disable_guard_t guard; // Silence expected error logs
  ipv4_addr_t addr = {0};
  SECTION("Null pointer input") {
    bool result = ipv4_addr_try_parse(nullptr, &addr);
    REQUIRE(result == false);
  }
  SECTION("Null output pointer") {
    bool result = ipv4_addr_try_parse("192.168.1.1", nullptr);
    REQUIRE(result == false);
  }
  SECTION("Octet overflow - value > 255") {
    bool result = ipv4_addr_try_parse("192.168.1.256", &addr);
    REQUIRE(result == false);
  }
  SECTION("Too many digits in octet") {
    bool result = ipv4_addr_try_parse("192.168.1.0001", &addr);
    REQUIRE(result == false);
  }
  SECTION("Too many periods") {
    bool result = ipv4_addr_try_parse("192.168.1.1.1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Trailing period") {
    bool result = ipv4_addr_try_parse("192.168.1.", &addr);
    REQUIRE(result == false);
  }
  SECTION("Leading period") {
    bool result = ipv4_addr_try_parse(".192.168.1.1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Double periods") {
    bool result = ipv4_addr_try_parse("192..168.1.1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Invalid character - letter") {
    bool result = ipv4_addr_try_parse("192.168.a.1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Invalid character - special char") {
    bool result = ipv4_addr_try_parse("192.168.1-1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Too few octets") {
    bool result = ipv4_addr_try_parse("192.168.1", &addr);
    REQUIRE(result == false);
  }
  SECTION("Empty string") {
    bool result = ipv4_addr_try_parse("", &addr);
    REQUIRE(result == false);
  }
}

#pragma mark - IPv4 Address Mask Tests

TEST_CASE("IPv4 address mask application", "[ipv4][mask]") {
  ipv4_addr_t addr = {0};
  ipv4_addr_t result = {0};
  SECTION("Apply /24 mask to 192.168.1.100") {
    ipv4_addr_try_parse("192.168.1.100", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 24, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 192);
    REQUIRE(result.bytes[1] == 168);
    REQUIRE(result.bytes[2] == 1);
    REQUIRE(result.bytes[3] == 0);
  }
  SECTION("Apply /32 mask to 192.168.1.100") {
    ipv4_addr_try_parse("192.168.1.100", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 32, &result);
    REQUIRE(success == true);
    // /32 should preserve all bits
    REQUIRE(result.bytes[0] == 192);
    REQUIRE(result.bytes[1] == 168);
    REQUIRE(result.bytes[2] == 1);
    REQUIRE(result.bytes[3] == 100);
  }
  SECTION("Apply /16 mask to 192.168.1.100") {
    ipv4_addr_try_parse("192.168.1.100", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 16, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 192);
    REQUIRE(result.bytes[1] == 168);
    REQUIRE(result.bytes[2] == 0);
    REQUIRE(result.bytes[3] == 0);
  }
  SECTION("Apply /8 mask to 192.168.1.100") {
    ipv4_addr_try_parse("192.168.1.100", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 8, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 192);
    REQUIRE(result.bytes[1] == 0);
    REQUIRE(result.bytes[2] == 0);
    REQUIRE(result.bytes[3] == 0);
  }
  SECTION("Apply /0 mask to 192.168.1.100") {
    ipv4_addr_try_parse("192.168.1.100", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 0, &result);
    REQUIRE(success == true);
    // /0 should zero out all bits
    REQUIRE(result.value == 0);
  }
  SECTION("Apply /28 mask to 10.0.0.127") {
    ipv4_addr_try_parse("10.0.0.127", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 28, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 10);
    REQUIRE(result.bytes[1] == 0);
    REQUIRE(result.bytes[2] == 0);
    REQUIRE(result.bytes[3] == 112); // 127 & 240 = 112 (keep upper 4 bits)
  }
  SECTION("Apply /30 mask to 172.16.0.254") {
    ipv4_addr_try_parse("172.16.0.254", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 30, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 172);
    REQUIRE(result.bytes[1] == 16);
    REQUIRE(result.bytes[2] == 0);
    REQUIRE(result.bytes[3] == 252); // Keep upper 6 bits
  }
  SECTION("Apply /12 mask to 172.31.255.255") {
    ipv4_addr_try_parse("172.31.255.255", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 12, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 172);
    REQUIRE(result.bytes[1] == 16);
    REQUIRE(result.bytes[2] == 0);
    REQUIRE(result.bytes[3] == 0);
  }
  SECTION("Apply /20 mask to 10.128.191.255") {
    ipv4_addr_try_parse("10.128.191.255", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 20, &result);
    REQUIRE(success == true);
    REQUIRE(result.bytes[0] == 10);
    REQUIRE(result.bytes[1] == 128);
    REQUIRE(result.bytes[2] == 176); // Keep upper 4 bits of third octet
    REQUIRE(result.bytes[3] == 0);
  }
}

TEST_CASE("IPv4 address mask application - error handling", "[ipv4][mask][error]") {
  err_logging_disable_guard_t guard; // Silence expected error logs
  ipv4_addr_t addr = {0};
  ipv4_addr_t result = {0};
  SECTION("Null prefix pointer") {
    bool success = ipv4_addr_apply_mask(nullptr, 24, &result);
    REQUIRE(success == false);
  }
  SECTION("Null output pointer") {
    ipv4_addr_try_parse("192.168.1.1", &addr);
    bool success = ipv4_addr_apply_mask(&addr, 24, nullptr);
    REQUIRE(success == false);
  }
}

#pragma mark - IPv4 Address Rendering Tests

TEST_CASE("IPv4 address rendering", "[ipv4][render]") {
  ipv4_addr_t addr = {0};
  char buffer[16];
  SECTION("Render 192.168.1.1") {
    addr.bytes[0] = 192;
    addr.bytes[1] = 168;
    addr.bytes[2] = 1;
    addr.bytes[3] = 1;
    bool result = ipv4_addr_render(&addr, buffer);
    REQUIRE(result == true);
    REQUIRE(strcmp(buffer, "192.168.1.1") == 0);
  }
  SECTION("Render 0.0.0.0") {
    addr.value = 0;
    bool result = ipv4_addr_render(&addr, buffer);
    REQUIRE(result == true);
    REQUIRE(strcmp(buffer, "0.0.0.0") == 0);
  }
  SECTION("Render 255.255.255.255") {
    addr.bytes[0] = 255;
    addr.bytes[1] = 255;
    addr.bytes[2] = 255;
    addr.bytes[3] = 255;
    bool result = ipv4_addr_render(&addr, buffer);
    REQUIRE(result == true);
    REQUIRE(strcmp(buffer, "255.255.255.255") == 0);
  }
  SECTION("Render 10.0.0.1") {
    addr.bytes[0] = 10;
    addr.bytes[1] = 0;
    addr.bytes[2] = 0;
    addr.bytes[3] = 1;
    bool result = ipv4_addr_render(&addr, buffer);
    REQUIRE(result == true);
    REQUIRE(strcmp(buffer, "10.0.0.1") == 0);
  }
}

TEST_CASE("IPv4 address rendering - error handling", "[ipv4][render][error]") {
  err_logging_disable_guard_t guard; // Silence expected error logs
  ipv4_addr_t addr = {0};
  char buffer[16];
  SECTION("Null address pointer") {
    bool result = ipv4_addr_render(nullptr, buffer);
    REQUIRE(result == false);
  }
  SECTION("Null output buffer") {
    bool result = ipv4_addr_render(&addr, nullptr);
    REQUIRE(result == false);
  }
}

#pragma mark - IPv4 Address Round-trip Tests

TEST_CASE("IPv4 address parse-render round-trip", "[ipv4][roundtrip]") {
  ipv4_addr_t addr = {0};
  char buffer[16];
  SECTION("Round-trip 192.168.1.100") {
    const char *original = "192.168.1.100";
    ipv4_addr_try_parse(original, &addr);
    ipv4_addr_render(&addr, buffer);
    REQUIRE(strcmp(buffer, original) == 0);
  }
  SECTION("Round-trip 127.0.0.1") {
    const char *original = "127.0.0.1";
    ipv4_addr_try_parse(original, &addr);
    ipv4_addr_render(&addr, buffer);
    REQUIRE(strcmp(buffer, original) == 0);
  }
  SECTION("Round-trip 10.20.30.40") {
    const char *original = "10.20.30.40";
    ipv4_addr_try_parse(original, &addr);
    ipv4_addr_render(&addr, buffer);
    REQUIRE(strcmp(buffer, original) == 0);
  }
}

