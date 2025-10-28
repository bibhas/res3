// utils.h

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>

#define COPY_STRING_TO(DST, LITERAL, MAXLEN) \
  do { \
    int name_len = std::max((int)strlen((LITERAL)), MAXLEN); \
    strncpy((DST), (LITERAL), name_len); \
    *((DST) + name_len - 1) = '\0'; \
  } \
  while (false) 

#pragma mark -

// EXPECT assertions

#define EXPECT_FATAL(COND, MSG) if (!(COND)) { printf("%s\n", MSG); exit(EXIT_FAILURE); }
#define EXPECT_RETURN_BOOL(COND, MSG, RET) if (!(COND)) { printf("[ERR] %s\n", MSG); return RET; }
#define EXPECT_RETURN_VAL(COND, MSG, RET) if (!(COND)) { printf("[ERR] %s\n", MSG); return RET; }
#define EXPECT_RETURN(COND, MSG) if (!(COND)) { printf("[ERR] %s\n", MSG); return; }
#define EXPECT_CONTINUE(COND, MSG) if (!(COND)) { printf("[ERR] %s\n", MSG); continue; }
#define ERR_RETURN_BOOL(MSG, RET) printf("[ERR] %s\n", MSG); return RET;

#pragma mark -

// Dump (logging) functions

// Functions to perform printf with indentation
void dump_line(const char *fmt, ...);
void dump_line_indentation_add(uint8_t val);
void dump_line_indentation_push();
void dump_line_indentation_pop();
void dump_line_indentation_reset();

struct dump_line_indentation_guard_t {
  dump_line_indentation_guard_t() {
    dump_line_indentation_push();
  }
  virtual ~dump_line_indentation_guard_t() {
    dump_line_indentation_pop();
  }
};

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
  (IP).bytes[3], (IP).bytes[2], (IP).bytes[1], (IP).bytes[0]

#define IPV4_ADDR_BYTES_BE(IP) \
  (IP).bytes[0], (IP).bytes[1], (IP).bytes[2], (IP).bytes[3]

bool ipv4_addr_try_parse(const char *addrstr, ipv4_addr_t *out);
bool ipv4_addr_apply_mask(ipv4_addr_t *prefix, uint8_t mask, ipv4_addr_t *out);
bool ipv4_addr_render(ipv4_addr_t *addr, char *out);

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
  (MAC).bytes[0], (MAC).bytes[1], (MAC).bytes[2], \
  (MAC).bytes[3], (MAC).bytes[4], (MAC).bytes[5]

typedef struct mac_addr_t mac_addr_t;

bool mac_addr_fill_broadcast(mac_addr_t *addr);
bool mac_addr_try_parse(const char *addrstr, mac_addr_t *out);

#define MAC_ADDR_IS_BROADCAST(MAC) ( \
  (MAC).bytes[0] == 0xFF && \
  (MAC).bytes[1] == 0xFF && \
  (MAC).bytes[2] == 0xFF && \
  (MAC).bytes[3] == 0xFF && \
  (MAC).bytes[4] == 0xFF && \
  (MAC).bytes[5] == 0xFF \
)


