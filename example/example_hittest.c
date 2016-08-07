#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef NANOVG_GLEW
#	include <GL/glew.h>
#endif
#ifdef __APPLE__
#	define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
#include "perf.h"

void render(NVGcontext* vg, float mx, float my, int pickedID);

int loadFonts(NVGcontext* vg)
{
	int font;
	font = nvgCreateFont(vg, "sans", "../example/Roboto-Regular.ttf");
	if (font == -1) {
		printf("Could not add font regular.\n");
		return -1;
	}
	font = nvgCreateFont(vg, "sans-bold", "../example/Roboto-Bold.ttf");
	if (font == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}
	return 0;
}

void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	NVG_NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int main()
{
	GLFWwindow* window;
	NVGcontext* vg = NULL;
	PerfGraph fps;
	double prevt = 0;
	int pickedID = -1;

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
	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
#endif

#ifdef DEMO_MSAA
	vg = nvgCreateGL2(NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	vg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	loadFonts(vg);

	glfwSwapInterval(0);

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
		updateGraph(&fps, dt);

		glfwGetCursorPos(window, &mx, &my);
		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

		render(vg, mx,my, pickedID);
		renderGraph(vg, 5,5, &fps);

		pickedID = nvgHitTest(vg, mx, my, NVG_TEST_ALL);
		nvgEndFrame(vg);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	nvgDeleteGL2(vg);

	glfwTerminate();
	return 0;
}

void render(NVGcontext* vg, float mx, float my, int pickedID)
{
	int id = 0;
	int doPick = 1;
	
	// Filled rect
	nvgBeginPath(vg);
	nvgRect(vg, 40, 20, 100, 100);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);

	// Stroked rect
	nvgBeginPath(vg);
	nvgRect(vg, 40, 220, 100, 100);
	nvgStrokeWidth(vg, 50.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgLineJoin(vg, NVG_MITER);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	// Filled rect with hole
	nvgBeginPath(vg);
	nvgRect(vg, 150, 20, 100, 100);
	nvgRect(vg, 200, 50, 60, 60);
	nvgPathWinding(vg, NVG_CW);
	nvgLineJoin(vg, NVG_MITER);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);

	// Overlapping rects

	nvgBeginPath(vg);
	nvgRect(vg, 40, 420, 100, 100);
	nvgStrokeWidth(vg, 50.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgLineJoin(vg, NVG_MITER);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgRect(vg, 40, 420, 100, 100);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);
	
	// Lines
	
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 50);
	nvgLineTo(vg, 300, 150);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_BUTT);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 400, 50);
	nvgLineTo(vg, 400, 150);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_SQUARE);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 500, 50);
	nvgLineTo(vg, 500, 150);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 200);
	nvgLineTo(vg, 400, 200);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);


	// Curves
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 300);
	nvgBezierTo(vg, 200, 350,  400, 450,  300, 500);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_BUTT);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 400, 300);
	nvgBezierTo(vg, 300, 350,  500, 450,  400, 500);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_SQUARE);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 500, 300);
	nvgBezierTo(vg, 400, 350,  600, 450,  500, 500);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 550);
	nvgBezierTo(vg, 350, 550,  400, 550,  450, 550);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 225, 300);
	nvgBezierTo(vg, 225, 350,  225, 450,  225, 500);
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	// Curve Fill
	nvgBeginPath(vg);
	nvgMoveTo(vg, 600, 100);
	nvgBezierTo(vg, 650, 0,  750, 300,  800, 100);
	nvgLineTo(vg, 800, 250);
	nvgLineTo(vg, 600, 250);
	nvgClosePath(vg);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);

	// Curve Stroke
	nvgBeginPath(vg);
	nvgMoveTo(vg, 600, 350);
	nvgBezierTo(vg, 650, 250,  750, 550,  800, 350);
	nvgLineTo(vg, 800, 500);
	nvgLineTo(vg, 600, 500);
	nvgClosePath(vg);
	nvgLineJoin(vg, NVG_BEVEL);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgStroke(vg);
	if (doPick) nvgStrokeHitRegion(vg, id++);

	// Rect Fill - Alternate pick api
	nvgBeginPath(vg);
	nvgRect(vg, 850, 25, 100, 100);
	nvgFillColor(vg, (nvgInFill(vg, mx, my)) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgFill(vg);

	// Rect Stroke - Alternate pick api
	nvgBeginPath(vg);
	nvgRect(vg, 850, 175, 100, 100);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (nvgInStroke(vg, mx, my)) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgStroke(vg);

	// Rect Fill & Rotated Scissor
	nvgTranslate(vg, 900, 375);	
	nvgRotate(vg, nvgDegToRad(22.5f));
	nvgScissor(vg, -50, -50, 100, 100);
	nvgResetTransform(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 850, 325, 100, 100);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);
	nvgResetTransform(vg);
	nvgResetScissor(vg);
	
	// Filled rect made with curves
	nvgBeginPath(vg);
	nvgMoveTo(vg, 850, 450);
	nvgBezierTo(vg, 875, 450,  925, 450,  950, 450);
	nvgBezierTo(vg, 950, 475,  950, 525,  950, 550);
	nvgBezierTo(vg, 925, 550,  875, 550,  850, 550);
	nvgBezierTo(vg, 850, 525,  850, 475,  850, 450);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgFill(vg);
	if (doPick) nvgFillHitRegion(vg, id++);
}

