cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(unit-tests)

find_package(Catch2 3 REQUIRED)
find_package(MPI REQUIRED)

add_executable(unit-tests
  WorkSplitter.test.cpp
)

target_include_directories(unit-tests PRIVATE
  ../..
  ${MPI_CXX_INCLUDE_DIRS}
)

target_link_libraries(unit-tests PRIVATE
  Catch2::Catch2WithMain
  ${MPI_CXX_LIBRARIES}
)