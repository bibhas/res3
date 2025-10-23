// utils.cpp

#include <iostream>
#include <cstdarg>
#include <cstdint>
#include <stack>

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
