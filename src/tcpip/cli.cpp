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

void cli_init() {
  // Initializes libcli
  init_libcli();
  // Setup hooks
  param_t *show = libcli_get_show_hook();
  // Setup command
  {
    static param_t topology;
    init_param(&topology, CMD, "topology", topology_callback_handler, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(show, &topology);
    set_param_cmd_code(&topology, CLI_CMD_CODE_SHOW_TOPOLOGY);
  }
}

void cli_set_topology(graph_t *g) {
  EXPECT_RETURN(g != nullptr, "Empty graph param");
  __topology = g;
}

void cli_runloop() {
  start_shell();
}
