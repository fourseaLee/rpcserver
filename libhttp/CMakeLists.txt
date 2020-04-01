cmake_minimum_required(VERSION 3.5)

project(libhttp)

include_directories(./)

set(http_src httpserver.cpp)

ADD_LIBRARY(http ${http_src})

set(basic_link_lib c rt pthread event event_pthreads)

target_link_libraries(http ${basic_link_lib})

