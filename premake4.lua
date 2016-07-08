
local action = _ACTION or ""

function vgconfig (impl)
    	language "C"
	kind "StaticLib"
	includedirs { "src" }
	files { "src/*.c" }
	targetdir "build"
	defines { "_CRT_SECURE_NO_WARNINGS" , impl } --,"FONS_USE_FREETYPE" } Uncomment to compile with FreeType support
	buildoptions { "-Wfatal-errors" }

	configuration "Debug"
	    defines { "DEBUG" }
	    flags {"Symbols", "ExtraWarnings"}

	configuration "Release"
	    defines { "NDEBUG" }
	    flags { "Optimize", "ExtraWarnings"}
end

function exampleconfig (nanovg, fls)
	kind "ConsoleApp"
	language "C"
	files(fls)
	includedirs { "src", "example" }
	targetdir("build")
	links { nanovg }
	buildoptions { "-Wfatal-errors" }

	configuration { "linux" }
		    linkoptions { "`pkg-config --libs glfw3`" }
		    links { "GL", "GLU", "m", "GLEW" }

	configuration { "windows" }
		    links { "glfw3", "gdi32", "winmm", "user32", "GLEW", "glu32","opengl32", "kernel32" }
		    defines { "NANOVG_GLEW", "_CRT_SECURE_NO_WARNINGS" }

	configuration { "macosx" }
		links { "glfw3" }
		linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "ExtraWarnings"}

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize", "ExtraWarnings"}

end

solution "nanovg"
	location ( "build" )
	configurations { "Debug", "Release" }
	platforms {"native", "x64", "x32"}
	
	project "nanovg_gl2"
	    vgconfig "NANOVG_GL2_IMPLEMENTATION"
	
	project "nanovg_gl3"
	    vgconfig "NANOVG_GL3_IMPLEMENTATION"
	
	project "nanovg_gles2"
	    vgconfig "NANOVG_GLES2_IMPLEMENTATION"

	project "nanovg_gles3"
	    vgconfig "NANOVG_GLES3_IMPLEMENTATION"

	project "example_gl2"
	    exampleconfig("nanovg_gl2", { "example/example_gl2.c", "example/demo.c", "example/perf.c" })

	project "example_gl3"
	    exampleconfig("nanovg_gl3", { "example/example_gl3.c", "example/demo.c", "example/perf.c" })

	project "example_gl2_msaa"
		defines { "DEMO_MSAA" }
		exampleconfig("nanovg_gl2", { "example/example_gl2.c", "example/demo.c", "example/perf.c" })

	project "example_gl3_msaa"
		defines { "DEMO_MSAA" }
		exampleconfig("nanovg_gl3", { "example/example_gl3.c", "example/demo.c", "example/perf.c" })

	project "example_fbo"
	    exampleconfig("nanovg_gl3", { "example/example_fbo.c", "example/perf.c" })

	project "example_gles2"
	    exampleconfig("nanovg_gles2", { "example/example_gles2.c", "example/demo.c", "example/perf.c" })

	project "example_gles3"
	    exampleconfig("nanovg_gles3", { "example/example_gles3.c", "example/demo.c", "example/perf.c" })
    




