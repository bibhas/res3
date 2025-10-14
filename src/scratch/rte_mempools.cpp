// rte_mbuf.cpp

#include <iostream>
#include <rte_mbuf.h>
#include <rte_eal.h>
#include <rte_mempool.h>

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Create mbuf pool
  uint16_t _psize = 128+8;
  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
    // name of mbuf pool
    "mbuf_pool", 
    // Number of elements in mbuf pool. 
    // The optimum size is (some powers of 2) - 1
    8192, 
    // Number of elements in per-core object cache. 
    // Must be <= RTE_MEMPOOL_CACHE_MAX_SIZE.
    // Caching can be disabled by setting this value to 0.
    250,  
    // Size (in bytes) of private area reserved between mbufs.
    // This parameter must be aligned to `RTE_MBUF_PRIV_ALIGN`. 
    // In effect, however, the private area seems to occupy 
    // RTE_CACHE_LINE_SIZE aligned sizes.
    _psize,    
    // Size (in bytes) of data in each mbuf. Includes reserved headroom.
    2048, 
    // ID of socket to which the pool should have affinity. 
    // Use SOCKET_ID_ANY as a wildcard.
    rte_socket_id()
  );
  std::cout << "RTE_MBUF_PRIV_ALIGN: " << RTE_MBUF_PRIV_ALIGN << std::endl;
  EXPECT(mbuf_pool != nullptr, "rte_pktmbuf_pool_create failed");

  // Allocate two mbufs from pool
  struct rte_mbuf *m0= rte_pktmbuf_alloc(mbuf_pool);
  EXPECT(m0!= nullptr, "rte_pktmbuf_alloc failed");
  struct rte_mbuf *m1= rte_pktmbuf_alloc(mbuf_pool);
  EXPECT(m1!= nullptr, "rte_pktmbuf_alloc failed");

  // Take note of the difference in ptrs
  size_t diff = (uintptr_t)m0 - (uintptr_t)m1;
  printf("m0:%p, m1:%p, diff:%lu, priv:%hu\n", m0, m1, diff, _psize);
  printf("Note: effective priv_size is `diff - ~2240` = %lu\n", diff - 2240);

  // Return mbuf to the pool
  rte_pktmbuf_free(m0);
  rte_pktmbuf_free(m1);

  // Cleanup
  rte_mempool_free(mbuf_pool);
  rte_eal_cleanup();

  return 0;
}
