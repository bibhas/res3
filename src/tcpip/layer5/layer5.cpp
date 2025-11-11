// layer5.cpp

#include "layer5.h"
#include "layer3/layer3.h"

bool layer5_perform_ping(node_t *n, ipv4_addr_t *addr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty destination address param", false);
  layer3_demote(n, nullptr, 0, IPV4_PROT_ICMP, addr);
  return true;
}

void layer5_promote(node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, uint32_t l5_prot) {
  // TODO
}
