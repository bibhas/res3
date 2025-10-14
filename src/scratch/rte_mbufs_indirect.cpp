// rte_mbufs_indirect.cpp

#include <iostream>
#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#define DIRECT_BEFORE_INDIRECT 0
#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  // Create direct mbuf pool 
  struct rte_mempool *mp = rte_pktmbuf_pool_create("direct pool", 8192, 256, 0, 1024, rte_socket_id());
  EXPECT(mp != nullptr, "rte_pktmbuf_pool_create failed");

  // Create indirect mbuf pool
  struct rte_mempool *mp_indirect = rte_pktmbuf_pool_create("indirect pool", 4096, 256, 0, 0, rte_socket_id());   
  EXPECT(mp_indirect != nullptr, "rte_pktmbuf_pool_create failed");

  // Allocate and populate direct buffer
  struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
  EXPECT(m != nullptr, "rte_pktmbuf_alloc failed");
  char *data = rte_pktmbuf_append(m, 64);
  strcpy(data, "Original packet data");

  printf("Direct buffer created:\n");
  printf("  Data: \"%s\"\n", rte_pktmbuf_mtod(m, char *));
  printf("  Reference count: %u\n", rte_mbuf_refcnt_read(m));
    
  // Create first indirect buffer
  struct rte_mbuf *m_indirect0 = rte_pktmbuf_alloc(mp_indirect);
  EXPECT(m_indirect0 != nullptr, "rte_pktmbuf_alloc failed");
  rte_pktmbuf_attach(m_indirect0, m);
  
  printf("First indirect buffer attached:\n");
  printf("  Direct buffer refcount: %u\n", rte_mbuf_refcnt_read(m));
  printf("  Indirect data: %s\n", rte_pktmbuf_mtod(m_indirect0, char *));
  
  // Create second indirect buffer (clone)
  struct rte_mbuf *m_indirect1 = rte_pktmbuf_clone(m, mp_indirect);
  EXPECT(m_indirect1 != nullptr, "rte_pktmbuf_clone failed");
  
  printf("Second indirect buffer cloned:\n");
  printf("  Direct buffer refcount: %u\n", rte_mbuf_refcnt_read(m));
  printf("  Clone data: %s\n", rte_pktmbuf_mtod(m_indirect1, char *));

  // Modify offset in indirect buffer (doesn't affect others)
  rte_pktmbuf_adj(m_indirect0, 9);
  
  printf("After adjusting first indirect buffer (by 9 bytes):\n");
  printf("  Indirect 0 data: %s\n", rte_pktmbuf_mtod(m_indirect0, char *));
  printf("  Direct data: %s\n", rte_pktmbuf_mtod(m, char *));
  printf("  Indirect 1 data: %s\n", rte_pktmbuf_mtod(m_indirect1, char *));

  // Free buffers
#if DIRECT_BEFORE_INDIRECT
  // First free direct buffer + 1 indirect buffer
  rte_pktmbuf_free(m);
  rte_pktmbuf_free(m_indirect0);
  printf("After freeing direct buffer + indirect buffer:\n");
  // See if we can still access indirect buffer
  printf("  Clone data: %s\n", rte_pktmbuf_mtod(m_indirect1, char *));
  // Free indirect buffer
  rte_pktmbuf_free(m_indirect1);
#else
  // First both indirect buffers
  rte_pktmbuf_free(m_indirect0);
  rte_pktmbuf_free(m_indirect1);
  printf("After freeing both indirect buffers:\n");
  // Check the refcnt of direct buffer (should be 1)
  printf("  Direct buffer refcount: %u\n", rte_mbuf_refcnt_read(m));
  // Free direct buffer
  rte_pktmbuf_free(m);
#endif

  // Clean up
  rte_mempool_free(mp);
  rte_eal_cleanup();
    
  return 0;
}
