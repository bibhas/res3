// utils.h

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <iostream>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define __PACK__ __attribute__((packed))

#define COPY_STRING_TO(DST, LITERAL, MAXLEN) \
  do { \
    int name_len = std::max((int)strlen((LITERAL)), MAXLEN); \
    strncpy((DST), (LITERAL), name_len); \
    *((DST) + name_len - 1) = '\0'; \
  } \
  while (false) 

#define BYTES_FROM_BYTEARRAY_BE(ARR) \
  (ARR)[0], (ARR)[1], (ARR)[2], (ARR)[3], (ARR)[4], (ARR)[5]

#pragma mark -

// EXPECT assertions

struct err_state_t {
  std::atomic<bool> silent{false};
  FILE *file{stderr};
};

extern err_state_t __err;

struct err_logging_disable_guard_t {
  bool cache;
  err_logging_disable_guard_t() : cache(__err.silent.load()) {
    __err.silent.store(true);
  }
  virtual ~err_logging_disable_guard_t() {
    __err.silent.store(cache); // restore saved value
  }
};

#define ENABLE_ERR_LOGGING() (__err.silent.store(false))
#define DISABLE_ERR_LOGGING() (__err.silent.store(true))
#define SET_ERR_STREAM(f) (__err.file = (f))
#define LOG_ERR(...) do { if (!__err.silent.load()) fprintf(__err.file,"[ERR] " __VA_ARGS__); } while(0)

#define DEBUG 0

#if DEBUG
#define LOG_DEBUG(...) do { if (!__err.silent.load()) fprintf(__err.file,"[DEBUG] " __VA_ARGS__); } while(0)
#else
#define LOG_DEBUG(...) 
#endif

#define EXPECT_FATAL(c,m) do { if(!(c)){LOG_ERR("%s\n",m);exit(EXIT_FAILURE);} } while(0)
#define EXPECT_RETURN_BOOL(c,m,r) do { if(!(c)){LOG_ERR("%s\n",m);return(r);} } while(0)
#define EXPECT_RETURN_VAL(c,m,r) EXPECT_RETURN_BOOL(c,m,r)
#define EXPECT_RETURN(c,m) do { if(!(c)){LOG_ERR("%s\n",m);return;} } while(0)
#define EXPECT_CONTINUE(c,m) do { if(!(c)){LOG_ERR("%s\n",m);continue;} } while(0)
#define ERR_RETURN_BOOL(m,r) do { LOG_ERR("%s\n",m);return(r);} while(0)

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

struct __PACK__ ipv4_addr_t {
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

#define IPV4_ADDR_IS_EQUAL(IP0, IP1) \
  ((IP0).value == (IP1).value)

#define IPV4_ADDR_PTR_IS_EQUAL(IP0, IP1) \
  ((IP0)->value == (IP1)->value)

// Reads left to right (MSB is 0th index, LSB is 31st)
#define IPV4_ADDR_READ_BIT(ADDR, BIT) ((ADDR).value >> (31 - BIT)) & 0x1)
#define IPV4_ADDR_PTR_READ_BIT(ADDR, BIT) (((ADDR)->value >> (31 - BIT)) & 0x1)

bool ipv4_addr_try_parse(const char *addrstr, ipv4_addr_t *out);
bool ipv4_addr_apply_mask(ipv4_addr_t *prefix, uint8_t mask, ipv4_addr_t *out);
bool ipv4_addr_render(ipv4_addr_t *addr, char *out);

#pragma mark -

// MAC Address

struct __PACK__ mac_addr_t {
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

void mac_addr_fill_broadcast(mac_addr_t *addr);
bool mac_addr_try_parse(const char *addrstr, mac_addr_t *out);

#define MAC_ADDR_IS_BROADCAST(MAC) ( \
  (MAC).bytes[0] == 0xFF && \
  (MAC).bytes[1] == 0xFF && \
  (MAC).bytes[2] == 0xFF && \
  (MAC).bytes[3] == 0xFF && \
  (MAC).bytes[4] == 0xFF && \
  (MAC).bytes[5] == 0xFF \
)

#define MAC_ADDR_IS_EQUAL(MAC0, MAC1) \
  ((MAC0).value == (MAC1).value)

#define MAC_ADDR_PTR_IS_EQUAL(MAC0, MAC1) \
  ((MAC0)->value == (MAC1)->value)

extern mac_addr_t empty_mac_addr;

#define MAC_ADDR_ZEROED empty_mac_addr
#define MAC_ADDR_PTR_ZEROED (&empty_mac_addr)
