// cli.cpp

#include <CommandParser/libcli.h>
#include <CommandParser/cmdtlv.h>
#include "utils.h"
#include "cli.h"

static graph_t *__topology = nullptr;

int topology_callback_handler(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_SHOW_TOPOLOGY, "Incorrect code", -1);
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

int validate_ip_address(char *value) {
  printf("TODO: Implement ip address validation...\n");
  return VALIDATION_SUCCESS;
}

int node_resolve_arp_callback(param_t *p, ser_buff_t *tlvs, op_mode mode) {
  int code = EXTRACT_CMD_CODE(tlvs);
  EXPECT_RETURN_VAL(code == CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP, "Incorrect code", -1);
  printf("TODO: Implement ARP resolver...\n");
  return 0;
}

void cli_init() {
  // Initializes libcli
  init_libcli();
  // Setup hooks
  param_t *show = libcli_get_show_hook();
  param_t *run = libcli_get_run_hook();
  // Setup `show topology`
  {
    static param_t topology;
    init_param(&topology, CMD, "topology", topology_callback_handler, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(show, &topology);
    set_param_cmd_code(&topology, CLI_CMD_CODE_SHOW_TOPOLOGY);
  }
  // Setup `run node <node-name> resolve-arp <ip-address>`
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(run, &node);
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, nullptr, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      {
        static param_t resolve_arp;
        init_param(&resolve_arp, CMD, "resolve-arp", nullptr, nullptr, INVALID, nullptr, "Help : resolve-arp");
        libcli_register_param(&node_name, &resolve_arp);
        {
          static param_t ip_address;
          init_param(&ip_address, LEAF, nullptr, node_resolve_arp_callback, validate_ip_address, STRING, "ip-address", "Help : IP address");
          libcli_register_param(&resolve_arp, &ip_address);
          set_param_cmd_code(&ip_address, CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP);
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
