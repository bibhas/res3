#include <iostream>
#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

int main(int argc, char **argv) {
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Create rte_mempool
  struct rte_mempool *mp = rte_pktmbuf_pool_create(
    "mbuf_pool", 8192, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
  );
  EXPECT(mp != nullptr, "rte_pktmbuf_pool_create failed");

  // Allocate mbuf
  struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
  EXPECT(m != nullptr, "rte_pktmbuf_alloc failed");

  // Log mbuf memory info
  auto log_mbuf = [](struct rte_mbuf *m, const char *prefix) {
    printf("%s:\n", prefix);
    printf(
      "\t[ Headroom: %u bytes][ Data: %u bytes ][ Tailroom: %u bytes ]\n", 
      rte_pktmbuf_headroom(m), 
      rte_pktmbuf_data_len(m), 
      rte_pktmbuf_tailroom(m)
    );
    printf("\n");
  };
  log_mbuf(m, "Initial state");

  // Append data to mbuf
  const char *message = "Hello DPDK!";
  char *data = rte_pktmbuf_append(m, strlen(message) + 1);
  EXPECT(data != nullptr, "rte_pktmbuf_append failed");
  strcpy(data, message);
  log_mbuf(m, "After appending message");

  char *header = rte_pktmbuf_prepend(m, 14);
  EXPECT(header != nullptr, "rte_pktmbuf_prepend failed");
  log_mbuf(m, "After prepending 14 bytes");

  resp = rte_pktmbuf_trim(m, 5);
  EXPECT(resp >= 0, "rte_pktmbuf_trim failed");
  log_mbuf(m, "After trimming 5 bytes");

  char *new_data = rte_pktmbuf_adj(m, 14);
  EXPECT(new_data != nullptr, "rte_pktmbuf_adj failed");
  log_mbuf(m, "After removing 14-byte header");

  // Clean up
  rte_pktmbuf_free(m);
  rte_mempool_free(mp);
  rte_eal_cleanup();

  return 0;
}
