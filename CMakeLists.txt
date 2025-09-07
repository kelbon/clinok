cmake_minimum_required(VERSION 3.21)
project(clinok LANGUAGES CXX)

option(CLINOK_ENABLE_TESTING "enables testing" OFF)
option(CLINOK_ENABLE_EXAMPLES "enables examples" OFF)
option(CLINOK_SANITIZE "enables ASAN/UNSAN" OFF)

if (CLINOK_SANITIZE)
  # fno omit frame pointer to improve trace from sanitizers
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

add_library(clinoklib STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/clinok/utils.cpp")

target_include_directories(clinoklib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(clinoklib PROPERTIES
	CMAKE_CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	CMAKE_CXX_STANDARD_REQUIRED ON
	CXX_STANDARD 20)

if(MSVC)
  target_compile_options(clinoklib INTERFACE "/Zc:__cplusplus" "/Zc:preprocessor" "/permissive-" "/wd5051" "/wd4848")
endif()

if (CLINOK_ENABLE_EXAMPLES)
	add_subdirectory(examples)
endif()

if (CLINOK_ENABLE_TESTING)
	include(CTest)
	add_subdirectory(tests)
endif()
