// lpmtests.cpp

#include "catch2.hpp"
#include "layer3/layer3.h"
#include "graph.h"
#include "topo.h"

TEST_CASE("Local delivery test", "[layer3][promote]") {
/*
 *                        +----------+
 *               eth0/4   |          |eth0/0
 *       +----------------+    H0    +------------------+
 *       |     40.1.1.1/24|          |20.1.1.1/24       |
 *       |                +----------+                  |
 *       |                 122.1.1.0                    |
 *       |                                              |
 *       |                                              |
 *       |40.1.1.2/24                                   |20.1.1.2/24
 *       |eth0/5                                        |eth0/1
 *   +---+---+                                      +---+----+
 *   |       |eth0/3                          eth0/2|        |
 *   |   H2  +--------------------------------------+   H1   |
 *   |       |30.1.1.2/24            30.1.1.1/24    |        |
 *   +-------+                                      +--------+
 *   122.1.1.2                                      122.1.1.1
 */
  graph_t *topo = graph_create_three_node_ring_topology();
  node_t *H0 = graph_find_node_by_name(topo, "H0");
  // Run tests
  SECTION("-") {
  }
}
