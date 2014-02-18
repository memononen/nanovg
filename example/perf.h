#ifndef PERF_H
#define PERF_H

#include "nanovg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum FPSrenderStyle {
    FPS_RENDER_FPS,
    FPS_RENDER_MS,
};

#define FPS_HISTORY_COUNT 100
struct FPScounter {
	int style;
	char name[32];
	float values[FPS_HISTORY_COUNT];
	int head;
};

void initFPS(struct FPScounter* fps, int style, const char* name);
void updateFPS(struct FPScounter* fps, float frameTime);
void renderFPS(struct NVGcontext* vg, float x, float y, struct FPScounter* fps);

#define GPU_QUERY_COUNT 5
struct GPUtimer {
	int supported;
	int cur, ret;
	unsigned int queries[GPU_QUERY_COUNT];
};

void initGPUTimer(struct GPUtimer* timer);
void startGPUTimer(struct GPUtimer* timer);
int stopGPUTimer(struct GPUtimer* timer, float* times, int maxTimes);

#ifdef __cplusplus
}
#endif

#endif // PERF_H