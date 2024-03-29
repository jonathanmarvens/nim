#
# Manual is handy to grasp what's going on here:
# http://www.cmake.org/cmake/help/v2.8.8/cmake.html
#
cmake_minimum_required (VERSION 2.6)
project (nim)

set (NIM_VERSION "0.0.1")

find_package (BISON REQUIRED)
find_package (FLEX REQUIRED)
find_package (Threads REQUIRED)

set (LIB_SEARCH_PATH
        ${LIBRTDIR}/lib
        /usr/local/lib64
        /usr/local/lib
        /usr/lib64
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/i386-linux-gnu)

find_file(LIBRT_LIBRARIES librt.a PATHS ${LIB_SEARCH_PATH})

if (NOT LIBRT_LIBRARIES)
    find_library (LIBRT_LIBRARIES rt PATHS ${LIB_SEARCH_PATH})
    if (NOT LIBRT_LIBRARIES)
        set (LIBRT_LIBRARIES "")
    endif()
endif()

message (STATUS "librt: ${LIBRT_LIBRARIES}")

if (APPLE)
  find_library (COREFOUNDATION_LIBRARY CoreFoundation)
  message (STATUS "CoreFoundation: ${COREFOUNDATION_LIBRARY}")

  find_library (CORESERVICES_LIBRARY CoreServices)
  message (STATUS "CoreServices: ${CORESERVICES_LIBRARY}")
endif()

#
# Find valgrind
#
set (VALGRIND_HOME "$ENV{VALGRIND_HOME}")

if (VALGRIND_HOME)
    message (STATUS "VALGRIND_HOME: ${VALGRIND_HOME}")
endif (VALGRIND_HOME)

find_path(VALGRIND_INCLUDE_DIR memcheck.h
  /usr/include /usr/include/valgrind /usr/local/include /usr/local/include/valgrind
  ${VALGRIND_HOME}/include ${VALGRIND_HOME}/include/valgrind)

#
# TODO CFLAGS env var seems to be ignored by cmake ... ?
#
set (CMAKE_C_FLAGS "$ENV{CFLAGS}")

if (VALGRIND_INCLUDE_DIR)
    message (STATUS "memcheck.h [valgrind] found in ${VALGRIND_INCLUDE_DIR}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_VALGRIND=1 -I${VALGRIND_INCLUDE_DIR}")
else (VALGRIND_INCLUDE_DIR)
    message (STATUS "memcheck.h [valgrind] not found (valgrind warnings ahoy)")
endif (VALGRIND_INCLUDE_DIR)

set (SCANNER_L ${CMAKE_CURRENT_SOURCE_DIR}/libnim/scanner.l)
set (SCANNER_C ${CMAKE_CURRENT_BINARY_DIR}/libnim/scanner.c)

set (PARSER_Y ${CMAKE_CURRENT_SOURCE_DIR}/libnim/_parser.y)
set (PARSER_C ${CMAKE_CURRENT_BINARY_DIR}/libnim/_parser.c)
set (PARSER_H ${CMAKE_CURRENT_BINARY_DIR}/libnim/include/nim/_parser.h)

set (AST_C ${CMAKE_CURRENT_BINARY_DIR}/libnim/ast.c)
set (AST_H ${CMAKE_CURRENT_BINARY_DIR}/libnim/include/nim/ast.h)

set (_TEST_MAIN_C ${CMAKE_CURRENT_BINARY_DIR}/test/_test_main.c)

#
# XXX standard flex/bison macros left me doing a strange song & dance to
#     get the _parser.h header in the right place. Felt better to just
#     "do it right" rather than hack around the macros.
#
#     Could be a lot more DRY.
#
# ---- <FLEX-AND-BISON> ----
#
add_custom_command (OUTPUT ${SCANNER_C}
  COMMAND ${FLEX_EXECUTABLE}
  ARGS -o${SCANNER_C} ${SCANNER_L}
  MAIN_DEPENDENCY ${SCANNER_L}
  COMMENT "[flex] generating scanner"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command (OUTPUT ${PARSER_C} ${PARSER_H}
  COMMAND ${BISON_EXECUTABLE}
  ARGS -o${PARSER_C} --defines=${PARSER_H} ${PARSER_Y}
  MAIN_DEPENDENCY ${PARSER_Y}
  COMMENT "[bison] generating parser"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_source_files_properties (libnim/compile.c
  PROPERTIES OBJECT_DEPENDS ${PARSER_H})

set_source_files_properties (${CMAKE_CURRENT_BINARY_DIR}/libnim/scanner.c
  PROPERTIES OBJECT_DEPENDS ${PARSER_H})
#
# ---- </FLEX-AND-BISON> ----
#

#
# ---- <AST> ----
#
add_custom_command (
  OUTPUT ${AST_C} ${AST_H}
  COMMAND ${PROJECT_SOURCE_DIR}/script/astgen
  ARGS ${AST_H} ${AST_C}
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/script/astgen
  COMMENT "[astgen] generating ast"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#
# ---- </AST> ----
#

#
# ---- <TEST-DRIVER> ----
#
add_custom_command (
  OUTPUT ${_TEST_MAIN_C}
  COMMAND ${PROJECT_SOURCE_DIR}/script/testgen
  ARGS ${PROJECT_SOURCE_DIR}/test "${_TEST_MAIN_C}"
  COMMENT "[testgen] generating test driver")
#
# ---- </TEST-DRIVER> ----
#

include_directories ("${CMAKE_CURRENT_BINARY_DIR}/libnim/include" "${PROJECT_SOURCE_DIR}/libnim/include")
add_library (libnim STATIC
  ${PARSER_C}
  libnim/array.c
  ${AST_C}
  libnim/class.c
  libnim/code.c
  libnim/compile.c
  libnim/core.c
  libnim/frame.c
  libnim/gc.c
  libnim/hash.c
  libnim/int.c
  libnim/float.c
  libnim/lwhash.c
  libnim/method.c
  libnim/module.c
  libnim/msg.c
  libnim/object.c
  ${SCANNER_C}
  libnim/str.c
  libnim/symtable.c
  libnim/task.c
  libnim/var.c
  libnim/vm.c
  libnim/error.c
  libnim/module_mgr.c
  libnim/modules/net.c
  libnim/modules/assert.c
  libnim/modules/nimunit.c
  libnim/modules/gc.c
  libnim/modules/io.c
  libnim/modules/os.c
  libnim/modules/http/http.c
  libnim/modules/http/http-parser-2.0/http_parser.c)
#
# Default behaviour is to generate liblibnim.a ...
#
set_target_properties (libnim PROPERTIES OUTPUT_NAME nim)
set_target_properties (libnim PROPERTIES
  COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/libnim/modules/http/http-parser-2.0")

set (NIM_LIBRARIES
    libnim
    ${FLEX_LIBRARIES}
    ${BISON_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${COREFOUNDATION_LIBRARY}
    ${CORESERVICES_LIBRARY}
    ${LIBRT_LIBRARIES})

add_executable (nim main.c)
target_link_libraries (nim ${NIM_LIBRARIES})

#
# TODO test for the presence of check
#
add_executable (test-native
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_BINARY_DIR}/test/_test_main.c)
target_link_libraries (test-native ${NIM_LIBRARIES} check)

add_custom_target (test test-native
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/script/test
  DEPENDS test-native nim)

add_custom_target (dist 
    COMMAND git archive --format=tar --prefix=${CMAKE_PROJECT_NAME}-${NIM_VERSION}/ master | gzip -9 >${CMAKE_PROJECT_NAME}-${NIM_VERSION}.tar.gz)

install(PROGRAMS nim DESTINATION bin)

