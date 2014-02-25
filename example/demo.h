#ifndef DEMO_H
#define DEMO_H

#include "nanovg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DemoData {
	int fontNormal, fontBold, fontIcons; 
	int images[12];
};

int loadDemoData(struct NVGcontext* vg, struct DemoData* data);
void freeDemoData(struct NVGcontext* vg, struct DemoData* data);
void renderDemo(struct NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup, struct DemoData* data);

void saveScreenShot(int w, int h, int premult, const char* name);

#ifdef __cplusplus
}
#endif

#endif // DEMO_H