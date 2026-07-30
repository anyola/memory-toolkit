cmake_minimum_required(VERSION 3.16)
project(mem_toolkit CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# Sanitizers
option(MEM_ENABLE_ASAN "Enable ASan + UBSan" OFF)
option(MEM_ENABLE_TSAN "Enable TSan" OFF)
 
if(MEM_ENABLE_ASAN AND MEM_ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be enabled at the same time")
endif()
if(MEM_ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()
if(MEM_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()
 
# Main library (header-only)
add_library(mem_toolkit INTERFACE)
target_include_directories(mem_toolkit INTERFACE ${CMAKE_SOURCE_DIR}/include)
 
# GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
find_package(Threads REQUIRED)
 
enable_testing()
 
# Main test suite
add_executable(mtk_tests test/all_tests.cpp)
target_link_libraries(mtk_tests PRIVATE mem_toolkit GTest::gtest_main Threads::Threads)
add_test(NAME mtk_tests COMMAND mtk_tests)
 
# Single-allocation proof (no GTest — conflicts with ASan)
add_executable(make_shared_allocation_check test/make_shared_allocation_check.cpp)
target_link_libraries(make_shared_allocation_check PRIVATE mem_toolkit)
add_test(NAME make_shared_allocation_check COMMAND make_shared_allocation_check)