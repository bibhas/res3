utils_add_interface_library(gluethread
  SOURCES gluethread/glthread.c
)

utils_add_interface_library(libcli
  SOURCES
    cmd_hier.c
    parser.c
    serialize.c
    string_util.c
    clistd.c
    clicbext.c
    ut/utinfra/ut_parser.c
  LINKS gluethread readline
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}   # TODO: Make this private
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  CFLAGS "-Wno-unused-result"
)
