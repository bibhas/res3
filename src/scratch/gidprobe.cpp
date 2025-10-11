#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>
#include <infiniband/verbs.h>
#include <linux/in6.h>

static inline void process_port(struct ibv_context *ctx, int port, struct ibv_port_attr& port_attr) {
  for (int i = 0; i < port_attr.gid_tbl_len; i++) {
    struct ibv_gid_entry gid_entry;
    int resp = ibv_query_gid_ex(ctx, port, i, &gid_entry, 0);
    if (resp != 0) { continue; } // gid table can have empty items
    if (gid_entry.gid_type != IBV_GID_TYPE_ROCE_V2) { continue; }
    uint8_t *gid_bytes_net = gid_entry.gid.raw; // network order
    char gid_chars[INET6_ADDRSTRLEN]; // presentation order
    if (!inet_ntop(AF_INET6, gid_bytes_net, gid_chars, sizeof(gid_chars))) { 
      perror("inet_ntop"); 
      continue; 
    }
    printf("GID: %-40s\n", gid_chars);
    // v4mapped
    struct in6_addr *a = reinterpret_cast<struct in6_addr *>(gid_entry.gid.raw);
    uint32_t prefix = 0;
    prefix |= a->s6_addr32[0];
    prefix |= a->s6_addr32[1];
    prefix |= a->s6_addr32[2] ^ htonl(0x0000ffff);
    bool mapped = prefix == 0UL;
    if (mapped) {
      char ipv4_chars[INET_ADDRSTRLEN];
      void *out = (void *)&a->s6_addr32[3];
      if (inet_ntop(AF_INET, out, ipv4_chars, sizeof(ipv4_chars)) == nullptr) {
        perror("inet_ntop");
        continue;
      }
      printf(" IPv4: %s\n", ipv4_chars);
    }
  }
}

int main(int argc, const char **argv) {
  // Find number of devices
  int num_devices = 0;
  struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
  if (!dev_list) {
    perror("ib_get_device_list error");
    return EXIT_FAILURE;
  }
  printf("Found %d RDMA device%s\n", num_devices, (num_devices == 1 ? "" : "s"));
  // Log device properties
  for (int d = 0; d < num_devices; d++) {
    struct ibv_device *dev = dev_list[d];
    const char *devname = ibv_get_device_name(dev);
    if (!devname) {
      perror("ibv_get_device_name");
      continue;
    }
    printf("Device Name: %s\n", devname);
    struct ibv_context *ctx = ibv_open_device(dev);
    if (!ctx) {
      perror("ib_open_device");
      continue;
    }
    struct ibv_device_attr_ex dev_attr = {0};
    if (ibv_query_device_ex(ctx, NULL, &dev_attr)) {
      perror("ibv_query_device_ex");
      ibv_close_device(ctx);
      continue;
    }
    const int num_ports = dev_attr.orig_attr.phys_port_cnt;
    for (int port = 1; port <= num_ports; port++) {
      struct ibv_port_attr port_attr{};
      if (ibv_query_port(ctx, port, &port_attr)) {
        perror("ibv_query_port");
        continue;
      }
      process_port(ctx, port, port_attr);
    }
    ibv_close_device(ctx);
  }
  ibv_free_device_list(dev_list);
  return EXIT_SUCCESS;
}

