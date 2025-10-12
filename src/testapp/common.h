// common.h

#pragma once

#include <iostream>

struct rdma_qp_metadata_t {
  uint32_t qpn = 0;
  uint32_t psn = 0;
  uint32_t lid = 0;
  uint64_t offset = 0;
  uint32_t rkey = 0;
};

void print_metadata(std::string prefix, rdma_qp_metadata_t& metadata) {
  std::cout << prefix << ": ";
  std::cout << " psn:" << metadata.psn;
  std::cout << " lid:" << metadata.lid;
  std::cout << " offset:" << metadata.offset;
  std::cout << " rkey:" << metadata.rkey;
  std::cout << std::endl;
}
