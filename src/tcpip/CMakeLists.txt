set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
  "phy.cpp"
  "topo.cpp"
  "pcap.cpp"
  "layer2/layer2.cpp"
  "layer2/arp.cpp"
  "layer2/mac.cpp"
  "layer3/layer3.cpp"
  "layer3/rt.cpp"
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
          "layer2/tests/arptests.cpp"
          "layer2/tests/layer2tests.cpp"
)

utils_add_executable(pcaptest
  EXTENDS tcpip_base
  SOURCES "tests/pcaptest.cpp" 
)
