

FIND_PACKAGE(OpenGL REQUIRED)

add_library(OpenGL INTERFACE)

if (APPLE)
    target_include_directories(OpenGL INTERFACE "${OPENGL_INCLUDE_DIR}/Headers")
else()
    target_include_directories(OpenGL INTERFACE "${OPENGL_INCLUDE_DIR}")
endif()

target_link_libraries(OpenGL INTERFACE "${OPENGL_LIBRARY}")

OPTION(OGL_USE_MSAA "Use MSAA instead of NanoVG antialiasing" OFF)

macro(add_example NAME LOADER_NAME)
	add_executable(${NAME} ${ARGN} "loader/${LOADER_NAME}/src/glad.c" "demo.c" "demo.h" "perf.c" "perf.h")
	
	target_include_directories(${NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
	target_include_directories(${NAME} PRIVATE "loader/${LOADER_NAME}/include")
	
	target_link_libraries(${NAME} PRIVATE OpenGL nanovg)
	
	target_compile_definitions(${NAME} PRIVATE -DEXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
	target_compile_definitions(${NAME} PRIVATE -DOGL_LOADER="${CMAKE_CURRENT_SOURCE_DIR}/loader/${LOADER_NAME}/include/glad/glad.h")
	
	if (OGL_USE_MSAA)
		target_compile_definitions(${NAME} PRIVATE -DDEMO_MSAA)
	endif()
	
	if (APPLE)
		FIND_LIBRARY(COCOA_LIBRARY Cocoa)

		if (NOT COCOA_LIBRARY)
			message(FATAL_ERROR "Couldn't find Cocoa!")
		endif()

		target_link_libraries(${NAME} PRIVATE ${COCOA_LIBRARY})
	endif()
	
	if (UNIX)
		find_package(PkgConfig REQUIRED)
		pkg_search_module(GLFW REQUIRED glfw3)
		
		target_include_directories(${NAME} PRIVATE ${GLFW_INCLUDE_DIRS})
		
		target_link_libraries(${NAME} PRIVATE ${GLFW_LIBRARIES})
		
		target_link_libraries(${NAME} PRIVATE m dl)
	else()
		set(GLFW_CHECKOUT_DIR "" CACHE PATH "The location of the GLFW source checkout")
		
		if (IS_DIRECTORY "${GLFW_CHECKOUT_DIR}")
			if (NOT TARGET glfw)
				set(BUILD_SHARED_LIBS OFF)
				set(GLFW_BUILD_EXAMPLES OFF)
				set(GLFW_BUILD_TESTS OFF)
				set(GLFW_BUILD_DOCS OFF)
				set(GLFW_INSTALL OFF)
				
				add_subdirectory(${GLFW_CHECKOUT_DIR} glfw)
			endif()
			
			target_include_directories(${NAME} PRIVATE "${GLFW_CHECKOUT_DIR}/include")
			target_link_libraries(${NAME} PRIVATE glfw ${GLFW_LIBRARIES})
		else()
			message(FATAL_ERROR "GLFW source directory isn't set!")
		endif()
	endif()
endmacro(add_example)

add_example(example_gl2 gl_2_1 example_gl2.c)

add_example(example_gl3 gl_3_0 example_gl3.c)

add_example(example_gles2 es_2_0 example_gles2.c)

add_example(example_gles3 es_3_0 example_gles3.c)
