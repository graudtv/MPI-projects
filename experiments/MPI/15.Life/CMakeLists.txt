cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(Life)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(MPI REQUIRED C)
find_package(OpenGL REQUIRED)

set(ROOT_DIR ${CMAKE_SOURCE_DIR}/../../..)

add_library(ImGui
  ${ROOT_DIR}/external/imgui/imgui.cpp
  ${ROOT_DIR}/external/imgui/imgui_demo.cpp
  ${ROOT_DIR}/external/imgui/imgui_draw.cpp
  ${ROOT_DIR}/external/imgui/imgui_tables.cpp
  ${ROOT_DIR}/external/imgui/imgui_widgets.cpp
  ${ROOT_DIR}/external/imgui-sfml/imgui-SFML.cpp
)

target_include_directories(ImGui PUBLIC
  /usr/local/include # SFML
  ${ROOT_DIR}/external/imgui
  ${ROOT_DIR}/external/imgui-sfml
)

target_compile_definitions(ImGui PRIVATE GL_SILENCE_DEPRECATION)

add_executable(life
  main.cpp
)

# SFML is not properly detected on my platform, this can be fixed later
target_link_directories(life PRIVATE /usr/local/lib)
target_include_directories(life PRIVATE /usr/local/include)
target_link_libraries(life PRIVATE sfml-graphics sfml-window sfml-system)

target_include_directories(life PRIVATE
  ${ROOT_DIR}
  ${MPI_C_INCLUDE_DIRS}
)

target_link_libraries(life PRIVATE
  ${MPI_C_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ImGui
)