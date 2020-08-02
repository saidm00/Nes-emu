cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(NESEMU LANGUAGES C)

set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=large -m64 -std=c2x -Ofast -Os")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=large -m64 -std=c2x -O0 -g")
endif()

set(ROOT ${CMAKE_CURRENT_LIST_DIR})

# Building GLAD for OpenGL
set(GLAD_PATH ${ROOT}/deps/glad)

add_executable(nesemu
	${GLAD_PATH}/src/glad.c
	src/main.c
	src/stb_truetype.h
	src/nes_cpu.c
	src/nes_cpu.h
	src/nes_cartridge.h
	src/debugger.h
	src/debugger.c)

if (MSYS OR MINGW OR WIN64)
	# Win32 libraries required
	set(GLFW_PATH ${ROOT}/deps/glfw-3.3.2.bin.WIN64)
	set(GLFW_LIBRARY_PATH ${GLFW_PATH}/lib-mingw-w64)
	target_link_options(nesemu PRIVATE "-L${GLFW_LIBRARY_PATH}")
	target_link_libraries(nesemu PRIVATE glfw3 OpenGL32)
	target_include_directories(nesemu PRIVATE "${GLFW_PATH}/include" "${GLAD_PATH}/include" "${ROOT}/deps/glm-c")
elseif (LINUX)
	# Linux libraries required
endif()