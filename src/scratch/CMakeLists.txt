set(ALL_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# We want to trigger cmake reconfiguration should any .cpp file be added.
set_property(DIRECTORY PROPERTY 
  CMAKE_CONFIGURE_DEPENDS ${ALL_CPP_FILES}
)
# Collect all .cpp files
file(GLOB CPP_FILES ${ALL_CPP_FILES})
# Process each file
foreach(FILE ${CPP_FILES})
  # Extract name
  get_filename_component(TARGETNAME ${FILE} NAME_WE) # No extension, just filename
  # Create target
  utils_add_executable(${TARGETNAME}
    SOURCES ${FILE}
    # Edit target properties below
    INCLUDES ${IBVERBS_INCLUDE_DIRS} ${DPDK_INCLUDE_DIRS}
    LINKS ${IBVERBS_LIBRARIES} ${DPDK_LIBRARIES} libcli
    CFLAGS "-DVERBOSE"
    DEFINES "TARGETNAME=\"${TARGETNAME}\""
  )
endforeach()

