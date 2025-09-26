cmake_minimum_required(VERSION 3.16)
project(clear_bomb_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CLEAR_BOMB_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(BUILD_TESTS "Build Clear Bomb unit tests" ON)

add_library(clear_bomb_core
    src/GameEngine.cpp
    src/MinesweeperBoard.cpp
    src/AutoMarker.cpp
)

target_include_directories(clear_bomb_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (CLEAR_BOMB_ENABLE_WARNINGS)
    if (MSVC)
        target_compile_options(clear_bomb_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(clear_bomb_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endif()

add_executable(clear_bomb_server
    src/main.cpp
    src/ApiServer.cpp
)

target_link_libraries(clear_bomb_server PRIVATE clear_bomb_core)

find_package(Threads REQUIRED)

target_link_libraries(clear_bomb_core PRIVATE Threads::Threads)
target_link_libraries(clear_bomb_server PRIVATE Threads::Threads)

if (BUILD_TESTS)
    enable_testing()
    add_executable(clear_bomb_tests tests/GameEngineTests.cpp)
    target_link_libraries(clear_bomb_tests PRIVATE clear_bomb_core)
    add_test(NAME GameEngineSmokeTests COMMAND clear_bomb_tests)
endif()
