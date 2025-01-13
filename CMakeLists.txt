cmake_minimum_required(VERSION 3.20)

project("muhle-intelligence-gui" LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Variable used to control the build type")

set(GUI_BASE_INCLUDE_FILE_DIALOG ON)
set(GLFW_BUILD_X11 OFF)  # TODO temp

set(BOOST_INCLUDE_LIBRARIES
    process
    algorithm
    array
    asio
    assert
    align
    config
    bind
    core
    context
    concept_check
    static_assert
    date_time
    mp11
    filesystem
    exception
    throw_exception
    system
    preprocessor
    pool
    io
    container_hash
    fusion
    function
    type_traits
    describe
    predef
    lexical_cast
    smart_ptr
    detail
    function_types
    integer
    iterator
    variant2
    mpl
    tuple
    scope
    container
    winapi
    move
    range
    intrusive
    atomic
    typeof
    optional
    utility
    conversion
    regex
    tokenizer
    functional
    unordered
    type_index
)

add_subdirectory(extern/tiny-gui-base)
add_subdirectory(extern/boost EXCLUDE_FROM_ALL)

add_subdirectory(muhle_player)

message(STATUS "Muhle: Build type: ${CMAKE_BUILD_TYPE}")
