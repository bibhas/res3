// common.h

#pragma once

#include <iostream>

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
  std::cout << " sgid_index:" << metadata.sgid_index;
  char gid_chars[INET6_ADDRSTRLEN];
  if (!inet_ntop(AF_INET6, metadata.gid.raw, gid_chars, sizeof(gid_chars))) {
    perror("inet_ntop");
    return;
  }
  printf(" gid: %-40s", gid_chars);
  std::cout << std::endl;
}
