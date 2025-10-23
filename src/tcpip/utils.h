// utils.h

#pragma once

#include <cmath>
#include <cstring>

#define COPY_STRING_TO(DST, LITERAL, MAXLEN) \
  do { \
    int name_len = std::max((int)strlen((LITERAL)), MAXLEN); \
    strncpy((DST), (LITERAL), name_len); \
    *((DST) + name_len - 1) = '\0'; \
  } \
  while (false) 

