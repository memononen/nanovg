cmake_minimum_required(VERSION 2.8)

project("NanoVG_Vulkan")

find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message("ERROR: Vulkan not found")
endif(NOT Vulkan_FOUND)
set(Vulkan_LIBRARY)
set(SOURCES
	"../src/nanovg.c" 
	"../example/example_vulkan.c"
	"../example/demo.c"
	"../example/perf.c"
)

include_directories("../src")
add_definitions(-DNANOVG_OPENGL -DGL_QUERY_RESULT_AVAILABLE=0 -DNVG_NO_GPU_TIMER -DNANOVG_VULKAN_IMPLEMENTATION)

add_executable(
		nanovg 
		${SOURCES} 
)

target_link_libraries(nanovg glfw ${Vulkan_LIBRARY} m)
