cmake_minimum_required(VERSION 3.1)

project(nanovg_example)


set(CMAKE_VERBOSE_MAKEFILE ON)

set (BUILD_SHARED_LIBS OFF)
set( GLFW_BUILD_DOCS OFF CACHE BOOL ""  )
set( GLFW_BUILD_TESTS OFF CACHE BOOL ""  )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL ""  )
set( GLFW_BUILD_INSTALL OFF CACHE BOOL ""  )

add_subdirectory(../glfw glfw)

option(BUILD_EXAMPLE_GL3 "Build Example using OpenGL3" ON)
option(BUILD_EXAMPLE_VULKAN "Build Example using Vulkan" ON)
option(DEMO_ANTIALIAS "NVG_ANTIALIAS" ON)
option(DEMO_STENCIL_STROKES "NVG_STENCIL_STROKES" ON)


if (DEMO_ANTIALIAS)
    add_compile_definitions(DEMO_ANTIALIAS)
endif()
if (DEMO_STENCIL_STROKES)
    add_compile_definitions(DEMO_STENCIL_STROKES)
endif()

include_directories(../src)

set(COMMON_SRC
    ../src/fontstash.h
    ../src/nanovg.c
    ../src/nanovg.h
    ../src/stb_image.h
    ../src/stb_truetype.h
    ../src/nanovg.c
    ../example/demo.c
    ../example/perf.c
)


if (BUILD_EXAMPLE_VULKAN)

    find_package(Vulkan)

    if (Vulkan_FOUND)
        message(STATUS "Vulkan found ${Vulkan_LIBRARIES}")
    else()
        message(FATAL "Vulkan not found")
    endif()

    add_executable(
        example-vk
        ${COMMON_SRC}
        ../src/nanovg_vk.h
        ../example/example_vulkan.c
        ../example/vulkan_util.h
    )

    target_include_directories(example-vk PRIVATE ../src)
    target_include_directories(example-vk PRIVATE ${Vulkan_INCLUDE_DIR})
    target_compile_definitions(example-vk PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
    target_link_libraries(example-vk PRIVATE glfw ${Vulkan_LIBRARY})

endif() 

if (BUILD_EXAMPLE_GL3)

    # Build glew
    set( ONLY_LIBS ON CACHE BOOL ""  )
    set( glew-cmake_BUILD_SHARED OFF CACHE BOOL ""  )
    set( glew-cmake_BUILD_STATIC ON CACHE BOOL ""  )
    add_subdirectory(../glew-cmake glew)

    add_executable(
        example-gl3
        ${COMMON_SRC}
        ../src/nanovg_gl.h
        ../src/nanovg_gl_utils.h
        ../example/example_gl3.c
    )

    target_include_directories(example-gl3 PRIVATE ../src)
    target_compile_definitions(example-gl3 PRIVATE NANOVG_GLEW)
    target_link_libraries(example-gl3 PRIVATE glfw libglew_static)

endif()


if (MSVC) 
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set_property(TARGET example-vk example-gl3 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    set_property(TARGET example-vk example-gl3 PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}$<0:...>)
endif()