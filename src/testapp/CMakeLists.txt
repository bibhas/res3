utils_add_executable(testapp
  INCLUDES ${IBVERBS_INCLUDE_DIRS}
  LINKS ${IBVERBS_LIBRARIES}
  SOURCES
    "main.cpp"
  CFLAGS "-DVERBOSE"
)

