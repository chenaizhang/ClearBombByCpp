cmake_minimum_required(VERSION 3.16)
project(clear_bomb_backend LANGUAGES CXX)

# TODO: Set the C++ standard once target platforms are finalized.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: Allow user to toggle sanitizers, warnings-as-errors, LTO, etc., via options.
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

add_executable(clear_bomb_server
    src/main.cpp
    src/ApiServer.cpp
)

target_link_libraries(clear_bomb_server PRIVATE clear_bomb_core)

# TODO: Link HTTP/WebSocket library (e.g., Drogon, Crow, Restinio) once selected.
# TODO: Add unit tests and integrate with CTest.
