cmake_minimum_required(VERSION 2.8)

project("NanoVG_Vulkan")

find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message("ERROR: OpenGL not found")
endif(NOT OPENGL_FOUND)
set(GL_LIBRARY GL GLU X11)
set(SOURCES
	"../src/nanovg.c" 
	"../example/example_gl3.c"
	"../example/demo.c"
	"../example/perf.c"
)

include_directories("../src")
add_definitions(-DNANOVG_OPENGL)

add_executable(
		nanovg 
		${SOURCES} 
)

target_link_libraries(nanovg glfw ${GL_LIBRARY} m)
