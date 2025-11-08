// rt.cpp

#include "rt.h"
#include "utils.h"

void rt_init(rt_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
}

bool rt_add_direct_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  return false;
}

bool rt_add_route(rt_t *t, ipv4_addr_t *addr, uint8_t mask, ipv4_addr_t *gw_addr, interface_t *ointf) {
  EXPECT_RETURN_BOOL(t != nullptr, "Empty rt param", false);
  return false;
}

void rt_dump(rt_t *t) {
  EXPECT_RETURN(t != nullptr, "Empty rt param");
}
