cmake_minimum_required(VERSION 3.21)

add_executable(clinok-example "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(clinok-example PUBLIC clinoklib)

set_target_properties(clinok-example PROPERTIES
	CMAKE_CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	CXX_STANDARD 20
	CMAKE_CXX_STANDARD_REQUIRED ON
)
