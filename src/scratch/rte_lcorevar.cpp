#include <rte_common.h>
#include <iostream>
#include <rte_lcore.h>
#include <rte_lcore_var.h>
#include <rte_per_lcore.h>

struct foo_lcore_state {
  int a;
  long b;
};

static RTE_LCORE_VAR_HANDLE(struct foo_lcore_state, lcore_states);

long foo_get_a_plus_b(void) {
  const struct foo_lcore_state *state = RTE_LCORE_VAR(lcore_states);
  return state->a + state->b + 2;
}

RTE_INIT(rte_foo_init) {
  std::cout << "rte_foo_init..." << std::endl;
  RTE_LCORE_VAR_ALLOC(lcore_states);

  unsigned int lcore_id;
  struct foo_lcore_state *state;

  RTE_LCORE_VAR_FOREACH(lcore_id, state, lcore_states) {
    state->a = lcore_id;
    state->b = lcore_id * 10;
    std::cout << "id:" << lcore_id << " a:" << state->a << " b:" << state->b << std::endl;
  }
}

int main(int argc, char **argv) {
  printf("entering main...\n");
  rte_eal_init(argc, argv);
  printf("Sum on main lcore = %ld\n", foo_get_a_plus_b());
  return 0;
}

