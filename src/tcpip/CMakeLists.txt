set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
  "phy.cpp"
  "topo.cpp"
  "pcap.cpp"
  # Layer 2
  "layer2/layer2.cpp"
  "layer2/arp.cpp"
  "layer2/mac.cpp"
  # Layer 3
  "layer3/layer3.cpp"
  "layer3/rt.cpp"
  # Layer 5
  "layer5/layer5.cpp"
)

utils_add_interface_library(tcpip_base
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
           "${CMAKE_CURRENT_SOURCE_DIR}/layer2"
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

utils_add_executable(nettest
  EXTENDS tcpip_base
  SOURCES "tests/nettest.cpp"
)

utils_add_executable(tests
  EXTENDS tcpip_tests_base
  LINKS Catch2
  SOURCES "tests/tests_main.cpp"
          "tests/nettests.cpp"
          "tests/utiltests.cpp"
          # Layer 2
          "layer2/tests/arptests.cpp"
          "layer2/tests/layer2tests.cpp"
          # Layer 3
          "layer3/tests/lpmtests.cpp"
          "layer3/tests/layer3tests.cpp"
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
