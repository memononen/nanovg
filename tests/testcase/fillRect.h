
static void test_fillRect1(NVGcontext *vg, int width, int height, float pxRatio)
{
    nvgBeginFrame(vg, width, height, pxRatio);
    nvgBeginPath(vg);
    nvgRect(vg, 100, 100, 120, 30);
    nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
    nvgFill(vg);
    nvgEndFrame(vg);
}