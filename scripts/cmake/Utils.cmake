# Utils.cmake

cmake_minimum_required(VERSION 3.20.0)

include_guard()

# ---
# Target related Macros

macro(utils_add_interface_library USER_LIB_NAME)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs EXTENDS INCLUDES SOURCES LINKS DEFINES CFLAGS LFLAGS)
  cmake_parse_arguments(USER_LIB
    "${options}" 
    "${oneValueArgs}" 
    "${multiValueArgs}" 
    ${ARGN})
  add_library(${USER_LIB_NAME} INTERFACE)
  if(DEFINED USER_LIB_EXTENDS)
    target_link_libraries(${USER_LIB_NAME} INTERFACE ${USER_LIB_EXTENDS})
  endif()
  if(DEFINED USER_LIB_INCLUDES)
    target_include_directories(${USER_LIB_NAME} INTERFACE ${USER_LIB_INCLUDES})
  endif()
  if(DEFINED USER_LIB_SOURCES)
    target_sources(${USER_LIB_NAME} INTERFACE ${USER_LIB_SOURCES})
  endif()
  if(DEFINED USER_LIB_LINKS)
    target_link_libraries(${USER_LIB_NAME} INTERFACE ${USER_LIB_LINKS})
  endif()
  if(DEFINED USER_LIB_DEFINES)
    target_compile_definitions(${USER_LIB_NAME} INTERFACE ${USER_LIB_DEFINES})
  endif()
  if(DEFINED USER_LIB_CFLAGS)
    target_compile_options(${USER_LIB_NAME} INTERFACE ${USER_LIB_CFLAGS})
  endif()
  if(DEFINED USER_LIB_LFLAGS)
    target_link_options(${USER_LIB_NAME} INTERFACE ${USER_LIB_LFLAGS})
  endif()
endmacro()

macro(utils_add_executable USER_EXE_NAME)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs EXTENDS INCLUDES SOURCES LINKS DEFINES CFLAGS LFLAGS)
  cmake_parse_arguments(USER_EXE
    "${options}" 
    "${oneValueArgs}" 
    "${multiValueArgs}" 
    ${ARGN})
  add_executable(${USER_EXE_NAME})
  if(DEFINED USER_EXE_EXTENDS)
    target_link_libraries(${USER_EXE_NAME} PUBLIC ${USER_EXE_EXTENDS})
  endif()
  if(DEFINED USER_EXE_INCLUDES)
    target_include_directories(${USER_EXE_NAME} PUBLIC ${USER_EXE_INCLUDES})
  endif()
  if(DEFINED USER_EXE_SOURCES)
    target_sources(${USER_EXE_NAME} PUBLIC ${USER_EXE_SOURCES})
  endif()
  if(DEFINED USER_EXE_LINKS)
    target_link_libraries(${USER_EXE_NAME} PUBLIC ${USER_EXE_LINKS})
  endif()
  if(DEFINED USER_EXE_DEFINES)
    target_compile_definitions(${USER_EXE_NAME} PUBLIC ${USER_EXE_DEFINES})
  endif()
  if(DEFINED USER_EXE_CFLAGS)
    target_compile_options(${USER_EXE_NAME} PUBLIC ${USER_EXE_CFLAGS})
  endif()
  if(DEFINED USER_EXE_LFLAGS)
    target_link_options(${USER_EXE_NAME} PUBLIC ${USER_EXE_LFLAGS})
  endif()
  set_target_properties(${USER_EXE_NAME} PROPERTIES 
    LINKER_LANGUAGE C
  )
endmacro()

macro(utils_add_static_library USER_LIB_NAME)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs EXTENDS INCLUDES SOURCES LINKS DEFINES CFLAGS LFLAGS)
  cmake_parse_arguments(USER_LIB
    "${options}" 
    "${oneValueArgs}" 
    "${multiValueArgs}" 
    ${ARGN})
  add_library(${USER_LIB_NAME} STATIC)
  if(DEFINED USER_LIB_EXTENDS)
    target_link_libraries(${USER_LIB_NAME} PUBLIC ${USER_LIB_EXTENDS})
  endif()
  if(DEFINED USER_LIB_INCLUDES)
    target_include_directories(${USER_LIB_NAME} PUBLIC ${USER_LIB_INCLUDES})
  endif()
  if(DEFINED USER_LIB_SOURCES)
    target_sources(${USER_LIB_NAME} PUBLIC ${USER_LIB_SOURCES})
  endif()
  if(DEFINED USER_LIB_LINKS)
    target_link_libraries(${USER_LIB_NAME} PUBLIC ${USER_LIB_LINKS})
  endif()
  if(DEFINED USER_LIB_DEFINES)
    target_compile_definitions(${USER_LIB_NAME} PUBLIC ${USER_LIB_DEFINES})
  endif()
  if(DEFINED USER_LIB_CFLAGS)
    target_compile_options(${USER_LIB_NAME} PUBLIC ${USER_LIB_CFLAGS})
  endif()
  if(DEFINED USER_LIB_LFLAGS)
    target_link_options(${USER_LIB_NAME} PUBLIC ${USER_LIB_LFLAGS})
  endif()
endmacro()


# ---
# General Utility Macros

macro(utils_add_subdirectories)
  foreach(SUBDIR ${ARGN})
    message(DEBUG "Adding subdir: ${SUBDIR}")
    add_subdirectory(${SUBDIR})
  endforeach()
endmacro()

macro(utils_read_git_rev GIT_REV_VAR)
  get_git_head_revision(GIT_REFSPEC GIT_SHA1)
  string(SUBSTRING "${GIT_SHA1}" 0 12 ${GIT_REV_VAR})
  if(NOT GIT_SHA1)
    set(${GIT_REV_VAR} "0")
  endif()
endmacro()

macro(utils_set_from_env ENV_VAR ELSE DEFAULT_VAL)
  if(NOT DEFINED ENV{${ENV_VAR}})
    set(${ENV_VAR} ${DEFAULT_VAL})
  else() 
    set(${ENV_VAR} $ENV{${ENV_VAR}})
  endif()
endmacro()

macro(utils_set OUTPUT_VAR VALUE_VAR IF_DEFINED CHECK_VAR ELSE DEFAULT_VAL_VAR)
  if(DEFINED ${CHECK_VAR})
    set(${OUTPUT_VAR} ${VALUE_VAR})
  else()
    set(${OUTPUT_VAR} ${DEFAULT_VAL_VAR})
  endif()
endmacro()

macro(utils_set_list OUTPUT_VAR IF DEFINED CHECK_VAR)
  if(DEFINED ${CHECK_VAR})
    set(${OUTPUT_VAR} ${ARGN})
  endif()
endmacro()

macro(utils_glob_append GLOB_APPEND_VAR)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs GLOB GLOB_RECURSE VISITING IGNORING)
  cmake_parse_arguments(GLOB_APPEND 
    "${options}" 
    "${oneValueArgs}" 
    "${multiValueArgs}" 
    ${ARGN})
  if(DEFINED GLOB_APPEND_GLOB)
    set(GLOB_APPEND_PATTERNS ${GLOB_APPEND_GLOB})
    set(GLOB_FLAG GLOB)
  else()
    set(GLOB_APPEND_PATTERNS ${GLOB_APPEND_GLOB_RECURSE})
    set(GLOB_FLAG GLOB_RECURSE)
  endif()
  foreach(GLOB_DIR IN ITEMS ${GLOB_APPEND_VISITING})
    foreach(GLOB_PATTERN IN ITEMS ${GLOB_APPEND_PATTERNS})
      cmake_path(APPEND GLOB_DIR ${GLOB_PATTERN} OUTPUT_VARIABLE GLOB_INPUT)      
      file(${GLOB_FLAG} GLOB_ACC ${GLOB_INPUT})
      list(APPEND ${GLOB_APPEND_VAR} ${GLOB_ACC})
    endforeach()
  endforeach()
  if(DEFINED GLOB_APPEND_IGNORING)
    utils_glob_append(GLOB_IGNORING
      ${GLOB_FLAG} ${GLOB_APPEND_PATTERNS}
      VISITING ${GLOB_APPEND_IGNORING}
    )
    list(REMOVE_ITEM ${GLOB_APPEND_VAR} ${GLOB_IGNORING})
  endif()
endmacro()

macro(utils_glob_set GLOB_SET_VAR)
  set(${GLOB_SET_VAR} "")
  utils_glob_append(${ARGV})
endmacro()

# ---
# Testing

macro(utils_platform_add_test_subdirectory)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROJECT WIN32 APPLE)
  cmake_parse_arguments(PLAT_ADD_TEST_SUBDIR 
    "${options}" 
    "${oneValueArgs}" 
    "${multiValueArgs}" 
    ${ARGN})
  if(NOT CMAKE_GENERATOR STREQUAL Xcode)
    if(DEFINED PLAT_ADD_TEST_SUBDIR_PROJECT)
      project(${PLAT_ADD_TEST_SUBDIR_PROJECT})
      set(CMAKE_CXX_STANDARD 17)
    endif()
  endif()
  if(WIN32)
    foreach(SUBDIR IN LISTS PLAT_ADD_TEST_SUBDIR_WIN32)
      message("Adding subdir: ${SUBDIR}")
      utils_add_test_subdirectory(${SUBDIR})
    endforeach()
  elseif(APPLE)
    foreach(SUBDIR IN LISTS PLAT_ADD_TEST_SUBDIR_APPLE)
      message("Adding subdir: ${SUBDIR}")
      utils_add_test_subdirectory(${SUBDIR})
    endforeach()
  else()
    message(FATAL_ERROR "This platform is not supported!")
  endif()
endmacro()

macro(utils_add_test_subdirectory TEST_SUBDIR)
  add_subdirectory(${TEST_SUBDIR})
  if(NOT CMAKE_GENERATOR STREQUAL Xcode)
    if(NOT TARGET ${TEST_SUBDIR})
      return()
    endif()
    add_dependencies(test ${TEST_SUBDIR})
    if(WIN32)
      target_link_libraries(${TEST_SUBDIR} PUBLIC 
        gtest_main gtest
      )
    elseif(APPLE)
      find_package(GTest)
      target_link_libraries(${TEST_SUBDIR} PUBLIC 
        GTest::GTest
      )
    else()
      message(FATAL_ERROR "This platform is not supported!")
    endif()
    generate_coverage_report(${TEST_SUBDIR})
  endif()
endmacro()

macro(utils_add_test TEST_TARGET TESTING TESTEE_TARGET)
  if(CMAKE_GENERATOR STREQUAL Xcode)
    find_package(XCTest REQUIRED)
    # Setup xctest bundle target
    xctest_add_bundle(${TEST_TARGET} 
      ${TESTEE_TARGET}
    )
    set(test_bundleid "com.cinemagrade.${TESTEE_TARGET}.tests.${TEST_TARGET}")
    set_target_properties(${TEST_TARGET} PROPERTIES
      XCODE_GENERATE_SCHEME OFF
      MACOSX_BUNDLE_GUI_IDENTIFIER ${test_bundleid}
      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${test_bundleid}
      FOLDER "${TESTEE_TARGET}/tests"
    )
    set(_scheme_envs
      "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}"
      "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    )
    set_target_properties(${TESTEE_TARGET} PROPERTIES
      XCODE_SCHEME_ENVIRONMENT "${_scheme_envs}"
      XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # Add GoogleTestRunner.mm source file to the xcbundle
    set(GoogleTestCaseClassName "Ignore_Me")
    set(GoogleTestRunnerTemplatePath
      ${CMAKE_SOURCE_DIR}/internals/cinema_grade_cmake-src/templates/GoogleTestRunner.mm.in
    )
    set(GoogleTestRunnerGeneratedPath
      "${CMAKE_CURRENT_BINARY_DIR}/${GoogleTestCaseClassName}.mm"
    )
    # Will replace @GoogleTestCaseClassName@ in the template file.
    configure_file(
      ${GoogleTestRunnerTemplatePath}
      ${GoogleTestRunnerGeneratedPath} 
      @ONLY
    )
    target_sources(${TEST_TARGET} PRIVATE
      ${GoogleTestRunnerGeneratedPath}
    )
    set_source_files_properties(${GoogleTestRunnerGeneratedPath} PROPERTIES 
      COMPILE_FLAGS "-fobjc-arc"
    )
    source_group(
      generated
      FILES ${GoogleTestRunnerGeneratedPath}
    )
  else()
    add_executable(${TEST_TARGET})
    add_executable(${META_PROJECT_NAME}::${TEST_TARGET} ALIAS ${TEST_TARGET})
    set_target_properties(${target}
      PROPERTIES ${DEFAULT_PROJECT_OPTIONS}
      FOLDER "${IDE_FOLDER}"
    )
    target_include_directories(${TEST_TARGET} PRIVATE 
      ${DEFAULT_INCLUDE_DIRECTORIES} 
    )
    get_target_property(target_type ${TESTEE_TARGET} TYPE)
    if (NOT target_type STREQUAL "EXECUTABLE")
      target_link_libraries(${TEST_TARGET} PUBLIC
        ${DEFAULT_LIBRARIES}
        ${TESTEE_TARGET}
      )
    endif ()
    if(WIN32)
      target_link_libraries(${TEST_TARGET} PUBLIC
        gtest_main gtest
      )
    elseif(APPLE)
      find_package(GTest)
      target_link_libraries(${TEST_TARGET} PUBLIC
        GTest::GTest
      )
    else()
      message(FATAL_ERROR "This platform is not supported!")
    endif()
    target_compile_definitions(${TEST_TARGET} PRIVATE
      ${DEFAULT_COMPILE_OPTIONS}
    )
    target_compile_options(${TEST_TARGET} PRIVATE
      ${DEFAULT_COMPILE_OPTIONS}
    )
    target_link_libraries(${TEST_TARGET} PRIVATE
      ${DEFAULT_LINKER_OPTIONS}
    )
    install(TARGETS ${TEST_TARGET}
      RUNTIME DESTINATION ${INSTALL_BIN} COMPONENT runtime
    )
  endif()
endmacro()

# ---
# Build Info

set(__BUILD_INFO_DOT_DIR ".build_info")
 
macro(utils_log_build_info_target_build_path TARGET_VAR)
  utils_log_build_info_aliased_target_build_path(
    TARGET ${TARGET_VAR}
    ALIAS ${TARGET_VAR}
  )
endmacro()

macro(utils_log_build_info_aliased_target_build_path TARGET TARGET_VAR ALIAS ALIAS_VAR)
  file(MAKE_DIRECTORY 
    "${CMAKE_SOURCE_DIR}/${__BUILD_INFO_DOT_DIR}"
  )
  add_custom_command(
    TARGET ${TARGET_VAR} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env ${PYTHON_CMD} -c "with open('${CMAKE_SOURCE_DIR}/${__BUILD_INFO_DOT_DIR}/${ALIAS_VAR}', 'w+') as f: f.write('$<TARGET_FILE:${TARGET_VAR}>')"
    VERBATIM
  )
endmacro()

macro(utils_log_build_info_named_value NAME NAME_VAR VALUE VALUE_VAR)
  # FYI: Will also build missing parent directory, if needed.
  # Also, unlike the target variant, `file` command will be run at configuration time. Beware!
  file(WRITE "${CMAKE_SOURCE_DIR}/${__BUILD_INFO_DOT_DIR}/${NAME_VAR}" "${VALUE_VAR}")
endmacro()

# ---

string(TOLOWER ${CMAKE_SYSTEM_NAME} _SYSTEM_NAME)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _SYSTEM_PROCESSOR)
string(CONCAT _SYSTEM_MNEMONIC ${_SYSTEM_NAME} "-" ${_SYSTEM_PROCESSOR})

macro(utils_add_vanilla_executable BASE FLAVOR)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CMAKE_LOWER_SYSTEM_NAME)
  utils_add_executable(${FLAVOR}.${SYSTEM_MNEMONIC}
    EXTENDS ${BASE}
  )
endmacro()
