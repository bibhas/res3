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

