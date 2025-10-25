// tcpip.cpp

#include <iostream>
#include <string.h>
#include "glthread.h"
#include "graph.h"
#include "utils.h"

graph_t *build_first_topo() {
  graph_t *topo = graph_init("Hello World Generic Graph");
  node_t *R0_re = graph_add_node(topo, "R0_re");
  node_t *R1_re = graph_add_node(topo, "R1_re");
  node_t *R2_re = graph_add_node(topo, "R2_re");

  link_nodes(R0_re, R1_re, "eth0/0", "eth0/1", 1);
  link_nodes(R1_re, R2_re, "eth0/2", "eth0/3", 1);
  link_nodes(R0_re, R2_re, "eth0/4", "eth0/5", 1);

  node_set_loopback_address(R0_re, "122.1.1.0");
  node_set_interface_ipv4_address(R0_re, "eth0/4", "40.1.1.1", 24);
  node_set_interface_ipv4_address(R0_re, "eth0/0", "20.1.1.1", 24);

  node_set_loopback_address(R1_re, "122.1.1.1");
  node_set_interface_ipv4_address(R1_re, "eth0/1", "20.1.1.2", 24);
  node_set_interface_ipv4_address(R1_re, "eth0/2", "30.1.1.1", 24);

  node_set_loopback_address(R2_re, "122.1.1.2");
  node_set_interface_ipv4_address(R2_re, "eth0/3", "30.1.1.2", 24);
  node_set_interface_ipv4_address(R2_re, "eth0/5", "40.1.1.2", 24);

  // Test subnet

  ipv4_addr_t subnet = {.bytes = {20, 1, 1, 0}};
  interface_t *intf = nullptr;
  node_t *node = R0_re;
  bool resp = node_get_interface_matching_subnet(node, &subnet, &intf);
  if (resp) {
    printf("Interface named `%s` in node `%s` matches subnet `" IPV4_ADDR_FMT "`\n", intf->if_name, node->node_name, IPV4_ADDR_BYTES_BE(subnet));
  }
  else {
    printf("No interfaces in node `%s` matches subnet `" IPV4_ADDR_FMT "`\n", node->node_name, IPV4_ADDR_BYTES_BE(subnet));
  }
  return topo;
}

int main(int argc, const char **argv) {
  graph_t *topo = build_first_topo();
  graph_dump(topo);

  printf("\nMisc. tests:\n-------------\n");

  // Test masking
  uint32_t resp;
  const char *inp = "192.168.12.98";
  uint8_t mask = 21;
  ipv4_addr_t ipv4, masked_ipv4;
  bool status = ipv4_addr_try_parse(inp, &ipv4);
  EXPECT_FATAL(status == true, "ipv4_addr_try_parse failed");
  status = ipv4_addr_apply_mask(&ipv4, mask, &masked_ipv4);
  EXPECT_FATAL(status == true, "ipv4_addr_apply_mask failed");
  printf("%s / %i = " IPV4_ADDR_FMT "\n", inp, mask, IPV4_ADDR_BYTES_BE(masked_ipv4));
  
  // Test broadcast MAC
  mac_addr_t mac;
  status = mac_addr_fill_broadcast(&mac);
  EXPECT_FATAL(status == true, "mac_addr_fill_broadcast failed");
  printf("Broadcast mac: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(mac));
  printf("Is broadcast? : %s\n", (MAC_ADDR_IS_BROADCAST(mac) ? "true" : "false"));

  // IPv4 rendering
  char buff[16];
  status = ipv4_addr_render(&ipv4, buff);
  EXPECT_FATAL(status == true, "ipv4_addr_render failed");
  printf("Rendered ipv4: %s\n", buff);

  return 0;
}

