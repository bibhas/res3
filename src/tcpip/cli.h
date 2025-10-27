// cli.h

#pragma once

#include "graph.h"

#define CLI_CMD_CODE_SHOW_TOPOLOGY 1
#define CLI_CMD_CODE_RUN_NODE_RESOLVE_ARP 2

void cli_init();
void cli_set_topology(graph_t *g);
void cli_runloop();
