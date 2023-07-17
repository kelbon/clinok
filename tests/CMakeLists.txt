cmake_minimum_required(VERSION 3.21)

add_executable(test1 "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp")
target_link_libraries(test1 PUBLIC clinoklib)
target_include_directories(test1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
add_test(NAME test_cli_kek COMMAND test1)
set_target_properties(test1 PROPERTIES
	CMAKE_CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	CXX_STANDARD 20
	CMAKE_CXX_STANDARD_REQUIRED ON
)
