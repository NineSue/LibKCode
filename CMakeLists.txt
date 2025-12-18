cmake_minimum_required(VERSION 3.20)
project(kcode C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)

include_directories(lib/include kernel)

# C 库
file(GLOB LIB_SOURCES "lib/src/*.c")
if(LIB_SOURCES)
    add_library(kcode_lib SHARED ${LIB_SOURCES})
    target_link_libraries(kcode_lib pthread)
endif()

# 测试 - tests/c/
file(GLOB TEST_C_SOURCES "tests/c/*.c")
foreach(test_file ${TEST_C_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} pthread)
endforeach()

# 测试 - tests/kernel/
file(GLOB TEST_KERNEL_SOURCES "tests/kernel/*.c")
foreach(test_file ${TEST_KERNEL_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
endforeach()
