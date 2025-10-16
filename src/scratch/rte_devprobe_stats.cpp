// rte_devprobe_stats.cpp

#include <tuple>
#include <string>
#include <iostream>
#include <rte_ethdev.h>
#include <rte_common.h>
#include <rte_ether.h>

// Utils

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

using rx_stats_t = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>;

static rx_stats_t get_rx_stats(uint32_t port_id) {
  struct rte_eth_stats stats;
  int resp = rte_eth_stats_get(port_id, &stats);
  EXPECT(resp == 0, "rte_eth_stats_get failed");
  return {stats.ipackets, stats.ibytes, stats.ierrors, stats.imissed};
}

using tx_stats_t = std::tuple<uint64_t, uint64_t, uint64_t>;

static tx_stats_t get_tx_stats(uint32_t port_id) {
  struct rte_eth_stats stats;
  int resp = rte_eth_stats_get(port_id, &stats);
  EXPECT(resp == 0, "rte_eth_stats_get failed");
  return {stats.opackets, stats.obytes, stats.oerrors};
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

    // Rx stats
    auto [rx_packets, rx_bytes, rx_errors, rx_missed] = get_rx_stats(port_id);
    std::cout << "  Rx statistics:" << std::endl;
    std::cout << "    Rx Packets: " << rx_packets << std::endl;
    std::cout << "    Rx Bytes: " << rx_bytes << std::endl;
    std::cout << "    Rx Errors: " << rx_errors << std::endl;
    std::cout << "    Rx Missed: " << rx_missed << std::endl;

    // Tx stats
    auto [tx_packets, tx_bytes, tx_errors] = get_tx_stats(port_id);
    std::cout << "  Tx statistics:" << std::endl;
    std::cout << "    Tx Packets: " << tx_packets << std::endl;
    std::cout << "    Tx Bytes: " << tx_bytes << std::endl;
    std::cout << "    Tx Errors: " << tx_errors << std::endl;

    // Extended stats
    int nb_stats = rte_eth_xstats_get_names(port_id, nullptr, 0);
    EXPECT(nb_stats > 0, "rte_eth_xstats is zero");
    auto names = (struct rte_eth_xstat_name *)malloc(sizeof(struct rte_eth_xstat_name) * nb_stats);
    rte_eth_xstats_get_names(port_id, names, nb_stats);
    auto xstats = (struct rte_eth_xstat *)malloc(sizeof(struct rte_eth_xstat) * nb_stats);
    rte_eth_xstats_get(port_id, xstats, nb_stats);
    std::cout << "  Extended statistics:" << std::endl;
    for (int i = 0; i < nb_stats; i++) {
      std::cout << "    " << names[i].name << ": " << xstats[i].value << std::endl;
    }
    free(names);
    free(xstats);

    // Line separator
    std::cout << std::endl;
  }

  // Cleanup
  rte_eal_cleanup();
  return 0;
}
