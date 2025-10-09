// byteorder.cpp

#include <iostream>
#include <cstdint>
#include <endian.h>

static inline void print_bits(char val) {
  std::cout << "0b";
  for (int64_t i = (sizeof(val) * 8) - 1; i >= 0; i--) {
    std::cout << ((val >> i) & 1);
  }
  std::cout << std::endl;
}

static inline void print_bytes(std::uint64_t value) {
  std::cout << sizeof(value) << std::endl;
  for (int64_t i = sizeof(value) - 1; i >= 0  ; i--) {
    std::uint64_t byte = (value >> (i * 8)) & 0b11111111;
    std::cout << byte << " ";
  }
  std::cout << std::endl;
}

int main(int argc, const char **argv) {
  //std::uint64_t netOrder = 16783740446165225840;
  std::uint64_t netOrder = 8129287598537698280;
  print_bytes(netOrder);
  print_bytes(be64toh(netOrder));
  print_bits(255);
  return 0;
}
