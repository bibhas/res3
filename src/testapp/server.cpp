// server.cpp

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstdint>
#include <memory>
#include <infiniband/verbs.h>
#include "common.h"

static constexpr size_t kBufferSize = 4096;
static constexpr size_t kTCPPort = 9999;

struct rdma_t {
  struct ibv_context *context = nullptr;
  struct ibv_pd *pd = nullptr;
  struct ibv_mr *mr = nullptr;
  struct ibv_qp *qp = nullptr;
  struct ibv_cq *cq = nullptr;
  struct ibv_device **devices = nullptr;
  char *buffer = nullptr;
  uint8_t port_number = std::numeric_limits<std::uint8_t>::max();
  // Destructor
  virtual ~rdma_t();
};

rdma_t::~rdma_t() {
  std::cout << "~rdma_t" << std::endl;
  if (mr) ibv_dereg_mr(mr);
  if (qp) ibv_destroy_qp(qp);
  if (cq) ibv_destroy_cq(cq);
  if (pd) ibv_dealloc_pd(pd);
  if (context) ibv_close_device(context);
  if (devices) ibv_free_device_list(devices);
  if (buffer) free(buffer);
}

#define TERMINATE(msg) perror(msg); return EXIT_FALURE;
#define EXPECT(BVAL, ERRMSG) if (!(BVAL)) { perror(ERRMSG); return EXIT_FAILURE; }

int main(int argc, const char **argv) {
  // Setup rdma_t
  auto rdma = std::make_unique<rdma_t>();

  /*
   * Setup data structures
   */

  // Allocate buffer
  void *ptr = malloc(kBufferSize);
  EXPECT(ptr != nullptr, "malloc error");
  rdma->buffer = reinterpret_cast<char *>(ptr); 

  // Retrieve device
  rdma->devices = ibv_get_device_list(NULL);
  EXPECT(rdma->devices != nullptr, "ibv_get_device_list failed");
  rdma->context = ibv_open_device(rdma->devices[0]); // Select first device
  EXPECT(rdma->context != nullptr, "ibv_open_devices failed");

  // Allocate protection domain
  rdma->pd = ibv_alloc_pd(rdma->context);
  EXPECT(rdma->pd != nullptr, "ibv_alloc_pd failed!");

  // Register memory region
  auto mrflags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
  rdma->mr = ibv_reg_mr(rdma->pd, rdma->buffer, kBufferSize, mrflags);
  EXPECT(rdma->mr != nullptr, "ibv_reg_mr failed");

  // Create completion queue
  int _cqe = 10;
  void *_cq_context = nullptr;
  struct ibv_comp_channel *_channel = nullptr;
  int _comp_vector = 0;
  rdma->cq = ibv_create_cq(rdma->context, _cqe, _cq_context, _channel, _comp_vector);
  EXPECT(rdma->cq != nullptr, "ibv_create_cq failed");

  // Create qp (transport = RC)
  struct ibv_qp_cap _qp_cap = {
    .max_send_wr = 10,
    .max_recv_wr = 10,
    .max_send_sge = 1,
    .max_recv_sge = 1
  };
  struct ibv_qp_init_attr _qp_init_attr = {
    .send_cq = rdma->cq,
    .recv_cq = rdma->cq,
    .cap = _qp_cap,
    .qp_type = IBV_QPT_RC
  };
  rdma->qp = ibv_create_qp(rdma->pd, &_qp_init_attr) ;
  EXPECT(rdma->qp != nullptr, "ibv_create_qp failed");
  
  /*
   * Transition QP state machine to RTS
   */

  // Ensure port exists
  struct ibv_port_attr port_attr;
  rdma->port_number = 1; // ports are 1-indexed
  int resp = ibv_query_port(rdma->context, rdma->port_number, &port_attr);
  EXPECT(resp == 0, "ibv_query_port failed");

  // Transition QP from RESET to INIT state
  struct ibv_qp_attr init_attr = {
    .qp_state = IBV_QPS_INIT,
    .qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ,
    .pkey_index = 0,
    .port_num = rdma->port_number
  };
  unsigned int init_attr_changes = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
  resp = ibv_modify_qp(rdma->qp, &init_attr, init_attr_changes);
  EXPECT(resp == 0, "ibv_modify_qp to INIT failed");

  // Setup tcp server
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT(listenfd >= 0, "socket failed");
  struct sockaddr_in addr{0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(kTCPPort);
  resp = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
  EXPECT(resp >= 0, "bind failed");
  resp = listen(listenfd, 1);
  EXPECT(resp >= 0, "listen failed");

  // Wait for tcp client connection
  printf("Waiting for client...\n");
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
  EXPECT(connfd >= 0, "accept failed");

  // We have a client connection. Exchange QP metadata.
  struct rdma_qp_metadata_t local_metadata = {
    .qpn = rdma->qp->qp_num,
    .psn = 0,
    .lid = port_attr.lid,
    .offset = (uintptr_t)rdma->buffer,
    .rkey = rdma->mr->rkey
  };
  struct rdma_qp_metadata_t remote_metadata{0};
  ssize_t sent = write(connfd, (void *)&local_metadata, sizeof(local_metadata));
  EXPECT(sent == sizeof(local_metadata), "write failed");
  ssize_t recvd = read(connfd, (void *)&remote_metadata, sizeof(remote_metadata));
  EXPECT(recvd == sizeof(remote_metadata), "read failed");

  print_metadata("Client", local_metadata);
  print_metadata("Server", remote_metadata);

  // Transition QP from INIT to RTR state
  struct ibv_ah_attr rtr_ah_attr = {
    .dlid = (uint16_t)remote_metadata.lid,
    .sl = 0,
    .src_path_bits = 0,
    .is_global = 1,
    .port_num = rdma->port_number
  };
  struct ibv_qp_attr rtr_attr = {
    .qp_state = IBV_QPS_RTR,
    .path_mtu = IBV_MTU_1024,
    .rq_psn = remote_metadata.psn,
    .dest_qp_num = remote_metadata.qpn,
    .ah_attr = rtr_ah_attr,
    .max_dest_rd_atomic = 1,
    .min_rnr_timer = 12
  };
  unsigned int rtr_attr_changes = IBV_QP_STATE;
  rtr_attr_changes |= IBV_QP_PATH_MTU;
  rtr_attr_changes |= IBV_QP_AV;
  rtr_attr_changes |= IBV_QP_DEST_QPN;
  rtr_attr_changes |= IBV_QP_RQ_PSN;
  rtr_attr_changes |= IBV_QP_MAX_DEST_RD_ATOMIC;
  rtr_attr_changes |= IBV_QP_MIN_RNR_TIMER;
  resp = ibv_modify_qp(rdma->qp, &rtr_attr, rtr_attr_changes);
  EXPECT(resp == 0, "ibv_modify_qp RTR");
  
  // Transition QP from RTR to RTS state
  struct ibv_qp_attr rts_attr = {
    .qp_state = IBV_QPS_RTS,
    .sq_psn = local_metadata.psn,
    .max_rd_atomic = 1,
    .timeout = 14,
    .retry_cnt = 7,
    .rnr_retry = 7
  };
  unsigned int rts_attr_changes = IBV_QP_STATE;
  rts_attr_changes |= IBV_QP_SQ_PSN;
  rts_attr_changes |= IBV_QP_TIMEOUT;
  rts_attr_changes |= IBV_QP_RETRY_CNT;
  rts_attr_changes |= IBV_QP_RNR_RETRY;
  rts_attr_changes |= IBV_QP_MAX_QP_RD_ATOMIC;
  resp = ibv_modify_qp(rdma->qp, &rts_attr, rts_attr_changes);
  EXPECT(resp == 0, "ibv_modify_qp RTS");

  printf("QP in RTS. All systems are go. T- 10s and counting...\n");

  sleep(10);
  close(connfd);
  close(listenfd);

  return 0;
}
