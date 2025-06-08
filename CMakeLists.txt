cmake_minimum_required(VERSION 3.10)
project(SFMLGame)
set(CMAKE_CXX_STANDARD 17)
include_directories(headers)
# Find SFML
set(SFML_DIR "/usr/lib/x86_64-linux-gnu/cmake/SFML")
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add the executable
add_executable(game src/game.cpp)
# Link SFML librari
target_link_libraries(game sfml-graphics sfml-window sfml-system sfml-network sfml-audio)
