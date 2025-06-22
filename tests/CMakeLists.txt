cmake_minimum_required(VERSION 3.21)

add_executable(test_clinok "${CMAKE_CURRENT_SOURCE_DIR}/test_clinok.cpp")
target_link_libraries(test_clinok PUBLIC clinoklib)

add_test(NAME test_clinok COMMAND test_clinok)
set_target_properties(test_clinok PROPERTIES
	CMAKE_CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	CXX_STANDARD 20
	CMAKE_CXX_STANDARD_REQUIRED ON
)
