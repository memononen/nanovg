target("nanovg")
    set_kind("static")

    add_includedirs("src")

    add_files("src/*.c")

    add_defines("_CRT_SECURE_NO_WARNINGS")



target("example_gl2")
    set_kind("binary")

    add_files("example/example_gl2.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")
        add_defines("NANOVG_GLEW")
        add_links("GL", "GLU", "m", "GLEW")
    end




target("example_gl3")
    set_kind("binary")

    add_files("example/example_gl3.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")
        add_defines("NANOVG_GLEW")
        add_links("GL", "GLU", "m", "GLEW")
    end




target("example_gl2_mesa")
    set_kind("binary")

    add_defines("DEMO_MSAA")

    add_files("example/example_gl2.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")
        add_defines("NANOVG_GLEW")
        add_links("GL", "GLU", "m", "GLEW")
    end




target("example_gl3_mesa")
    set_kind("binary")

    add_defines("DEMO_MSAA")

    add_files("example/example_gl3.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")
        add_defines("NANOVG_GLEW")
        add_links("GL", "GLU", "m", "GLEW")
    end




target("example_fbo")
    set_kind("binary")

    add_files("example/example_fbo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")
        add_links("GL", "GLU", "m", "GLEW")
    end



target("example_gles2")
    set_kind("binary")

    add_files("example/example_gles2.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")

        add_links("GL", "GLU", "m", "GLEW")
    end



target("example_gles3")
    set_kind("binary")

    add_files("example/example_gles3.c",
            "example/demo.c",
            "example/perf.c" )

    add_includedirs("src", "example")

    add_deps("nanovg")

    if is_plat("linux") then
        add_ldflags(
            "$(shell pkg-config --static --libs glfw3)")

        add_links("GL", "GLU", "m", "GLEW")
    end