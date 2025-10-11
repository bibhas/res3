#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>

static inline void process_port(struct ibv_context *ctx, int port, struct ibv_port_attr& port_attr) {
  printf(" port: %d", port);
  // Link Layer info
  switch (port_attr.link_layer) {
    case IBV_LINK_LAYER_ETHERNET: {
      printf(" Ethernet");
      break;
    }
    case IBV_LINK_LAYER_INFINIBAND: {
      printf(" InfiniBand");
      break;
    }
    default: {
      printf(" Unknown");
      break;
    }
  }
  printf(" speed=%u", port_attr.active_speed);
  printf(" width=%u", port_attr.active_width);
  printf(" MTU=%u", port_attr.active_mtu);
  printf(" gids=%i", (int)port_attr.gid_tbl_len);
  printf(" state=%s\n", ibv_port_state_str(port_attr.state));
  printf("\n");
  // GID (IP address + type)
  for (int i = 0; i < port_attr.gid_tbl_len; i++) {
    struct ibv_gid_entry gid_entry = {0};
    if (ibv_query_gid_ex(ctx, port, i, &gid_entry, 0)) {
      continue;
    }
    char gid_str[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, gid_entry.gid.raw, gid_str, sizeof(gid_str))) {
      continue;
    }
    const char *type_str = 
      (gid_entry.gid_type == IBV_GID_TYPE_ROCE_V2) ? "RoCEv2" : 
      (gid_entry.gid_type == IBV_GID_TYPE_ROCE_V1) ? "RoCEv1" :
      "Other";
    printf("   GID[%02d]: %-40s (%s)\n", i, gid_str, type_str);
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
      struct ibv_port_attr port_attr;
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
