//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
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
#ifndef NANOVG_GL_H
#define NANOVG_GL_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined NANOVG_GL2_IMPLEMENTATION
#  define NANOVG_GL2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GL3_IMPLEMENTATION
#  define NANOVG_GL3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#  define NANOVG_GL_USE_UNIFORMBUFFER 1
#elif defined NANOVG_GLES2_IMPLEMENTATION
#  define NANOVG_GLES2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GLES3_IMPLEMENTATION
#  define NANOVG_GLES3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#endif

#define NANOVG_GL_USE_STATE_FILTER (1)

// Data types
//
struct GLNVGcall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
};
typedef struct GLNVGcall GLNVGcall;

struct GLNVGtexture {
	int id;
	GLuint tex;
	int width, height;
	int type;
	int flags;
};
typedef struct GLNVGtexture GLNVGtexture;


enum GLNVGuniformLoc {
	GLNVG_LOC_VIEWSIZE,
	GLNVG_LOC_TEX,
	GLNVG_LOC_FRAG,
	GLNVG_MAX_LOCS
};

struct GLNVGshader {
	GLuint prog;
	GLuint frag;
	GLuint vert;
	GLint loc[GLNVG_MAX_LOCS];
};
typedef struct GLNVGshader GLNVGshader;

struct GLNVGpath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};
typedef struct GLNVGpath GLNVGpath;

struct GLNVGcontext {
	GLNVGshader shader;
	GLNVGtexture* textures;
	float view[2];
	int ntextures;
	int ctextures;
	int textureId;
	GLuint vertBuf;
#if defined NANOVG_GL3
	GLuint vertArr;
#endif
#if NANOVG_GL_USE_UNIFORMBUFFER
	GLuint fragBuf;
#endif
	int fragSize;
	int flags;

	// Per frame buffers
	GLNVGcall* calls;
	int ccalls;
	int ncalls;
	GLNVGpath* paths;
	int cpaths;
	int npaths;
	struct NVGvertex* verts;
	int cverts;
	int nverts;
	unsigned char* uniforms;
	int cuniforms;
	int nuniforms;

	// cached state
	#if NANOVG_GL_USE_STATE_FILTER
	GLuint boundTexture;
	GLuint stencilMask;
	GLenum stencilFunc;
	GLint stencilFuncRef;
	GLuint stencilFuncMask;
	#endif
};
typedef struct GLNVGcontext GLNVGcontext;

// These are additional flags on top of NVGimageFlags.
enum NVGimageFlagsGL {
	NVG_IMAGE_NODELETE			= 1<<16,	// Do not delete GL texture handle.
};

GLNVGtexture* glnvg__allocTexture(GLNVGcontext* gl); 

GLNVGtexture* glnvg__findTexture(GLNVGcontext* gl, int id); 

int glnvg__renderCreate(GLNVGcontext* gl);

int glnvg__renderCreateTexture(GLNVGcontext* gl, int type, int w, int h, int imageFlags, const unsigned char* data); 

int glnvg__renderDeleteTexture(GLNVGcontext* gl, int image); 

int glnvg__renderUpdateTexture(GLNVGcontext* gl, int image, int x, int y, int w, int h, const unsigned char* data); 

int glnvg__renderGetTextureSize(GLNVGcontext* gl, int image, int* w, int* h); 

void glnvg__renderViewport(GLNVGcontext* gl, int width, int height); 

void glnvg__renderCancel(GLNVGcontext* gl);

void glnvg__renderFlush(GLNVGcontext* gl);

void glnvg__renderFill(GLNVGcontext* gl, NVGpaint* paint, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths); 

void glnvg__renderStroke(GLNVGcontext* gl, NVGpaint* paint, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths); 

void glnvg__renderTriangles(GLNVGcontext* gl, NVGpaint* paint, NVGscissor* scissor, const NVGvertex* verts, int nverts); 

void glnvg__renderDelete(GLNVGcontext* gl); 


#ifdef __cplusplus
}
#endif

#endif /* NANOVG_GL_H */

