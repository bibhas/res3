// net.h

#pragma once

#include <cstdint>
#include "graph.h"

// Forward declarations

typedef struct node_t node_t;

#pragma mark -

// IPv4 Address

struct __attribute__((packed)) ipv4_addr_t {
  union {
    uint32_t value;
    uint8_t bytes[4];
  };
};

typedef struct ipv4_addr_t ipv4_addr_t;

#define IPV4_ADDR_FMT "%u.%u.%u.%u"

#define IPV4_ADDR_BYTES_LE(IP) \
  IP.bytes[3], IP.bytes[2], IP.bytes[1], IP.bytes[0]

#define IPV4_ADDR_BYTES_BE(IP) \
  IP.bytes[0], IP.bytes[1], IP.bytes[2], IP.bytes[3]

static inline bool ipv4_addr_try_parse(const char *addstr, ipv4_addr_t *out) {
  // Sanity check
  if (!out) { return false; }  
  // Use a union so we can easily fill the individual bytes and then read the
  // combined 32-bit integer without worrying about bit shifting manually.
  union {
    uint32_t value;
    uint8_t bytes[4];
  } resp = {0};
  int acc = 0;    // Accumulator for current octet (0-255)
  int mult = 1;   // Decimal multipler (1, 10, 100)
  int digits = 0; // Digits in current octet
  int bytes = 3;  // Bytes to process
  // Traverse the string from right-to-left
  for (int i = strlen(addstr) - 1; i >= 0; i--) {
    char c = addstr[i];
    if (c == '\0' && i == strlen(addstr) - 1) {
      continue; // Ignore null terminator at the end
    }
    else if (c == '.') {
      // A period marks the end of an octet.
      if (digits == 0) { 
        // We a period without seeing any numbers
        // For eg. 192.168.1. <-
        return false; 
      }
      if (acc > 255) { 
        // Overflows the octet
        return false; 
      }
      if (bytes < 0) {
        // We got more than 3 periods
        return false;
      }
      resp.bytes[bytes] = acc;
      bytes--;
      acc = 0;    // Reset accumulator for next octet
      mult = 1;   // Reset decimal multipler for next octet
      digits = 0; // Reset digit counter for next octet
    }
    else if (c >= '0' && c <= '9') {
      acc += (c - '0') * mult; 
      mult *= 10;
      digits++;
      if (digits > 3) {
        // We got more than 3 digits
        // For eg. 192.168.0.0001 <- 
        return false;
      }
    }
    else {
      // Got something other than '.' or digits
      return false;
    }
  }
  // The 0th octet will not have a period before it, so we need to manually
  // consider it.
  if (digits == 0 || acc > 255 || bytes != 0) {
    return false;
  }
  resp.bytes[0] = acc;
  // Fill the return object
  out->value = resp.value;
  // And, we're done
  return true;
}

#pragma mark -

// MAC Address

struct __attribute__((packed)) mac_addr_t {
  union {
    uint64_t value:48;
    uint8_t bytes[6];
  };
};

#define MAC_ADDR_FMT "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X"

#define MAC_ADDR_BYTES_BE(MAC) \
  MAC.bytes[0], MAC.bytes[1], MAC.bytes[2], \
  MAC.bytes[3], MAC.bytes[4], MAC.bytes[5]

typedef struct mac_addr_t mac_addr_t;

#pragma mark -

// Node Network Properties

struct node_netprop_t {
  // L3 properties 
  struct {
    bool configured;
    ipv4_addr_t addr;
  } loopback;
};

typedef struct node_netprop_t node_netprop_t;

static inline void node_netprop_init(node_netprop_t *prop) {
  if (!prop) { return; }
  prop->loopback.configured = false;
  prop->loopback.addr.value = 0;
}

static inline bool node_set_loopback_address(node_t *n, uint32_t addr) {
  return false;
}

static inline bool node_set_interface_ipv4_address(node_t *n, const char *intf, uint32_t addr, uint8_t mask) {
  return false;
}

#pragma mark -

// Interface Network Properties

struct interface_netprop_t {
  // L2 properties
  mac_addr_t mac_addr;
  // L3 properties
  struct {
    bool configured;
    ipv4_addr_t addr;
    uint8_t mask;
  } ip;
};

typedef struct interface_netprop_t interface_netprop_t;

static inline void interface_netprop_init(interface_netprop_t *prop) {
  if (!prop) { return; } 
  prop->mac_addr.value = 0;
  prop->ip.configured = false;
  prop->ip.addr.value = 0;
}
