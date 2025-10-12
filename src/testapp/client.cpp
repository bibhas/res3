// client.cpp

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstdint>
#include <memory>
#include <infiniband/verbs.h>
#include "common.h"

static const char *kServerIP = "192.168.4.2";

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
   * Transition QP state machine from RESET to RTS
   */

  // Ensure port exists
  struct ibv_port_attr port_attr;
  rdma->port_number = 1; // ports are 1-indexed
  int resp = ibv_query_port(rdma->context, rdma->port_number, &port_attr);
  EXPECT(resp == 0, "ibv_query_port failed");

  // Select gid (TODO: write better gid selection logic)
  rdma->sgid_index = 1;
  resp = ibv_query_gid(rdma->context, rdma->port_number, rdma->sgid_index, &rdma->gid);
  EXPECT(resp == 0, "ibv_query_gid failed");

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

  // Connect to tcp server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT(sockfd >= 0, "socket failed");
  struct sockaddr_in server_addr{0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(kTCPPort);
  resp = inet_pton(AF_INET, kServerIP, &server_addr.sin_addr);
  EXPECT(resp == 1, "inet_pton failed");
  resp = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  EXPECT(resp == 0, "connect failed");

  // We have a server connection. Exchange QP metadata.
  struct rdma_qp_metadata_t local_metadata = {
    .qpn = rdma->qp->qp_num,
    .psn = 0,
    .offset = (uintptr_t)rdma->buffer,
    .rkey = rdma->mr->rkey,
    .sgid_index = rdma->sgid_index,
    .gid = rdma->gid
  };
  struct rdma_qp_metadata_t remote_metadata{0};
  ssize_t recvd = read(sockfd, (void *)&remote_metadata, sizeof(remote_metadata));
  EXPECT(recvd == sizeof(remote_metadata), "read failed");
  ssize_t sent = write(sockfd, (void *)&local_metadata, sizeof(local_metadata));
  EXPECT(sent == sizeof(local_metadata), "write failed");

  print_metadata("Client", local_metadata);
  print_metadata("Server", remote_metadata);

  // Transition QP from INIT to RTR state
  struct ibv_global_route grh = {
    .dgid = remote_metadata.gid,
    .sgid_index = remote_metadata.sgid_index,
    .hop_limit = 64,
    .traffic_class = 0
  };
  struct ibv_ah_attr rtr_ah_attr = {
    .grh = grh,
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

  /*
   * Perform RDMA operations
   */ 

  // Send RDMA_READ first
  struct ibv_sge sge = {
    .addr = (uintptr_t) rdma->buffer,
    .length = kBufferSize,
    .lkey = rdma->mr->lkey
  };
  struct ibv_send_wr wr = {
    .wr_id = 1,
    .sg_list = &sge,
    .num_sge = 1,
    .opcode = IBV_WR_RDMA_READ,
    .send_flags = IBV_SEND_SIGNALED,
  };
  wr.wr.rdma.remote_addr = remote_metadata.offset;
  wr.wr.rdma.rkey = remote_metadata.rkey;
  struct ibv_send_wr *bad_wr = nullptr;
  resp = ibv_post_send(rdma->qp, &wr, &bad_wr);
  EXPECT(resp == 0, "ibv_post_send READ failed");

  // Wait for RDMA_READ completion
  struct ibv_wc wc;
  int completions = -1;
  do {
    int _num_entries = 1;
    completions = ibv_poll_cq(rdma->cq, _num_entries, &wc);
  }
  while (completions == 0);
  EXPECT(completions > 0 && wc.status == IBV_WC_SUCCESS, "ibv_poll_cq READ failed");
  std::cout << "Client RDMA_READ resp: " << (const char *)rdma->buffer << std::endl;
  
  // Send a reply to the server using RDMA_WRITE
  strcpy(rdma->buffer, "Namaste from client RDMA!");
  wr.opcode = IBV_WR_RDMA_WRITE;
  resp = ibv_post_send(rdma->qp, &wr, &bad_wr);
  EXPECT(resp == 0, "ibv_post_send WRITE failed");

  // Wait for RDMA_WRITE completion
  do {
    int _num_entries = 1;
    completions = ibv_poll_cq(rdma->cq, _num_entries, &wc);
  }
  while (completions == 0);
  EXPECT(completions > 0 && wc.status == IBV_WC_SUCCESS, "ibv_poll_cq WRITE failed");
  std::cout << "Client RDMA_WRITE completed." << std::endl;

  close(sockfd);
  return 0;
}
