// client.cpp

#include <iostream>
#include <cstdint>
#include <infiniband/verbs.h>

int main(int argc, const char **argv) {
  int num_devices = 0;
  struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
  if (dev_list == nullptr) {
    perror("Failed to get IB devices list!");
    return -1;
  }
  for (int i = 0; i < num_devices; i++) {
    const char *name = ibv_get_device_name(dev_list[i]);
    std::uint64_t guid = be64toh(ibv_get_device_guid(dev_list[i]));
    std::cout << "Device name: " << name << std::endl;
    std::cout << "GUID: " << guid << std::endl;
  }
  ibv_free_device_list(dev_list);
  printf("Hello client!\n");
  return 0;
}
