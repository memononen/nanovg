
static void test_compositeOperation(NVGcontext *vg, int width, int height, float pxRatio, enum NVGcompositeOperation op)
{
    nvgBeginFrame(vg, width, height, pxRatio);

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, width * 0.6, height * 0.6);
    nvgFillColor(vg, nvgRGBA(0, 0, 255, 255));
    nvgFill(vg);

    nvgGlobalCompositeOperation(vg, op);

    nvgBeginPath(vg);
    nvgCircle(vg, width * 0.6, height * 0.6, height * 0.3);
    nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, width, height);
    nvgPathWinding(vg, NVG_HOLE); // Mark circle as a hole.
    nvgCircle(vg, width * 0.6, height * 0.6, height * 0.3);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
    nvgFill(vg);

    nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
    nvgEndFrame(vg);
}

static void test_compositeOperationSourceOver(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_SOURCE_OVER);
}
static void test_compositeOperationSourceIn(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_SOURCE_IN);
}
static void test_compositeOperationSourceOut(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_SOURCE_OUT);
}
static void test_compositeOperationSourceAtop(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_ATOP);
}
static void test_compositeOperationDestinationOver(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_DESTINATION_OVER);
}
static void test_compositeOperationDestinationIn(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_DESTINATION_IN);
}
static void test_compositeOperationDestinationOut(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_DESTINATION_OUT);
}
static void test_compositeOperationDestinationAtop(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_DESTINATION_ATOP);
}
static void test_compositeOperationLighter(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_LIGHTER);
}
static void test_compositeOperationCopy(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_COPY);
}
static void test_compositeOperationXOR(NVGcontext *vg, int width, int height, float pxRatio)
{
    test_compositeOperation(vg, width, height, pxRatio, NVG_XOR);
}