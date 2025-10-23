// utils.h

#pragma once

#include <cmath>
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

#define EXPECT_FATAL(COND, MSG) if (!(COND)) { printf("%s\n", MSG); exit(EXIT_FAILURE); }
#define EXPECT_RETURN_BOOL(COND, MSG, RET) if (!(COND)) { printf("[ERR] %s\n", MSG); return RET; }
#define EXPECT_RETURN(COND, MSG) if (!(COND)) { printf("[ERR] %s\n", MSG); return; }
#define ERR_RETURN_BOOL(MSG, RET) printf("[ERR] %s\n", MSG); return RET;

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

