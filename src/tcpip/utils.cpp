// utils.cpp

#include <iostream>
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdint>
#include <stack>
#include "net.h"
#include "utils.h"

mac_addr_t empty_mac_addr {.value = 0};

bool __ipv4_addr_str_try_parse_host(const char *addrstr, uint32_t *out);
bool __ipv4_addr_str_apply_mask(const char *prefix, uint8_t mask, uint32_t *out);

err_state_t __err;

#pragma mark -

// Dump logging

#define INDENTATION_WIDTH 2

static struct {
  int value = 0;
  std::stack<int> stack;
} __dump_line_indentation;
void dump_line(const char *fmt, ...) {
  // TODO: Update string below so that we can show indentation lines/dots
  printf("%.*s", __dump_line_indentation.value, "          ");
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void dump_line_indentation_add(uint8_t w) {
  __dump_line_indentation.value += (w * INDENTATION_WIDTH);
}

void dump_line_indentation_reset() {
  __dump_line_indentation.value = 0;
}

void dump_line_indentation_push() {
  __dump_line_indentation.stack.push(__dump_line_indentation.value);
}

void dump_line_indentation_pop() {
  __dump_line_indentation.value = __dump_line_indentation.stack.top();
  __dump_line_indentation.stack.pop();
}

#pragma mark -

// IPv4

bool __ipv4_addr_str_apply_mask(const char *prefix, uint8_t mask, uint32_t *out) {
  // Sanity check
  EXPECT_RETURN_BOOL(prefix != nullptr, "Empty prefix string param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  uint32_t src;
  // First parse address to uint32_t
  bool status = __ipv4_addr_str_try_parse_host(prefix, &src); // In host byte order
  EXPECT_RETURN_BOOL(status == true, "ipv4_addr_str_try_parse_host failed", false);
  // Next, convert host to network byte order
  uint32_t n_value = htonl(src);
  // Mask off first `mask` bits of n_value
  *out = ntohl(n_value & ((uint32_t)(((uint64_t)1 << mask) - 1) << (32 - mask)));
  return true;
}

bool ipv4_addr_apply_mask(ipv4_addr_t *prefix, uint8_t mask, ipv4_addr_t *out) {
  // Sanity check
  EXPECT_RETURN_BOOL(prefix != nullptr, "Empty prefix param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  out->value = ntohl(htonl(prefix->value) & ((uint32_t)(((uint64_t)1 << mask) - 1) << (32 - mask)));
  return true;
}

bool ipv4_addr_try_parse(const char *addrstr, ipv4_addr_t *out) {
  // Sanity check
  EXPECT_RETURN_BOOL(addrstr != nullptr, "Empty address string param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out ptr param", false);
  uint32_t resp = 0;
  bool status = __ipv4_addr_str_try_parse_host(addrstr, &resp);
  EXPECT_RETURN_BOOL(status == true, "ipv4_addr_str_try_parse_host failed", false);
  // Fill the return object
  out->value = resp;
  // And, we're done
  return true;
}

bool __ipv4_addr_str_try_parse_host(const char *addrstr, uint32_t *out) {
  /*
   * This is an exercise in hand rolling the entire code so we're refraining
   * from using the OS provided function(s) to parse address strings.
   */
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
  int mult = 1;   // Decimal multiplier (1, 10, 100)
  int digits = 0; // Digits in current octet
  int bytes = 3;  // Bytes to process
  int octets = 0;
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
      mult = 1;   // Reset decimal multiplier for next octet
      digits = 0; // Reset digit counter for next octet
      octets++;
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
  EXPECT_RETURN_BOOL(acc <= 255, "Octet overflow", false);
  EXPECT_RETURN_BOOL(bytes >= 0, "More than three periods", false);
  resp.bytes[0] = acc;
  octets++;
  // Make sure we got four octets
  if (octets != 4) {
    return false;
  }
  // Fill the return value
  *out = resp.value;
  // And, we're done
  return true;
}

bool ipv4_addr_render(ipv4_addr_t *addr, char *out) {
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty addr param", false);
  EXPECT_RETURN_BOOL(out != nullptr, "Empty out str param", false);
  snprintf(out, 16, IPV4_ADDR_FMT, IPV4_ADDR_BYTES_BE(*addr));
  return true;
}

#pragma mark -

// MAC

void mac_addr_fill_broadcast(mac_addr_t *addr) {
  memset(addr->bytes, 0xFF, 6);
}

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

