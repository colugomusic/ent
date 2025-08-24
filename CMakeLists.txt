cmake_minimum_required(VERSION 3.30)
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
include(CMakePackageConfigHelpers)
install(TARGETS ent EXPORT ent-targets FILE_SET HEADERS)
install(EXPORT ent-targets FILE ent-targets.cmake NAMESPACE ent:: DESTINATION lib/cmake/ent)
configure_package_config_file("${CMAKE_CURRENT_LIST_DIR}/cmake/ent-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/ent-config.cmake" INSTALL_DESTINATION lib/cmake/ent)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ent-config.cmake" DESTINATION lib/cmake/ent)