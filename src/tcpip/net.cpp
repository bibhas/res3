// net.cpp

#include "net.h"
#include "graph.h"
#include "utils.h"

#pragma mark -

// IPv4 Address

bool ipv4_addr_try_parse(const char *addrstr, ipv4_addr_t *out) {
  // Sanity check
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
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
  for (int i = strlen(addrstr) - 1; i >= 0; i--) {
    char c = addrstr[i];
    if (c == '\0' && i == strlen(addrstr) - 1) {
      continue; // Ignore null terminator at the end
    }
    else if (c == '.') {
      // A period marks the end of an octet.
      // We could get a period without seeing any numbers. That is invalid.
      // For eg. 192.168.1. <-
      EXPECT_RETURN_BOOL(digits != 0, "No digits", false);
      EXPECT_RETURN_BOOL(acc <= 255, "Octet overflow", false);
      EXPECT_RETURN_BOOL(bytes >= 0, "More than three periods", false);
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
      // We could get more than 3 digits, which is invalid.
      // For eg. 192.168.0.0001 <- 
      EXPECT_RETURN_BOOL(digits <= 3, "More than 3 digits per octet", false);
    }
    else {
      // Got something other than '.' or digits
      ERR_RETURN_BOOL("Invalid character", false);
    }
  }
  // The 0th octet will not have a period before it, so we need to manually
  // consider it.
  EXPECT_RETURN_BOOL(digits != 0, "No digits", false);
  EXPECT_RETURN_BOOL(acc <= 255, "Octer overflow", false);
  EXPECT_RETURN_BOOL(bytes >= 0, "More than three periods", false);
  resp.bytes[0] = acc;
  // Fill the return object
  out->value = resp.value;
  // And, we're done
  return true;
}

#pragma mark -

// MAC address

bool mac_addr_try_parse(const char *addrstr, mac_addr_t *out) {
  // Sanity check
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  // Use a union so we can easily fill the individual bytes and then read the
  // combined 32-bit integer without worrying about bit shifting manually.
  union {
    uint64_t value:48;
    uint8_t bytes[6];
  } resp = {0};
  int acc = 0;    // Accumulator for current octet (0-255)
  int mult = 1;   // Decimal multipler (1, 10, 100)
  int digits = 0; // Digits in current octet
  int bytes = 5;  // Bytes to process
  // Traverse the string from right-to-left
  int i = strlen(addrstr) - 1;
  while (i >= 0) {
    char c = addrstr[i];
    if (c == '\0' && i == strlen(addrstr) - 1) {
      continue; // Ignore null terminator at the end
    }
    else if (c == ':') {
      // A colon marks the end of an octet.
      // We could get a colon without seeing any numbers. That is valid
      EXPECT_RETURN_BOOL(bytes >= 0, "More than six bytes", false);
      EXPECT_RETURN_BOOL(acc <= 255, "Octet overflow", false);
      resp.bytes[bytes] = acc;
      bytes--;
      acc = 0;    // Reset accumulator for next octet
      mult = 1;   // Reset decimal multipler for next octet
      digits = 0; // Reset digit counter for next octet
    }
    else if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
      uint8_t val = 0;
      if (c >= '0' && c <= '9') { val = c - '0'; }
      else if (c >= 'A' && c <= 'F') { val = 10 + c - 'A'; }
      else { val = 10 + c - 'a'; }
      acc += val * mult; 
      mult *= 16; // Base 16
      digits++;
      // We could get more than 3 digits, which is invalid.
      // For eg. 192.168.0.0001 <- 
      EXPECT_RETURN_BOOL(digits <= 2, "More than 2 digits per octet", false);
    }
    else if (c == 'x' || c == 'X') {
      EXPECT_RETURN_BOOL(i > 0, "Got a leading x character", false);
      EXPECT_RETURN_BOOL(addrstr[i - 1] == '0', "Invalid x character", false);
      i--; // Skip the 0 in 0x
    }
    else {
      // Got something other than '.' or digits
      ERR_RETURN_BOOL("Invalid character", false);
    }
    i--;
  }
  // The 0th octet will not have a colon before it, so we need to manually
  // consider it.
  EXPECT_RETURN_BOOL(digits != 0, "No digits", false);
  EXPECT_RETURN_BOOL(acc <= 255, "Octer overflow", false);
  EXPECT_RETURN_BOOL(bytes >= 0, "More than five colons", false);
  resp.bytes[0] = acc;
  // Fill the return object
  out->value = resp.value;
  // And, we're done
  return true;
}

#pragma mark -

// Node

void node_netprop_init(node_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty prop");
  prop->loopback.configured = false;
  prop->loopback.addr.value = 0;
}

bool node_set_loopback_address(node_t *n, const char *addrstr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  ipv4_addr_t addr = {0};
  bool resp = ipv4_addr_try_parse(addrstr, &addr);
  EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_try_parse failed", false);
  n->netprop.loopback.configured = true;
  // We could've passed addr directly to the parsing function, but didn't, for
  // the sake of readability.
  n->netprop.loopback.addr = addr;
  return true;
}

bool node_set_interface_ipv4_address(node_t *n, const char *intf, const char *addrstr, uint8_t mask) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  // Parse address string
  ipv4_addr_t addr = {0};
  bool resp = ipv4_addr_try_parse(addrstr, &addr);
  EXPECT_RETURN_BOOL(resp == true, "ipv4_addr_try_parse failed", false);
  candidate->netprop.ip = {
    .configured = true,
    .addr = addr,
    .mask = mask
  };
  return true;
}

bool node_unset_interface_ipv4_address(node_t *n, const char *intf) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface param", false);
  // Find interface
  interface_t *candidate = node_get_interface_by_name(n, intf);
  EXPECT_RETURN_BOOL(candidate != nullptr, "node_get_interface_by_name failed", false);
  // Parse address string
  ipv4_addr_t addr = {0};
  candidate->netprop.ip = {
    .configured = false,
    .addr = addr,
    .mask = 0
  };
  return true;
}

#pragma mark -

// Interface

void interface_netprop_init(interface_netprop_t *prop) {
  EXPECT_RETURN(prop != nullptr, "Empty interface property param");
  prop->mac_addr.value = 0;
  prop->ip.configured = false;
  prop->ip.addr.value = 0;
}

bool interface_assign_mac_address(interface_t *interface, const char *addrstr) {
  EXPECT_RETURN_BOOL(interface != nullptr, "Empty interface param", false);
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  return false;
}
