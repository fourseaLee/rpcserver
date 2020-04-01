cmake_minimum_required(VERSION 3.5)

project(librpc)

set(rpc_src protocol.cpp
            server.cpp
            client.cpp
			httprpc.cpp
			fs.cpp
			)
		
ADD_LIBRARY(rpc ${rpc_src})

target_link_libraries(rpc http boost_system boost_chrono boost_program_options boost_filesystem boost_thread)

