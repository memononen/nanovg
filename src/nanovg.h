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

#ifndef NANOVG_H
#define NANOVG_H

#define NVG_PI 3.14159265358979323846264338327f

enum NVGdir {
	NVG_CCW = 1,
	NVG_CW = 2,
};
enum NVGdir2 {
	NVG_SOLID = 1,	// ccw
	NVG_HOLE = 2,	// cw
};

enum NVGpatternRepeat {
	NVG_REPEATX = 0x01,
	NVG_REPEATY = 0x02,
};

enum NVGaling {
	// Horizontal align
	NVG_ALIGN_LEFT 		= 1<<0,	// Default
	NVG_ALIGN_CENTER 	= 1<<1,
	NVG_ALIGN_RIGHT 	= 1<<2,
	// Vertical align
	NVG_ALIGN_TOP 		= 1<<3,
	NVG_ALIGN_MIDDLE	= 1<<4,
	NVG_ALIGN_BOTTOM	= 1<<5,
	NVG_ALIGN_BASELINE	= 1<<6, // Default
};


// Used by the rendering API
enum NVGtexture {
	NVG_TEXTURE_ALPHA = 0x01,
	NVG_TEXTURE_RGBA = 0x02,
};

struct NVGpaint
{
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	unsigned int innerColor;
	unsigned int outerColor;
	int image;
	int repeat;
};

struct NVGscissor
{
	float xform[6];
	float extent[2];
};

struct NVGvertex {
	float x,y,u,v;
};

struct NVGpath {
	int first;
	int count;
	unsigned char closed;
	int nbevel;
	struct NVGvertex* fill;
	int nfill;
	struct NVGvertex* stroke;
	int nstroke;
	int winding;
};

struct NVGparams {
	void* userPtr;
	int atlasWidth, atlasHeight;
	int (*renderCreate)(void* uptr);
	int (*renderCreateTexture)(void* uptr, int type, int w, int h, const unsigned char* data);
	int (*renderDeleteTexture)(void* uptr, int image);
	int (*renderUpdateTexture)(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
	int (*renderGetTextureSize)(void* uptr, int image, int* w, int* h);
	void (*renderFill)(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float aasize, const float* bounds, const struct NVGpath* paths, int npaths);
	void (*renderStroke)(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float aasize, float strokeWidth, const struct NVGpath* paths, int npaths);
	void (*renderTriangles)(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, int image, const struct NVGvertex* verts, int nverts);
	void (*renderDelete)(void* uptr);
};

// Contructor and destructor.
struct NVGcontext* nvgCreateInternal(struct NVGparams* params);
void nvgDeleteInternal(struct NVGcontext* ctx);

// Color utils
unsigned int nvgRGB(unsigned char r, unsigned char g, unsigned char b);
unsigned int nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
unsigned int nvgLerpRGBA(unsigned int c0, unsigned int c1, float u);
unsigned int nvgTransRGBA(unsigned int c0, unsigned char a);
unsigned int nvgHSL(float h, float s, float l);
unsigned int nvgHSLA(float h, float s, float l, unsigned char a);

// State handling
void nvgSave(struct NVGcontext* ctx);
void nvgRestore(struct NVGcontext* ctx);
void nvgReset(struct NVGcontext* ctx);

// State setting
void nvgStrokeColor(struct NVGcontext* ctx, unsigned int color);
void nvgStrokePaint(struct NVGcontext* ctx, struct NVGpaint paint);

void nvgFillColor(struct NVGcontext* ctx, unsigned int color);
void nvgFillPaint(struct NVGcontext* ctx, struct NVGpaint paint);

void nvgMiterLimit(struct NVGcontext* ctx, float limit);
void nvgStrokeWidth(struct NVGcontext* ctx, float size);

void nvgResetTransform(struct NVGcontext* ctx);
void nvgTransform(struct NVGcontext* ctx, float a, float b, float c, float d, float e, float f);
void nvgTranslate(struct NVGcontext* ctx, float x, float y);
void nvgRotate(struct NVGcontext* ctx, float angle);
void nvgScale(struct NVGcontext* ctx, float x, float y);

// Images
int nvgCreateImage(struct NVGcontext* ctx, const char* filename);
int nvgCreateImageMem(struct NVGcontext* ctx, unsigned char* data, int ndata, int freeData);
int nvgCreateImageRGBA(struct NVGcontext* ctx, int w, int h, const unsigned char* data);
void nvgUpdateImage(struct NVGcontext* ctx, int image, const unsigned char* data);
void nvgImageSize(struct NVGcontext* ctx, int image, int* w, int* h);
void nvgDeleteImage(struct NVGcontext* ctx, int image);

// Paints
struct NVGpaint nvgLinearGradient(struct NVGcontext* ctx, float sx, float sy, float ex, float ey, unsigned int icol, unsigned int ocol);
struct NVGpaint nvgBoxGradient(struct NVGcontext* ctx, float x, float y, float w, float h, float r, float f, unsigned int icol, unsigned int ocol);
struct NVGpaint nvgRadialGradient(struct NVGcontext* ctx, float cx, float cy, float inr, float outr, unsigned int icol, unsigned int ocol);
struct NVGpaint nvgImagePattern(struct NVGcontext* ctx, float ox, float oy, float ex, float ey, float angle, int image, int repeat);

// Scissoring
void nvgScissor(struct NVGcontext* ctx, float x, float y, float w, float h);
void nvgResetScissor(struct NVGcontext* ctx);

// Draw
void nvgBeginFrame(struct NVGcontext* ctx);

void nvgBeginPath(struct NVGcontext* ctx);
void nvgMoveTo(struct NVGcontext* ctx, float x, float y);
void nvgLineTo(struct NVGcontext* ctx, float x, float y);
void nvgBezierTo(struct NVGcontext* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);
void nvgArcTo(struct NVGcontext* ctx, float x1, float y1, float x2, float y2, float radius);
void nvgClosePath(struct NVGcontext* ctx);
void nvgPathWinding(struct NVGcontext* ctx, int dir);

void nvgArc(struct NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir);
void nvgRect(struct NVGcontext* ctx, float x, float y, float w, float h);
void nvgRoundedRect(struct NVGcontext* ctx, float x, float y, float w, float h, float r);
void nvgEllipse(struct NVGcontext* ctx, float cx, float cy, float rx, float ry);
void nvgCircle(struct NVGcontext* ctx, float cx, float cy, float r);

void nvgFill(struct NVGcontext* ctx);
void nvgStroke(struct NVGcontext* ctx);

// Text

// Add fonts
int nvgCreateFont(struct NVGcontext* ctx, const char* name, const char* path);
int nvgCreateFontMem(struct NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData);
int nvgFindFont(struct NVGcontext* ctx, const char* name);

// State setting
void nvgFontSize(struct NVGcontext* ctx, float size);
void nvgLetterSpacing(struct NVGcontext* ctx, float spacing);
void nvgFontBlur(struct NVGcontext* ctx, float blur);
void nvgTextAlign(struct NVGcontext* ctx, int align);
void nvgFontFaceId(struct NVGcontext* ctx, int font);
void nvgFontFace(struct NVGcontext* ctx, const char* font);

// Draw text
float nvgText(struct NVGcontext* ctx, float x, float y, const char* string, const char* end);

// Measure text
float nvgTextBounds(struct NVGcontext* ctx, const char* string, const char* end, float* bounds);
void nvgVertMetrics(struct NVGcontext* ctx, float* ascender, float* descender, float* lineh);


#endif // NANOVG_H
