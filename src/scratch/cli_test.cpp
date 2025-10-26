// cli_test.cpp

#include <iostream>
#include <CommandParser/libcli.h>
#include <CommandParser/cmdtlv.h>

/*
 * We're going to add the following two commands:
 *
 * CMD1: show node <node_name::string>
 * CMD2: show node <node_name::string> loopback <ipv4_addrress>
 */

#define CMD_CODE_SHOW_NODE 1

int node_callback_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
  printf("%s() is called\n", __FUNCTION__);
  return 0;
}

int validate_node_name(char *value) {
  printf("%s() is called with value = %s\n", __FUNCTION__, value);
  return VALIDATION_SUCCESS;
}

#define CMD_CODE_SHOW_NODE_LOOPBACK 2

int node_loopback_callback_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
  printf("%s() is called\n", __FUNCTION__);
  return 0;
}

int validate_loopback_address(char *value) {
  printf("%s() is called with value = %s\n", __FUNCTION__, value);
  return VALIDATION_SUCCESS;
}


int main(int argc, const char **argv) {
  init_libcli();

  // Setup hooks
  param_t *show   = libcli_get_show_hook();
  param_t *debug  = libcli_get_debug_hook();
  param_t *config = libcli_get_config_hook();
  param_t *clear  = libcli_get_clear_hook();
  param_t *run    = libcli_get_run_hook();

  /*CMD1: show node <node_name::string> */
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(show, &node); 
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, node_callback_handler, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      set_param_cmd_code(&node_name, CMD_CODE_SHOW_NODE);
      {
        /* Extension for CMD2 */
        static param_t loopback;
        init_param(&loopback, CMD, "loopback", nullptr, nullptr, INVALID, nullptr, "Help : loopback");
        libcli_register_param(&node_name, &loopback);
        {
          static param_t loopback_address;
          init_param(&loopback_address, LEAF, nullptr, node_loopback_callback_handler, validate_loopback_address, IPV4, "lo-address", "Help : Node's loopback address");
          libcli_register_param(&loopback, &loopback_address);
          set_param_cmd_code(&loopback_address, CMD_CODE_SHOW_NODE_LOOPBACK);
        }
      }
    }
  }  

  support_cmd_negation(config);

  // Start shell
  start_shell();

  return 0;
}
