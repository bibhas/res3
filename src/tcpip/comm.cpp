// comm.cpp

#include <atomic>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include "layer2/layer2.h"
#include "comm.h"
#include "config.h"

static uint8_t __recv_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static uint8_t __temp_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static uint8_t __send_buffer[CONFIG_MAX_PACKET_BUFFER_SIZE];
static std::atomic<bool> __receiver_thread_ready(false);

int comm_pkt_send_bytes(uint8_t *pkt, uint32_t pktlen, interface_t *intf) {
  EXPECT_RETURN_BOOL(pkt != nullptr, "Empty packet ptr param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface ptr param", false);
  // Gather nodes
  node_t *n = intf->att_node;
  node_t *nbr = interface_get_neighbor_node(intf);
  EXPECT_RETURN_VAL(nbr != nullptr, "interface_get_neighbor_node failed", -1);
  // Create socket
  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  EXPECT_RETURN_VAL(fd >= 0, "socket failed", -1);
  // Begin preparing data payload (including aux info)
  interface_t *intf2 = nullptr;
  bool found = link_get_other_interface(intf->link, intf, &intf2);
  EXPECT_RETURN_VAL(found == true, "link_get_other_interface failed", -1);
  memset(__send_buffer, 0, CONFIG_MAX_PACKET_BUFFER_SIZE);
  // Append null terminated dest interface name
  strncpy((char *)__send_buffer, intf2->if_name, CONFIG_IF_NAME_SIZE);
  __send_buffer[CONFIG_IF_NAME_SIZE] = '\0';
  // Append rest of the data
  memcpy((void *)(__send_buffer + CONFIG_IF_NAME_SIZE), (void *)pkt, pktlen);
  // Finally, send packet
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(nbr->udp.port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int resp = sendto(fd, __send_buffer, pktlen + CONFIG_IF_NAME_SIZE, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  EXPECT_RETURN_VAL(resp >= 0, "sendto failed", -1);
  // Cleanup
  close(fd);
  return resp; // Number of bytes sent
}

int comm_pkt_receive_bytes(node_t *n, interface_t *intf, uint8_t *pkt, uint32_t pktlen) {
  EXPECT_RETURN_BOOL(n != nullptr, "Empty node ptr param", false);
  EXPECT_RETURN_BOOL(intf != nullptr, "Empty interface ptr param", false);
  EXPECT_RETURN_BOOL(pkt != nullptr, "Empty packet ptr param", false);
  // TODO: Why exactly are we shifting the data since we distinguish between
  // send and receive buffers, and as such, they are two different allocations?
  bool resp = comm_pkt_buffer_shift_right(&pkt, pktlen, CONFIG_MAX_PACKET_BUFFER_SIZE - CONFIG_IF_NAME_SIZE);
  EXPECT_RETURN_VAL(resp == true, "comm_pkt_buffer_shift_right failed", -1);
  return layer2_frame_recv(n, intf, pkt, pktlen);
}

int comm_pkt_send_flood_bytes(node_t *n, interface_t *ign_intf, uint8_t *pkt, uint32_t pktlen) {
  EXPECT_RETURN_VAL(n != nullptr, "Empty node ptr param", -1);
  EXPECT_RETURN_VAL(pkt != nullptr, "Empty packet ptr param", -1);
  int acc = 0;
  for (int i = 0; i < CONFIG_MAX_INTF_PER_NODE; i++) {
    if (!n->intf[i]) { continue; }
    interface_t *candidate = n->intf[i];
    if (candidate == ign_intf) { continue; } // ignored interface
    int resp = comm_pkt_send_bytes(pkt, pktlen, candidate); 
    EXPECT_CONTINUE(resp == pktlen, "comp_pkt_send_bytes failed");
    acc += resp;
  }
  return acc; // Number of bytes sent
}

bool comm_udp_socket_setup(uint32_t *port, int *fd) {
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

void comm_pkt_receiver_thread_main(graph_t *topo) {
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
    // Process ready fds
    GLTHREAD_FOREACH_BEGIN(&topo->node_list, curr) {
      node_t *n = node_ptr_from_graph_glue(curr);
      if (FD_ISSET(n->udp.fd, &ready_fds)) {
        socklen_t addrlen = sizeof(struct sockaddr);
        memset(__recv_buffer, 0, CONFIG_MAX_PACKET_BUFFER_SIZE);
        struct sockaddr_in sender;
        size_t bytes = recvfrom(n->udp.fd, (char *)__recv_buffer, CONFIG_MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *)&sender, &addrlen);
        printf("read %lu bytes\n", bytes);
        EXPECT_FATAL(bytes >= 0, "recvfrom failed");
        // Do something with received data
        char *target_intf_name = (char *)__recv_buffer; // of size IF_NAME_SIZE
        interface_t *target_intf = node_get_interface_by_name(n, target_intf_name);
        EXPECT_CONTINUE(target_intf != nullptr, "Packet received on unknown interface");
        resp = comm_pkt_receive_bytes(n, target_intf, __recv_buffer + CONFIG_IF_NAME_SIZE, bytes - CONFIG_IF_NAME_SIZE);
        // TODO: What to do with resp??
      }
    }
    GLTHREAD_FOREACH_END();
  }
}

bool comm_pkt_receiver_thread_ready() {
  return __receiver_thread_ready.load();
}

bool comm_pkt_buffer_shift_right(uint8_t **pktptr, uint32_t pktlen, uint32_t buflen) {
  EXPECT_RETURN_BOOL(pktptr != nullptr, "Empty pkt out param", false);
  uint32_t offset = buflen - pktlen; // Make sure buffer is at least 2 x MTU
  if (likely(pktlen * 2 < buflen)) {
    memcpy(*pktptr + offset, *pktptr, pktlen);
    memset(*pktptr, 0, offset);
  }
  else {
    // Expensive
    memcpy(__temp_buffer, *pktptr, pktlen);
    memset(*pktptr, 0, offset);
    memcpy(*pktptr + offset, __temp_buffer, pktlen);
  }
  *pktptr += offset;
  return true; 
}

