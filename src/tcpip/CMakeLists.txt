set(common_sources
  "net.cpp"
  "graph.cpp"
  "utils.cpp"
)

utils_add_executable(tcpip
  SOURCES ${common_sources} 
          "tcpip.cpp" 
          "cli.cpp"
  LINKS libcli
  CFLAGS "-DVERBOSE" "-mssse3"
  DEFINES "TARGETNAME=\"tcpip\""
)

utils_add_executable(nettest
  SOURCES "nettest.cpp" ${common_sources} 
)


