
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(nanovg C)

option(NVG_USE_FREETYPE "Use FreeType instead of stb_truetype for font rasterization" OFF)

add_subdirectory(src)

add_subdirectory(example)
