cmake_minimum_required(VERSION 3.8)

add_definitions(-DFONS_USE_FREETYPE)
add_definitions(-DNVG_NO_STB)

add_library (
	nanovg
	src/nanovg.c
)

include_directories(../freetype/include)

target_link_libraries(
	nanovg 
	PUBLIC
	freetype
)

target_include_directories (nanovg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
