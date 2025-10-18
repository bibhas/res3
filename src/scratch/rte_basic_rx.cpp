// rte_basic_rx.cpp

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
  EXPECT(eth_hdr->ether_type == ntohs(RTE_ETHER_TYPE_IPV4), "Got ~IPv4 header");

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
    8092 * 4, 256, 0, 4096, rte_socket_id()
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
   * Setup device for RX
   */

  // Configure ethernet device
  uint16_t rx_queues = 1;
  uint16_t tx_queues = 1;
  struct rte_eth_conf conf{0};
  resp = rte_eth_dev_configure(port_id, rx_queues, tx_queues, &conf);
  EXPECT(resp == 0, "rte_eth_dev_configure failed");

  // Setup rx queue
  uint16_t rx_queue_id = 0; // We just have one rx queue
  uint16_t rx_descriptors = info.rx_desc_lim.nb_min;
  printf("Using rx_descriptors: %hu\n", rx_descriptors);
  struct rte_eth_rxconf *rx_conf = nullptr; // default will be used
  struct rte_mempool *rx_mp = mp; // We'll reuse our existing mp for all rx mbufs
  resp = rte_eth_rx_queue_setup(
    port_id, rx_queue_id, rx_descriptors, rte_eth_dev_socket_id(port_id), rx_conf, rx_mp
  );
  EXPECT(resp == 0, "rte_eth_rx_queue_setup failed");

  // Setup tx queue
  uint16_t tx_queue_id = 0; // We just have one tx queue
  uint16_t tx_descriptors = info.tx_desc_lim.nb_min;
  printf("Using tx_descriptors: %hu\n", tx_descriptors);
  struct rte_eth_txconf *tx_conf = nullptr; // default will be used
  resp = rte_eth_tx_queue_setup(
    port_id, tx_queue_id, tx_descriptors, rte_eth_dev_socket_id(port_id), tx_conf
  );
  EXPECT(resp == 0, "rte_eth_tx_queue_setup failed");

  // Start device
  resp = rte_eth_dev_start(port_id);
  EXPECT(resp == 0, "rte_eth_dev_start failed");

  // Enable promiscouous mode
  resp = rte_eth_promiscuous_enable(port_id);
  EXPECT(resp == 0, "rte_eth_promiscuous_enable failed");
  printf("port_id: %hu ready to receive packets...\n", port_id);

  uint16_t burst_size = info.default_rxportconf.burst_size;
  printf("Using burst_size: %hu\n", burst_size);
  auto rx_pkts = (struct rte_mbuf *)malloc(sizeof(rte_mbuf) * burst_size);
  while (true) {
    uint16_t nb_rx = rte_eth_rx_burst(port_id, rx_queue_id, &rx_pkts, burst_size);
    EXPECT(nb_rx <= 1, "rte_eth_rx_burst received more than on packets");
    if (nb_rx > 0) {
      // We'll just process the first packet
      log_pktmbuf(rx_pkts);
      rte_pktmbuf_free(rx_pkts);
      break;
    }
  }

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
