// layer2test.cpp

#include "layer2.h"
#include "comm.h"

extern bool comm_pkt_buffer_shift_right(uint8_t **pktptr, uint32_t pktlen, uint32_t buflen);

static inline void test_hotpath(uint32_t pktlen) {
  uint8_t buf0[] = {0xFF, 0xFF, 0x0, 0x0};
  printf("{0x%02X, 0x%02X, 0x%02X, 0x%02X} >> pktlen: %u >> ", buf0[0], buf0[1], buf0[2], buf0[3], pktlen);
  uint8_t *buf0_ptr = (uint8_t *)buf0;
  bool resp = comm_pkt_buffer_shift_right(&buf0_ptr, pktlen, 4);
#pragma unused(resp)
  printf("{0x%02X, 0x%02X, 0x%02X, 0x%02X}\n", buf0[0], buf0[1], buf0[2], buf0[3]);
}

int main(int argc, const char **argv) {
  test_hotpath(2);
  test_hotpath(3);
  test_hotpath(1);
  return 0;
}
