
#include "../example/demo.h"

static void test_demo(NVGcontext *vg, int width, int height, float pxRatio)
{
    DemoData data;
    if (loadDemoData(vg, &data) == -1)
        return;

    int mousex = 600;
    int mousey = 130;

    nvgBeginFrame(vg, width, height, pxRatio);

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, width, height);
    nvgFillColor(vg, nvgRGBAf(0.3f, 0.3f, 0.32f, 1.0f));
    nvgFill(vg);

    renderDemo(vg, mousex, mousey, width, height, 50, false, &data);
    nvgEndFrame(vg);
    freeDemoData(vg, &data);
}