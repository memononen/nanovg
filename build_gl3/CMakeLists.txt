cmake_minimum_required(VERSION 3.1)

project(NanoVG_GL3)

set (BUILD_SHARED_LIBS OFF)
set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )
set( ONLY_LIBS ON CACHE BOOL "" FORCE )
set( glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE )

add_subdirectory( ${CMAKE_CURRENT_LIST_DIR}/../glfw glfw)
add_subdirectory( ${CMAKE_CURRENT_LIST_DIR}/../glew-cmake glew)

option(DEMO_ANTIALIAS "NVG_ANTIALIAS" ON)
option(DEMO_STENCIL_STROKES "NVG_STENCIL_STROKES" ON)

if (DEMO_ANTIALIAS)
add_definitions(-DDEMO_ANTIALIAS)
endif()
if (DEMO_STENCIL_STROKES)
add_definitions(-DDEMO_STENCIL_STROKES)
endif()

add_executable(
	nanovg-gl3
    ../src/fontstash.h
    ../src/nanovg.c
    ../src/nanovg.h
    ../src/nanovg_gl.h
    ../src/nanovg_gl_utils.h
    ../src/stb_image.h
    ../src/stb_truetype.h
	../src/nanovg.c 
	../example/example_gl3.c
	../example/demo.c
	../example/perf.c
)

target_include_directories(nanovg-gl3 PRIVATE ../src)

target_compile_definitions(nanovg-gl3 PRIVATE NANOVG_OPENGL NANOVG_GLEW _CRT_SECURE_NO_WARNINGS)
target_link_libraries(nanovg-gl3 PRIVATE glfw libglew_static)
set_target_properties(nanovg-gl3 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
set_target_properties(nanovg-gl3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}$<0:...>)

if (NOT WIN32)
	target_link_libraries(m)
endif()
