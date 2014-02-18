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
#ifdef NANOVG_GLEW
#	include <GL/glew.h>
#endif
#define GLFW_NO_GLU
#ifndef _WIN32
#   define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
//#include "nanovg_gl3.h"
#include "nanovg_gl3buf.h"
#include "demo.h"

void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

int blowup = 0;

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	NVG_NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		blowup = !blowup;
}

enum numqueries {
    NUM_QUERIES = 5
};

int main()
{
	GLFWwindow* window;
	struct DemoData data;
	struct NVGcontext* vg = NULL;
	struct FPScounter fps, cpuTimes, gpuTimes;
	double prevt = 0, cpuTime = 0;
    int timerquery = GL_FALSE, currquery = 0, retquery = 0;
    GLuint timerqueryid[NUM_QUERIES];

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	initFPS(&fps);
	initFPS(&cpuTimes);
	initFPS(&gpuTimes);

	glfwSetErrorCallback(errorcb);
#ifndef _WIN32 // don't require this on win32, and works with more cards
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

#ifdef DEMO_MSAA
	glfwWindowHint(GLFW_SAMPLES, 4);
#endif
	window = glfwCreateWindow(1000, 600, "NanoVG", NULL, NULL);
//	window = glfwCreateWindow(1000, 600, "NanoVG", glfwGetPrimaryMonitor(), NULL);
    if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);

	glfwMakeContextCurrent(window);
#ifdef NANOVG_GLEW
	glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
		printf("Could not init glew.\n");
		return -1;
	}
#endif

#ifdef DEMO_MSAA
	vg = nvgCreateGL3(512, 512, 0);
#else
	vg = nvgCreateGL3(512, 512, NVG_ANTIALIAS);
#endif
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	if (loadDemoData(vg, &data) == -1)
		return -1;

	glfwSwapInterval(0);

	timerquery = glfwExtensionSupported("GL_ARB_timer_query");
	if( timerquery ) {
		glGenQueries(NUM_QUERIES, timerqueryid);
	}

	glfwSetTime(0);
	prevt = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		double mx, my, t, dt;
		int winWidth, winHeight;
		int fbWidth, fbHeight;
		float pxRatio;

		t = glfwGetTime();
		dt = t - prevt;
		prevt = t;
		updateFPS(&fps, dt);
        updateFPS(&cpuTimes, cpuTime);
		if( timerquery ) {
            glBeginQuery(GL_TIME_ELAPSED, timerqueryid[currquery % NUM_QUERIES] );
			currquery = ++currquery;
		}

		glfwGetCursorPos(window, &mx, &my);
		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

		renderDemo(vg, mx,my, winWidth,winHeight, t, blowup, &data);
		renderFPS(vg, 5,5, &fps);
		renderFPSEx(vg, 310,5, &cpuTimes, RENDER_MS, "CPU Time");
		renderFPSEx(vg, 615,5, &gpuTimes, RENDER_MS, "GPU Time");

		nvgEndFrame(vg);

		glEnable(GL_DEPTH_TEST);

        // Measure the CPU time taken excluding swap buffers (as the swap may wait for GPU)
        cpuTime = glfwGetTime() - t;

		if( timerquery ) {
			GLint available = 1;
			glEndQuery(GL_TIME_ELAPSED);
			while( available && retquery <= currquery ) {
				// check for results if there are any
				glGetQueryObjectiv(timerqueryid[retquery % NUM_QUERIES], GL_QUERY_RESULT_AVAILABLE, &available);
				if( available ) {
					GLuint64 timeElapsed = 0;
					double gpuTime;
					glGetQueryObjectui64v(timerqueryid[retquery % NUM_QUERIES], GL_QUERY_RESULT, &timeElapsed);
					retquery = ++retquery ;
					gpuTime = (double)timeElapsed * 1e-9;
					updateFPS(&gpuTimes, (float)gpuTime);
				}
			}

		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	freeDemoData(vg, &data);

	nvgDeleteGL3(vg);

	glfwTerminate();
	return 0;
}
