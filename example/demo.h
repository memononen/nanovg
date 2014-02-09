#ifndef WIDGETS_H
#define WIDGETS_H

#include "nanovg.h"

struct DemoData {
	int fontNormal, fontBold, fontIcons; 
	int images[12];
};

int loadDemoData(struct NVGcontext* vg, struct DemoData* data);
void freeDemoData(struct NVGcontext* vg, struct DemoData* data);
void renderDemo(struct NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup, struct DemoData* data);

#define FPS_HISTORY_COUNT 100
struct FPScounter
{
	float values[FPS_HISTORY_COUNT];
	int head;
};

void initFPS(struct FPScounter* fps);
void updateFPS(struct FPScounter* fps, float frameTime);
void renderFPS(struct NVGcontext* vg, float x, float y, struct FPScounter* fps);

#endif // WIDGETS_H