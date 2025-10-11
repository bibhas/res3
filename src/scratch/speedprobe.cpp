#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>
#include <infiniband/verbs.h>

static inline void process_port(struct ibv_context *ctx, int port, struct ibv_port_attr& port_attr) {
  static std::unordered_map<int, float> speedlut = {
    {0, 0},           // unused
    {1, 2.5},         // SDR
    {2, 5.0},         // DDR
    {4, 10.0},        // QDR
    {8, 10.3125},     // FDR10
    {16, 14.0625},    // FDR
    {32, 25.78125},   // EDR
    {64, 50.0},       // HDR
    {128, 100.0},     // NDR
    {256, 250.0}      // XDR
  };
  static std::unordered_map<int, int> widthlut = {
    {0, 0},   // unused
    {1, 1},   // 1x
    {2, 4},   // 4x
    {3, 0},   // unused
    {4, 8},   // 8x
    {5, 12},  // 12x
  };
  uint8_t speed = port_attr.active_speed;
  uint8_t width = port_attr.active_width;
  if (port_attr.state == IBV_PORT_DOWN) {
    return;
  }
  if (speedlut.contains(speed) && widthlut.contains(width)) {
    double lanespeed = speedlut[speed];
    double lanes = widthlut[width];
    printf("Port speed: %0.2f Gbps, state: %d\n", lanespeed * lanes, port_attr.state);
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

