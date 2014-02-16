
local action = _ACTION or ""

solution "nanovg"
	location ( "build" )
	configurations { "Debug", "Release" }
	platforms {"native", "x64", "x32"}
  
   	project "nanovg"
		language "C"
		kind "StaticLib"
		includedirs { "src" }
		files { "src/*.c" }
		targetdir("build")

	project "example_gl2"
		kind "ConsoleApp"
		language "C"
		files { "example/example_gl2.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
		defines { "NANOVG_GL2_IMPLEMENTATION" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3", "GLEW" }
			 defines { "NANOVG_GLEW" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }
			 defines { "NANOVG_GLEW" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    

	project "example_gl3"
		kind "ConsoleApp"
		language "C"
		files { "example/example_gl3.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
		defines { "NANOVG_GL3_IMPLEMENTATION" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3", "GLEW" }
			 defines { "NANOVG_GLEW", "GLFW_INCLUDE_GLCOREARB" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }
			 defines { "NANOVG_GLEW" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }
			defines { "GLFW_INCLUDE_GLCOREARB" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    

	project "example_gl2_msaa"
		kind "ConsoleApp"
		language "C"
		defines { "DEMO_MSAA", "NANOVG_GL2_IMPLEMENTATION" }
		files { "example/example_gl2.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3", "GLEW" }
			 defines { "NANOVG_GLEW" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }
			 defines { "NANOVG_GLEW" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    

	project "example_gl3_msaa"
		kind "ConsoleApp"
		language "C"
		defines { "DEMO_MSAA", "NANOVG_GL3_IMPLEMENTATION" }
		files { "example/example_gl3.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3", "GLEW" }
			 defines { "NANOVG_GLEW", "GLFW_INCLUDE_GLCOREARB" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }
			 defines { "NANOVG_GLEW" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }
			defines { "GLFW_INCLUDE_GLCOREARB" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    

	project "example_gles2"
		kind "ConsoleApp"
		language "C"
		files { "example/example_gles2.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}

	project "example_gles3"
		kind "ConsoleApp"
		language "C"
		files { "example/example_gles3.c", "example/demo.c" }
		includedirs { "src", "example" }
		targetdir("build")
		links { "nanovg" }
	 
		configuration { "linux" }
			 links { "X11","Xrandr", "rt", "GL", "GLU", "pthread", "m", "glfw3" }

		configuration { "windows" }
			 links { "glu32","opengl32", "gdi32", "winmm", "user32" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    
