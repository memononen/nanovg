#include "perf.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef NANOVG_GLEW
#  include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#include "nanovg.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#elif !defined(__MINGW32__)
#include <iconv.h>
#endif

// timer query support
#ifndef GL_ARB_timer_query
#define GL_TIME_ELAPSED                   0x88BF
//typedef void (APIENTRY *pfnGLGETQUERYOBJECTUI64V)(GLuint id, GLenum pname, GLuint64* params);
//pfnGLGETQUERYOBJECTUI64V glGetQueryObjectui64v = 0;
#endif

void initGPUTimer(GPUtimer* timer)
{
	memset(timer, 0, sizeof(*timer));

/*	timer->supported = glfwExtensionSupported("GL_ARB_timer_query");
	if (timer->supported) {
#ifndef GL_ARB_timer_query
		glGetQueryObjectui64v = (pfnGLGETQUERYOBJECTUI64V)glfwGetProcAddress("glGetQueryObjectui64v");
		printf("glGetQueryObjectui64v=%p\n", glGetQueryObjectui64v);
		if (!glGetQueryObjectui64v) {
			timer->supported = GL_FALSE;
			return;
		}
#endif
		glGenQueries(GPU_QUERY_COUNT, timer->queries);
	}*/
}

void startGPUTimer(GPUtimer* timer)
{
	if (!timer->supported)
		return;
	glBeginQuery(GL_TIME_ELAPSED, timer->queries[timer->cur % GPU_QUERY_COUNT] );
	timer->cur++;
}

int stopGPUTimer(GPUtimer* timer, float* times, int maxTimes)
{
	NVG_NOTUSED(times);
	NVG_NOTUSED(maxTimes);
	GLint available = 1;
	int n = 0;
	if (!timer->supported)
		return 0;

	glEndQuery(GL_TIME_ELAPSED);
	while (available && timer->ret <= timer->cur) {
		// check for results if there are any
		glGetQueryObjectiv(timer->queries[timer->ret % GPU_QUERY_COUNT], GL_QUERY_RESULT_AVAILABLE, &available);
		if (available) {
/*			GLuint64 timeElapsed = 0;
			glGetQueryObjectui64v(timer->queries[timer->ret % GPU_QUERY_COUNT], GL_QUERY_RESULT, &timeElapsed);
			timer->ret++;
			if (n < maxTimes) {
				times[n] = (float)((double)timeElapsed * 1e-9);
				n++;
			}*/
		}
	}
	return n;
}


void initGraph(PerfGraph* fps, int style, const char* name)
{
	memset(fps, 0, sizeof(PerfGraph));
	fps->style = style;
	strncpy(fps->name, name, sizeof(fps->name));
	fps->name[sizeof(fps->name)-1] = '\0';
}

void updateGraph(PerfGraph* fps, float frameTime)
{
	fps->head = (fps->head+1) % GRAPH_HISTORY_COUNT;
	fps->values[fps->head] = frameTime;
}

float getGraphAverage(PerfGraph* fps)
{
	int i;
	float avg = 0;
	for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
		avg += fps->values[i];
	}
	return avg / (float)GRAPH_HISTORY_COUNT;
}

void renderGraph(NVGcontext* vg, float x, float y, PerfGraph* fps)
{
	int i;
	float avg, w, h;
	char str[64];

	avg = getGraphAverage(fps);

	w = 200;
	h = 35;

	nvgBeginPath(vg);
	nvgRect(vg, x,y, w,h);
	nvgFillColor(vg, nvgRGBA(0,0,0,128));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x, y+h);
	if (fps->style == GRAPH_RENDER_FPS) {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = 1.0f / (0.00001f + fps->values[(fps->head+i) % GRAPH_HISTORY_COUNT]);
			float vx, vy;
			if (v > 80.0f) v = 80.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 80.0f) * h);
			nvgLineTo(vg, vx, vy);
		}
	} else if (fps->style == GRAPH_RENDER_PERCENT) {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = fps->values[(fps->head+i) % GRAPH_HISTORY_COUNT] * 1.0f;
			float vx, vy;
			if (v > 100.0f) v = 100.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 100.0f) * h);
			nvgLineTo(vg, vx, vy);
		}
	} else {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = fps->values[(fps->head+i) % GRAPH_HISTORY_COUNT] * 1000.0f;
			float vx, vy;
			if (v > 20.0f) v = 20.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 20.0f) * h);
			nvgLineTo(vg, vx, vy);
		}
	}
	nvgLineTo(vg, x+w, y+h);
	nvgFillColor(vg, nvgRGBA(255,192,0,128));
	nvgFill(vg);

	nvgFontFace(vg, "sans");

	if (fps->name[0] != '\0') {
		nvgFontSize(vg, 12.0f);
		nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(240,240,240,192));
		nvgText(vg, x+3,y+3, fps->name, NULL);
	}

	if (fps->style == GRAPH_RENDER_FPS) {
		nvgFontSize(vg, 15.0f);
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(240,240,240,255));
		sprintf(str, "%.2f FPS", 1.0f / avg);
		nvgText(vg, x+w-3,y+3, str, NULL);

		nvgFontSize(vg, 13.0f);
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_BASELINE);
		nvgFillColor(vg, nvgRGBA(240,240,240,160));
		sprintf(str, "%.2f ms", avg * 1000.0f);
		nvgText(vg, x+w-3,y+h-3, str, NULL);
	}
	else if (fps->style == GRAPH_RENDER_PERCENT) {
		nvgFontSize(vg, 15.0f);
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(240,240,240,255));
		sprintf(str, "%.1f %%", avg * 1.0f);
		nvgText(vg, x+w-3,y+3, str, NULL);
	} else {
		nvgFontSize(vg, 15.0f);
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(240,240,240,255));
		sprintf(str, "%.2f ms", avg * 1000.0f);
		nvgText(vg, x+w-3,y+3, str, NULL);
	}
}
