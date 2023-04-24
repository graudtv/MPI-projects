cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(unit-tests)

find_package(Catch2 3 REQUIRED)
find_package(MPI REQUIRED C)

add_executable(unit-tests
  WorkSplitter.test.cpp
)

target_include_directories(unit-tests PRIVATE
  ../..
  ${MPI_C_INCLUDE_DIRS}
)

target_link_libraries(unit-tests PRIVATE
  Catch2::Catch2WithMain
  ${MPI_C_LIBRARIES}
)