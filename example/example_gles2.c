//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL2_IMPLEMENTATION
#define NANOVG_GLES2
#include "nanovg_gl2.h"
#include "demo.h"


void errorcb(int error, const char* desc)
{
	printf("GLFW error: %s\n", desc);
}

int blowup = 0;

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		blowup = !blowup;
}

int main()
{
	GLFWwindow* window;
	struct DemoData data;
	struct NVGcontext* vg = NULL;
	struct FPScounter fps;
	double prevt = 0;

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	initFPS(&fps);

	glfwSetErrorCallback(errorcb);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(1000, 600, "NanoVG", NULL, NULL);
//	window = glfwCreateWindow(1000, 600, "NanoVG", glfwGetPrimaryMonitor(), NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);

	glfwMakeContextCurrent(window);

	vg = nvgCreateGLES2(512, 512, NVG_ANTIALIAS);
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	if (loadDemoData(vg, &data) == -1)
		return -1;

	glfwSwapInterval(0);

	glfwSetTime(0);
	prevt = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		double mx, my, t, dt;
		int width, height;

		t = glfwGetTime();
		dt = t - prevt;
		prevt = t;
		updateFPS(&fps, dt);

		glfwGetCursorPos(window, &mx, &my);
		glfwGetFramebufferSize(window, &width, &height);

		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		nvgBeginFrame(vg, width, height);

		renderDemo(vg, mx,my, width,height, t, blowup, &data);
		renderFPS(vg, 5,5, &fps);

		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	freeDemoData(vg, &data);

	nvgDeleteGLES2(vg);

	glfwTerminate();
	return 0;
}
