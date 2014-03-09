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
#ifndef NANOVG_GL3_H
#define NANOVG_GL3_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVG_ANTIALIAS 1

#ifdef NANOVG_GLES3_IMPLEMENTATION
#	ifndef NANOVG_GLES3
#		define NANOVG_GLES3
#	endif
#	ifndef NANOVG_GL3_IMPLEMENTATION
#		define NANOVG_GL3_IMPLEMENTATION
#	endif
#endif

#ifdef NANOVG_GLES3

struct NVGcontext* nvgCreateGLES3(int atlasw, int atlash, int edgeaa);
void nvgDeleteGLES3(struct NVGcontext* ctx);

#else

struct NVGcontext* nvgCreateGL3(int atlasw, int atlash, int edgeaa);
void nvgDeleteGL3(struct NVGcontext* ctx);

#endif

#ifdef __cplusplus
}
#endif

#endif

#ifdef NANOVG_GL3_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"

enum GLNVGuniformLoc {
	GLNVG_LOC_VIEW,
	GLNVG_LOC_FRAG,
	GLNVG_MAX_LOCS
};

enum GLNVGshaderType {
	NSVG_SHADER_FILLGRAD,
	NSVG_SHADER_FILLIMG,
	NSVG_SHADER_SIMPLE,
	NSVG_SHADER_IMG
};

enum GLNVGuniformBindings {
	GLNVG_VIEW_BINDING = 0,
	GLNVG_FRAG_BINDING = 1,
};

struct GLNVGshader {
	GLuint prog;
	GLuint frag;
	GLuint vert;
	GLint loc[GLNVG_MAX_LOCS];
};

struct GLNVGtexture {
	int id;
	GLuint tex;
	int width, height;
	int type;
};

enum GLNVGcallType {
	GLNVG_FILL,
	GLNVG_CONVEXFILL,
	GLNVG_STROKE,
	GLNVG_TRIANGLES,
};

struct GLNVGcall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
};

struct GLNVGpath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};

struct GLNVGfragUniforms {
   float scissorMat[12]; // matrices are actually 3 vec4s
   float paintMat[12];
   float innerCol[4];
   float outerCol[4];
   float scissorExt[2];
   float scissorScale[2];
   float extent[2];
   float radius;
   float feather;
   float strokeMult;
   int texType;
   int type;
};

struct GLNVGcontext {
	struct GLNVGshader shader;
	struct GLNVGtexture* textures;
	float view[2];
	int ntextures;
	int ctextures;
	int textureId;
	GLuint vertArr;
	GLuint vertBuf;
	GLuint viewBuf;
	GLuint fragBuf;
	int fragSize;
	int edgeAntiAlias;

	// Per frame buffers
	struct GLNVGcall* calls;
	int ccalls;
	int ncalls;
	struct GLNVGpath* paths;
	int cpaths;
	int npaths;
	struct NVGvertex* verts;
	int cverts;
	int nverts;
	unsigned char* uniforms;
	int cuniforms;
	int nuniforms;
};


static struct GLNVGtexture* glnvg__allocTexture(struct GLNVGcontext* gl)
{
	struct GLNVGtexture* tex = NULL;
	int i;

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == 0) {
			tex = &gl->textures[i];
			break;
		}
	}
	if (tex == NULL) {
		if (gl->ntextures+1 > gl->ctextures) {
			gl->ctextures = (gl->ctextures == 0) ? 2 : gl->ctextures*2;
			gl->textures = (struct GLNVGtexture*)realloc(gl->textures, sizeof(struct GLNVGtexture)*gl->ctextures);
			if (gl->textures == NULL) return NULL;
		}
		tex = &gl->textures[gl->ntextures++];
	}
	
	memset(tex, 0, sizeof(*tex));
	tex->id = ++gl->textureId;
	
	return tex;
}

static struct GLNVGtexture* glnvg__findTexture(struct GLNVGcontext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++)
		if (gl->textures[i].id == id)
			return &gl->textures[i];
	return NULL;
}

static int glnvg__deleteTexture(struct GLNVGcontext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == id) {
			if (gl->textures[i].tex != 0)
				glDeleteTextures(1, &gl->textures[i].tex);
			memset(&gl->textures[i], 0, sizeof(gl->textures[i]));
			return 1;
		}
	}
	return 0;
}

static void glnvg__dumpShaderError(GLuint shader, const char* name, const char* type)
{
	char str[512+1];
	int len = 0;
	glGetShaderInfoLog(shader, 512, &len, str);
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Shader %s/%s error:\n%s\n", name, type, str);
}

static void glnvg__dumpProgramError(GLuint prog, const char* name)
{
	char str[512+1];
	int len = 0;
	glGetProgramInfoLog(prog, 512, &len, str);
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Program %s error:\n%s\n", name, str);
}

static int glnvg__checkError(const char* str)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("Error %08x after %s\n", err, str);
		return 1;
	}
	return 0;
}

static int glnvg__createShader(struct GLNVGshader* shader, const char* name, const char* header, const char* opts, const char* vshader, const char* fshader)
{
	GLint status;
	GLuint prog, vert, frag;
	const char* str[3];
	str[0] = header;
	str[1] = opts != NULL ? opts : "";

	memset(shader, 0, sizeof(*shader));

	prog = glCreateProgram();
	vert = glCreateShader(GL_VERTEX_SHADER);
	frag = glCreateShader(GL_FRAGMENT_SHADER);
	str[2] = vshader;
	glShaderSource(vert, 3, str, 0);
	str[2] = fshader;
	glShaderSource(frag, 3, str, 0);

	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(vert, name, "vert");
		return 0;
	}

	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(frag, name, "frag");
		return 0;
	}

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);

	glBindAttribLocation(prog, 0, "vertex");
	glBindAttribLocation(prog, 1, "tcoord");

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpProgramError(prog, name);
		return 0;
	}

	shader->prog = prog;
	shader->vert = vert;
	shader->frag = frag;

	return 1;
}

static void glnvg__deleteShader(struct GLNVGshader* shader)
{
	if (shader->prog != 0)
		glDeleteProgram(shader->prog);
	if (shader->vert != 0)
		glDeleteShader(shader->vert);
	if (shader->frag != 0)
		glDeleteShader(shader->frag);
}

static void glnvg__getUniforms(struct GLNVGshader* shader)
{
	shader->loc[GLNVG_LOC_VIEW] = glGetUniformBlockIndex(shader->prog, "view");
	shader->loc[GLNVG_LOC_FRAG] = glGetUniformBlockIndex(shader->prog, "frag");
}

static int glnvg__renderCreate(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int align = 1;

	static const char* shaderHeader =
#ifdef NANOVG_GLES3
		"#version 300 es\n"
		"precision mediump float;\n";
#else
		"#version 150 core\n";
#endif

	static const char* fillVertShader =
		"layout(std140) uniform view {\n"
		"   vec2 viewSize;\n"
		"};\n"
		"in vec2 vertex;\n"
		"in vec2 tcoord;\n"
		"out vec2 ftcoord;\n"
		"out vec2 fpos;\n"
		"void main(void) {\n"
		"	ftcoord = tcoord;\n"
		"	fpos = vertex;\n"
		"	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"
		"}\n";

	static const char* fillFragShaderEdgeAA = 
		"layout(std140) uniform frag {\n"
		"	mat3 scissorMat;\n"
		"	mat3 paintMat;\n"
		"	vec4 innerCol;\n"
		"	vec4 outerCol;\n"
		"	vec2 scissorExt;\n"
		"	vec2 scissorScale;\n"
		"	vec2 extent;\n"
		"	float radius;\n"
		"	float feather;\n"
		"	float strokeMult;\n"
		"	int texType;\n"
		"	int type;\n"
		"};\n"
		"uniform sampler2D tex;\n"
		"in vec2 ftcoord;\n"
		"in vec2 fpos;\n"
		"out vec4 outColor;\n"
		"\n"
		"float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
		"	vec2 ext2 = ext - vec2(rad,rad);\n"
		"	vec2 d = abs(pt) - ext2;\n"
		"	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
		"}\n"
		"\n"
		"// Scissoring\n"
		"float scissorMask(vec2 p) {\n"
		"	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);\n"
		"	sc = vec2(0.5,0.5) - sc * scissorScale;\n"
		"	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
		"}\n"
		"#ifdef EDGE_AA\n"
		"// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.\n"
		"float strokeMask() {\n"
		"	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);\n"
		"}\n"
		"#endif\n"
		"\n"
		"void main(void) {\n"
		"	if (type == 0) {			// Gradient\n"
		"		float scissor = scissorMask(fpos);\n"
		"#ifdef EDGE_AA\n"
		"		float strokeAlpha = strokeMask();\n"
		"#else\n"
		"		float strokeAlpha = 1.0f;\n"
		"#endif\n"
		"		// Calculate gradient color using box gradient\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
		"		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
		"		vec4 color = mix(innerCol,outerCol,d);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		outColor = color;\n"
		"	} else if (type == 1) {		// Image\n"
		"		float scissor = scissorMask(fpos);\n"
		"#ifdef EDGE_AA\n"
		"		float strokeAlpha = strokeMask();\n"
		"#else\n"
		"		float strokeAlpha = 1.0f;\n"
		"#endif\n"
		"		// Calculate color fron texture\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
		"		vec4 color = texture(tex, pt);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		outColor = color;\n"
		"	} else if (type == 2) {		// Stencil fill\n"
		"		outColor = vec4(1,1,1,1);\n"
		"	} else if (type == 3) {		// Textured tris\n"
		"		float scissor = scissorMask(fpos);\n"
		"		vec4 color = texture(tex, ftcoord);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		color.w *= scissor;\n"
		"		outColor = color * innerCol;\n"
		"	}\n"
		"}\n";

	glnvg__checkError("init");

	if (gl->edgeAntiAlias) {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, "#define EDGE_AA 1\n", fillVertShader, fillFragShaderEdgeAA) == 0)
			return 0;
	} else {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, NULL, fillVertShader, fillFragShaderEdgeAA) == 0)
			return 0;
	}

	glnvg__checkError("uniform locations");
	glnvg__getUniforms(&gl->shader);

	// Create dynamic vertex array
	glGenVertexArrays(1, &gl->vertArr);
	glGenBuffers(1, &gl->vertBuf);

	// Create UBOs
	glUniformBlockBinding(gl->shader.prog, gl->shader.loc[GLNVG_LOC_VIEW], GLNVG_VIEW_BINDING);
	glGenBuffers(1, &gl->viewBuf); 
	glUniformBlockBinding(gl->shader.prog, gl->shader.loc[GLNVG_LOC_FRAG], GLNVG_FRAG_BINDING);
	glGenBuffers(1, &gl->fragBuf); 
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
	gl->fragSize = sizeof(struct GLNVGfragUniforms) + align - sizeof(struct GLNVGfragUniforms) % align;

	glnvg__checkError("done");

	return 1;
}

static int glnvg__renderCreateTexture(void* uptr, int type, int w, int h, const unsigned char* data)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__allocTexture(gl);
	int align,length,pixels,rows;

	if (tex == NULL) return 0;
	glGenTextures(1, &tex->tex);
	tex->width = w;
	tex->height = h;
	tex->type = type;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glGetIntegerv(GL_UNPACK_ALIGNMENT,&align);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH,&length);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&pixels);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS,&rows);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	if (type == NVG_TEXTURE_RGBA)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
#ifdef NANOVG_GLES3
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT,align);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,length);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,pixels);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,rows);

	if (glnvg__checkError("create tex"))
		return 0;

	return tex->id;
}


static int glnvg__renderDeleteTexture(void* uptr, int image)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	return glnvg__deleteTexture(gl, image);
}

static int glnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
	int align,length,pixels,rows;

	if (tex == NULL) return 0;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glGetIntegerv(GL_UNPACK_ALIGNMENT,&align);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH,&length);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&pixels);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS,&rows);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, y);

	if (tex->type == NVG_TEXTURE_RGBA)
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RED, GL_UNSIGNED_BYTE, data);

	glPixelStorei(GL_UNPACK_ALIGNMENT,align);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,length);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,pixels);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,rows);

	return 1;
}

static int glnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
	if (tex == NULL) return 0;
	*w = tex->width;
	*h = tex->height;
	return 1;
}

static void glnvg__toFloatColor(float* fc, unsigned int c)
{
	fc[0] = ((c) & 0xff) / 255.0f;
	fc[1] = ((c>>8) & 0xff) / 255.0f;
	fc[2] = ((c>>16) & 0xff) / 255.0f;
	fc[3] = ((c>>24) & 0xff) / 255.0f;
}

static void glnvg__xformIdentity(float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void glnvg__xformInverse(float* inv, float* t)
{
	double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
	if (det > -1e-6 && det < 1e-6) {
		glnvg__xformIdentity(t);
		return;
	}
	invdet = 1.0 / det;
	inv[0] = (float)(t[3] * invdet);
	inv[2] = (float)(-t[2] * invdet);
	inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
	inv[1] = (float)(-t[1] * invdet);
	inv[3] = (float)(t[0] * invdet);
	inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
}

static void glnvg__xformToMat3x4(float* m3, float* t)
{
	m3[0] = t[0];
	m3[1] = t[1];
	m3[2] = 0.0f;
	m3[3] = 0.0f;
	m3[4] = t[2];
	m3[5] = t[3];
	m3[6] = 0.0f;
	m3[7] = 0.0f;
	m3[8] = t[4];
	m3[9] = t[5];
	m3[10] = 1.0f;
	m3[11] = 0.0f;
}

static int glnvg__convertPaint(struct GLNVGcontext* gl, struct GLNVGfragUniforms* frag, struct NVGpaint* paint, struct NVGscissor* scissor, float width)
{
	struct GLNVGtexture* tex = NULL;
	float invxform[6];

	glnvg__toFloatColor(frag->innerCol, paint->innerColor);
	glnvg__toFloatColor(frag->outerCol, paint->outerColor);

	glnvg__xformInverse(invxform, paint->xform);
	glnvg__xformToMat3x4(frag->paintMat, invxform);

	if (scissor->extent[0] < 0.5f || scissor->extent[1] < 0.5f) {
		memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
		frag->scissorExt[0] = 1.0f;
		frag->scissorExt[1] = 1.0f;
		frag->scissorScale[0] = 1.0f;
		frag->scissorScale[1] = 1.0f;
	} else {
		glnvg__xformInverse(invxform, scissor->xform);
		glnvg__xformToMat3x4(frag->scissorMat, invxform);
		frag->scissorExt[0] = scissor->extent[0];
		frag->scissorExt[1] = scissor->extent[1];
		frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]);
		frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]);
	}
	memcpy(frag->extent, paint->extent, sizeof(frag->extent));
	frag->strokeMult = width*0.5f + 0.5f;

	if (paint->image != 0) {
		tex = glnvg__findTexture(gl, paint->image);
		if (tex == NULL) return 0;
		frag->type = NSVG_SHADER_FILLIMG;
		frag->texType = tex->type == NVG_TEXTURE_RGBA ? 0 : 1;
	} else {
		frag->type = NSVG_SHADER_FILLGRAD;
		frag->radius = paint->radius;
		frag->feather = paint->feather;
	}
	return 1;
}

static void glnvg__setUniforms(struct GLNVGcontext* gl, int uniformOffset, int image)
{
	glBindBufferRange(GL_UNIFORM_BUFFER, GLNVG_FRAG_BINDING, gl->fragBuf, uniformOffset, sizeof(struct GLNVGfragUniforms));
	if (image != 0) {
		struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
		glBindTexture(GL_TEXTURE_2D, tex != NULL ? tex->tex : 0);
		glnvg__checkError("tex paint tex");
	}
}

static void glnvg__renderViewport(void* uptr, int width, int height, int alphaBlend)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	NVG_NOTUSED(alphaBlend);
	gl->view[0] = (float)width;
	gl->view[1] = (float)height;
}

static void glnvg__fill(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	// Draw shapes
	glDisable(GL_BLEND);
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xff);
	glStencilFunc(GL_ALWAYS, 0, ~0L);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// set bindpoint for solid loc
	glnvg__setUniforms(gl, call->uniformOffset, 0);
	glnvg__checkError("fill simple");

	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
	glEnable(GL_CULL_FACE);

	// Draw aliased off-pixels
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_BLEND);

	glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
	glnvg__checkError("fill fill");

	if (gl->edgeAntiAlias) {
		glStencilFunc(GL_EQUAL, 0x00, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		// Draw fringes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}

	// Draw fill
	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);

	glDisable(GL_STENCIL_TEST);
}

static void glnvg__convexFill(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("convex fill");

	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
	if (gl->edgeAntiAlias) {
		// Draw fringes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}
}

static void glnvg__stroke(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int npaths = call->pathCount, i;

	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("stroke fill");

	// Draw Strokes
	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
}

static void glnvg__triangles(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("triangles fill");

	glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void glnvg__renderFlush(void* uptr, int alphaBlend)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int i;

	if (gl->ncalls > 0) {

		glUseProgram(gl->shader.prog);
		glEnable(GL_CULL_FACE);
		if (alphaBlend == NVG_PREMULTIPLIED_ALPHA)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Upload ubo for frag shaders
		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
		glBufferData(GL_UNIFORM_BUFFER, gl->nuniforms * gl->fragSize, gl->uniforms, GL_STREAM_DRAW);

		// Upload vertex data
		glBindVertexArray(gl->vertArr);
		glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
		glBufferData(GL_ARRAY_BUFFER, gl->nverts * sizeof(struct NVGvertex), gl->verts, GL_STREAM_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(0 + 2*sizeof(float)));
		
		// once per frame set ubo for view
		glBindBuffer(GL_UNIFORM_BUFFER, gl->viewBuf);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(gl->view), 0, GL_STREAM_DRAW);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(gl->view), gl->view, GL_STREAM_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, GLNVG_VIEW_BINDING, gl->viewBuf);

		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);

		for (i = 0; i < gl->ncalls; i++) {
			struct GLNVGcall* call = &gl->calls[i];
			if (call->type == GLNVG_FILL)
				glnvg__fill(gl, call);
			else if (call->type == GLNVG_CONVEXFILL)
				glnvg__convexFill(gl, call);
			else if (call->type == GLNVG_STROKE)
				glnvg__stroke(gl, call);
			else if (call->type == GLNVG_TRIANGLES)
				glnvg__triangles(gl, call);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glUseProgram(0);
	}

	// Reset calls
	gl->nverts = 0;
	gl->npaths = 0;
	gl->ncalls = 0;
	gl->nuniforms = 0;
}

static int glnvg__maxVertCount(const struct NVGpath* paths, int npaths)
{
	int i, count = 0;
	for (i = 0; i < npaths; i++) {
		count += paths[i].nfill;
		count += paths[i].nstroke;
	}
	return count;
}

static int glnvg__maxi(int a, int b) { return a > b ? a : b; }

static struct GLNVGcall* glnvg__allocCall(struct GLNVGcontext* gl)
{
	struct GLNVGcall* ret = NULL;
	if (gl->ncalls+1 > gl->ccalls) {
		gl->ccalls = gl->ccalls == 0 ? 32 : gl->ccalls * 2;
		gl->calls = (struct GLNVGcall*)realloc(gl->calls, sizeof(struct GLNVGcall) * gl->ccalls);
	}
	ret = &gl->calls[gl->ncalls++];
	memset(ret, 0, sizeof(struct GLNVGcall));
	return ret;
}

static int glnvg__allocPaths(struct GLNVGcontext* gl, int n)
{
	int ret = 0;
	if (gl->npaths+n > gl->cpaths) {
		gl->cpaths = gl->cpaths == 0 ? glnvg__maxi(n, 32) : gl->cpaths * 2;
		gl->paths = (struct GLNVGpath*)realloc(gl->paths, sizeof(struct GLNVGpath) * gl->cpaths);
	}
	ret = gl->npaths;
	gl->npaths += n;
	return ret;
}

static int glnvg__allocVerts(struct GLNVGcontext* gl, int n)
{
	int ret = 0;
	if (gl->nverts+n > gl->cverts) {
		gl->cverts = gl->cverts == 0 ? glnvg__maxi(n, 256) : gl->cverts * 2;
		gl->verts = (struct NVGvertex*)realloc(gl->verts, sizeof(struct NVGvertex) * gl->cverts);
	}
	ret = gl->nverts;
	gl->nverts += n;
	return ret;
}

static int glnvg__allocFragUniforms(struct GLNVGcontext* gl, int n)
{
	int ret = 0, structSize = gl->fragSize;
	if (gl->nuniforms+n > gl->cuniforms) {
		gl->cuniforms = gl->cuniforms == 0 ? glnvg__maxi(n, 32) : gl->cuniforms * 2;
		gl->uniforms = (unsigned char*)realloc(gl->uniforms, gl->cuniforms * structSize);
	}
	ret = gl->nuniforms * structSize;
	gl->nuniforms += n;
	return ret;
}

static struct GLNVGfragUniforms* nvg__fragUniformPtr(struct GLNVGcontext* gl, int i)
{
	return (struct GLNVGfragUniforms*)&gl->uniforms[i];
}

static void glnvg__vset(struct NVGvertex* vtx, float x, float y, float u, float v)
{
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
}

static void glnvg__renderFill(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
							  const float* bounds, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	struct NVGvertex* quad;
	struct GLNVGfragUniforms* frag;
	int i, maxverts, offset;

	call->type = GLNVG_FILL;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	call->pathCount = npaths;
	call->image = paint->image;

	if (npaths == 1 && paths[0].convex)
		call->type = GLNVG_CONVEXFILL;

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths);
	offset = glnvg__allocVerts(gl, maxverts + 6);

	for (i = 0; i < npaths; i++) {
		struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
		const struct NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(struct GLNVGpath));
		if (path->nfill) {
			copy->fillOffset = offset;
			copy->fillCount = path->nfill;
			memcpy(&gl->verts[offset], path->fill, sizeof(struct NVGvertex) * path->nfill);
			offset += path->nfill;
		}
		if (path->nstroke) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Quad
	call->triangleOffset = offset;
	call->triangleCount = 6;
	quad = &gl->verts[call->triangleOffset];
	glnvg__vset(&quad[0], bounds[0], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[1], bounds[2], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[2], bounds[2], bounds[1], 0.5f, 1.0f);

	glnvg__vset(&quad[3], bounds[0], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[4], bounds[2], bounds[1], 0.5f, 1.0f);
	glnvg__vset(&quad[5], bounds[0], bounds[1], 0.5f, 1.0f);

	// Setup uniforms for draw calls
	if (call->type == GLNVG_FILL) {
		call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
		// Simple shader for stencil
		frag = nvg__fragUniformPtr(gl, call->uniformOffset);
		frag->type = NSVG_SHADER_SIMPLE;
		// Fill shader
		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, 1.0001f);
	} else {
		call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
		// Fill shader
		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, 1.0001f);
	}
}

static void glnvg__renderStroke(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
								float strokeWidth, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	int i, maxverts, offset;

	call->type = GLNVG_STROKE;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	call->pathCount = npaths;
	call->image = paint->image;

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths);
	offset = glnvg__allocVerts(gl, maxverts + 6);

	for (i = 0; i < npaths; i++) {
		struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
		const struct NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(struct GLNVGpath));
		if (path->nstroke) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Fill shader
	call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
	glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth);
}

static void glnvg__renderTriangles(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
								   const struct NVGvertex* verts, int nverts)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	struct GLNVGfragUniforms* frag;

	call->type = GLNVG_TRIANGLES;
	call->image = paint->image;

	// Allocate vertices for all the paths.
	call->triangleOffset = glnvg__allocVerts(gl, nverts);
	call->triangleCount = nverts;
	memcpy(&gl->verts[call->triangleOffset], verts, sizeof(struct NVGvertex) * nverts);

	// Fill shader
	call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
	frag = nvg__fragUniformPtr(gl, call->uniformOffset);
	glnvg__convertPaint(gl, frag, paint, scissor, 1.0f);
	frag->type = NSVG_SHADER_IMG;
}

static void glnvg__renderDelete(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int i;
	if (gl == NULL) return;

	glnvg__deleteShader(&gl->shader);

	if (gl->vertArr != 0)
		glDeleteVertexArrays(1, &gl->vertArr);
	if (gl->vertBuf != 0)
		glDeleteBuffers(1, &gl->vertBuf);
	if (gl->viewBuf != 0)
		glDeleteBuffers(1, &gl->viewBuf); 
	if (gl->fragBuf != 0)
		glDeleteBuffers(1, &gl->fragBuf); 

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].tex != 0)
			glDeleteTextures(1, &gl->textures[i].tex);
	}
	free(gl->textures);

	free(gl);
}


#ifdef NANOVG_GLES3
struct NVGcontext* nvgCreateGLES3(int atlasw, int atlash, int edgeaa)
#else
struct NVGcontext* nvgCreateGL3(int atlasw, int atlash, int edgeaa)
#endif
{
	struct NVGparams params;
	struct NVGcontext* ctx = NULL;
	struct GLNVGcontext* gl = (struct GLNVGcontext*)malloc(sizeof(struct GLNVGcontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(struct GLNVGcontext));

	memset(&params, 0, sizeof(params));
	params.renderCreate = glnvg__renderCreate;
	params.renderCreateTexture = glnvg__renderCreateTexture;
	params.renderDeleteTexture = glnvg__renderDeleteTexture;
	params.renderUpdateTexture = glnvg__renderUpdateTexture;
	params.renderGetTextureSize = glnvg__renderGetTextureSize;
	params.renderViewport = glnvg__renderViewport;
	params.renderFlush = glnvg__renderFlush;
	params.renderFill = glnvg__renderFill;
	params.renderStroke = glnvg__renderStroke;
	params.renderTriangles = glnvg__renderTriangles;
	params.renderDelete = glnvg__renderDelete;
	params.userPtr = gl;
	params.atlasWidth = atlasw;
	params.atlasHeight = atlash;
	params.edgeAntiAlias = edgeaa;

	gl->edgeAntiAlias = edgeaa;

	ctx = nvgCreateInternal(&params);
	if (ctx == NULL) goto error;

	return ctx;

error:
	// 'gl' is freed by nvgDeleteInternal.
	if (ctx != NULL) nvgDeleteInternal(ctx);
	return NULL;
}

#ifdef NANOVG_GLES3
void nvgDeleteGLES3(struct NVGcontext* ctx)
#else
void nvgDeleteGL3(struct NVGcontext* ctx)
#endif
{
	nvgDeleteInternal(ctx);
}

#endif
