// rte_basic_tx.cpp

#include <iostream>
#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>

#define EXPECT(CND, MSG) if (!(CND)) { rte_exit(EXIT_FAILURE, MSG); }

// Xeon
#define BIB2_MCX60_MAC {0x08,0xc0,0xeb,0x62,0x40,0xb8}
#define BIB2_MCX61_MAC {0x08,0xc0,0xeb,0x62,0x40,0xb9}
#define BIB2_MCX60_IP4 0xC0A80001
#define BIB2_MCX61_IP4 0xC0A80002

// KR87
#define BIB3_MCX60_MAC {0xe8,0xeb,0xd3,0x08,0xd1,0x70}
#define BIB3_MCX61_MAC {0xe8,0xeb,0xd3,0x08,0xd1,0x71}
#define BIB3_MCX60_IP4 0xC0A80003
#define BIB3_MCX61_IP4 0xC0A80004

static struct rte_mbuf* create_udp_packet(struct rte_mempool *mp) {
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
  // e8:eb:d3:08:d1:71
  const uint8_t src_mac[6] = BIB3_MCX61_MAC;
  const uint8_t dst_mac[6] = BIB2_MCX60_MAC;
  memcpy(&eth_hdr->dst_addr, dst_mac, RTE_ETHER_ADDR_LEN /* 6 */);
  memcpy(&eth_hdr->src_addr, src_mac, RTE_ETHER_ADDR_LEN);
  eth_hdr->ether_type = htons(RTE_ETHER_TYPE_IPV4);

  // Setup ip header
  auto ip_hdr = (struct rte_ipv4_hdr *)rte_pktmbuf_append(m, sizeof(struct rte_ipv4_hdr));
  EXPECT(ip_hdr, "rte_pktmbuf_append failed");
  const uint32_t src_ip = BIB3_MCX61_IP4;
  const uint32_t dst_ip = BIB2_MCX60_IP4;
  ip_hdr->version_ihl = (4 << 4) | (sizeof(*ip_hdr) / 4); // >= 0x45
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) + payload_len);
  ip_hdr->packet_id = 0;
  ip_hdr->fragment_offset = htons(0);
  ip_hdr->time_to_live = 64;
  ip_hdr->next_proto_id = IPPROTO_UDP;
  ip_hdr->src_addr = htonl(src_ip);
  ip_hdr->dst_addr = htonl(dst_ip);
  ip_hdr->hdr_checksum = 0; // Clear checksum field first
  ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

  // Setup udp header
  uint16_t src_port = 12345;
  uint16_t dst_port = 80;
  auto udp_hdr = (struct rte_udp_hdr *)rte_pktmbuf_append(m, sizeof(struct rte_udp_hdr));
  EXPECT(udp_hdr, "rte_pktmbuf_append failed");
  udp_hdr->src_port = htons(src_port);
  udp_hdr->dst_port = htons(dst_port);
  udp_hdr->dgram_len = htons(sizeof(struct rte_udp_hdr) + payload_len);
  udp_hdr->dgram_cksum = 0; // Clear checksum field first

  // Setup payload
  char *data = (char *)rte_pktmbuf_append(m, payload_len);
  memcpy(data, payload, payload_len);

  // Compute UDP checksum
  udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ip_hdr, udp_hdr);

  return m;
}

int main(int argc, char **argv) {

  /*
   * Basic setup
   */

  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Ensure we have usable ethernet devices
  resp = rte_eth_dev_count_avail();
  EXPECT(resp > 0, "rte_eth_dev_count_avail zero");

  // Create memory pool
  struct rte_mempool *mp = rte_pktmbuf_pool_create("pool",
    8092, 256, 0, 1024, rte_socket_id()
  );
  EXPECT(mp != nullptr, "rte_pktmbuf_pool_create failed");

  // Find usable port
  uint16_t port_id = 0;
  bool found = false;
  RTE_ETH_FOREACH_DEV(port_id) {
    struct rte_eth_link link;
    resp = rte_eth_link_get(port_id, &link);
    EXPECT(resp == 0, "rte_eth_link_get failed");
    bool link_up = (link.link_status == RTE_ETH_LINK_UP);
    bool link_25gbps = (link.link_speed == RTE_ETH_SPEED_NUM_25G);
    if (link_up && link_25gbps) {
      found = true;
      break;
    }
  } 
  EXPECT(found == true, "No up port found");
  printf("Usable port_id: %hu\n", port_id);

  // Fetch device info
  struct rte_eth_dev_info info;
  resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info failed");

  /*
   * Device setup for TX
   */

  // Configure ethernet device
  uint16_t rx_queues = 0;
  uint16_t tx_queues = 1;
  struct rte_eth_conf conf{0};
  resp = rte_eth_dev_configure(port_id, rx_queues, tx_queues, &conf);
  EXPECT(resp == 0, "rte_eth_dev_configure failed");

  // Setup tx queue
  uint16_t queue_id = 0; // We just have one tx queue
  uint16_t tx_descriptors = info.tx_desc_lim.nb_min;
  printf("Using tx_descriptors: %hu\n", tx_descriptors);
  struct rte_eth_txconf *txconf = NULL; // default will be used
  resp = rte_eth_tx_queue_setup(
    port_id, queue_id, tx_descriptors, rte_eth_dev_socket_id(port_id), txconf
  );
  EXPECT(resp == 0, "rte_eth_tx_queue_setup failed");

  // Start device
  resp = rte_eth_dev_start(port_id);
  EXPECT(resp == 0, "rte_eth_dev_start failed");

  // Create one udp packet (code taken from earlier sample, see obsidian)
  struct rte_mbuf *udp_packet = create_udp_packet(mp);

  // Send packet
  uint16_t nb_requested = 1;
  uint16_t nb_actual = rte_eth_tx_burst(port_id, queue_id, &udp_packet, nb_requested);
  EXPECT(nb_actual == nb_requested, "rte_eth_tx_burst failed");

  // Free packet mbuf
  rte_pktmbuf_free(udp_packet);

  // Stop device
  resp = rte_eth_dev_stop(port_id);
  EXPECT(resp == 0, "rte_eth_dev_stop failed");

  // Close device (stopping allows re-starting; closing means
  // we're done with the device.)
  resp = rte_eth_dev_close(port_id);
  EXPECT(resp == 0, "rte_eth_dev_close failed");

  // Free mempool
  rte_mempool_free(mp);

  // Cleanup EAL
  resp = rte_eal_cleanup();
  EXPECT(resp == 0, "rte_eal_cleanup failed");

  printf("Done.\n");
  return 0;
}
