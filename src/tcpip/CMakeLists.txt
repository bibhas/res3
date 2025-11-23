set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
  "phy.cpp"
  "topo.cpp"
  "pcap.cpp"
  # Layer 2
  "layer2/layer2_io.cpp"
  "layer2/layer2_vlan.cpp"
  "layer2/layer2_switch.cpp"
  "layer2/layer2_arp.cpp"
  "layer2/arp_table.cpp"
  "layer2/mac_table.cpp"
  # Layer 3
  "layer3/layer3.cpp"
  "layer3/rt_cbtrie.cpp"
  # Layer 5
  "layer5/layer5.cpp"
)

utils_add_interface_library(tcpip_base
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
           "${CMAKE_CURRENT_SOURCE_DIR}/layer2"
           ${CMAKE_CURRENT_SOURCE_DIR}/..
  SOURCES ${common_sources} 
  CFLAGS "-DVERBOSE"
  LINKS libcli
)

utils_add_interface_library(tcpip_tests_base
  EXTENDS tcpip_base
  LINKS Catch2 
)

utils_add_executable(tcpip
  EXTENDS tcpip_base
  SOURCES "tests/tcpip.cpp" 
          "cli.cpp"
)

utils_add_executable(tests
  EXTENDS tcpip_tests_base
  LINKS Catch2
  SOURCES "tests/tests_main.cpp"
          "tests/nettests.cpp"
          "tests/utiltests.cpp"
          "tests/endtoendtests.cpp"
          # Layer 2
          "layer2/tests/arptests.cpp"
          "layer2/tests/layer2tests.cpp"
          # Layer 3
          "layer3/tests/rttests.cpp"
          "layer3/tests/layer3tests.cpp"
          # Layer 5
          "layer5/tests/layer5tests.cpp"
)

utils_add_executable(pcaptest
  EXTENDS tcpip_base
  SOURCES "tests/pcaptest.cpp" 
)

# Copy cmds.txt to binary dir (to use with $ `config load cmds.txt`)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/cmds.txt
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmds.txt
          ${CMAKE_BINARY_DIR}/cmds.txt
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmds.txt
)

add_custom_target(copy_cmds ALL DEPENDS ${CMAKE_BINARY_DIR}/cmds.txt)


# temporary, delete me once migration is complete

