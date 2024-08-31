cmake_minimum_required(VERSION 3.20)
project(ent)

add_library(ent INTERFACE)
target_include_directories(ent INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
