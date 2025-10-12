// common.h

#pragma once

#include <iostream>

static constexpr size_t kBufferSize = 4096;
static constexpr size_t kTCPPort = 9999;

#pragma mark -

struct rdma_t {
  struct ibv_context *context = nullptr;
  struct ibv_pd *pd = nullptr;
  struct ibv_mr *mr = nullptr;
  struct ibv_qp *qp = nullptr;
  struct ibv_cq *cq = nullptr;
  struct ibv_device **devices = nullptr;
  char *buffer = nullptr;
  union ibv_gid gid{0};
  uint8_t sgid_index = 0;
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

#pragma mark -

#define TERMINATE(msg) perror(msg); return EXIT_FALURE;
#define EXPECT(BVAL, ERRMSG) if (!(BVAL)) { perror(ERRMSG); return EXIT_FAILURE; }

#pragma mark -

struct rdma_qp_metadata_t {
  uint32_t qpn = 0;
  uint32_t psn = 0;
  uint64_t offset = 0;
  uint32_t rkey = 0;
  uint8_t sgid_index = 0;
  union ibv_gid gid{0};
};

void print_metadata(std::string prefix, rdma_qp_metadata_t& metadata) {
  std::cout << prefix << ": ";
  std::cout << " psn:" << metadata.psn;
  std::cout << " offset:" << metadata.offset;
  std::cout << " rkey:" << metadata.rkey;
  std::cout << " sgid_index:" << (uint32_t)metadata.sgid_index;
  char gid_chars[INET6_ADDRSTRLEN];
  if (!inet_ntop(AF_INET6, metadata.gid.raw, gid_chars, sizeof(gid_chars))) {
    perror("inet_ntop");
    return;
  }
  printf(" gid: %-40s", gid_chars);
  std::cout << std::endl;
}

