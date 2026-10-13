cmake_minimum_required(VERSION 3.20)
project(emptying_bookshop LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The engine lives beside the films in this repository.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine ${CMAKE_CURRENT_BINARY_DIR}/engine)

add_executable(emptying-bookshop
  src/main.cpp
  src/script.cpp
  src/kit.cpp
  src/world.cpp
  src/overlay.cpp
  src/music.cpp
  src/pageturn.cpp
  src/chapters/c1.cpp
  src/chapters/c2.cpp
  src/chapters/c3.cpp
  src/chapters/c4.cpp
  src/chapters/c5.cpp
  src/chapters/c6.cpp
  src/chapters/c7.cpp)
target_link_libraries(emptying-bookshop PRIVATE eng)
target_compile_definitions(emptying-bookshop PRIVATE FILM_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT MSVC)
  target_compile_options(emptying-bookshop PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()
