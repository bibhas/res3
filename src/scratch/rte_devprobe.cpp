// rte_devprobe.cpp

#include <tuple>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <rte_ethdev.h>
#include <rte_common.h>
#include <rte_ether.h>

// Utils

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

static std::string get_device_name(uint32_t port_id) {
  struct rte_eth_dev_info info;
  int resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info_get failed");
  struct rte_device *device = info.device;
  const char *name = rte_dev_name(device);
  return std::string(name);
}

static std::string get_driver_name(uint32_t port_id) {
  struct rte_eth_dev_info info;
  int resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info failed");
  return std::string(info.driver_name);
}

static std::string get_mac_address(uint32_t port_id) {
  struct rte_ether_addr mac_addr;
  int resp = rte_eth_macaddr_get(port_id, &mac_addr);
  EXPECT(resp == 0, "rte_eth_macaddr_get failed");
  char buf[RTE_ETHER_ADDR_FMT_SIZE] = {0};
  sprintf(buf, 
    RTE_ETHER_ADDR_PRT_FMT, RTE_ETHER_ADDR_BYTES(&mac_addr)
  );
  return std::string(buf);
}

static uint16_t get_rx_queues(uint32_t port_id) {
  struct rte_eth_dev_info info;
  int resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info failed");
  return info.max_rx_queues;
}

static uint16_t get_tx_queues(uint32_t port_id) {
  struct rte_eth_dev_info info;
  int resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info failed");
  return info.max_tx_queues;
}

static bool get_is_link_up(uint32_t port_id) {
  struct rte_eth_link link;
  int resp = rte_eth_link_get(port_id, &link);
  EXPECT(resp == 0, "rte_eth_link_get failed");
  return link.link_status == RTE_ETH_LINK_UP;
}

using speed_t = std::pair<std::string, uint32_t>;

static speed_t get_link_speed_negotiated(uint32_t port_id) {
  struct rte_eth_link link;
  int resp = rte_eth_link_get(port_id, &link);
  EXPECT(resp == 0, "rte_eth_link_get failed");
  switch (link.link_speed) {
    case RTE_ETH_SPEED_NUM_NONE:  return {"0 bps", 0};
    case RTE_ETH_SPEED_NUM_10M:   return {"10 Mbps", 10};
    case RTE_ETH_SPEED_NUM_100M:  return {"100 Mbps", 100};
    case RTE_ETH_SPEED_NUM_1G:    return {"1 Gbps", 1000};
    case RTE_ETH_SPEED_NUM_2_5G:  return {"2.5 Gbps", 2500};
    case RTE_ETH_SPEED_NUM_5G:    return {"5 Gbps", 5000};
    case RTE_ETH_SPEED_NUM_10G:   return {"10 Gbps", 10000};
    case RTE_ETH_SPEED_NUM_20G:   return {"20 Gbps", 20000};
    case RTE_ETH_SPEED_NUM_25G:   return {"25 Gbps", 25000};
    case RTE_ETH_SPEED_NUM_40G:   return {"40 Gbps", 40000};
    case RTE_ETH_SPEED_NUM_50G:   return {"50 Gbps", 50000};
    case RTE_ETH_SPEED_NUM_56G:   return {"56 Gbps", 56000};
    case RTE_ETH_SPEED_NUM_100G:  return {"100 Gbps", 100000};
    case RTE_ETH_SPEED_NUM_200G:  return {"200 Gbps", 200000};
    case RTE_ETH_SPEED_NUM_400G:  return {"400 Gbps", 400000};
    default: return {
      std::to_string(link.link_speed) + " Mbps ??", 
      link.link_speed
    };
  }
}

static std::vector<speed_t> get_link_speeds_supported(uint32_t port_id) {
  struct rte_eth_dev_info info;
  int resp = rte_eth_dev_info_get(port_id, &info);
  EXPECT(resp == 0, "rte_eth_dev_info_get failed");
  std::vector<speed_t> acc;
  uint32_t capa = info.speed_capa;
  #define CHECK(C, S, V) if (capa & C) { acc.push_back({S, V}); }
  CHECK(RTE_ETH_LINK_SPEED_AUTONEG, "Auto", 0);
  CHECK(RTE_ETH_LINK_SPEED_FIXED, "Fixed", 0);
  CHECK(RTE_ETH_LINK_SPEED_10M_HD, "10 Mbps (HD)", 10);
  CHECK(RTE_ETH_LINK_SPEED_10M, "10 Mbps", 10);
  CHECK(RTE_ETH_LINK_SPEED_100M_HD, "100 Mbps (HD)", 100);
  CHECK(RTE_ETH_LINK_SPEED_100M, "100 Mbps", 100);
  CHECK(RTE_ETH_LINK_SPEED_1G, "1 Gbps", 1000);
  CHECK(RTE_ETH_LINK_SPEED_2_5G, "2.5 Gbps", 2500);
  CHECK(RTE_ETH_LINK_SPEED_5G, "5 Gbps", 5000);
  CHECK(RTE_ETH_LINK_SPEED_10G, "10 Gbps", 10000);
  CHECK(RTE_ETH_LINK_SPEED_20G, "20 Gbps", 20000);
  CHECK(RTE_ETH_LINK_SPEED_25G, "25 Gbps", 25000);
  CHECK(RTE_ETH_LINK_SPEED_40G, "40 Gbps", 40000);
  CHECK(RTE_ETH_LINK_SPEED_50G, "50 Gbps", 50000);
  CHECK(RTE_ETH_LINK_SPEED_56G, "56 Gbps", 56000);
  CHECK(RTE_ETH_LINK_SPEED_100G, "100 Gbps", 100000);
  CHECK(RTE_ETH_LINK_SPEED_200G, "200 Gbps", 200000);
  CHECK(RTE_ETH_LINK_SPEED_400G, "400 Gbps", 400000);
  #undef CHECK
  return acc;
}

static std::string get_link_duplex(uint32_t port_id) {
  struct rte_eth_link link;
  int resp = rte_eth_link_get(port_id, &link);
  EXPECT(resp == 0, "rte_eth_link_get failed");
  return link.link_duplex == RTE_ETH_LINK_HALF_DUPLEX ? "Half" : "Full";
}

static uint16_t get_mtu(uint32_t port_id) {
  // Get MTU
  uint16_t mtu;
  int resp = rte_eth_dev_get_mtu(port_id, &mtu);
  EXPECT(resp == 0, "rte_eth_dev_get_mtu failed");
  return mtu;
}

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Get number of available ports
  uint16_t nb_ports = rte_eth_dev_count_avail();
  EXPECT(nb_ports > 0, "rte_eth_dev_count returned 0 devices");
  printf("Number of ports available: %hu\n", nb_ports);

  // Iterate through each port
  uint32_t port_id = 0;
  RTE_ETH_FOREACH_DEV(port_id) {
    // Preamble
    printf("== Port %u ==\n", port_id);

    /*
     * Basic info
     */

    std::cout << "  Device: " << get_device_name(port_id) << std::endl;
    std::cout << "  Driver: " << get_driver_name(port_id) << std::endl;
    std::cout << "  MAC Address: " << get_mac_address(port_id) << std::endl;
    std::cout << "  Rx Queues: " << get_rx_queues(port_id) << std::endl;
    std::cout << "  Tx Queues: " << get_tx_queues(port_id) << std::endl;
    std::cout << "  MTU: " << get_mtu(port_id) << std::endl;

    /*
     * Link info
     */

    std::cout << "  Link:" << std::endl;

    // Supported speeds
    std::ostringstream oss;
    auto speeds = get_link_speeds_supported(port_id);
    for (int i = 0; i < speeds.size(); i++) {
      oss << speeds[i].first;
      if (i < speeds.size() - 1) {
        oss << ", ";
      }
    }
    std::cout << "    Supported Speeds: " << oss.str() << std::endl;

    // Status
    bool status = get_is_link_up(port_id);
    std::cout << "    Status: " << (status ? "Up" : "Down") << std::endl;

    // Negotiated speed
    if (status) {
      std::string speed = get_link_speed_negotiated(port_id).first;
      std::cout << "    Negotiated Speed: " << speed << std::endl;
    }

    // Mode
    std::cout << "    Duplex: " << get_link_duplex(port_id) << std::endl;

    // Line separator
    std::cout << std::endl;
  }

  // Cleanup
  rte_eal_cleanup();
  return 0;
}
