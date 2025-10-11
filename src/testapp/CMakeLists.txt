utils_add_executable(server
  INCLUDES ${IBVERBS_INCLUDE_DIRS}
  LINKS ${IBVERBS_LIBRARIES}
  SOURCES "server.cpp"
)

utils_add_executable(client
  INCLUDES ${IBVERBS_INCLUDE_DIRS}
  LINKS ${IBVERBS_LIBRARIES}
  SOURCES "client.cpp"
)

