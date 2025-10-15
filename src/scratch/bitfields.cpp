// bitfields.cpp

#include <iostream>
#include <cstdint>

struct mixed_t {
  uint8_t l2_len:7;
  uint16_t l3_len:9;
  uint8_t l4_len:8;
  uint16_t tso_segz:16;
};

struct same_t {
  uint64_t l2_len:7;
  uint64_t l3_len:9;
  uint64_t l4_len:8;
  uint64_t tso_segz:16;
};

int main(int argc, char **argv) {
  printf("Size comparision:\n");
  printf("  mixed: %2lu\n", sizeof(mixed_t));
  printf("  same: %2lu\n", sizeof(same_t));
  return 0;
}
