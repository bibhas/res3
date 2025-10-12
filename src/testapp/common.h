// common.h

#pragma once

struct rdma_qp_metadata_t {
  uint32_t qpn = 0;
  uint32_t psn = 0;
  uint32_t lid = 0;
  uint64_t offset = 0;
  uint32_t rkey = 0;
};
