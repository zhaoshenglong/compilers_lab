project(tiger-compiler)

cmake_minimum_required(VERSION 2.8)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()

find_program(FLEXCPP flexc++)
if(NOT FLEXCPP)
    message(FATAL_ERROR "Program flexc++ not found!")
endif()

find_program(BISONCPP bisonc++)
if(NOT BISONCPP)
    message(FATAL_ERROR "Program bison++ not found!")
endif()

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(src/tiger/lex)
include_directories(src/tiger/parse)


file(GLOB SLP_SOURCES "src/straightline/*.cc")

file(GLOB TIGER_SOURCES
  "src/tiger/symbol/*.cc"
  "src/tiger/absyn/*.cc"
  "src/tiger/errormsg/*.cc"
  "src/tiger/env/*.cc"
  "src/tiger/escape/*.cc"
  "src/tiger/lex/*.cc"
  "src/tiger/parse/*.cc"
  "src/tiger/semant/*.cc"
  "src/tiger/frame/*.cc"
  "src/tiger/translate/*.cc"
  "src/tiger/canon/*.cc"
  "src/tiger/codegen/*.cc"
  "src/tiger/liveness/*.cc"
  "src/tiger/regalloc/*.cc"
)

add_custom_target(lex_files
    COMMAND  flexc++ tiger.lex
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/tiger/lex
)

add_custom_target(parse_files
  COMMAND  bisonc++ tiger.y
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/tiger/parse
)

# lab 1
add_executable(test_slp ${SLP_SOURCES})

# lab 2
add_executable(test_lex  "src/tiger/main/test_lex.cc" ${TIGER_SOURCES})
add_dependencies(test_lex lex_files parse_files)

# lab 3
add_executable(test_parse  "src/tiger/main/test_parse.cc" ${TIGER_SOURCES})
add_dependencies(test_parse lex_files parse_files)

# lab 4
add_executable(test_semant  "src/tiger/main/test_semant.cc" ${TIGER_SOURCES})
add_dependencies(test_semant lex_files parse_files)

# lab 5
add_executable(test_translate  "src/tiger/main/test_translate.cc" ${TIGER_SOURCES})
add_dependencies(test_translate lex_files parse_files)

# lab 6
add_executable(tiger-compiler  "src/tiger/main/main.cc" ${TIGER_SOURCES})
add_dependencies(tiger-compiler lex_files parse_files)
