cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0091 NEW) # for CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0077 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
# set(CMAKE_DISABLE_SOURCE_CHANGES ON)

project(nanovg)

set(CMAKE_C_STANDARD 11)
if (MSVC)
set(CMAKE_CXX_STANDARD 23)
else()
set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF) # force use -std=c++20 flag for gcc
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

MESSAGE(STATUS "Set cxx standard: C++20")

string(TIMESTAMP VERSION_YEAR "%Y")
string(TIMESTAMP VERSION_MONTH_YEAR "%m")
string(TIMESTAMP VERSION_DAY_YEAR "%j")

math(EXPR VERSION_MAJOR "${VERSION_YEAR} - 2022")
math(EXPR VERSION_MINOR "${VERSION_MONTH_YEAR}")
math(EXPR VERSION_TINY "${VERSION_DAY_YEAR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/cmake")

include(CheckCXXCompilerFlag)
include(CheckLibraryExists)
include(CheckCXXSymbolExists)
include(CheckLinkerFlag)

option(ENABLE_BUILD_WERROR "All warnings being treated as errors" ON)

option(ENABLE_TCMALLOC_STATIC "Build with Tcmalloc support" OFF)
option(ENABLE_JEMALLOC_STATIC "Build with Jemalloc support" OFF)
option(ENABLE_LLD "build using lld" OFF)
option(ENABLE_MOLD "build using mold" OFF)
option(USE_BUNDLED_ODB_FILE "build using pre generated odb file" OFF)
option(ENABLE_IOURING "build with liburing" ON)
option(ENABLE_LTO "enable LTO support" OFF)

find_program(MOLD_LINKER mold)
find_program(LLD_LINKER lld)

if (ENABLE_LTO)
	set(ENABLE_MOLD OFF FORCE)
endif()

IF(NOT MSVC)

if (ENABLE_LLD)
	list(APPEND CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
elseif (ENABLE_MOLD)
	list(APPEND CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")
else()
	if (MOLD_LINKER)
		if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			list(APPEND CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")
		endif()
	elseif(LLD_LINKER)
		list(APPEND CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
	endif()
endif()
ENDIF(NOT MSVC)

if (ENABLE_LTO)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(-flto=thin)
		add_link_options(-flto=thin)
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-flto)
		add_link_options(-flto)
	elseif (MSVC)
		add_compile_options(/GL)
		add_link_options(/LTCG)
	endif()
endif()

CHECK_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden COMPILER_HAS_VISIBILITY_INLINE_HIDDEN)
CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden COMPILER_HAS_VISIBILITY_HIDDEN)
CHECK_CXX_COMPILER_FLAG(-fdiagnostics-color=always COMPILER_HAS_COLOR)
CHECK_CXX_COMPILER_FLAG(-fcoroutines COMPILER_HAS_FCOROUTINES)

if (COMPILER_HAS_VISIBILITY_INLINE_HIDDEN)
	# C 不能使用这个 flag. gcc 会报错
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
endif()

if (COMPILER_HAS_VISIBILITY_HIDDEN)
	add_compile_options(-fvisibility=hidden)
endif()

if (COMPILER_HAS_COLOR)
	add_compile_options(-fdiagnostics-color=always)
endif()

if (COMPILER_HAS_FCOROUTINES)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
endif()

if (MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	add_compile_options("/utf-8")
	add_definitions(
		-D_CRT_SECURE_NO_DEPRECATE
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_NONSTDC_NO_DEPRECATE
		-D_CRT_NONSTDC_NO_WARNINGS
		-D_SCL_SECURE_NO_DEPRECATE
		-D_SCL_SECURE_NO_WARNINGS
	)

	add_definitions(
		-DWIN32_LEAN_AND_MEAN
		-D_WIN32_WINNT=0x0601
		-DNOMINMAX
		-DUNICODE
		-D_UNICODE
		-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
	)

	add_compile_options(/IGNORE:4267,4244,4005)

	add_compile_options(/bigobj /MP /Zc:__cplusplus)
	MESSAGE(STATUS "Using parallel compiling (/MP)")
	set(CMAKE_CXX_STACK_SIZE "100000000")
	MESSAGE(STATUS "Set stack size: 100000000")

elseif(WIN32)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	add_definitions(
		-DWIN32_LEAN_AND_MEAN
		-D_WIN32_WINNT=0x0601
		-DNOMINMAX
		-DUNICODE
		-D_UNICODE
		-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
	)

elseif (ANDROID)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
		add_link_options(-stdlib=libc++)

		CHECK_LINKER_FLAG(CXX "-rtlib=compiler-rt -libunwind=libunwind" IS_COMPILER_RT_SUPPORTED)

		if (IS_COMPILER_RT_SUPPORTED)
			message(STATUS "clang supports compiler-rt, use it")
			add_link_options(-rtlib=compiler-rt -unwindlib=libunwind)
		endif()
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

		CHECK_CXX_COMPILER_FLAG(-fconcepts-diagnostics-depth=99 COMPILER_HAS_fconcepts_diagnostics)

		if (COMPILER_HAS_fconcepts_diagnostics)
			add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fconcepts-diagnostics-depth=99>)
		endif()

	endif()

endif()

check_cxx_symbol_exists(__cpp_lib_format "version" HAS_LIBCPP_FORMAT)

find_package(Threads)
link_libraries(${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

if (ENABLE_TCMALLOC_STATIC)
	find_package(Tcmalloc)
	link_libraries(${Tcmalloc_LIBRARY})
endif()

if (ENABLE_JEMALLOC_STATIC)
	find_package(JeMalloc)
	link_libraries(${JEMALLOC_LIBRARIES})
endif()

if (WIN32)
	link_libraries(
		Secur32.lib
		Bcrypt.lib
		Winmm.lib
		Mswsock.lib
	)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_LOGGER -DLOGGER_DBG_VIEW")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")

find_package(OpenGL)

CHECK_LIBRARY_EXISTS(m sin "" HAVE_LIB_M)

if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

add_library(nanovg STATIC
	src/nanovg.c
)

target_include_directories(nanovg PUBLIC include)

target_link_libraries(nanovg OpenGL::OpenGL ${EXTRA_LIBS})

add_subdirectory(example)
