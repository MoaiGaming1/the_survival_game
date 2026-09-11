cmake_minimum_required(VERSION 3.15)

project(TheSurvivalGame VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Threads REQUIRED)

set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS ON)
set(CROSS_PLATFORM_DETERMINISTIC OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extern/JoltPhysics/Build)

add_executable(tsg src/main.cpp src/glad.c)

file(GLOB_RECURSE ALL_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/assets_copied
    DEPENDS ${ALL_ASSETS}
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_SOURCE_DIR}/assets 
        ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/assets_copied
    COMMENT "Copying assets to build directory"
)

add_custom_target(CopyAssets ALL DEPENDS ${CMAKE_BINARY_DIR}/assets_copied)

add_dependencies(tsg CopyAssets)

target_include_directories(tsg PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glad
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glfw
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glm
	${CMAKE_CURRENT_SOURCE_DIR}/extern/JoltPhysics
)

if(APPLE)
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(IOKIT_LIBRARY IOKit REQUIRED)
	find_library(OPENGL_LIBRARY OpenGL REQUIRED)
	find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
	find_library(QUARTZCORE_LIBRARY QuartzCore REQUIRED)

	target_link_libraries(tsg PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libglfw3.a
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
		${OPENGL_LIBRARY}
		${COREVIDEO_LIBRARY}
		${QUARTZCORE_LIBRARY}
		Threads::Threads
		Jolt
	)
elseif(UNIX)
	target_link_libraries(tsg PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libglfw3.a
		GL
		m
		Threads::Threads
		Jolt
	)
endif()
