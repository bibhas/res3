// net.h

#pragma once

#include <cstdint>

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

bool ipv4_addr_try_parse(const char *addstr, ipv4_addr_t *out);

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

void node_netprop_init(node_netprop_t *prop);
bool node_set_loopback_address(node_t *n, const char *addr);
bool node_set_interface_ipv4_address(node_t *n, const char *intf, const char *addr, uint8_t mask);

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

void interface_netprop_init(interface_netprop_t *prop);

