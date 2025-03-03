cmake_minimum_required(VERSION 3.20)
project(ent)
add_library(ent INTERFACE include/ent.hpp)
add_library(ent::ent ALIAS ent)
target_sources(ent INTERFACE
	FILE_SET ent
	TYPE HEADERS
	BASE_DIRS include
	FILES
		include/ent.hpp
)
target_include_directories(ent INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
