cmake_minimum_required(VERSION 3.21)
project(clinok LANGUAGES CXX)
# TODO change file extension
add_library(clinoklib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/cli_interface.hpp")

target_include_directories(clinoklib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(clinoklib PROPERTIES
	CMAKE_CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	CMAKE_CXX_STANDARD_REQUIRED ON
	CXX_STANDARD 20)

if(MSVC)
  target_compile_options(clinoklib INTERFACE "/Zc:__cplusplus" "/Zc:preprocessor" "/permissive-" "/wd5051" "/wd4848")
endif()

include(CTest)
add_subdirectory(tests)
