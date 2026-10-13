cmake_minimum_required(VERSION 3.20)
project(eng LANGUAGES CXX)

# A small C++/Vulkan engine for rendering films offline: scene graph, geometry, lighting with sun
# shadows, bloom and grading, 2D text and shapes for overlays, and MP4 output through ffmpeg.

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FT2 REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(HB REQUIRED IMPORTED_TARGET harfbuzz)
pkg_check_modules(PNG REQUIRED IMPORTED_TARGET libpng)
find_program(GLSLC glslc REQUIRED)

# Compile each shader to SPIR-V as a C array initialiser that the renderer #includes.
set(ENG_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(ENG_SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${ENG_SHADER_OUT})
set(ENG_SHADERS mesh.vert points.vert sprite.vert mesh.frag shadow.vert sky.vert sky.frag fullscreen.vert bright.frag blur.frag final.frag)
set(ENG_SHADER_INCS)
foreach(s ${ENG_SHADERS})
  set(out ${ENG_SHADER_OUT}/${s}.inc)
  add_custom_command(
    OUTPUT ${out}
    COMMAND ${GLSLC} --target-env=vulkan1.2 -O -mfmt=c -I ${ENG_SHADER_DIR} -o ${out} ${ENG_SHADER_DIR}/${s}
    DEPENDS ${ENG_SHADER_DIR}/${s} ${ENG_SHADER_DIR}/common.glsl
    COMMENT "glslc ${s}")
  list(APPEND ENG_SHADER_INCS ${out})
endforeach()
add_custom_target(eng_shaders DEPENDS ${ENG_SHADER_INCS})

add_library(eng STATIC
  src/math.cpp
  src/scene.cpp
  src/geometry.cpp
  src/curve.cpp
  src/canvas.cpp
  src/text.cpp
  src/renderer.cpp
  src/video.cpp)
add_dependencies(eng eng_shaders)
target_compile_features(eng PUBLIC cxx_std_20)
target_include_directories(eng PUBLIC include PRIVATE ${ENG_SHADER_OUT})
target_link_libraries(eng PUBLIC Vulkan::Vulkan PkgConfig::FT2 PkgConfig::HB PkgConfig::PNG)
if(NOT MSVC)
  target_compile_options(eng PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()
