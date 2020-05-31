cmake_minimum_required(VERSION 2.8)

project (libcoro-cpp)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wextra")
elseif (WIN32)
    add_compile_options("/std:c++17")
endif()


add_library(coro
    coro_await.hpp
    coroutine.hpp
    coroutine.cpp
    coroutine_cache.hpp
    mutex.hpp
    mutex.cpp
    when_all.hpp
    detail/libcoro/coro.h
    detail/libcoro/coro.c)
target_include_directories(coro
    PRIVATE detail .)

enable_testing()
find_package(GTest REQUIRED)

add_executable(coro_test
    test/coro_test.cpp
    test/coro_test_base.cpp
    test/coro_test_cache.cpp
    test/coro_test_mutex.cpp
    test/coro_test_wait.cpp)

target_include_directories(coro_test
    PRIVATE . ${GTEST_INCLUDE_DIRS})
target_link_libraries(coro_test ${GTEST_BOTH_LIBRARIES} coro)
