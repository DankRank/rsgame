cmake_minimum_required(VERSION 3.11)
project(rsgame)
option(BUILD_LOCALCLIENT "build localclient" ON)
option(BUILD_NETCLIENT "build netclient" ON)
option(BUILD_SERVER "build server" ON)
if (BUILD_LOCALCLIENT OR BUILD_NETCLIENT)
	find_package(SDL2 REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(PNG REQUIRED)
	find_package(epoxy)
	if(NOT(epoxy_FOUND))
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
		add_library(epoxy::epoxy INTERFACE IMPORTED)
		target_link_libraries(epoxy::epoxy INTERFACE PkgConfig::EPOXY)
	endif()
endif()
add_subdirectory(extlib/glm)

add_library(rsgame_common INTERFACE IMPORTED)
if(MSVC)
	set_target_properties(rsgame_common PROPERTIES
		INTERFACE_COMPILE_OPTIONS "/Zc:__cplusplus;/D_CRT_SECURE_NO_WARNINGS;/W3;/wd4267;/wd4244")
else()
	set_target_properties(rsgame_common PROPERTIES
		INTERFACE_COMPILE_OPTIONS "-Wall;-Wextra;-Wno-parentheses")
endif()
set_target_properties(rsgame_common PROPERTIES
	INTERFACE_COMPILE_FEATURES cxx_std_17)
if(WIN32)
	option(RSGAME_BUNDLE "look for assets within the exe file" OFF)
	set_target_properties(rsgame_common PROPERTIES
		INTERFACE_COMPILE_DEFINITIONS "WINVER=0x0600;_WIN32_WINNT=0x0600;$<$<BOOL:${RSGAME_BUNDLE}>:RSGAME_BUNDLE>")
endif()
if(NOT(CMAKE_VERSION VERSION_LESS 3.16.0))
	set_target_properties(rsgame_common PROPERTIES
		INTERFACE_PRECOMPILE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/common.hh")
endif()

set(SOURCES_COMMON
	src/common.hh
	src/level.cc src/level.hh
	src/tile.cc src/tile.hh)
set(SOURCES_CLIENT
	src/main.cc
	src/render.cc src/render.hh
	src/util.cc src/util.hh
	src/raycast.cc src/raycast.hh)
set(SOURCES_SERVER
	src/maind.cc
	src/net.hh)

if(BUILD_LOCALCLIENT)
	add_executable(rsgame  ${SOURCES_COMMON} ${SOURCES_CLIENT})
	target_link_libraries(rsgame  PRIVATE rsgame_common SDL2::SDL2 SDL2::SDL2main OpenGL::GL epoxy::epoxy glm::glm PNG::PNG)
endif()
if(BUILD_NETCLIENT)
	add_executable(rsgamec ${SOURCES_COMMON} ${SOURCES_CLIENT} src/net.hh)
	target_link_libraries(rsgamec PRIVATE rsgame_common SDL2::SDL2 SDL2::SDL2main OpenGL::GL epoxy::epoxy glm::glm PNG::PNG $<$<BOOL:${WIN32}>:ws2_32>)
	target_compile_definitions(rsgamec PRIVATE RSGAME_NETCLIENT)
endif()
if(BUILD_SERVER)
	add_executable(rsgamed ${SOURCES_COMMON} ${SOURCES_SERVER})
	target_link_libraries(rsgamed PRIVATE rsgame_common glm::glm $<$<BOOL:${WIN32}>:ws2_32>)
	target_compile_definitions(rsgamed PRIVATE RSGAME_SERVER)
endif()
