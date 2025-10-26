// cli.h

#pragma once

#include "graph.h"

#define CLI_CMD_CODE_SHOW_TOPOLOGY 1

void cli_init();
void cli_set_topology(graph_t *g);
void cli_runloop();
