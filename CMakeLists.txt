#
# Manual is handy to grasp what's going on here:
# http://www.cmake.org/cmake/help/v2.8.8/cmake.html
#
cmake_minimum_required (VERSION 2.6)
project (chimp)

find_package (BISON)
find_package (FLEX)

set (SCANNER_L ${CMAKE_CURRENT_SOURCE_DIR}/libchimp/scanner.l)
set (SCANNER_C ${CMAKE_CURRENT_BINARY_DIR}/libchimp/scanner.c)

set (PARSER_Y ${CMAKE_CURRENT_SOURCE_DIR}/libchimp/_parser.y)
set (PARSER_C ${CMAKE_CURRENT_BINARY_DIR}/libchimp/_parser.c)
set (PARSER_H ${CMAKE_CURRENT_BINARY_DIR}/libchimp/include/chimp/_parser.h)

set (AST_C ${CMAKE_CURRENT_BINARY_DIR}/libchimp/ast.c)
set (AST_H ${CMAKE_CURRENT_BINARY_DIR}/libchimp/include/chimp/ast.h)

set (_TEST_MAIN_C ${CMAKE_CURRENT_BINARY_DIR}/test/_test_main.c)

#
# TODO CFLAGS env var seems to be ignored by cmake ... ?
#
set (CMAKE_C_FLAGS "-Wall -Werror -Wno-error=nonnull -O0 -g3")

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

set_source_files_properties (libchimp/compile.c
  PROPERTIES OBJECT_DEPENDS ${PARSER_H})

set_source_files_properties (${CMAKE_CURRENT_BINARY_DIR}/libchimp/scanner.c
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

include_directories ("${CMAKE_CURRENT_BINARY_DIR}/libchimp/include" "${PROJECT_SOURCE_DIR}/libchimp/include")
add_library (libchimp STATIC
  ${PARSER_C}
  libchimp/array.c
  ${AST_C}
  libchimp/class.c
  libchimp/code.c
  libchimp/compile.c
  libchimp/core.c
  libchimp/frame.c
  libchimp/gc.c
  libchimp/hash.c
  libchimp/int.c
  libchimp/lwhash.c
  libchimp/method.c
  libchimp/module.c
  libchimp/msg.c
  libchimp/object.c
  ${SCANNER_C}
  libchimp/str.c
  libchimp/symtable.c
  libchimp/task.c
  libchimp/var.c
  libchimp/vm.c
  libchimp/error.c
  libchimp/modules/assertmod.c
  libchimp/modules/gcmod.c
  libchimp/modules/iomod.c
  libchimp/modules/osmod.c)
#
# Default behaviour is to generate liblibchimp.a ...
#
set_target_properties (libchimp PROPERTIES OUTPUT_NAME chimp)

add_executable (chimp main.c)
target_link_libraries (chimp libchimp ${FLEX_LIBRARIES} ${BISON_LIBRARIES})

#
# TODO test for the presence of check
#
add_executable (test-native
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_BINARY_DIR}/test/_test_main.c)
target_link_libraries (test-native libchimp ${FLEX_LIBRARIES} ${BISON_LIBRARIES} check)
add_custom_target (test test-native DEPENDS test-native)

