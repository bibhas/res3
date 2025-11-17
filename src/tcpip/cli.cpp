// cli.cpp

#include <cstdlib>
#include <climits>
#include <CommandParser/libcli.h>
#include <CommandParser/cmdtlv.h>
#include "layer5/layer5.h"
#include "layer2/layer2.h"
#include "layer2/arp_table.h"
#include "layer2/mac.h"
#include "utils.h"
#include "cli.h"

#define CLI_CMD_CODE_SHOW_TOPOLOGY 1
#define CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP 2
#define CLI_CMD_CODE_SHOW_NODE_ARP 3
#define CLI_CMD_CODE_SHOW_NODE_MAC 4
#define CLI_CMD_CODE_SHOW_NODE_RT 5
#define CLI_CMD_CODE_CONFIG_NODE_ROUTE 5
#define CLI_CMD_CODE_RUN_NODE_PING 6
#define CLI_CMD_CODE_RUN_NODE_PING_ERO 7

static graph_t *__topology = nullptr;

int show_topology_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_SHOW_TOPOLOGY, "Incorrect CMD code", -1);
  if (!__topology) {
    dump_line("No topology to show!\n");
    return -1; // TODO: return better error code
  }
  graph_dump(__topology);
  return 0;
}

int validate_node_name(char *value) {
  EXPECT_RETURN_VAL(value != nullptr, "Empty value param", VALIDATION_FAILED);
  EXPECT_RETURN_VAL(__topology != nullptr, "Missing topology", VALIDATION_FAILED);
  node_t *resp = graph_find_node_by_name(__topology, value);
  return resp != nullptr ? VALIDATION_SUCCESS : VALIDATION_FAILED;
}

int show_arp_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_SHOW_NODE_ARP, "Incorrect CMD code", -1);
  if (!__topology) {
    dump_line("No topology to show!\n");
    return -1; // TODO: return better error code
  }
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Dump ARP table
  dump_line("ARP table for node: %s\n", node->node_name);
  dump_line("======================\n", node->node_name);
  dump_line_indentation_guard_t guard;
  dump_line_indentation_add(1);
  arp_table_dump(node->netprop.arp_table);
  return 0;
}

int show_mac_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_SHOW_NODE_MAC, "Incorrect CMD code", -1);
  if (!__topology) {
    dump_line("No topology to show!\n");
    return -1; // TODO: return better error code
  }
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Dump MAC table
  dump_line("MAC table for node: %s\n", node->node_name);
  dump_line("======================\n", node->node_name);
  dump_line_indentation_guard_t guard;
  dump_line_indentation_add(1);
  mac_table_dump(node->netprop.mac_table);
  return 0;
}

int show_rt_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_SHOW_NODE_RT, "Incorrect CMD code", -1);
  if (!__topology) {
    dump_line("No topology to show!\n");
    return -1; // TODO: return better error code
  }
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Dump MAC table
  dump_line("Routing Table for node: %s\n", node->node_name);
  dump_line("======================\n", node->node_name);
  dump_line_indentation_guard_t guard;
  dump_line_indentation_add(1);
  rt_dump(node->netprop.r_table);
  return 0;
}

int config_node_route_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_CONFIG_NODE_ROUTE, "Incorrect CMD code", -1);
  if (!__topology) {
    dump_line("No topology to config!\n");
    return -1; // TODO: return better error code
  }
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  char *dst_ip_addr_str = nullptr;
  char *dst_mask_str = nullptr;
  char *gw_ip_addr_str = nullptr;
  char *oif_name = nullptr;
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
    if (strncmp(tlv->leaf_id, "dst-ip-address", strlen("dst-ip-address")) == 0) {
      dst_ip_addr_str = tlv->value;
    }
    if (strncmp(tlv->leaf_id, "dst-mask", strlen("dst-mask")) == 0) {
      dst_mask_str = tlv->value;
    }
    if (strncmp(tlv->leaf_id, "gw-ip-addr", strlen("gw-ip-addr")) == 0) {
      gw_ip_addr_str = tlv->value;
    }
    if (strncmp(tlv->leaf_id, "oif-name", strlen("oif-name")) == 0) {
      oif_name = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  EXPECT_RETURN_VAL(dst_ip_addr_str != nullptr, "Couldn't parse destination ip", -1);
  EXPECT_RETURN_VAL(dst_mask_str != nullptr, "Couldn't destination mask", -1);
  EXPECT_RETURN_VAL(gw_ip_addr_str != nullptr, "Couldn't parse gateway ip", -1);
  EXPECT_RETURN_VAL(oif_name != nullptr, "Couldn't parse outgoing interface name", -1);
  // Parse addresses into ipv4_addr_t
  ipv4_addr_t dst_ip_addr;
  bool resp = ipv4_addr_try_parse(dst_ip_addr_str, &dst_ip_addr);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", -1);
  ipv4_addr_t gw_ip_addr;
  resp = ipv4_addr_try_parse(gw_ip_addr_str, &gw_ip_addr);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", -1);
  // Parse mask value
  uint8_t dst_mask = strtol(dst_mask_str, nullptr, 10); // base 10
  EXPECT_RETURN_VAL(dst_mask != 0, "strol failed", -1);
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Find interface
  interface_t *oif = node_get_interface_by_name(node, oif_name);
  EXPECT_RETURN_VAL(oif != nullptr, "node_get_interface_by_name failed", -1);
  EXPECT_RETURN_VAL(INTF_IN_L3_MODE(oif) == true, "Provided interface argument is not in L3 mode!", -1);
  // Add route
  resp = rt_add_route(node->netprop.r_table, &dst_ip_addr, dst_mask, &gw_ip_addr, oif);
  EXPECT_RETURN_VAL(resp == true, "rt_add_route failed", -1);
  printf("Route added!\n");
  return 0;
}

int validate_ip_address(char *value) {
  ipv4_addr_t out;
  return ipv4_addr_try_parse(value, &out) ? VALIDATION_SUCCESS : VALIDATION_FAILED;
}

int run_node_resolve_arp_callback(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP, "Incorrect CMD code", -1);
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  char *ip_addr_str = nullptr;
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
    else if (strncmp(tlv->leaf_id, "ip-address", strlen("ip-address")) == 0) {
      ip_addr_str = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  EXPECT_RETURN_VAL(ip_addr_str != nullptr, "Couldn't parse ip address", -1);
  // Parse ip address into ipv4_addr_t
  ipv4_addr_t ip_addr;
  bool resp = ipv4_addr_try_parse(ip_addr_str, &ip_addr);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", -1);
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Fine node interface that matches this subnet
  interface_t *ointf = nullptr;
  resp = node_get_interface_matching_subnet(node, &ip_addr, &ointf);
  EXPECT_RETURN_VAL(resp == true, "node_get_interface_matching_subnet failed", -1);
  // Perform ARP broadcast request
  resp = node_arp_send_broadcast_request(node, ointf, &ip_addr);
  EXPECT_RETURN_VAL(resp == true, "node_arp_send_broadcast_request failed", -1);
  return 0;
}

int run_node_ping_callback(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(
    code == CLI_CMD_CODE_RUN_NODE_PING || code == CLI_CMD_CODE_RUN_NODE_PING_ERO, 
    "Incorrect CMD code", -1
  );
  // Parse out the node name and ip address
  tlv_struct_t *tlv = nullptr;
  char *node_name = nullptr; 
  char *ip_addr_str = nullptr;
  char *ero_addr_str = nullptr;
  TLV_FOREACH_BEGIN(tlvs, tlv) {
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
    else if (strncmp(tlv->leaf_id, "ip-address", strlen("ip-address")) == 0) {
      ip_addr_str = tlv->value;
    }
    else if (strncmp(tlv->leaf_id, "ero-address", strlen("ip-address")) == 0) {
      ero_addr_str = tlv->value;
    }
  } 
  TLV_FOREACH_END();
  EXPECT_RETURN_VAL(node_name != nullptr, "Couldn't parse node name", -1);
  EXPECT_RETURN_VAL(ip_addr_str != nullptr, "Couldn't parse ip address", -1);
  // Parse ip address into ipv4_addr_t
  ipv4_addr_t ip_addr;
  bool resp = ipv4_addr_try_parse(ip_addr_str, &ip_addr);
  EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", -1);
  ipv4_addr_t ero_addr;
  if (ero_addr_str) {
    resp = ipv4_addr_try_parse(ero_addr_str, &ero_addr);
    EXPECT_RETURN_VAL(resp == true, "ipv4_addr_try_parse failed", -1);
  }
  // Find node
  node_t *node = graph_find_node_by_name(__topology, node_name);
  EXPECT_RETURN_VAL(node != nullptr, "graph_find_node_by_name failed", -1);
  // Perform ping
  ipv4_addr_t *ero_addr_ptr = (ero_addr_str ? &ero_addr : nullptr);
  resp = layer5_perform_ping(node, &ip_addr, ero_addr_ptr);
  EXPECT_RETURN_VAL(resp == true, "layer5_perform_ping failed", -1); 
  return 0;
}

void cli_init() {
  // Initializes libcli
  init_libcli();
  // Setup hooks
  param_t *show = libcli_get_show_hook();
  // Setup `show topology`
  {
    static param_t topology;
    init_param(&topology, CMD, "topology", show_topology_callback_handler, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(show, &topology);
    set_param_cmd_code(&topology, CLI_CMD_CODE_SHOW_TOPOLOGY);
  }
  // Setup `show node <...> arp | mac | rt`
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(show, &node);
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, nullptr, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      {
        static param_t arp;
        init_param(&arp, CMD, "arp", show_arp_callback_handler, nullptr, INVALID, nullptr, "Help : arp");
        libcli_register_param(&node_name, &arp);
        set_param_cmd_code(&arp, CLI_CMD_CODE_SHOW_NODE_ARP);
      }
      {
        static param_t mac;
        init_param(&mac, CMD, "mac", show_mac_callback_handler, nullptr, INVALID, nullptr, "Help : mac");
        libcli_register_param(&node_name, &mac);
        set_param_cmd_code(&mac, CLI_CMD_CODE_SHOW_NODE_MAC);
      }
      {
        static param_t rt;
        init_param(&rt, CMD, "rt", show_rt_callback_handler, nullptr, INVALID, nullptr, "Help : rt");
        libcli_register_param(&node_name, &rt);
        set_param_cmd_code(&rt, CLI_CMD_CODE_SHOW_NODE_RT);
      }
    }
  }
  param_t *run = libcli_get_run_hook();
  // Setup `run node <node-name> resolve-arp <ip-address>`
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(run, &node);
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, nullptr, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      // resolve-arp
      {
        static param_t resolve_arp;
        init_param(&resolve_arp, CMD, "resolve-arp", nullptr, nullptr, INVALID, nullptr, "Help : resolve-arp");
        libcli_register_param(&node_name, &resolve_arp);
        {
          static param_t ip_address;
          init_param(&ip_address, LEAF, nullptr, run_node_resolve_arp_callback, validate_ip_address, STRING, "ip-address", "Help : IP address");
          libcli_register_param(&resolve_arp, &ip_address);
          set_param_cmd_code(&ip_address, CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP);
        }
      }
      // ping
      {
        static param_t ping;
        init_param(&ping, CMD, "ping", nullptr, nullptr, INVALID, nullptr, "Help : ping");
        libcli_register_param(&node_name, &ping);
        {
          static param_t ip_address;
          init_param(&ip_address, LEAF, nullptr, run_node_ping_callback, validate_ip_address, STRING, "ip-address", "Help : IP address");
          libcli_register_param(&ping, &ip_address);
          set_param_cmd_code(&ip_address, CLI_CMD_CODE_RUN_NODE_PING);
          {
            static param_t ero;
            init_param(&ero, CMD, "ero", nullptr, nullptr, INVALID, nullptr, "Help : Explicit Route Object");
            libcli_register_param(&ip_address, &ero);
            {
              static param_t ero_address;
              init_param(&ero_address, LEAF, nullptr, run_node_ping_callback, validate_ip_address, STRING, "ero-address", "Help : ERO IP address");
              libcli_register_param(&ero, &ero_address);
              set_param_cmd_code(&ero_address, CLI_CMD_CODE_RUN_NODE_PING_ERO);
            }
          }
        }
      }
    }
  }
  param_t *config = libcli_get_config_hook();
  // Setup `config node <node-name> route <dest> <mask> <gw-ip> <oif-name>`
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(config, &node);
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, nullptr, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      {
        static param_t route;
        init_param(&route, CMD, "route", nullptr, nullptr, INVALID, nullptr, "Help : route");
        libcli_register_param(&node_name, &route);
        {
          static param_t dst;
          init_param(&dst, LEAF, nullptr, nullptr, validate_ip_address, STRING, "dst-ip-address", "Help : Destination IP address");
          libcli_register_param(&route, &dst);
          {
            static param_t mask;
            init_param(&mask, LEAF, nullptr, nullptr, nullptr, INT, "dst-mask", "Help : Destination Network Mask");
            libcli_register_param(&dst, &mask);
            {
              static param_t gw;
              init_param(&gw, LEAF, nullptr, nullptr, validate_ip_address, STRING, "gw-ip-address", "Help : Gateway IP address");
              libcli_register_param(&mask, &gw);
              {
                static param_t oif;
                init_param(&oif, LEAF, nullptr, config_node_route_callback_handler, nullptr, STRING, "oif-name", "Help : Outgoing Network Interface");
                libcli_register_param(&gw, &oif);
                set_param_cmd_code(&oif, CLI_CMD_CODE_CONFIG_NODE_ROUTE);
              }
            }
          }
        }
      }
    }
  }
}

void cli_set_topology(graph_t *g) {
  EXPECT_RETURN(g != nullptr, "Empty graph param");
  __topology = g;
}

void cli_runloop() {
  start_shell();
}
