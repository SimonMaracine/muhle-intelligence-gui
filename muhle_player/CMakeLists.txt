cmake_minimum_required(VERSION 3.20)

add_executable(muhle_player
    "src/board.cpp"
    "src/board.hpp"
    "src/clock.cpp"
    "src/clock.hpp"
    "src/engine.cpp"
    "src/engine.hpp"
    "src/main.cpp"
    "src/muhle_player.cpp"
    "src/muhle_player.hpp"
    "src/subprocess.cpp"
    "src/subprocess.hpp"
)

target_include_directories(muhle_player PRIVATE "src")

target_link_libraries(muhle_player PRIVATE gui_base Boost::process)

if(UNIX)
    target_compile_options(muhle_player PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Wconversion")
elseif(MSVC)
    target_compile_options(muhle_player PRIVATE "/W4")
else()
    message(WARNING "Warnings are not enabled")
endif()

target_compile_features(muhle_player PRIVATE cxx_std_17)
set_target_properties(muhle_player PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(muhle_player PRIVATE "/utf-8")
endif()
