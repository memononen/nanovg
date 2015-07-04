#include "demo.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef NANOVG_GLEW
#  include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int loadDemoData(NVGcontext* vg, DemoData* data)
{
	int i;

	if (vg == NULL)
		return -1;

	for (i = 0; i < 12; i++) {
		char file[128];
		snprintf(file, 128, "../example/images/image%d.jpg", i+1);
		data->images[i] = nvgCreateImage(vg, file, 0);
		if (data->images[i] == 0) {
			printf("Could not load %s.\n", file);
			return -1;
		}
	}

	data->fontIcons = nvgCreateFont(vg, "icons", "../example/entypo.ttf");
	if (data->fontIcons == -1) {
		printf("Could not add font icons.\n");
		return -1;
	}
	data->fontNormal = nvgCreateFont(vg, "sans", "../example/Roboto-Regular.ttf");
	if (data->fontNormal == -1) {
		printf("Could not add font italic.\n");
		return -1;
	}
	data->fontBold = nvgCreateFont(vg, "sans-bold", "../example/Roboto-Bold.ttf");
	if (data->fontBold == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}

	return 0;
}

void freeDemoData(NVGcontext* vg, DemoData* data)
{
	if (vg == NULL)
		return;
}

void renderDemo(NVGcontext* vg, float mx, float my, float width, float height,
				float t, int blowup, int pickedID, DemoData* data)
{
	int id = 0;
	int DoPick = 1;
	
	// Filled rect
	nvgBeginPath(vg);
	nvgRect(vg, 40, 20, 100, 100);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgFill(vg);
	if (DoPick) nvgPickFill(vg, id++);

	// Stroked rect
	nvgBeginPath(vg);
	nvgRect(vg, 40, 220, 100, 100);
	nvgStrokeWidth(vg, 50.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgLineJoin(vg, NVG_MITER);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	// Overlapping rects

	nvgBeginPath(vg);
	nvgRect(vg, 40, 420, 100, 100);
	nvgStrokeWidth(vg, 50.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgLineJoin(vg, NVG_MITER);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	nvgBeginPath(vg);
	nvgRect(vg, 40, 420, 100, 100);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 128, 0) );
	nvgFill(vg);
	if (DoPick) nvgPickFill(vg, id++);
	
	// Lines
	
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 50);
	nvgLineTo(vg, 300, 150);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_BUTT);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 400, 50);
	nvgLineTo(vg, 400, 150);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_SQUARE);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 500, 50);
	nvgLineTo(vg, 500, 150);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	// Curves
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 300, 300);
	nvgBezierTo(vg, 200, 350,  400, 450,  300, 500);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_BUTT);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 400, 300);
	nvgBezierTo(vg, 300, 350,  500, 450,  400, 500);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_SQUARE);
	nvgStrokeWidth(vg, 25.0f);
	nvgStrokeColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgMoveTo(vg, 500, 300);
	nvgBezierTo(vg, 400, 350,  600, 450,  500, 500);
	nvgStroke(vg);
	if (DoPick) nvgPickStroke(vg, id++);

	// Curve Fill
	nvgBeginPath(vg);
	nvgMoveTo(vg, 600, 100);
	nvgBezierTo(vg, 650, 0,  750, 300,  800, 100);
	nvgLineTo(vg, 800, 250);
	nvgLineTo(vg, 600, 250);
	nvgClosePath(vg);
	nvgFillColor(vg, (pickedID == id) ? nvgRGB(255, 0, 0) : nvgRGB(0, 255, 0) );
	nvgFill(vg);
	if (DoPick) nvgPickFill(vg, id++);

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
	if (DoPick) nvgPickStroke(vg, id++);

}

void saveScreenShot(int w, int h, int premult, const char* name)
{
}

