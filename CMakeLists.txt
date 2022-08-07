cmake_minimum_required(VERSION 3.11)
project(rsgame)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
add_subdirectory(extlib/glm)

add_executable(rsgame
	src/common.hh
	src/main.cc
	src/render.cc src/render.hh
	src/level.cc src/level.hh
	src/tile.cc src/tile.hh
	src/util.cc src/util.hh
)
target_link_libraries(rsgame PRIVATE SDL2::SDL2 SDL2::SDL2main OpenGL::GL PkgConfig::EPOXY glm::glm PNG::PNG)
target_compile_features(rsgame PRIVATE cxx_std_17)
if(MSVC)
	target_compile_options(rsgame PRIVATE /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS /W3 /wd4267 /wd4244)
else()
	target_compile_options(rsgame PRIVATE -Wall -Wextra -Wno-parentheses)
endif()
if(NOT(CMAKE_VERSION VERSION_LESS 3.16.0))
	target_precompile_headers(rsgame PRIVATE src/common.hh)
endif()
