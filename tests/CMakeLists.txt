
include (CTest)


# Download and unpack swiftshader
include(ExternalProject)
ExternalProject_Add(swiftshader
  GIT_REPOSITORY    https://github.com/google/swiftshader.git
  GIT_TAG            master
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/swiftshader
  CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  INSTALL_COMMAND   ""
)
ExternalProject_Get_Property(swiftshader SOURCE_DIR BINARY_DIR)
include_directories(${SOURCE_DIR}/include)
set(SWIFTSHADER_BINARY_DIR ${BINARY_DIR})
link_directories(${SWIFTSHADER_BINARY_DIR})

include_directories(../src)

#include_directories(../example)#for stb_image_write.h and demo

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX ON)

    find_program( MEMORYCHECK_COMMAND valgrind )
endif()


add_executable(gles2_pbuffer gles2_pbuffer.c ../example/demo.c)
add_dependencies(gles2_pbuffer swiftshader)
target_compile_definitions(gles2_pbuffer PRIVATE RENDERER_NAME="gles2")
if(LINUX)
target_link_libraries(gles2_pbuffer nanovg GLESv2 EGL m)
elseif(WIN32)
target_link_libraries(gles2_pbuffer nanovg libGLESv2 libEGL)
else()
target_link_libraries(gles2_pbuffer nanovg GLESv2 EGL)
endif()


add_executable(gles3_pbuffer gles3_pbuffer.c ../example/demo.c)
add_dependencies(gles3_pbuffer swiftshader)
target_compile_definitions(gles3_pbuffer PRIVATE RENDERER_NAME="gles3")
if(LINUX)
target_link_libraries(gles3_pbuffer nanovg GLESv2 EGL m)
elseif(WIN32)
target_link_libraries(gles3_pbuffer nanovg libGLESv2 libEGL)
else()
target_link_libraries(gles3_pbuffer nanovg GLESv2 EGL)
endif()


if(WIN32)
if(MSVC)
set(SWIFT_SHADER_LIB_DIR ${SWIFTSHADER_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else(MSVC)
set(SWIFT_SHADER_LIB_DIR ${SWIFTSHADER_BINARY_DIR})
endif(MSVC)
add_custom_command(TARGET gles3_pbuffer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SWIFT_SHADER_LIB_DIR}/libGLESv2.dll" $<TARGET_FILE_DIR:gles3_pbuffer>
)
add_custom_command(TARGET gles3_pbuffer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SWIFT_SHADER_LIB_DIR}/libEGL.dll" $<TARGET_FILE_DIR:gles3_pbuffer>
)
endif()

add_test(NAME gles2_pbuffer COMMAND $<TARGET_FILE:gles2_pbuffer> WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME gles3_pbuffer COMMAND $<TARGET_FILE:gles3_pbuffer> WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


