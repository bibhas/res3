set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
)

utils_add_executable(tcpip
  SOURCES "tcpip.cpp" ${common_sources} 
)

utils_add_executable(nettest
  SOURCES "nettest.cpp" ${common_sources} 
)


