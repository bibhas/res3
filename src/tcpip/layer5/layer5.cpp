// layer5.cpp

#include "layer5.h"
#include "graph.h"
#include "layer3/layer3.h"

bool layer5_perform_ping(node_t *n, ipv4_addr_t *addr, ipv4_addr_t *ero_addr) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node param", false);
  EXPECT_RETURN_BOOL(addr != nullptr, "Empty destination address param", false);
  uint32_t payload = 659;
  if (ero_addr != nullptr) {
    // Perform IP-in-IP encapsulation
    layer3_demote_ipnip(n, (uint8_t *)&payload, sizeof(uint32_t), PROT_ICMP, addr, ero_addr);
  }
  else {
    NODE_NETSTACK(n).l3.demote(n, (uint8_t *)&payload, sizeof(uint32_t), PROT_ICMP, addr);
  }
  return true;
}

void __layer5_promote(node_t *n, interface_t *intf, uint8_t *payload, uint32_t len, ipv4_addr_t *addr, uint32_t prot) {
  EXPECT_RETURN(n != nullptr, "Empty node param");
  //EXPECT_RETURN(intf != nullptr, "Empty interface param");
  EXPECT_RETURN(payload != nullptr, "Empty payload param");
  EXPECT_RETURN(addr != nullptr, "Empty address param");
  EXPECT_RETURN(prot == PROT_ICMP, "layer5_promote got non-ICMP packet");
  printf("[%s] Received ping from " IPV4_ADDR_FMT " on %s\n", n->node_name, IPV4_ADDR_BYTES_BE(*addr), intf != nullptr ? intf->if_name : "<none>");
  printf("Secret message: %u\n", *(uint32_t *)payload);
}
