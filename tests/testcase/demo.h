
#include "../example/demo.h"

static void test_demo(NVGcontext* vg, int width, int height, int offsetx, int offsety)
{
	DemoData data;
	if (loadDemoData(vg, &data) == -1)
		return;

	int mousex = 600;
	int mousey = 130;

	nvgBeginFrame(vg, width, height, 1);

	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBAf(0.3f, 0.3f, 0.32f, 1.0f));
	nvgFill(vg);

	nvgTranslate(vg, offsetx, offsety);

	renderDemo(vg, mousex, mousey, 1000, 600, 50, false, &data);
	nvgEndFrame(vg);
	freeDemoData(vg, &data);
}

static void test_demo01(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, 0, 0);
}
static void test_demo02(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -250, 0);
}
static void test_demo03(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -500, 0);
}
static void test_demo04(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -750, 0);
}
static void test_demo05(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, 0, -250);
}
static void test_demo06(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -250, -250);
}
static void test_demo07(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -500, -250);
}
static void test_demo08(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -750, -250);
}
static void test_demo09(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, 0, -500);
}
static void test_demo10(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -250, -500);
}
static void test_demo11(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -500, -500);
}
static void test_demo12(NVGcontext* vg, int width, int height)
{
	test_demo(vg, width, height, -750, -500);
}
