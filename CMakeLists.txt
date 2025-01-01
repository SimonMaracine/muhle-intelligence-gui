cmake_minimum_required(VERSION 3.20)

project("muhle-intelligence-gui" LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Variable used to control the build type")

set(GUI_BASE_INCLUDE_FILE_DIALOG ON)
set(GLFW_BUILD_X11 OFF)  # TODO temp

add_subdirectory(extern/tiny-gui-base)

add_subdirectory(muhle_player)

message(STATUS "Muhle: Build type: ${CMAKE_BUILD_TYPE}")
