// cli_test.cpp

#include <iostream>
#include <CommandParser/libcli.h>
#include <CommandParser/cmdtlv.h>

/*
 * We're going to add the following two commands:
 *
 * CMD1: show node <node-name::STRING>
 * CMD2: show node <node-name::STRING> loopback <lo-address::IPV4>
 */

#define CMD_CODE_SHOW_NODE 1

int node_callback_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
  printf("%s() is called\n", __FUNCTION__);
  return 0;
}

int validate_node_name(char *value) {
  printf("%s() is called with value = %s\n", __FUNCTION__, value);
  if (strcmp(value, "notnode") == 0) {
    return VALIDATION_FAILED;
  }
  return VALIDATION_SUCCESS;
}

#define CMD_CODE_SHOW_NODE_LOOPBACK 2

int node_loopback_callback_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
  printf("%s() is called\n", __FUNCTION__);
  int cmd_code = EXTRACT_CMD_CODE(tlv_buf);
  printf("cmd_code = %d (!)\n", cmd_code);

  tlv_struct_t *tlv = NULL;
  char *node_name = NULL;
  char *lo_address = NULL;

  int count = 0;
  TLV_LOOP_BEGIN(tlv_buf, tlv) {
    count++;
    if (strncmp(tlv->leaf_id, "node-name", strlen("node-name")) == 0) {
      node_name = tlv->value;
    }
    else if (strncmp(tlv->leaf_id, "lo-address", strlen("lo-address")) == 0) {
      lo_address = tlv->value;
    }
  } TLV_LOOP_END;

  printf("TLV buffer size: %d\n", count);

  switch (cmd_code) {
    case CMD_CODE_SHOW_NODE_LOOPBACK:
      printf("node_name = %s, lo-address = %s\n", node_name, lo_address);
      break;
    default:
      ;
  }
  return 0;
}

int validate_loopback_address(char *value) {
  printf("%s() is called with value = %s\n", __FUNCTION__, value);
  return VALIDATION_SUCCESS;
}

#define CMD_CODE_CONFIG_NODE_LOOPBACK 3

int config_node_loopback_callback_handler(param_t *param, ser_buff_t *tlv_buf, op_mode mode) {
  printf("%s() is called\n", __FUNCTION__);
  switch (mode) {
    case CONFIG_ENABLE: printf("ENABLE\n"); break;
    case CONFIG_DISABLE: printf("DISABLE\n"); break;
  }
  return 0;
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

  /*CMD3: config node <node-name::STRING> loopback <lo-address::IPV4> */
  {
    static param_t node;
    init_param(&node, CMD, "node", nullptr, nullptr, INVALID, nullptr, "Help : node");
    libcli_register_param(config, &node); 
    {
      static param_t node_name;
      init_param(&node_name, LEAF, nullptr, nullptr, validate_node_name, STRING, "node-name", "Help : Node name");
      libcli_register_param(&node, &node_name);
      {
        static param_t loopback;
        init_param(&loopback, CMD, "loopback", nullptr, nullptr, INVALID, nullptr, "Help : loopback");
        libcli_register_param(&node_name, &loopback);
        {
          static param_t loopback_address;
          init_param(&loopback_address, LEAF, nullptr, config_node_loopback_callback_handler, validate_loopback_address, IPV4, "lo-address", "Help : Node's loopback address");
          libcli_register_param(&loopback, &loopback_address);
          set_param_cmd_code(&loopback_address, CMD_CODE_CONFIG_NODE_LOOPBACK);
        }
      }
    }
  }  

  support_cmd_negation(config);

  // Start shell
  start_shell();

  return 0;
}
