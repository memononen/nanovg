cmake_minimum_required(VERSION 2.8)

project("NanoVG_Vulkan")

find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message("ERROR: Vulkan not found")
endif(NOT Vulkan_FOUND)
set(Vulkan_LIBRARY)

if (Vulkan_FOUND)
    MESSAGE(STATUS "  [Found Vulkan library]")
    MESSAGE(STATUS "  Vulkan include directory:" ${Vulkan_INCLUDE_DIR})
    FOREACH(item ${Vulkan_LIBRARIES})
        MESSAGE(STATUS "  using lib: " ${item})
    ENDFOREACH()
    include_directories(${Vulkan_INCLUDE_DIR})
endif()

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

add_subdirectory("glfw")

set(SOURCES
	"../src/nanovg.c" 
	"../example/example_vulkan.c"
	"../example/demo.c"
	"../example/perf.c"
)

include_directories("glfw/include")

include_directories("../src")
add_definitions(-DNANOVG_OPENGL -DNANOVG_VULKAN_IMPLEMENTATION)

add_executable(
		nanovg 
		${SOURCES} 
)

target_link_libraries(nanovg glfw ${Vulkan_LIBRARY})
