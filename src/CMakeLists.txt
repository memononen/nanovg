
set(SOURCE_FILES
	 fontstash.h
	 nanovg.c
	 nanovg.h
	 nanovg_gl.h
	 nanovg_gl_utils.h
	 stb_image.h
	 stb_truetype.h
)

add_library(nanovg STATIC ${SOURCE_FILES})

target_include_directories(nanovg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (NVG_USE_FREETYPE)
	target_compile_definitions(nanovg PUBLIC -DFONS_USE_FREETYPE)
	
	find_package(Freetype REQUIRED)
	
	target_link_libraries(nanovg PUBLIC ${FREETYPE_LIBRARIES})
	target_include_directories(nanovg PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif()