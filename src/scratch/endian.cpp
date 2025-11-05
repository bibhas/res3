#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>

void log_short(uint16_t val) {
  unsigned char *p = (unsigned char *)&val;
  for (size_t i = 0; i < sizeof(val); ++i) {
    printf("  byte[%zu] = 0x%02x = 0b", i, p[i]);
    for (int b = 7; b >= 0; --b)
      putchar((p[i] >> b) & 1 ? '1' : '0');
    putchar('\n');
  }
}

int main(int argc, char *argv[]) {
  uint16_t val = 1;
  printf("host:\n");
  log_short(val); 
  return 0;
}
