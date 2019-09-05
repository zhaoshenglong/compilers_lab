project(tiger-compiler)

cmake_minimum_required(VERSION 2.8)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif()


set(CMAKE_CXX_STANDARD 17)

include_directories(src)

# chapter 1
file(GLOB SLP_SOURCES "src/straightline/*.cc")

# Lab 1
add_executable(test_slp ${SLP_SOURCES})
