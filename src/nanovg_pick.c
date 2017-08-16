#include "nanovg_internal.h"

static void nvg__segmentDir(NVGpickScene* ps, NVGpickSubPath* psp, NVGsegment* seg, float t, float d[2]);
static void nvg__bezierBounds(const float* points, float* bounds);
static void nvg__bezierInflections(const float* points, int coord, int* ninflections, float* inflections);
static void nvg__splitBezier(const float* points, float t, float* pointsA, float* pointsB);
static void nvg__smallsort(float* values, int n);
static void nvg__pickSceneInsert(NVGpickScene* ps, NVGpickPath* pp);
static NVGpickScene* nvg__pickSceneGet(NVGcontext* ctx);
static NVGpickPath* nvg__allocPickPath(NVGpickScene* ps);
static NVGpickSubPath* nvg__allocPickSubPath(NVGpickScene* ps);

//
// Bounds Utilities
//

static void nvg__initBounds(float bounds[4])
{
	bounds[0] = bounds[1] = 1e6f;
	bounds[2] = bounds[3] = -1e6f;
}

static void nvg__expandBounds(float bounds[4], const float *points, int npoints)
{
	int i;
	npoints *= 2;
	for (i = 0; i < npoints; i += 2) {
		bounds[0] = nvg__minf(bounds[0], points[i]);
		bounds[1] = nvg__minf(bounds[1], points[i+1]);
		bounds[2] = nvg__maxf(bounds[2], points[i]);
		bounds[3] = nvg__maxf(bounds[3], points[i+1]);
	}
}

static void nvg__unionBounds(float bounds[4], const float boundsB[4])
{
	bounds[0] = nvg__minf(bounds[0], boundsB[0]);
	bounds[1] = nvg__minf(bounds[1], boundsB[1]);
	bounds[2] = nvg__maxf(bounds[2], boundsB[2]);
	bounds[3] = nvg__maxf(bounds[3], boundsB[3]);
}

static void nvg__intersectBounds(float bounds[4], const float boundsB[4])
{
	bounds[0] = nvg__maxf(boundsB[0], bounds[0]);
	bounds[1] = nvg__maxf(boundsB[1], bounds[1]);
	bounds[2] = nvg__minf(boundsB[2], bounds[2]);
	bounds[3] = nvg__minf(boundsB[3], bounds[3]);

	bounds[2] = nvg__maxf(bounds[0], bounds[2]);
	bounds[3] = nvg__maxf(bounds[1], bounds[3]);
}

static int nvg__pointInBounds(float x, float y, const float bounds[4])
{
	if (x >= bounds[0] && x <= bounds[2] &&
		y >= bounds[1] && y <= bounds[3])
		return 1;
	else
		return 0;
}

//
// Building paths & sub paths
//

static int nvg__pickSceneAddPoints(NVGpickScene* ps, const float *xy, int n)
{
	int i;

	if (ps->npoints + n > ps->cpoints) {
		int cpoints = ps->npoints + n + (ps->cpoints << 1);
		float* points = realloc(ps->points, sizeof(float) * 2 * cpoints);
		if (points == NULL) return -1;
		ps->points = points;
		ps->cpoints = cpoints;
	}
	i = ps->npoints;
	if (xy != NULL)
		memcpy(&ps->points[i * 2], xy, sizeof(float) * 2 * n);
	ps->npoints += n;
	return i;
}

static void nvg__pickSubPathAddSegment(NVGpickScene* ps, NVGpickSubPath* psp, int firstPoint, int type, short flags)
{
	NVGsegment* seg = NULL;
	if (ps->nsegments == ps->csegments) {
		int csegments = 1 + ps->csegments + (ps->csegments << 1);
		NVGsegment* segments = (NVGsegment*)realloc(ps->segments, sizeof(NVGsegment) * csegments);
		if (segments == NULL) return;
		ps->segments = segments;
		ps->csegments = csegments;
	}

	if (psp->firstSegment == -1)
		psp->firstSegment = ps->nsegments;

	seg = &ps->segments[ps->nsegments];
	++ps->nsegments;
	seg->firstPoint = firstPoint;
	seg->type = (short)type;
	seg->flags = flags;
	++psp->nsegments;

	nvg__segmentDir(ps, psp, seg,  0, seg->startDir);
	nvg__segmentDir(ps, psp, seg,  1, seg->endDir);
}

static void nvg__segmentDir(NVGpickScene* ps, NVGpickSubPath* psp, NVGsegment* seg, float t, float d[2])
{
	float const* points = &ps->points[seg->firstPoint * 2];
	float omt, omt2, t2;
	float x0 = points[0*2+0], x1 = points[1*2+0], x2, x3;
	float y0 = points[0*2+1], y1 = points[1*2+1], y2, y3;

	switch(seg->type) {
	case NVG_LINETO:
		d[0] = x1 - x0;
		d[1] = y1 - y0;
		nvg__normalize(&d[0], &d[1]);
		break;
	case NVG_BEZIERTO:
		x2 = points[2*2+0];
		y2 = points[2*2+1];
		x3 = points[3*2+0];
		y3 = points[3*2+1];

		omt = 1.0f - t;
		omt2 = omt * omt;
		t2 = t * t;

		d[0] = 3.0f * omt2 * (x1 - x0) +
			6.0f * omt * t * (x2 - x1) +
			3.0f * t2 * (x3 - x2);
		d[1] = 3.0f * omt2 * (y1 - y0) +
			6.0f * omt * t * (y2 - y1) +
			3.0f * t2 * (y3 - y2);

		nvg__normalize(&d[0], &d[1]);
		break;
	}
}

static void nvg__pickSubPathAddFillSupports(NVGpickScene* ps, NVGpickSubPath* psp)
{
	NVGsegment* segments = &ps->segments[psp->firstSegment];
	int s;
	for (s = 0; s < psp->nsegments; ++s) {
		NVGsegment* seg = &segments[s];
		const float* points = &ps->points[seg->firstPoint * 2];

		if (seg->type == NVG_LINETO) {
			nvg__initBounds(seg->bounds);
			nvg__expandBounds(seg->bounds, points, 2);
		} else {
			nvg__bezierBounds(points, seg->bounds);
		}
	}
}

static void nvg__pickSubPathAddStrokeSupports(NVGpickScene* ps, NVGpickSubPath* psp, float strokeWidth, int lineCap, int lineJoin, float miterLimit)
{
	int closed = psp->closed;
	const float* points = ps->points;

	NVGsegment* seg = NULL;
	NVGsegment* segments = &ps->segments[psp->firstSegment];
	int nsegments = psp->nsegments;
	NVGsegment* prevseg = psp->closed ? &segments[psp->nsegments - 1] : NULL;

	int ns = 0; // nsupports
	float supportingPoints[32];
	int firstPoint, lastPoint;
	int s;

	if (closed == 0) {
		segments[0].flags |= NVG_PICK_CAP;
		segments[nsegments - 1].flags |= NVG_PICK_ENDCAP;
	}

	for (s = 0; s < nsegments; ++s) {
		seg = &segments[s];
		nvg__initBounds(seg->bounds);

		firstPoint = seg->firstPoint * 2;
		lastPoint = firstPoint + ((seg->type == NVG_LINETO) ? 2 : 6);

		ns = 0;

		// First two supporting points are either side of the start point
		supportingPoints[ns++] = points[firstPoint] - seg->startDir[1] * strokeWidth;
		supportingPoints[ns++] = points[firstPoint+1] + seg->startDir[0] * strokeWidth;

		supportingPoints[ns++] = points[firstPoint] + seg->startDir[1] * strokeWidth;
		supportingPoints[ns++] = points[firstPoint+1] - seg->startDir[0] * strokeWidth;

		// Second two supporting points are either side of the end point
		supportingPoints[ns++] = points[lastPoint] - seg->endDir[1] * strokeWidth;
		supportingPoints[ns++] = points[lastPoint+1] + seg->endDir[0] * strokeWidth;

		supportingPoints[ns++] = points[lastPoint] + seg->endDir[1] * strokeWidth;
		supportingPoints[ns++] = points[lastPoint+1] - seg->endDir[0] * strokeWidth;

		if (seg->flags & NVG_PICK_CORNER && prevseg != NULL) {
			float M2;

			seg->miterDir[0] = 0.5f * (-prevseg->endDir[1] - seg->startDir[1]);
			seg->miterDir[1] = 0.5f * (prevseg->endDir[0] + seg->startDir[0]);

			M2 = seg->miterDir[0] * seg->miterDir[0] + seg->miterDir[1] * seg->miterDir[1];

			if (M2 > 0.000001f) {
				float scale = 1.0f / M2;
				if (scale > 600.0f) {
					scale = 600.0f;
				}
				seg->miterDir[0] *= scale;
				seg->miterDir[1] *= scale;
			}

			NVG_PICK_DEBUG_VECTOR_SCALE(&points[firstPoint], seg->miterDir, 10);

			// Add an additional support at the corner on the other line
			supportingPoints[ns++] = points[firstPoint] - prevseg->endDir[1] * strokeWidth;
			supportingPoints[ns++] = points[firstPoint+1] + prevseg->endDir[0] * strokeWidth;

			if (lineJoin == NVG_MITER || lineJoin == NVG_BEVEL) {
				// Set a corner as beveled if the join type is bevel or mitered and
				// miterLimit is hit.
				if (lineJoin == NVG_BEVEL ||
					((M2 * miterLimit * miterLimit) < 1.0f)) {
					seg->flags |= NVG_PICK_BEVEL;
				} else {
					// Corner is mitered - add miter point as a support
					supportingPoints[ns++] = points[firstPoint] + seg->miterDir[0] * strokeWidth;
					supportingPoints[ns++] = points[firstPoint+1] + seg->miterDir[1] * strokeWidth;
				}
			} else if (lineJoin == NVG_ROUND) {
				// ... and at the midpoint of the corner arc

				float vertexN[2] = { -seg->startDir[0] + prevseg->endDir[0], -seg->startDir[1] + prevseg->endDir[1] };
				nvg__normalize(&vertexN[0], &vertexN[1]);

				supportingPoints[ns++] = points[firstPoint] + vertexN[0] * strokeWidth;
				supportingPoints[ns++] = points[firstPoint+1] + vertexN[1] * strokeWidth;
			}
		}

		if (seg->flags & NVG_PICK_CAP) {
			switch(lineCap) {
			case NVG_BUTT:
				// Supports for butt already added.
				break;
			case NVG_SQUARE:
				// Square cap supports are just the original two supports moved
				// out along the direction
				supportingPoints[ns++] = supportingPoints[0] - seg->startDir[0] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[1] - seg->startDir[1] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[2] - seg->startDir[0] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[3] - seg->startDir[1] * strokeWidth;
				break;
			case NVG_ROUND:
				// Add one additional support for the round cap along the dir
				supportingPoints[ns++] = points[firstPoint] - seg->startDir[0] * strokeWidth;
				supportingPoints[ns++] = points[firstPoint+1] - seg->startDir[1] * strokeWidth;
				break;
			}
		}

		if (seg->flags & NVG_PICK_ENDCAP) {
			// End supporting points, either side of line
			int end = 4;

			switch(lineCap) {
			case NVG_BUTT:
				// Supports for butt already added.
				break;
			case NVG_SQUARE:
				// Square cap supports are just the original two supports moved
				// out along the direction
				supportingPoints[ns++] = supportingPoints[end + 0] + seg->endDir[0] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[end + 1] + seg->endDir[1] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[end + 2] + seg->endDir[0] * strokeWidth;
				supportingPoints[ns++] = supportingPoints[end + 3] + seg->endDir[1] * strokeWidth;
				break;
			case NVG_ROUND:
				// Add one additional support for the round cap along the dir
				supportingPoints[ns++] = points[lastPoint] + seg->endDir[0] * strokeWidth;
				supportingPoints[ns++] = points[lastPoint+1] + seg->endDir[1] * strokeWidth;
				break;
			}
		}

		nvg__expandBounds(seg->bounds, supportingPoints, ns / 2);

		prevseg = seg;
	}
}

static NVGpickPath* nvg__pickPathCreate(NVGcontext* context, int id, int forStroke)
{
	NVGpickScene* ps = nvg__pickSceneGet(context);

	int i = 0;

	int ncommands = context->ncommands;
	float* commands = context->commands;

	NVGpickPath* pp = NULL;
	NVGpickSubPath* psp = NULL;
	float start[2];
	int firstPoint;

	int hasHoles = 0;
	NVGpickSubPath* prev = NULL;

	float points[8];
	float inflections[2];
	int ninflections = 0;

	NVGstate* state = nvg__getState(context);
	float totalBounds[4];
	NVGsegment* segments = NULL;
	const NVGsegment* seg = NULL;
	NVGpickSubPath *curpsp;
	int s;

	pp = nvg__allocPickPath(ps);
	if (pp == NULL) return NULL;

	pp->id = id;

	while (i < ncommands) {
		int cmd = (int)commands[i];

		switch (cmd) {
		case NVG_MOVETO:
			start[0] = commands[i+1];
			start[1] = commands[i+2];

			// Start a new path for each sub path to handle sub paths that
			// intersect other sub paths.
			prev = psp;
			psp = nvg__allocPickSubPath(ps);
			if (psp == NULL) { psp = prev; break; }
			psp->firstSegment = -1;
			psp->winding = NVG_SOLID;
			psp->next = prev;

			nvg__pickSceneAddPoints(ps, &commands[i+1], 1);
			i += 3;
			break;
		case NVG_LINETO:
			firstPoint = nvg__pickSceneAddPoints(ps, &commands[i+1], 1);
			nvg__pickSubPathAddSegment(ps, psp, firstPoint - 1, cmd, NVG_PICK_CORNER);
			i += 3;
			break;
		case NVG_BEZIERTO:
			// Split the curve at it's dx==0 or dy==0 inflection points.
			// Thus:
			//		A horizontal line only ever interects the curves once.
			//	and
			//		Finding the closest point on any curve converges more reliably.

			// NOTE: We could just split on dy==0 here.

			memcpy(&points[0], &ps->points[(ps->npoints - 1) * 2], sizeof(float) * 2);
			memcpy(&points[2], &commands[i+1], sizeof(float) * 2 * 3);

			ninflections = 0;
			nvg__bezierInflections(points, 1, &ninflections, inflections);
			nvg__bezierInflections(points, 0, &ninflections, inflections);

			if (ninflections) {
				float previnfl = 0;
				float t;

				float pointsA[8], pointsB[8];

				int infl;

				nvg__smallsort(inflections, ninflections);

				for (infl = 0; infl < ninflections; ++infl) {
					if (nvg__absf(inflections[infl] - previnfl) < NVG_PICK_EPS)
						continue;

					t = (inflections[infl] - previnfl) * (1.0f / (1.0f - previnfl));

					previnfl = inflections[infl];

					nvg__splitBezier(points, t, pointsA, pointsB);

					firstPoint = nvg__pickSceneAddPoints(ps, &pointsA[2], 3);
					nvg__pickSubPathAddSegment(ps, psp, firstPoint - 1, cmd, (infl == 0) ? NVG_PICK_CORNER : 0);

					memcpy(points, pointsB, sizeof(float) * 8);
				}

				firstPoint = nvg__pickSceneAddPoints(ps, &pointsB[2], 3);
				nvg__pickSubPathAddSegment(ps, psp, firstPoint - 1, cmd, 0);
			} else {
				firstPoint = nvg__pickSceneAddPoints(ps, &commands[i+1], 3);
				nvg__pickSubPathAddSegment(ps, psp, firstPoint - 1, cmd, NVG_PICK_CORNER);
			}
			i += 7;
			break;
		case NVG_CLOSE:
			if (ps->points[(ps->npoints - 1) * 2] != start[0] ||
				ps->points[(ps->npoints - 1) * 2 + 1] != start[1]) {
				firstPoint = nvg__pickSceneAddPoints(ps, start, 1);
				nvg__pickSubPathAddSegment(ps, psp, firstPoint - 1, NVG_LINETO, NVG_PICK_CORNER);
			}
			psp->closed = 1;

			i++;
			break;
		case NVG_WINDING:
			psp->winding = (short)(int)commands[i+1];
			if (psp->winding == NVG_HOLE)
				hasHoles = 1;
			i += 2;
			break;
		default:
			i++;
			break;
		}
	}

	pp->flags = forStroke ? NVG_PICK_STROKE : NVG_PICK_FILL;
	pp->subPaths = psp;
	pp->strokeWidth = state->strokeWidth * 0.5f;
	pp->miterLimit = state->miterLimit;
	pp->lineCap = (short)state->lineCap;
	pp->lineJoin = (short)state->lineJoin;

	nvg__initBounds(totalBounds);

	for (curpsp = psp; curpsp; curpsp = curpsp->next) {
		if (forStroke)
			nvg__pickSubPathAddStrokeSupports(ps, curpsp, pp->strokeWidth, pp->lineCap, pp->lineJoin, pp->miterLimit);
		else
			nvg__pickSubPathAddFillSupports(ps, curpsp);

		segments = &ps->segments[curpsp->firstSegment];
		nvg__initBounds(curpsp->bounds);
		for (s = 0; s < curpsp->nsegments; ++s) {
			seg = &segments[s];
			NVG_PICK_DEBUG_BOUNDS(seg->bounds);
			nvg__unionBounds(curpsp->bounds, seg->bounds);
		}

		nvg__unionBounds(totalBounds, curpsp->bounds);
	}

	// Store the scissor rect if present.
	if (state->scissor.extent[0] != -1.0f) {
		// Use points storage to store the scissor data
		float* scissor = NULL;
		pp->scissor = nvg__pickSceneAddPoints(ps, NULL, 4);
		scissor = &ps->points[pp->scissor*2];

		memcpy(scissor, state->scissor.xform, 6 * sizeof(float));
		memcpy(scissor + 6, state->scissor.extent, 2 * sizeof(float));

		pp->flags |= NVG_PICK_SCISSOR;
	}

	memcpy(pp->bounds, totalBounds, sizeof(float) * 4);

	return pp;
}

// Struct management

static NVGpickPath* nvg__allocPickPath(NVGpickScene* ps)
{
	NVGpickPath* pp = ps->freePaths;
	if (pp) {
		ps->freePaths = pp->next;
	} else {
		pp = (NVGpickPath*)malloc(sizeof(NVGpickPath));
	}
	memset(pp, 0, sizeof(NVGpickPath));
	return pp;
}

// Return a pick path and any sub paths to the free lists
static void nvg__freePickPath(NVGpickScene* ps, NVGpickPath* pp)
{
	// Add all sub paths to the sub path free list.
	// Finds the end of the path sub paths, links that to the current
	// sub path free list head and replaces the head ptr with the
	// head path sub path entry.
	NVGpickSubPath* psp = NULL;
	for (psp = pp->subPaths; psp && psp->next; psp = psp->next) {
	}

	if (psp) {
		psp->next = ps->freeSubPaths;
		ps->freeSubPaths = pp->subPaths;
	}
	pp->subPaths = NULL;

	// Add the path to the path freelist
	pp->next = ps->freePaths;
	ps->freePaths = pp;
	if (pp->next == NULL)
		ps->lastPath = pp;
}

static NVGpickSubPath* nvg__allocPickSubPath(NVGpickScene* ps)
{
	NVGpickSubPath* psp = ps->freeSubPaths;
	if (psp) {
		ps->freeSubPaths = psp->next;
	} else {
		psp = (NVGpickSubPath*)malloc(sizeof(NVGpickSubPath));
		if (psp == NULL) return NULL;
	}
	memset(psp, 0, sizeof(NVGpickSubPath));
	return psp;
}

static void nvg__returnPickSubPath(NVGpickScene* ps, NVGpickSubPath* psp)
{
	psp->next = ps->freeSubPaths;
	ps->freeSubPaths = psp;
}

static NVGpickScene* nvg__allocPickScene()
{
	NVGpickScene* ps = (NVGpickScene*)malloc(sizeof(NVGpickScene));
	if (ps == NULL) return NULL;
	memset(ps, 0, sizeof(NVGpickScene));
	ps->nlevels = 5;
	return ps;
}

void nvg__deletePickScene(NVGpickScene* ps)
{
	NVGpickPath* pp;
	NVGpickSubPath* psp;

	// Add all paths (and thus sub paths)  to the free list(s).
	while(ps->paths) {
		pp = ps->paths->next;
		nvg__freePickPath(ps, ps->paths);
		ps->paths = pp;
	}

	// Delete all paths
	while(ps->freePaths) {
		pp = ps->freePaths;
		ps->freePaths = pp->next;

		while (pp->subPaths) {
			psp = pp->subPaths;
			pp->subPaths = psp->next;
			free(psp);
		}
		free(pp);
	}

	// Delete all sub paths
	while(ps->freeSubPaths) {
		psp = ps->freeSubPaths->next;
		free(ps->freeSubPaths);
		ps->freeSubPaths = psp;
	}

	ps->npoints = 0;
	ps->nsegments = 0;

	if (ps->levels) {
		free(ps->levels[0]);
		free(ps->levels);
	}

	if (ps->picked)
		free(ps->picked);
	if (ps->points)
		free(ps->points);
	if (ps->segments)
		free(ps->segments);

	free(ps);
}

static NVGpickScene* nvg__pickSceneGet(NVGcontext* ctx)
{
	if (ctx->pickScene == NULL)
		ctx->pickScene = nvg__allocPickScene();

	return ctx->pickScene;
}

// Marks the fill of the current path as pickable with the specified id.
void nvgFillHitRegion(NVGcontext* ctx, int id)
{
	NVGpickScene* ps = nvg__pickSceneGet(ctx);

	NVGpickPath* pp = nvg__pickPathCreate(ctx, id, 0);

	nvg__pickSceneInsert(ps, pp);
}

// Marks the stroke of the current path as pickable with the specified id.
void nvgStrokeHitRegion(NVGcontext* ctx, int id)
{
	NVGpickScene* ps = nvg__pickSceneGet(ctx);

	NVGpickPath* pp = nvg__pickPathCreate(ctx, id, 1);

	nvg__pickSceneInsert(ps, pp);
}

// Applies Casteljau's algorithm to a cubic bezier for a given parameter t
// points is 4 points (8 floats)
// lvl1 is 3 points (6 floats)
// lvl2 is 2 points (4 floats)
// lvl3 is 1 point (2 floats)
static void nvg__casteljau(const float* points, float t, float* lvl1, float* lvl2, float* lvl3)
{
	int x0 = 0*2+0, x1 = 1*2+0, x2 = 2*2+0, x3 = 3*2+0;
	int y0 = 0*2+1, y1 = 1*2+1, y2 = 2*2+1, y3 = 3*2+1;

	// Level 1
	lvl1[x0] = (points[x1] - points[x0]) * t + points[x0];
	lvl1[y0] = (points[y1] - points[y0]) * t + points[y0];

	lvl1[x1] = (points[x2] - points[x1]) * t + points[x1];
	lvl1[y1] = (points[y2] - points[y1]) * t + points[y1];

	lvl1[x2] = (points[x3] - points[x2]) * t + points[x2];
	lvl1[y2] = (points[y3] - points[y2]) * t + points[y2];

	// Level 2
	lvl2[x0] = (lvl1[x1] - lvl1[x0]) * t + lvl1[x0];
	lvl2[y0] = (lvl1[y1] - lvl1[y0]) * t + lvl1[y0];

	lvl2[x1] = (lvl1[x2] - lvl1[x1]) * t + lvl1[x1];
	lvl2[y1] = (lvl1[y2] - lvl1[y1]) * t + lvl1[y1];

	// Level 3
	lvl3[x0] = (lvl2[x1] - lvl2[x0]) * t + lvl2[x0];
	lvl3[y0] = (lvl2[y1] - lvl2[y0]) * t + lvl2[y0];
}

// Calculates a point on a bezier at point t.
static void nvg__bezierEval(const float* points, float t, float* tpoint)
{
	float omt = 1 - t;

	float omt3 = omt * omt * omt;
	float omt2 = omt * omt;
	float t3 = t * t * t;
	float t2 = t * t;

	tpoint[0] = points[0] * omt3 +
				points[2] * 3.0f * omt2 * t +
				points[4] * 3.0f * omt * t2 +
				points[6] * t3;

	tpoint[1] = points[1] * omt3 +
				points[3] * 3.0f * omt2 * t +
				points[5] * 3.0f * omt * t2 +
				points[7] * t3;
}

// Splits a cubic bezier curve into two parts at point t.
static void nvg__splitBezier(const float* points, float t, float* pointsA, float* pointsB)
{
	int x0 = 0*2+0, x1 = 1*2+0, x2 = 2*2+0, x3 = 3*2+0;
	int y0 = 0*2+1, y1 = 1*2+1, y2 = 2*2+1, y3 = 3*2+1;

	float lvl1[6], lvl2[4], lvl3[2];

	nvg__casteljau(points, t, lvl1, lvl2, lvl3);

	// First half
	pointsA[x0] = points[x0];
	pointsA[y0] = points[y0];

	pointsA[x1] = lvl1[x0];
	pointsA[y1] = lvl1[y0];

	pointsA[x2] = lvl2[x0];
	pointsA[y2] = lvl2[y0];

	pointsA[x3] = lvl3[x0];
	pointsA[y3] = lvl3[y0];

	// Second half
	pointsB[x0] = lvl3[x0];
	pointsB[y0] = lvl3[y0];

	pointsB[x1] = lvl2[x1];
	pointsB[y1] = lvl2[y1];

	pointsB[x2] = lvl1[x2];
	pointsB[y2] = lvl1[y2];

	pointsB[x3] = points[x3];
	pointsB[y3] = points[y3];
}

// Calculates the inflection points in coordinate coord (X = 0, Y = 1) of a cubic bezier.
// Appends any found inflection points to the array inflections and increments *ninflections.
// So finds the parameters where dx/dt or dy/dt is 0
static void nvg__bezierInflections(const float* points, int coord, int* ninflections, float* inflections)
{
	float v0 = points[0*2+coord], v1 = points[1*2+coord], v2 = points[2*2+coord], v3 = points[3*2+coord];
	float t[2];
	float a,b,c,d;
	int nvalid = *ninflections;

	a = 3.0f * ( -v0 + 3.0f * v1 - 3.0f * v2 + v3 );
	b = 6.0f * ( v0 - 2.0f * v1 + v2 );
	c = 3.0f * ( v1 - v0 );

	d = b*b - 4.0f * a * c;
	if ( nvg__absf(d - 0.0f) < NVG_PICK_EPS) {
		// Zero or one root
		t[0] = -b / 2.0f * a;
		if (t[0] > NVG_PICK_EPS && t[0] < (1.0f - NVG_PICK_EPS)) {
			inflections[nvalid] = t[0];
			++nvalid;
		}
	} else if (d > NVG_PICK_EPS) {
		int i;

		// zero, one or two roots
		d = nvg__sqrtf(d);

		t[0] = (-b + d) / (2.0f * a);
		t[1] = (-b - d) / (2.0f * a);

		for (i = 0; i < 2; ++i) {
			if (t[i] > NVG_PICK_EPS && t[i] < (1.0f - NVG_PICK_EPS)) {
				inflections[nvalid] = t[i];
				++nvalid;
			}
		}
	} else {
		// zero roots
	}

	*ninflections = nvalid;
}

// Sort a small number of floats in ascending order (0 < n < 6)
static void nvg__smallsort(float* values, int n)
{
	float tmp;
	int bSwapped = 1;
	int i,j;

	for (j = 0; (j < n - 1) && bSwapped;++j) {
		bSwapped = 0;
		for (i = 0; i < n - 1; ++i) {
			if (values[i] > values[i+1]) {
				tmp = values[i];
				values[i] = values[i+1];
				values[i+1] = tmp;
			}
		}
	}
}

// Calculates the bounding rect of a given cubic bezier curve.
static void nvg__bezierBounds(const float* points, float* bounds)
{
	float inflections[4];
	int ninflections = 0;
	float tpoint[2];
	int i;

	nvg__initBounds(bounds);

	// Include start and end points in bounds
	nvg__expandBounds(bounds, &points[0], 1);
	nvg__expandBounds(bounds, &points[6], 1);

	// Calculate dx==0 and dy==0 inflection points and add then
	// to the bounds

	nvg__bezierInflections(points, 0, &ninflections, inflections);
	nvg__bezierInflections(points, 1, &ninflections, inflections);

	for (i = 0; i < ninflections; ++i) {
		nvg__bezierEval(points, inflections[i], tpoint);
		nvg__expandBounds(bounds, tpoint, 1);
	}
}

// Checks to see if a line originating from x,y along the +ve x axis
// intersects the given line (points[0],points[1]) -> (points[2], points[3]).
// Returns 1 on intersection, 0 on no intersection.
// Horizontal lines are never hit.
static int nvg__intersectLine(const float* points, float x, float y)
{
	float x1 = points[0];
	float y1 = points[1];
	float x2 = points[2];
	float y2 = points[3];

	float d = y2 - y1;
	float s, lineX;

	if ( d > NVG_PICK_EPS || d < -NVG_PICK_EPS ) {
		s = (x2 - x1) / d;
		lineX = x1 + (y - y1) * s;
		return lineX > x;
	}
	else return 0;
}

// Checks to see if a line originating from x,y along the +ve x axis
// intersects the given bezier.
// It is assumed that the line originates from within the bounding box of
// the bezier and that the curve has no dy=0 inflection points.
// Returns the number of intersections found (which is either 1 or 0)
static int nvg__intersectBezier(const float* points, float x, float y)
{
	float x0 = points[0*2+0], x1 = points[1*2+0], x2 = points[2*2+0], x3 = points[3*2+0];
	float y0 = points[0*2+1], y1 = points[1*2+1], y2 = points[2*2+1], y3 = points[3*2+1];

	float t;
	float ty;
	float dty;
	float omt, omt2, omt3, t2, t3;

	if (y0 == y1 && y1 == y2 && y2 == y3) return 0;

	// Initial t guess
	if (y3 != y0) t = (y - y0) / (y3 - y0);
	else if (x3 != x0) t = (x - x0) / (x3 - x0);
	else t = 0.5f;

	// A few Newton iterations
	int iter;
	for (iter = 0; iter < 6; ++iter) {
		omt = 1 - t;
		omt2 = omt * omt;
		t2 = t * t;
		omt3 = omt2 * omt;
		t3 = t2 * t;

		ty = y0 * omt3 +
			y1 * 3.0f * omt2 * t +
			y2 * 3.0f * omt * t2 +
			y3 * t3;

		// Newton iteration
		dty = 3.0f * omt2 * (y1 - y0) +
			6.0f * omt * t * (y2 - y1) +
			3.0f * t2 * (y3 - y2);

		// dty will never == 0 since:
		//  Either omt, omt2 are zero OR t2 is zero
		//	y0 != y1 != y2 != y3 (checked above)
		t = t - ( ty - y ) / dty;
 	}

	omt = 1 - t;
	omt2 = omt * omt;
	t2 = t * t;
	omt3 = omt2 * omt;
	t3 = t2 * t;
	float tx;

	tx = x0 * omt3 +
		x1 * 3.0f * omt2 * t +
		x2 * 3.0f * omt * t2 +
		x3 * t3;

	if (tx > x)
		return 1;
	else
		return 0;
}

// Finds the closest point on a line to a given point
static void nvg__closestLine(const float* points, float x, float y, float* closest, float* ot)
{
	float x1 = points[0];
	float y1 = points[1];
	float x2 = points[2];
	float y2 = points[3];
	float pqx = x2 - x1;
	float pqz = y2 - y1;
	float dx = x - x1;
	float dz = y - y1;
	float d = pqx*pqx + pqz*pqz;
	float t = pqx*dx + pqz*dz;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	closest[0] = x1 + t*pqx;
	closest[1] = y1 + t*pqz;
	*ot = t;
}

// Finds the closest point on a curve for a given point (x,y).
// Assumes that the curve has no dx==0 or dy==0 inflection points.
static void nvg__closestBezier(const float* points, float x, float y, float* closest, float *ot)
{
	float x0 = points[0*2+0], x1 = points[1*2+0], x2 = points[2*2+0], x3 = points[3*2+0];
	float y0 = points[0*2+1], y1 = points[1*2+1], y2 = points[2*2+1], y3 = points[3*2+1];

	// This assumes that the curve has no dy=0 inflection points.

	// Initial t guess
	float t = 0.5f;
	float ty, dty, ddty, errory;
	float tx, dtx, ddtx, errorx;
	float omt, omt2, omt3, t2, t3;
	float n, d;

	// A few Newton iterations
	int iter;
	for (iter = 0; iter < 6; ++iter) {
		omt = 1 - t;
		omt2 = omt * omt;
		t2 = t * t;
		omt3 = omt2 * omt;
		t3 = t2 * t;

		ty = y0 * omt3 +
			y1 * 3.0f * omt2 * t +
			y2 * 3.0f * omt * t2 +
			y3 * t3;

		tx = x0 * omt3 +
			x1 * 3.0f * omt2 * t +
			x2 * 3.0f * omt * t2 +
			x3 * t3;

		// Newton iteration
		dty = 3.0f * omt2 * (y1 - y0) +
			6.0f * omt * t * (y2 - y1)+
			3.0f * t2 * (y3 - y2);

		ddty = 6.0f * omt * (y2 - 2.0f * y1 + y0) +
			6.0f * t * (y3 - 2.0f * y2 + y1);

		dtx = 3.0f * omt2 * (x1 - x0) +
			6.0f * omt * t * (x2 - x1) +
			3.0f * t2 * (x3 - x2);

		ddtx = 6.0f * omt * (x2 - 2.0f * x1 + x0) +
			6.0f * t * (x3 - 2.0f * x2 + x1);

		errorx = tx - x;
		errory = ty - y;

		n = errorx * dtx + errory * dty;
		if (n == 0.0f) break;

		d = dtx * dtx + dty * dty + errorx * ddtx + errory * ddty;
		if (d != 0.0f) t = t - n / d;
		else break;
 	}

 	t = nvg__maxf(0, nvg__minf(1.0 ,t));

 	*ot = t;

	omt = 1 - t;
	omt2 = omt * omt;
	t2 = t * t;
	omt3 = omt2 * omt;
	t3 = t2 * t;

	ty = y0 * omt3 +
		y1 * 3.0f * omt2 * t +
		y2 * 3.0f * omt * t2 +
		y3 * t3;

	tx = x0 * omt3 +
		x1 * 3.0f * omt2 * t +
		x2 * 3.0f * omt * t2 +
		x3 * t3;

	closest[0] = tx;
	closest[1] = ty;
}


// Returns:
//	1	If (x,y) is contained by the stroke of the path
//	0	If (x,y) is not contained by the path.
static int nvg__pickSubPathStroke(const NVGpickScene* ps, const NVGpickSubPath* psp, float x, float y, float strokeWidth, int lineCap, int lineJoin)
{
	int nsegments;
	const NVGsegment * seg;
	float closest[2];
	float t;

	float distSqd = 0;
	float d[2];
	float strokeWidthSqd;

	const NVGsegment* prevseg;
	int s;

	if (nvg__pointInBounds(x, y, psp->bounds) == 0)
		return 0;

	// Trace a line from x,y out along the positive x axis and count the
	// number of intersections.
	nsegments = psp->nsegments;
	seg = ps->segments + psp->firstSegment;

	strokeWidthSqd = strokeWidth * strokeWidth;

	prevseg = psp->closed ? &ps->segments[psp->firstSegment + nsegments - 1] : NULL;

	for (s = 0; s < nsegments; ++s, prevseg=seg, ++seg) {
		if (nvg__pointInBounds(x, y, seg->bounds) != 0) {
			// Line potentially hits stroke.
			switch(seg->type) {
			case NVG_LINETO:
				nvg__closestLine(&ps->points[seg->firstPoint * 2], x, y, closest, &t);
				break;

			case NVG_BEZIERTO:
				nvg__closestBezier(&ps->points[seg->firstPoint * 2], x, y, closest, &t);
				break;

			default:
					continue;
			}

			d[0] = x - closest[0];
			d[1] = y - closest[1];

			if ((t >= NVG_PICK_EPS && t <= (1.0f - NVG_PICK_EPS)) ||
				!(seg->flags & (NVG_PICK_CORNER | NVG_PICK_CAP | NVG_PICK_ENDCAP)) ||
				(lineJoin == NVG_ROUND)) {
				// Closest point is in the middle of the line/curve, at a rounded join/cap
				// or at a smooth join
				distSqd = d[0] * d[0] + d[1] * d[1];
				if (distSqd < strokeWidthSqd)
					return 1;
			} else if ( ( (t > (1.0f - NVG_PICK_EPS)) && (seg->flags & NVG_PICK_ENDCAP)) ||
						( (t < NVG_PICK_EPS) && (seg->flags & NVG_PICK_CAP) ) ) {
				float dirD;
				switch(lineCap) {
				case NVG_BUTT:
					distSqd = d[0] * d[0] + d[1] * d[1];
					if (t < NVG_PICK_EPS)
						dirD = -(d[0] * seg->startDir[0] + d[1] * seg->startDir[1]);
					else
						dirD = d[0] * seg->endDir[0] + d[1] * seg->endDir[1];

					if (dirD < -NVG_PICK_EPS && distSqd < strokeWidthSqd)
						return 1;
					break;
				case NVG_SQUARE:
					if ( nvg__absf(d[0]) < strokeWidth && nvg__absf(d[1]) < strokeWidth)
						return 1;
					break;
				case NVG_ROUND:
					distSqd = d[0] * d[0] + d[1] * d[1];
					if (distSqd < strokeWidthSqd)
						return 1;
					break;
				}
			} else if (seg->flags & NVG_PICK_CORNER) {
				// Closest point is at a corner

				const NVGsegment* seg0;
				const NVGsegment* seg1;

				if (t < NVG_PICK_EPS) {
					seg0 = prevseg;
					seg1 = seg;
				} else {
					seg0 = seg;
					seg1 = (s == (nsegments-1)) ? &ps->segments[psp->firstSegment] : (seg+1);
				}

				if (!(seg1->flags & NVG_PICK_BEVEL)) {
					float prevNDist, curNDist;

					prevNDist = -seg0->endDir[1] * d[0] + seg0->endDir[0] * d[1];
					curNDist = seg1->startDir[1] * d[0] - seg1->startDir[0] * d[1];

					if (nvg__absf(prevNDist) < strokeWidth &&
						nvg__absf(curNDist) < strokeWidth) {
						return 1;
					}
				} else {
					d[0] -= -seg1->startDir[1] * strokeWidth;
					d[1] -= +seg1->startDir[0] * strokeWidth;

					if ( (seg1->miterDir[0] * d[0] + seg1->miterDir[1] * d[1]) < 0.0f )
						return 1;
				}
			}
		}
	}

	return 0;

}

// Returns:
//	1	If (x,y) is contained by the path and the path is solid.
//	-1 	If (x,y) is contained by the path and the path is a hole.
//	0	If (x,y) is not contained by the path.
static int nvg__pickSubPath(const NVGpickScene* ps, const NVGpickSubPath* psp, float x, float y)
{
	int nsegments = psp->nsegments;
	NVGsegment const* seg = &ps->segments[psp->firstSegment];
	int nintersections = 0;
	int s;

	if (nvg__pointInBounds(x, y, psp->bounds) == 0)
		return 0;

	// Trace a line from x,y out along the positive x axis and count the
	// number of intersections.

	for (s = 0; s < nsegments; ++s, ++seg) {
		if ((seg->bounds[1] - NVG_PICK_EPS) < y &&
			(seg->bounds[3] - NVG_PICK_EPS) > y &&
			seg->bounds[2] > x) {
			// Line hits the box.
			switch(seg->type) {
			case NVG_LINETO:
				if (seg->bounds[0] > x)
					// Line originates outside the box.
					++nintersections;
				else
					// Line originates inside the box.
					nintersections += nvg__intersectLine(&ps->points[seg->firstPoint * 2], x, y);
				break;

			case NVG_BEZIERTO:
				if (seg->bounds[0] > x)
					// Line originates outside the box.
					++nintersections;
				else
					// Line originates inside the box.
					nintersections += nvg__intersectBezier(&ps->points[seg->firstPoint * 2], x, y);
				break;

			default:
				break;
			}
		}
	}

	if (nintersections & 1)
		return (psp->winding == NVG_SOLID) ? 1 : -1;
	else
		return 0;
}

static int nvg__pickPath(const NVGpickScene* ps, const NVGpickPath* pp, float x, float y)
{
	int pickCount = 0;
	NVGpickSubPath* psp = pp->subPaths;
	while(psp) {
		pickCount += nvg__pickSubPath(ps, psp, x, y);
		psp = psp->next;
	}

	return (pickCount != 0) ? 1 : 0;
}


static int nvg__pickPathStroke(const NVGpickScene* ps, const NVGpickPath* pp, float x, float y)
{
	NVGpickSubPath* psp = pp->subPaths;
	while(psp) {
		if (nvg__pickSubPathStroke(ps, psp, x, y, pp->strokeWidth, pp->lineCap, pp->lineJoin))
			return 1;
		psp = psp->next;
	}

	return 0;
}

static int nvg__comparePaths(const void* a, const void* b)
{
	NVGpickPath* pathA = *(NVGpickPath**)a;
	NVGpickPath* pathB = *(NVGpickPath**)b;

	return pathB->order - pathA->order;
}

static int nvg__pickPathTestBounds(const NVGpickScene* ps, const NVGpickPath* pp, float x, float y)
{
	if (nvg__pointInBounds(x, y, pp->bounds) != 0) {
		if (pp->flags & NVG_PICK_SCISSOR) {
			const float* scissor = &ps->points[pp->scissor*2];
			float rx = x - scissor[4];
			float ry = y - scissor[5];

			if ( nvg__absf((scissor[0] * rx) + (scissor[1] * ry)) > scissor[6] ||
				 nvg__absf((scissor[2] * rx) + (scissor[3] * ry)) > scissor[7])
				return 0;
		}

		return 1;
	}

	return 0;
}

// Fills ids with a list of the top most maxids ids under the specified position.
// Returns the number of ids written into ids, up to maxids.
int nvgHitTestAll(NVGcontext* ctx, float x, float y, int flags, int* ids, int maxids)
{
	NVGpickScene* ps;
	int npicked = 0;
	int hit;
	int lvl;
	int id;
	int levelwidth;
	int cellx, celly;

	if (ctx->pickScene == NULL)
		return 0;

	ps = ctx->pickScene;
	levelwidth = 1 << (ps->nlevels - 1);
	cellx = nvg__clampi((int)(x / ps->xdim), 0, levelwidth);
	celly = nvg__clampi((int)(y / ps->ydim), 0, levelwidth);

	for (lvl = ps->nlevels - 1; lvl >= 0; --lvl) {
		NVGpickPath* pp = ps->levels[lvl][celly*levelwidth + cellx];
		while(pp) {
			if (nvg__pickPathTestBounds(ps, pp, x, y)) {

				hit = 0;
				if ((flags & NVG_TEST_STROKE) && (pp->flags & NVG_PICK_STROKE))
					hit = nvg__pickPathStroke(ps, pp, x, y);

				if (!hit && (flags & NVG_TEST_FILL) && (pp->flags & NVG_PICK_FILL))
					hit = nvg__pickPath(ps, pp, x, y);

				if (hit) {
					if (npicked == ps->cpicked) {
						int cpicked = ps->cpicked + ps->cpicked;
						NVGpickPath** picked = realloc(ps->picked, sizeof(NVGpickPath*) * ps->cpicked);
						if (picked == NULL)
							break;
						ps->cpicked = cpicked;
						ps->picked = picked;
					}
					ps->picked[npicked] = pp;
					++npicked;
				}
			}
			pp = pp->next;
		}

		cellx >>= 1;
		celly >>= 1;
		levelwidth >>= 1;
	}

	qsort(ps->picked, npicked, sizeof(NVGpickPath*), nvg__comparePaths);

	maxids = nvg__mini(maxids, npicked);
	for (id = 0; id < maxids; ++id)
		ids[id] = ps->picked[id]->id;

	return maxids;
}

// Returns the id of the pickable shape containing x,y or -1 if not shape is found.
int nvgHitTest(NVGcontext* ctx, float x, float y, int flags)
{
	NVGpickScene* ps;
	int levelwidth;
	int cellx, celly;
	int bestOrder = -1;
	int bestID = -1;
	int hit = 0;
	int lvl;

	if (ctx->pickScene == NULL)
		return -1;

	ps = ctx->pickScene;

	levelwidth = 1 << (ps->nlevels - 1);
	cellx = nvg__clampi((int)(x / ps->xdim), 0, levelwidth-1);
	celly = nvg__clampi((int)(y / ps->ydim), 0, levelwidth-1);

	for (lvl = ps->nlevels - 1; lvl >= 0; --lvl) {
		NVGpickPath* pp = ps->levels[lvl][celly*levelwidth + cellx];
		while(pp) {
			if (nvg__pickPathTestBounds(ps, pp, x, y)) {

				if ((flags & NVG_TEST_STROKE) && (pp->flags & NVG_PICK_STROKE))
					hit = nvg__pickPathStroke(ps, pp, x, y);

				if (!hit && (flags & NVG_TEST_FILL) && (pp->flags & NVG_PICK_FILL))
					hit = nvg__pickPath(ps, pp, x, y);

				if (hit) {
					if (pp->order > bestOrder) {
						bestOrder = pp->order;
						bestID = pp->id;
					}
				}
			}
			pp = pp->next;
		}

		cellx >>= 1;
		celly >>= 1;
		levelwidth >>= 1;
	}

	return bestID;
}

int nvgInFill(NVGcontext* ctx, float x, float y)
{
	NVGpickScene* ps = nvg__pickSceneGet(ctx);

	int oldnpoints = ps->npoints;
	int oldnsegments = ps->nsegments;

	NVGpickPath* pp = nvg__pickPathCreate(ctx, 1, 0);
	int hit;

	if (nvg__pointInBounds(x, y, pp->bounds) == 0)
		return 0;

	hit = nvg__pickPath(ps, pp, x, y);

	nvg__freePickPath(ps, pp);

	ps->npoints = oldnpoints;
	ps->nsegments = oldnsegments;

	return hit;
}

int nvgInStroke(NVGcontext* ctx, float x, float y)
{
	NVGpickScene* ps = nvg__pickSceneGet(ctx);

	int oldnpoints = ps->npoints;
	int oldnsegments = ps->nsegments;

	NVGpickPath* pp = nvg__pickPathCreate(ctx, 1, 1);
	int hit;

	if (nvg__pointInBounds(x, y, pp->bounds) == 0)
		return 0;

	hit = nvg__pickPathStroke(ps, pp, x, y);

	nvg__freePickPath(ps, pp);

	ps->npoints = oldnpoints;
	ps->nsegments = oldnsegments;

	return hit;
}


static int nvg__countBitsUsed(int v)
{
	if (v == 0)
		return 0;
#if defined(__clang__) || defined(__GNUC__)
	return (sizeof(int) * 8) - __builtin_clz(v);
#elif defined(_MSC_VER)
	return (sizeof(int) * 8) -  __lzcnt(v);
#else
	// Poor mans 16bit clz variant
	int test;
	int nbits;

	if (v & 0xF000) {
		test = 1 << 15;
		nbits = 16;
	} else if (v & 0x0F00) {
		test = 1 << 11;
		nbits = 12;
	} else if (v & 0x00F0) {
		test = 1 << 7;
		nbits = 8;
	} else if (v & 0x000F) {
		test = 1 << 3;
		nbits = 4;
	} else {
		return 0;
	}

	while(nbits) {
		if (v & test)
			return nbits;
		--nbits;
		test >>= 1;
	}

	return 0;
#endif
}

static void nvg__pickSceneInsert(NVGpickScene* ps, NVGpickPath* pp)
{
	int cellbounds[4];
	int base = ps->nlevels - 1;
	int level;
	int levelwidth;
	int levelshift;
	int levelx;
	int levely;
	NVGpickPath** cell = NULL;

	// Bit tricks for inserting into an implicit quadtree.

	// Calc bounds of path in cells at the lowest level
	cellbounds[0] = (int)(pp->bounds[0] / ps->xdim);
	cellbounds[1] = (int)(pp->bounds[1] / ps->ydim);
	cellbounds[2] = (int)(pp->bounds[2] / ps->xdim);
	cellbounds[3] = (int)(pp->bounds[3] / ps->ydim);

	// Find which bits differ between the min/max x/y coords
	cellbounds[0] ^= cellbounds[2];
	cellbounds[1] ^= cellbounds[3];

	// Use the number of bits used (countBitsUsed(x) == sizeof(int) * 8 - clz(x);
	// to calculate the level to insert at (the level at which the bounds fit in a
	// single cell)
	level = nvg__mini(base - nvg__countBitsUsed( cellbounds[0] ), base - nvg__countBitsUsed( cellbounds[1] ) );
	if (level < 0)
		level = 0;

	// Find the correct cell in the chosen level, clamping to the edges.
	levelwidth = 1 << level;
	levelshift = (ps->nlevels - level) - 1;
	levelx = nvg__clampi(cellbounds[2] >> levelshift, 0, levelwidth - 1);
	levely = nvg__clampi(cellbounds[3] >> levelshift, 0, levelwidth - 1);

	// Insert the path into the linked list at that cell.
	cell = &ps->levels[level][levely * levelwidth + levelx];

	pp->cellnext = *cell;
	*cell = pp;

	if (ps->paths == NULL)
		ps->lastPath = pp;
	pp->next = ps->paths;
	ps->paths = pp;

	// Store the order (depth) of the path for picking ops.
	pp->order = (short)ps->npaths;
	++ps->npaths;
}

void nvg__pickBeginFrame(NVGcontext* ctx, int width, int height)
{
	NVGpickScene* ps = nvg__pickSceneGet(ctx);
	float lowestSubDiv;

	NVG_PICK_DEBUG_NEWFRAME();

	// Return all paths & sub paths from last frame to the free list
	while(ps->paths) {
		NVGpickPath* pp = ps->paths->next;
		nvg__freePickPath(ps, ps->paths);
		ps->paths = pp;
	}

	ps->paths = NULL;
	ps->npaths = 0;

	// Store the screen metrics for the quadtree
	ps->width = width;
	ps->height = height;

	lowestSubDiv = (float)(1 << (ps->nlevels - 1));
	ps->xdim = (float)width / lowestSubDiv;
	ps->ydim = (float)height / lowestSubDiv;

	// Allocate the quadtree if required.
	if (ps->levels == NULL) {
		int ncells = 1;
		int leveldim;
		int l, cell;

		ps->levels = (NVGpickPath***)malloc(sizeof(NVGpickPath**) * ps->nlevels);
		for (l = 0; l < ps->nlevels; ++l) {
			leveldim = 1 << l;

			ncells += leveldim * leveldim;
		}

		ps->levels[0] = (NVGpickPath**)malloc(sizeof(NVGpickPath*) * ncells);

		cell = 1;
		for (l = 1; l < ps->nlevels; l++) {
			ps->levels[l] = &ps->levels[0][cell];
			leveldim = 1 << l;
			cell += leveldim * leveldim;
		}

		ps->ncells = ncells;
	}
	memset(ps->levels[0], 0, ps->ncells * sizeof(NVGpickPath*));

	// Allocate temporary storage for nvgHitTestAll results if required.
	if (ps->picked == NULL) {
		ps->cpicked = 16;
		ps->picked = malloc(sizeof(NVGpickPath*) * ps->cpicked);
	}

	ps->npoints = 0;
	ps->nsegments = 0;
}
