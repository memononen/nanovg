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
#  include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "demo.h"
#include "perf.h"


void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

int blowup = 0;
int screenshot = 0;
int premult = 0;

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	NVG_NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		blowup = !blowup;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		screenshot = 1;
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		premult = !premult;
}

static int createFBO(int w, int h, int* fboId, int* rboId, int* texId)
{
	/* texture */
	glGenTextures(1, texId);
	glBindTexture(GL_TEXTURE_2D, *texId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* frame buffer object */
	glGenFramebuffers(1, fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, *fboId);

	/* render buffer object */
	glGenRenderbuffers(1, rboId);
	glBindRenderbuffer(GL_RENDERBUFFER, *rboId);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

	/* combine all */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texId, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rboId);

	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

int main()
{
	GLFWwindow* window;
	struct DemoData data;
	struct NVGcontext* vg = NULL;
	struct PerfGraph fps;
	double prevt = 0;
	int hasFBO, fboId, rboId, texId, image;

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");

	glfwSetErrorCallback(errorcb);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
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
    if(glewInit() != GLEW_OK) {
		printf("Could not init glew.\n");
		return -1;
	}
#endif

#ifdef DEMO_MSAA
	vg = nvgCreateGL2(512, 512, 0);
#else
	vg = nvgCreateGL2(512, 512, NVG_ANTIALIAS);
#endif
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	if (loadDemoData(vg, &data) == -1)
		return -1;

	glfwSwapInterval(0);

	glfwSetTime(0);
	prevt = glfwGetTime();

	hasFBO = createFBO(600, 600, &fboId, &rboId, &texId);
	if (hasFBO)
		image = nvgCreateImageGL(vg, texId);

	while (!glfwWindowShouldClose(window))
	{
		double mx, my, t, dt;
		int winWidth, winHeight;
		int fbWidth, fbHeight;
		float pxRatio;

		if (hasFBO) {
			int fboWidth, fboHeight;
			nvgImageSize(vg, image, &fboWidth, &fboHeight);
			// Draw some stull to an FBO as a test
			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glViewport(0, 0, fboWidth, fboHeight);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
			nvgBeginFrame(vg, fboWidth, fboHeight, pxRatio, NVG_PREMULTIPLIED_ALPHA);
			renderDemo(vg, mx, my, fboWidth, fboHeight, t, blowup, &data);
			nvgEndFrame(vg);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		t = glfwGetTime();
		dt = t - prevt;
		prevt = t;
		updateGraph(&fps, dt);

		glfwGetCursorPos(window, &mx, &my);
		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		if (premult)
			glClearColor(0,0,0,0);
		else
			glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vg, winWidth, winHeight, pxRatio, premult ? NVG_PREMULTIPLIED_ALPHA : NVG_STRAIGHT_ALPHA);

		renderDemo(vg, mx,my, winWidth,winHeight, t, blowup, &data);
		renderGraph(vg, 5,5, &fps);

		if (hasFBO) {
			struct NVGpaint img = nvgImagePattern(vg, 0, 0, 150, 150, 0, image, 0);
			nvgBeginPath(vg);
			nvgTranslate(vg, 540, 450);
			nvgScale(vg, 1.0f, -1.0f);
			nvgRect(vg, 0, 0, 150, 150);
			nvgFillPaint(vg, img);
			nvgFill(vg);
		}

		nvgEndFrame(vg);

		if (screenshot) {
			screenshot = 0;
			saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	freeDemoData(vg, &data);

	nvgDeleteGL2(vg);
	glDeleteFramebuffers(1, &fboId);
	glDeleteRenderbuffers(1, &rboId);
	glDeleteTextures(1, &texId);

	glfwTerminate();
	return 0;
}
