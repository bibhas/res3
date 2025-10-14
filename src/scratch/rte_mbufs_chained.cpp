// rte_mbufs_chained.cpp

#include <iostream>
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_common.h>

#define EXPECT(COND, MSG) if (!(COND)) { rte_exit(EXIT_FAILURE, MSG); }

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");

  printf("Detected %u NUMA nodes\n", rte_socket_count());
  
  // Create mempool
  struct rte_mempool *mp = rte_pktmbuf_pool_create(
    "mbuf_pool", 8192, 256, 0, 512, rte_socket_id()
  );
  EXPECT(mp != nullptr, "rte_pktmbuf_pool_create failed");

  // Allocate first mbuf
  struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
  EXPECT(m != nullptr, "rte_pktmbuf_alloc failed");
  char *data = rte_pktmbuf_append(m, 10);
  EXPECT(data != nullptr, "rte_pktmbuf_append failed");
  memset(data, 'A', 10);
  data[9] = '\0';
  m->pkt_len = m->data_len;

  // Allocate second mbuf
  struct rte_mbuf *m_next = rte_pktmbuf_alloc(mp);
  EXPECT(m_next != nullptr, "rte_pktmbuf_alloc failed");
  data = rte_pktmbuf_append(m_next, 10);
  EXPECT(data != nullptr, "rte_pktmbuf_append failed");
  memset(data, 'B', 10);
  data[9] = '\0';
  m->next = m_next;
  m->nb_segs = 2;
  m->pkt_len += m_next->data_len;

  // Allocate third mbuf
  struct rte_mbuf *m_third = rte_pktmbuf_alloc(mp);
  EXPECT(m_third != nullptr, "rte_pktmbuf_alloc failed");
  data = rte_pktmbuf_append(m_third, 10);
  memset(data, 'C', 10);
  data[9] = '\0';
  m_next->next = m_third;
  m->nb_segs++;
  m->pkt_len += m_third->data_len;

  // Function to log mbuf
  auto log_mbuf = [](struct rte_mbuf *m) {
    printf(
      "-> [ Headroom: %u bytes][ Data: %u bytes ][ Tailroom: %u bytes ]\n", 
      rte_pktmbuf_headroom(m), 
      rte_pktmbuf_data_len(m), 
      rte_pktmbuf_tailroom(m)
    );
    printf("\n");
  };

  // Iterate through segments
  struct rte_mbuf *seg = m;
  int i = 1;
  while (seg) {
    printf(
      "Segment %d: data_len=%u, first_byte=%s\n",
      i++, seg->data_len, (rte_pktmbuf_mtod(seg, char *))
    );
    log_mbuf(seg);
    seg = seg->next;
  }

  // Clean up
  rte_pktmbuf_free(m);
  rte_mempool_free(mp);
  rte_eal_cleanup();

  return 0;
}
