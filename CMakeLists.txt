cmake_minimum_required(VERSION 3.10)

project(Tutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(Tutorial src/main.cpp)

target_link_libraries(Tutorial
  PRIVATE
    Vulkan::Headers
    glfw
)

