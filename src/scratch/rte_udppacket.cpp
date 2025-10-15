// rte_udppacket.cpp

#include <iostream>
#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>
#include <arpa/inet.h>
#include <rte_mempool.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

static inline void log_pktmbuf(struct rte_mbuf *m) {
  printf("Packet Information:\n");
  printf("  Total length: %u bytes\n", m->pkt_len);

  // Ethernet header
  printf("Ethernet:\n");
  auto eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
  printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
    eth_hdr->src_addr.addr_bytes[0],
    eth_hdr->src_addr.addr_bytes[1],
    eth_hdr->src_addr.addr_bytes[2],
    eth_hdr->src_addr.addr_bytes[3],
    eth_hdr->src_addr.addr_bytes[4],
    eth_hdr->src_addr.addr_bytes[5]
  );
  printf("  Dst MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
    eth_hdr->dst_addr.addr_bytes[0],
    eth_hdr->dst_addr.addr_bytes[1],
    eth_hdr->dst_addr.addr_bytes[2],
    eth_hdr->dst_addr.addr_bytes[3],
    eth_hdr->dst_addr.addr_bytes[4],
    eth_hdr->dst_addr.addr_bytes[5]
  );

  // Ensure we have an IPv4 packet
  EXPECT(eth_hdr->ether_type == RTE_ETHER_TYPE_IPV4, "Encountered ~IPv4 header");

  // IPv4 header
  printf("IPv4: \n");
  auto ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
  printf("  Src IP: %u.%u.%u.%u\n",
    (ntohl(ip_hdr->src_addr) >> 24) & 0xFF,
    (ntohl(ip_hdr->src_addr) >> 16) & 0xFF,
    (ntohl(ip_hdr->src_addr) >> 8) & 0xFF,
    (ntohl(ip_hdr->src_addr) >> 0) & 0xFF
  );
  printf("  Dst IP: %u.%u.%u.%u\n",
    (ntohl(ip_hdr->dst_addr) >> 24) & 0xFF,
    (ntohl(ip_hdr->dst_addr) >> 16) & 0xFF,
    (ntohl(ip_hdr->dst_addr) >> 8) & 0xFF,
    (ntohl(ip_hdr->dst_addr) >> 0) & 0xFF
  );

  // Ensure we have a UDP datagram
  EXPECT(ip_hdr->next_proto_id == IPPROTO_UDP, "Encountered ~UDP header");

  // UDP header
  auto udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
  printf("UDP:\n");
  printf("  Src Port: %u\n", ntohs(udp_hdr->src_port));
  printf("  Dst Port: %u\n", ntohs(udp_hdr->dst_port));

  // Ensure we have a payload
  EXPECT(udp_hdr->dgram_len > sizeof(struct rte_udp_hdr), "No payload");

  // Payload
  char *payload = (char *)(udp_hdr + 1);
  printf("Payload:\n  %s\n", payload);
}

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Setup memory pool
  struct rte_mempool *mp = rte_pktmbuf_pool_create(
    "packet pool", 
    8192, // n
    256,  // per lcore cache size
    0,    // priv size
    1024, // data_len (bytes)
    rte_socket_id() // socket
  );
  EXPECT(mp != nullptr, "rte_pktmbuf_pool_create failed");

  // Create buffer
  struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
  EXPECT(m != nullptr, "rte_pktmbuf_alloc failed");

  /* 
   * Build UDP packet
   */

  // Prepare payload
  const char *payload = "Hello DPDK!";
  size_t payload_len = strlen(payload) + 1;

  // Setup ethernet header
  auto eth_hdr = (struct rte_ether_hdr *)rte_pktmbuf_append(m, sizeof(struct rte_ether_hdr));
  EXPECT(eth_hdr, "rte_pktmbuf_append failed");
  const uint8_t src_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  const uint8_t dst_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  memcpy(&eth_hdr->dst_addr, dst_mac, RTE_ETHER_ADDR_LEN /* 6 */);
  memcpy(&eth_hdr->src_addr, src_mac, RTE_ETHER_ADDR_LEN);
  eth_hdr->ether_type = RTE_ETHER_TYPE_IPV4;

  // Setup ip header
  auto ip_hdr = (struct rte_ipv4_hdr *)rte_pktmbuf_append(m, sizeof(struct rte_ipv4_hdr));
  EXPECT(ip_hdr, "rte_pktmbuf_append failed");
  const uint32_t src_ip = 0xC0A80001;
  const uint32_t dst_ip = 0xC0A800FE;
  ip_hdr->version_ihl = 0x45; //??
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) + payload_len);
  ip_hdr->packet_id = 0;
  ip_hdr->fragment_offset = 0;
  ip_hdr->time_to_live = 64;
  ip_hdr->next_proto_id = IPPROTO_UDP;
  ip_hdr->src_addr = htonl(src_ip);
  ip_hdr->dst_addr = htonl(dst_ip);
  ip_hdr->hdr_checksum = 0;

  // Setup udp header
  uint16_t src_port = 12345;
  uint16_t dst_port = 80;
  auto udp_hdr = (struct rte_udp_hdr *)rte_pktmbuf_append(m, sizeof(struct rte_udp_hdr));
  EXPECT(udp_hdr, "rte_pktmbuf_append failed");
  udp_hdr->src_port = htons(src_port);
  udp_hdr->dst_port = htons(dst_port);
  udp_hdr->dgram_len = htons(sizeof(struct rte_udp_hdr) + payload_len);
  udp_hdr->dgram_cksum = 0;

  // Setup payload
  char *data = (char *)rte_pktmbuf_append(m, payload_len);
  memcpy(data, payload, payload_len);

  // Update pkg length
  m->pkt_len = m->data_len;

  // Log packet
  log_pktmbuf(m);

  // Cleanup
  rte_pktmbuf_free(m);
  rte_mempool_free(mp);
  rte_eal_cleanup();
  return 0;
}
