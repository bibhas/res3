// rte_lcorevar.cpp

#include <iostream>
#include <rte_lcore.h>
#include <rte_lcore_var.h>
#include <rte_per_lcore.h>

#define EXPECT(X, MSG) if (!(X)) { rte_panic(MSG); }

struct workerstate_t {
  int stateval = 0;
};

static RTE_LCORE_VAR_HANDLE(workerstate_t, lcore_workerstates);

static int lcore_hello(__rte_unused void *arg) {
  unsigned lcore_id = rte_lcore_id();
  printf("Hello from lcore: %u\n", lcore_id);
  workerstate_t *state = RTE_LCORE_VAR(lcore_workerstates);
  printf("  The value of lcore_val `lcore_workerstates` is: %i\n", state->stateval);
  return 0;
}

RTE_INIT(workerstate_init) { // Called before main()
  // Allocate lcore variable
  workerstate_t * a = (workerstate_t *)RTE_LCORE_VAR_ALLOC(lcore_workerstates);
  // Initialize variable for all lcores
  unsigned int lcore_id;
  workerstate_t *instance;
  RTE_LCORE_VAR_FOREACH(lcore_id, instance, lcore_workerstates) {
    instance->stateval = lcore_id * 100 + 1;
  }
}

int main(int argc, char **argv) {
  // Initialize EAL
  int resp = rte_eal_init(argc, argv);
  EXPECT(resp >= 0, "rte_eal_init failed");
  
  // Invoke `lcore_hello` in each of the lcores
  unsigned lcore_id = 0;
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
  }

  // Wait for all worker lcores
  rte_eal_mp_wait_lcore();

  // Invoke same function in the main lcore directly
  lcore_hello(NULL);

  // Clean up EAL
  rte_eal_cleanup();
  
  // And, we're done.
  return resp;
}
