set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
  "comm.cpp"
  "layer2/layer2.cpp"
  "layer2/arp.cpp"
)

utils_add_interface_library(tcpip_base
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
           "${CMAKE_CURRENT_SOURCE_DIR}/layer2"
  SOURCES ${common_sources} 
  CFLAGS "-DVERBOSE" "-mssse3"
)

utils_add_executable(tcpip
  EXTENDS tcpip_base
  SOURCES "tests/tcpip.cpp" 
          "cli.cpp"
  LINKS libcli
)

utils_add_executable(nettest
  EXTENDS tcpip_base
  SOURCES "tests/nettest.cpp"
)

utils_add_executable(layer2test
  EXTENDS tcpip_base
  SOURCES "layer2/tests/layer2test.cpp"
)


