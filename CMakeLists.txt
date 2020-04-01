cmake_minimum_required(VERSION 3.5)
project(SimpleBit)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

include_directories(./)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR})

set(LIB_PATH ${PROJECT_BINARY_DIR})

add_subdirectory(libhttp)
add_subdirectory(librpc)
message(STATUS ${PROJECT_BINARY_DIR})
add_subdirectory(httprpc)

