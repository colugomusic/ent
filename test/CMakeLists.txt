cmake_minimum_required(VERSION 3.12)
project(ent_test)

add_executable(ent_test
	${CMAKE_CURRENT_SOURCE_DIR}/../include/ent.hpp
	doctest.h
	main.cpp
)
target_include_directories(ent_test PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/../include
)
set_target_properties(ent_test PROPERTIES
	CXX_STANDARD 17
)
