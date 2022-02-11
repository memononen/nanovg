cmake_minimum_required(VERSION 3.1)

project(nanovg_example)

set (BUILD_SHARED_LIBS OFF)
set( GLFW_BUILD_DOCS OFF CACHE BOOL ""  )
set( GLFW_BUILD_TESTS OFF CACHE BOOL ""  )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL ""  )
set( GLFW_BUILD_INSTALL OFF CACHE BOOL ""  )

add_subdirectory(../glfw glfw)

option(BUILD_USE_GLAD "Use glad instead of glew" ON)

option(BUILD_EXAMPLE_GL3 "Build Example using OpenGL3" ON)
option(BUILD_EXAMPLE_VULKAN "Build Example using Vulkan" ON)

option(DEMO_ANTIALIAS "Use NVG_ANTIALIAS" ON)
option(DEMO_MSAA "Use MSAA instead of NVG_ANTIALIAS" OFF)
option(DEMO_STENCIL_STROKES "NVG_STENCIL_STROKES" ON)

add_compile_definitions(DEMO_MSAA=$<BOOL:${DEMO_MSAA}>)
add_compile_definitions(DEMO_ANTIALIAS=$<BOOL:${DEMO_ANTIALIAS}>)
add_compile_definitions(DEMO_STENCIL_STROKES=$<BOOL:${DEMO_STENCIL_STROKES}>)

include_directories(../src)

set(COMMON_SRC
    ../src/fontstash.h
    ../src/nanovg.c
    ../src/nanovg.h
    ../src/stb_image.h
    ../src/stb_truetype.h
    ../src/nanovg.c
    demo.c
    perf.c
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
        example_vulkan.c
        vulkan_util.h
    )

    target_include_directories(example-vk PRIVATE ${Vulkan_INCLUDE_DIR})
    target_compile_definitions(example-vk PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
    target_link_libraries(example-vk PRIVATE glfw ${Vulkan_LIBRARY})
endif() 

if (BUILD_EXAMPLE_GL3)
if (BUILD_USE_GLAD)
    add_executable(
        example-gl3
        ${COMMON_SRC}
        ../src/nanovg_gl.h
        ../src/nanovg_gl_utils.h
        ../glad-3.2-compat/src/glad.c
        example_gl3.c
    )
    target_include_directories(example-gl3 PRIVATE ../glad-3.2-compat/include)
    target_compile_definitions(example-gl3 PRIVATE NANOVG_GLAD)
    target_link_libraries(example-gl3 PRIVATE glfw)
else()
    # Build glew
    set( ONLY_LIBS ON CACHE BOOL ""  )
    set( glew-cmake_BUILD_SHARED OFF CACHE BOOL ""  )
    set( glew-cmake_BUILD_STATIC ON CACHE BOOL ""  )
    add_subdirectory(glew-cmake glew)
    add_executable(
        example-gl3
        ${COMMON_SRC}
        ../src/nanovg_gl.h
        ../src/nanovg_gl_utils.h
        example_gl3.c
    )
    target_include_directories(example-gl3 PRIVATE src)
    target_compile_definitions(example-gl3 PRIVATE NANOVG_GLEW)
    target_link_libraries(example-gl3 PRIVATE glfw libglew_static)
endif()

endif()


if (MSVC) 
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS )
    set_source_files_properties(
        example_vulkan.c
        example_gl3.c
        demo.c 
        PROPERTIES COMPILE_FLAGS 
        /wd4244
    )

    set(BUILD_EXE_POSTFIX "-$<LOWER_CASE:${CMAKE_CXX_COMPILER_ID}-$<CONFIG>>" CACHE STRING "Append to executable name")
    set_property(TARGET example-gl3 PROPERTY OUTPUT_NAME example-gl3${BUILD_EXE_POSTFIX})
    set_property(TARGET example-vk PROPERTY OUTPUT_NAME example-vk${BUILD_EXE_POSTFIX})
    set_property(TARGET example-vk example-gl3 PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<0:...>)
endif()