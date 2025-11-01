// layer2test.cpp

#include "catch2.hpp"
#include "layer2.h"
#include "phy.h"

extern bool phy_frame_buffer_shift_right(uint8_t **pktptr, uint32_t pktlen, uint32_t buflen);

TEST_CASE("Packet buffer shift right with different packet lengths", "[layer2][phy][buffer]") {
  SECTION("Shift right with pktlen=2") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 2, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0x00, 0xFF, 0xFF};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
  SECTION("Shift right with pktlen=3") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 3, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0xFF, 0xFF, 0x00};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }

  SECTION("Shift right with pktlen=1") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 1, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0x00, 0x00, 0xFF};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
}

TEST_CASE("Packet buffer shift right boundary conditions", "[layer2][phy][buffer][boundary]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  SECTION("Shift with pktlen equal to buflen") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 4, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0xFF, 0xFF, 0x00, 0x00};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
  SECTION("Shift with pktlen greater than buflen") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 5, 4);
    REQUIRE(result == false); // Illegal
  }
  SECTION("Shift with pktlen=0") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 0, 4);
    REQUIRE(result == false); // Illegal
  }
}
