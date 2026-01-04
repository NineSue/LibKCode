cmake_minimum_required(VERSION 3.20)
project(kcode C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

include_directories(
        ${CMAKE_SOURCE_DIR}/lib/include
        ${CMAKE_SOURCE_DIR}/kernel
)

# C 库
file(GLOB LIB_SOURCES "lib/src/*.c")
if(LIB_SOURCES)
    add_library(kcode_lib SHARED ${LIB_SOURCES})
    target_include_directories(kcode_lib PRIVATE lib/internal)
    target_link_libraries(kcode_lib pthread)
endif()

# 测试 - tests/lib/
file(GLOB TEST_LIB_SOURCES "tests/lib/test_*.c")
foreach(test_file ${TEST_LIB_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/lib/internal
            ${CMAKE_SOURCE_DIR}/kernel)
    target_link_libraries(${test_name} kcode_lib pthread)
endforeach()

# C++ 性能测试 - benchmark.cpp
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/lib/benchmark.cpp")
    add_executable(benchmark tests/lib/benchmark.cpp)
    target_include_directories(benchmark PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/lib/internal
            ${CMAKE_SOURCE_DIR}/kernel)
    target_link_libraries(benchmark kcode_lib pthread)
endif()

# 测试 - tests/kernel/
file(GLOB TEST_KERNEL_SOURCES "tests/kernel/*.c")
foreach(test_file ${TEST_KERNEL_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
endforeach()
