// nettest.cpp

#include <iostream>
#include <linux/in.h>
#include <cstdlib>
#include "net.h"

#define EXPECT(COND, MSG) if (!(COND)) { printf("ERROR: %s\n", MSG); exit(EXIT_FAILURE); }

int main(int argc, const char **argv) {

  /*
   * IPv4 address
   */

  // Host order (Intel: little endian)
  ipv4_addr_t ip0 = {.value = 0xC0A80001};
  printf("ip0: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_LE(ip0));

  // Network order (Big endian)
  ipv4_addr_t ip1 = {.bytes = {192, 168, 4, 3}};
  printf("ip1: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(ip1));

  ipv4_addr_t ip2 = {0};
  bool resp = ipv4_addr_try_parse("192.168.9.002", &ip2);
  EXPECT(resp == true, "ipv4_addr_parse failed");
  printf("ip2: " IPV4_ADDR_FMT "\n", IPV4_ADDR_BYTES_BE(ip2));

  /*
   * MAC address
   */

  mac_addr_t mac0 = {.bytes = {0x08,0xc0,0xeb,0x62,0x40,0xb9}};
  printf( "mac0: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(mac0));

  mac_addr_t mac1 = {0};
  resp = mac_addr_try_parse("a:b:c:d:e:f", &mac1);
  EXPECT(resp == true, "mac_addr_try_parse failed");
  printf( "mac1: " MAC_ADDR_FMT "\n", MAC_ADDR_BYTES_BE(mac1));

  return 0;
}
