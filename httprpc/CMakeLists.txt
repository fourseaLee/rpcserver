cmake_minimum_required(VERSION 3.5)

project(httprpc)

add_executable(rpcserver main.cpp)

message(STATUS ${LIB_PATH})
link_directories(${LIB_PATH})
target_link_libraries(
						rpcserver
						rpc)


