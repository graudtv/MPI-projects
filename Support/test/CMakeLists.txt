cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(unit-tests)

find_package(Catch2 3 REQUIRED)

add_executable(unit-tests Algorithm.test.cpp)

target_link_libraries(unit-tests PRIVATE Catch2::Catch2WithMain)