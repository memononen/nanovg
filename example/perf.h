#ifndef PERF_H
#define PERF_H

#include "nanovg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum GraphrenderStyle {
    GRAPH_RENDER_FPS,
    GRAPH_RENDER_MS,
};

#define GRAPH_HISTORY_COUNT 100
struct PerfGraph {
	int style;
	char name[32];
	float values[GRAPH_HISTORY_COUNT];
	int head;
};

void initGraph(struct PerfGraph* fps, int style, const char* name);
void updateGraph(struct PerfGraph* fps, float frameTime);
void renderGraph(struct NVGcontext* vg, float x, float y, struct PerfGraph* fps);
float getGraphAverage(struct PerfGraph* fps);

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