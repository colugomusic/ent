cmake_minimum_required(VERSION 3.20)
project(ent)
add_library(ent INTERFACE include/ent.hpp)
add_library(ent::ent ALIAS ent)
target_sources(ent INTERFACE
	FILE_SET HEADERS
	BASE_DIRS include
	FILES
		include/ent.hpp
)
target_compile_features(ent INTERFACE cxx_std_20)
install(TARGETS ent FILE_SET HEADERS)