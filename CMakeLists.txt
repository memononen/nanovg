cmake_minimum_required(VERSION 3.2)
project(nanovg LANGUAGES C)

message(WARNING "Currently, this file is testing purpose only. If you want build demo, please use Premake")

add_library(nanovg STATIC src/nanovg.c)

enable_testing()
add_subdirectory(tests)