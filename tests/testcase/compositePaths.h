
static void test_compositePaths1(NVGcontext* vg, int width, int height)
{
	nvgBeginFrame(vg, width, height, 1);
	nvgBeginPath(vg);
	nvgRect(vg, 100, 100, 120, 30);
	nvgCircle(vg, 120, 120, 5);
	nvgPathWinding(vg, NVG_HOLE); // Mark circle as a hole.
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);
	nvgEndFrame(vg);
}
