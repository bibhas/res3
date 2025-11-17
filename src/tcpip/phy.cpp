// phy.cpp

#include <atomic>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include <CommandParser/libcli.h>
#include "layer2/layer2.h"
#include "phy.h"
#include "config.h"
#include "pcap.h"
#include "graph.h"

#pragma mark -

// Private static variables

static uint8_t __recv_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static uint8_t __temp_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static uint8_t __send_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static std::atomic<bool> __receiver_thread_ready(false);

#pragma mark -

// Private utility functions

bool phy_frame_buffer_shift_right(uint8_t **frameptr, uint32_t framelen, uint32_t buflen) {
  EXPECT_RETURN_BOOL(frameptr != nullptr, "Empty frame out param", false);
  EXPECT_RETURN_BOOL(framelen <= buflen, "Params framelen greater than buflen", false);
  EXPECT_RETURN_BOOL(framelen != 0, "Zero framelen param", false);
  uint32_t offset = buflen - framelen; // Make sure buffer is at least 2 x MTU
  if (likely(framelen * 2 < buflen)) {
    memcpy(*frameptr + offset, *frameptr, framelen);
    memset(*frameptr, 0, offset);
  }
  else {
    // Expensive
    memcpy(__temp_buffer, *frameptr, framelen);
    memset(*frameptr, 0, offset);
    memcpy(*frameptr + offset, __temp_buffer, framelen);
  }
  *frameptr += offset;
  return true; 
}

int phy_node_receive_interface_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node ptr param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface ptr param", false);
  EXPECT_RETURN_BOOL(frame != nullptr, "Empty packet ptr param", false);
  // TODO: Why exactly are we shifting the data since we distinguish between
  // send and receive buffers, and as such, they are two different allocations?
  bool resp = phy_frame_buffer_shift_right(&frame, framelen, CONFIG_MAX_PACKET_BUFFER_SIZE - CONFIG_IF_NAME_SIZE);
  EXPECT_RETURN_VAL(resp == true, "phy_frame_buffer_shift_right failed", -1);
  return layer2_node_recv_frame_bytes(n, intf, frame, framelen); // Entry point into Layer 2
}

#pragma mark -

// Public functions

void phy_receiver_thread_main(graph_t *topo) {
  // First, gather all node socket descriptors
  fd_set fds; FD_ZERO(&fds);
  int max_fd = 0;
  glthread_t *curr;
  GLTHREAD_FOREACH_BEGIN(&topo->node_list, curr) {
    node_t *n = node_ptr_from_graph_glue(curr);
    if (!n->udp.fd) { continue; }
    max_fd = std::max(max_fd, n->udp.fd);
    FD_SET(n->udp.fd, &fds);
  }
  GLTHREAD_FOREACH_END();
  __receiver_thread_ready.store(true);
  // Poll for ready to read fds in a busy loop
  while (true) {
    fd_set ready_fds; FD_ZERO(&ready_fds);
    memcpy(&ready_fds, &fds, sizeof(fd_set));
    int resp = select(max_fd + 1, &ready_fds, nullptr, nullptr, nullptr);
    EXPECT_FATAL(resp >= 0, "selct failed");
    if (resp == 0) { continue; } // select timed-out
    command_parser_lock();
    // Process ready fds
    GLTHREAD_FOREACH_BEGIN(&topo->node_list, curr) {
      node_t *n = node_ptr_from_graph_glue(curr);
      if (FD_ISSET(n->udp.fd, &ready_fds)) {
        socklen_t addrlen = sizeof(struct sockaddr);
        memset(__recv_buffer, 0, CONFIG_MAX_PACKET_BUFFER_SIZE);
        struct sockaddr_in sender;
        size_t bytes = recvfrom(n->udp.fd, (char *)__recv_buffer, CONFIG_MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *)&sender, &addrlen);
        printf("[%s] Read %lu bytes\n", n->node_name, bytes);
        EXPECT_FATAL(bytes >= 0, "recvfrom failed");
        // Do something with received data
        char *target_intf_name = (char *)__recv_buffer; // of size IF_NAME_SIZE
        interface_t *target_intf = node_get_interface_by_name(n, target_intf_name);
        EXPECT_CONTINUE(target_intf != nullptr, "Packet received on unknown interface");
        resp = phy_node_receive_interface_frame_bytes(n, target_intf, __recv_buffer + CONFIG_IF_NAME_SIZE, bytes - CONFIG_IF_NAME_SIZE);
#pragma unused(resp); // TODO: Fixme
      }
    }
    GLTHREAD_FOREACH_END();
    command_parser_unlock();
  }
}

bool phy_receiver_thread_ready() {
  return __receiver_thread_ready.load();
}

bool phy_setup_udp_socket(uint32_t *port, int *fd) {
  EXPECT_RETURN_BOOL(port != nullptr, "Empty port ptr param", false);
  EXPECT_RETURN_BOOL(fd != nullptr, "Empty socket fd ptr param", false);
  // Create a socket
  int resp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  EXPECT_RETURN_BOOL(resp != -1, "socket failed", false);
  *fd = resp;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(*port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  // Bind address to socket
  resp = bind(*fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  if (resp < 0) {
    if (errno == EADDRINUSE) {
      // Try again  
      addr.sin_port = 0; // port is auto assigned
      // Bind address to socket
      resp = bind(*fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
      EXPECT_RETURN_BOOL(resp == 0, "bind (retry) failed", false);
    }
    else {
      printf("error: %s\n", strerror(errno));
      ERR_RETURN_BOOL("bind failed", false);
    }
  }
  // Find assigned port
  socklen_t addrlen = sizeof(addr);
  getsockname(*fd, (struct sockaddr *) &addr, &addrlen);
  EXPECT_RETURN_BOOL(addrlen == sizeof(addr), "getsockname (addrlen) failed", false);
  *port = ntohs(addr.sin_port);
  return true;
}

int __phy_node_send_frame_bytes(node_t *n, interface_t *intf, uint8_t *frame, uint32_t framelen) {
  EXPECT_RETURN_BOOL(frame != nullptr, "Empty packet ptr param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface ptr param", false);
  // Create socket
  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  EXPECT_RETURN_VAL(fd >= 0, "socket failed", -1);
  // Begin preparing data payload (including aux info)
  interface_t *intf2 = nullptr;
  bool found = link_get_other_interface(intf->link, intf, &intf2);
  EXPECT_RETURN_VAL(found == true, "link_get_other_interface failed", -1);
  node_t *nbr = intf2->att_node;
  memset(__send_buffer, 0, CONFIG_MAX_PACKET_BUFFER_SIZE);
  // Append null terminated dest interface name
  strncpy((char *)__send_buffer, intf2->if_name, CONFIG_IF_NAME_SIZE);
  __send_buffer[CONFIG_IF_NAME_SIZE] = '\0';
  uint32_t auxlen = CONFIG_IF_NAME_SIZE;
  // Append rest of the data
  memcpy((void *)(__send_buffer + CONFIG_IF_NAME_SIZE), (void *)frame, framelen);
  // Finally, send packet
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(nbr->udp.port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int resp = sendto(fd, __send_buffer, framelen + auxlen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  EXPECT_RETURN_VAL(resp >= 0, "sendto failed", -1);
  printf("[%s] Sent %u bytes via %s\n", n->node_name, framelen + auxlen, intf->if_name);
  pcap_pkt_dump(frame, framelen);
  // Cleanup
  close(fd);
  return resp - auxlen ; // Number of bytes sent ()
}


