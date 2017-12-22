//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef NANOVG_INTERNAL_H
#define NANOVG_INTERNAL_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#include "nanovg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4204)  // nonstandard extension used : non-constant aggregate initializer
#pragma warning(disable: 4706)  // assignment within conditional expression
#endif

#define NVG_INIT_FONTIMAGE_SIZE  512
#define NVG_MAX_FONTIMAGE_SIZE   2048
#define NVG_MAX_FONTIMAGES       4

#define NVG_INIT_COMMANDS_SIZE 256
#define NVG_INIT_POINTS_SIZE 128
#define NVG_INIT_PATHS_SIZE 16
#define NVG_INIT_VERTS_SIZE 256
#define NVG_MAX_STATES 32

#define NVG_KAPPA90 0.5522847493f	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define NVG_COUNTOF(arr) (sizeof(arr) / sizeof(0[arr]))
#define NVG_PICK_EPS	0.0001f

// Segment flags
enum NVGsegmentFlags {
	NVG_PICK_CORNER	= 1,
	NVG_PICK_BEVEL = 2,
	NVG_PICK_INNERBEVEL	= 4,
	NVG_PICK_CAP = 8,
	NVG_PICK_ENDCAP = 16,
};

// Path flags
enum NVGpathFlags {
	NVG_PICK_SCISSOR = 1,
	NVG_PICK_STROKE = 2,
	NVG_PICK_FILL = 4,
};

//#define NVG_PICK_DEBUG
#ifdef NVG_PICK_DEBUG
	int g_ndebugBounds = 0;
	float g_debugBounds[256][4];
	int g_ndebugLines = 0;
	float g_debugLines[256][4];

	#define NVG_PICK_DEBUG_NEWFRAME() \
		{ \
			g_ndebugBounds = 0; \
			g_ndebugLines = 0; \
		}

	#define NVG_PICK_DEBUG_BOUNDS(bounds) memcpy(&g_debugBounds[g_ndebugBounds++][0], bounds, sizeof(float) * 4)
	#define NVG_PICK_DEBUG_LINE(A, B) \
		{	memcpy(&g_debugLines[g_ndebugLines][0], (A), sizeof(float) * 2); \
			memcpy(&g_debugLines[g_ndebugLines++][2], (B), sizeof(float) * 2); }
	#define NVG_PICK_DEBUG_VECTOR(A, D) \
		{	memcpy(&g_debugLines[g_ndebugLines][0], (A), sizeof(float) * 2); \
			g_debugLines[g_ndebugLines][2] = (A)[0] + (D)[0]; \
			g_debugLines[g_ndebugLines][3] = (A)[1] + (D)[1]; \
			++g_ndebugLines; \
		}
	#define NVG_PICK_DEBUG_VECTOR_SCALE(A, D, S) \
		{	memcpy(&g_debugLines[g_ndebugLines][0], (A), sizeof(float) * 2); \
			g_debugLines[g_ndebugLines][2] = (A)[0] + (D)[0] * (S); \
			g_debugLines[g_ndebugLines][3] = (A)[1] + (D)[1] * (S); \
			++g_ndebugLines; \
		}
#else
	#define NVG_PICK_DEBUG_NEWFRAME()
	#define NVG_PICK_DEBUG_BOUNDS(bounds)
	#define NVG_PICK_DEBUG_LINE(A, B)
	#define NVG_PICK_DEBUG_VECTOR(A, D)
	#define NVG_PICK_DEBUG_VECTOR_SCALE(A, D, S)
#endif

struct NVGsegment {
	int firstPoint;				// Index into NVGpickScene::points
	short type;					// NVG_LINETO or NVG_BEZIERTO
	short flags;				// Flags relate to the corner between the prev segment and this one.
	float bounds[4];
	float startDir[2];			// Direction at t == 0
	float endDir[2];			// Direction at t == 1
	float miterDir[2];			// Direction of miter of corner between the prev segment and this one.
};
typedef struct NVGsegment NVGsegment;

struct NVGpickSubPath {
	short winding;				// TODO: Merge to flag field
	short closed;				// TODO: Merge to flag field

	int firstSegment;			// Index into NVGpickScene::segments
	int nsegments;

	float bounds[4];

	struct NVGpickSubPath* next;
};
typedef struct NVGpickSubPath NVGpickSubPath;

struct NVGpickPath {
	int id;
	short flags;
	short order;
	float strokeWidth;
	float miterLimit;
	short lineCap;
	short lineJoin;

	float bounds[4];
	int scissor; // Indexes into ps->points and defines scissor rect as XVec, YVec and Center

	struct NVGpickSubPath*	subPaths;
	struct NVGpickPath* next;
	struct NVGpickPath* cellnext;
};
typedef struct NVGpickPath NVGpickPath;

struct NVGpickScene {
	int npaths;

	NVGpickPath* paths;	// Linked list of paths
	NVGpickPath* lastPath; // The last path in the paths linked list (the first path added)
	NVGpickPath* freePaths; // Linked list of free paths

	NVGpickSubPath* freeSubPaths; // Linked list of free sub paths

	int width;
	int height;

	// Points for all path sub paths.
	float* points;
	int npoints;
	int cpoints;

	// Segments for all path sub paths
	NVGsegment* segments;
	int nsegments;
	int csegments;

	// Implicit quadtree
	float xdim;		// Width / (1 << nlevels)
	float ydim;		// Height / (1 << nlevels)
	int ncells;		// Total number of cells in all levels
	int nlevels;
	NVGpickPath*** levels;	// Index: [Level][LevelY * LevelW + LevelX] Value: Linked list of paths

	// Temp storage for picking
	int cpicked;
	NVGpickPath** picked;
};
typedef struct NVGpickScene NVGpickScene;

enum NVGcommands {
	NVG_MOVETO = 0,
	NVG_LINETO = 1,
	NVG_BEZIERTO = 2,
	NVG_CLOSE = 3,
	NVG_WINDING = 4,
};

enum NVGpointFlags
{
	NVG_PT_CORNER = 0x01,
	NVG_PT_LEFT = 0x02,
	NVG_PT_BEVEL = 0x04,
	NVG_PR_INNERBEVEL = 0x08,
};

struct NVGstate {
	NVGcompositeOperationState compositeOperation;
	int shapeAntiAlias;
	NVGpaint fill;
	NVGpaint stroke;
	float strokeWidth;
	float miterLimit;
	int lineJoin;
	int lineCap;
	float alpha;
	float xform[6];
	NVGscissor scissor;
	float fontSize;
	float letterSpacing;
	float lineHeight;
	float fontBlur;
	int textAlign;
	int fontId;
};
typedef struct NVGstate NVGstate;

struct NVGpoint {
	float x,y;
	float dx, dy;
	float len;
	float dmx, dmy;
	unsigned char flags;
};
typedef struct NVGpoint NVGpoint;

struct NVGpathCache {
	NVGpoint* points;
	int npoints;
	int cpoints;
	NVGpath* paths;
	int npaths;
	int cpaths;
	NVGvertex* verts;
	int nverts;
	int cverts;
	float bounds[4];
};
typedef struct NVGpathCache NVGpathCache;

struct NVGcontext {
	NVGparams params;
	float* commands;
	int ccommands;
	int ncommands;
	float commandx, commandy;
	NVGstate states[NVG_MAX_STATES];
	int nstates;
	NVGpathCache* cache;
	float tessTol;
	float distTol;
	float fringeWidth;
	float devicePxRatio;
	struct FONScontext* fs;
	int fontImages[NVG_MAX_FONTIMAGES];
	int fontImageIdx;
	int drawCallCount;
	int fillTriCount;
	int strokeTriCount;
	int textTriCount;
	NVGpickScene* pickScene;
};

static float nvg__sqrtf(float a) { return sqrtf(a); }
static float nvg__modf(float a, float b) { return fmodf(a, b); }
static float nvg__sinf(float a) { return sinf(a); }
static float nvg__cosf(float a) { return cosf(a); }
static float nvg__tanf(float a) { return tanf(a); }
static float nvg__atan2f(float a,float b) { return atan2f(a, b); }
static float nvg__acosf(float a) { return acosf(a); }

static int nvg__mini(int a, int b) { return a < b ? a : b; }
static int nvg__maxi(int a, int b) { return a > b ? a : b; }
static int nvg__clampi(int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float nvg__minf(float a, float b) { return a < b ? a : b; }
static float nvg__maxf(float a, float b) { return a > b ? a : b; }
static float nvg__absf(float a) { return a >= 0.0f ? a : -a; }
static float nvg__signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static float nvg__clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float nvg__cross(float dx0, float dy0, float dx1, float dy1) { return dx1*dy0 - dx0*dy1; }

static float nvg__normalize(float *x, float* y)
{
	float d = nvg__sqrtf((*x)*(*x) + (*y)*(*y));
	if (d > 1e-6f) {
		float id = 1.0f / d;
		*x *= id;
		*y *= id;
	}
	return d;
}

// Required by nanovg_pick.c
NVGstate* nvg__getState(NVGcontext* ctx);

// Required by nanovg.c
void nvg__pickBeginFrame(NVGcontext* ctx, int width, int height);
void nvg__deletePickScene(NVGpickScene* ps);

#ifdef __cplusplus
}
#endif

#endif /* NANOVG_INTERNAL_H */
