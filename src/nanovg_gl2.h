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
#ifndef NANOVG_GL2_H
#define NANOVG_GL2_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVG_ANTIALIAS 1

#ifdef NANOVG_GLES2_IMPLEMENTATION
#  ifndef NANOVG_GLES2
#    define NANOVG_GLES2
#  endif
#  ifndef NANOVG_GL2_IMPLEMENTATION
#    define NANOVG_GL2_IMPLEMENTATION
#  endif
#endif

#ifdef NANOVG_GLES2

struct NVGcontext* nvgCreateGLES2(int atlasw, int atlash, int edgeaa);
void nvgDeleteGLES2(struct NVGcontext* ctx);

#else

struct NVGcontext* nvgCreateGL2(int atlasw, int atlash, int edgeaa);
void nvgDeleteGL2(struct NVGcontext* ctx);

#endif


#ifdef __cplusplus
}
#endif

#endif

#ifdef NANOVG_GL2_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"

enum GLNVGuniformLoc {
	GLNVG_LOC_VIEWSIZE,
	GLNVG_LOC_SCISSORMAT,
	GLNVG_LOC_SCISSOREXT,
	GLNVG_LOC_SCISSORSCALE,
	GLNVG_LOC_PAINTMAT,
	GLNVG_LOC_EXTENT,
	GLNVG_LOC_RADIUS,
	GLNVG_LOC_FEATHER,
	GLNVG_LOC_INNERCOL,
	GLNVG_LOC_OUTERCOL,
	GLNVG_LOC_STROKEMULT,
	GLNVG_LOC_TEX,
	GLNVG_LOC_TEXTYPE,
	GLNVG_LOC_TYPE,
	GLNVG_MAX_LOCS
};

enum GLNVGshaderType {
	NSVG_SHADER_FILLGRAD,
	NSVG_SHADER_FILLIMG,
	NSVG_SHADER_SIMPLE,
	NSVG_SHADER_IMG
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

struct GLNVGcontext {
	struct GLNVGshader shader;
	struct GLNVGtexture* textures;
	float viewWidth, viewHeight;
	int ntextures;
	int ctextures;
	int textureId;
	GLuint vertBuf;
	int edgeAntiAlias;
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

static int glnvg__createShader(struct GLNVGshader* shader, const char* name, const char* vshader, const char* fshader)
{
	GLint status;
	GLuint prog, vert, frag;

	memset(shader, 0, sizeof(*shader));

	prog = glCreateProgram();
	vert = glCreateShader(GL_VERTEX_SHADER);
	frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vert, 1, &vshader, 0);
	glShaderSource(frag, 1, &fshader, 0);

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
    glBindAttribLocation(prog, 2, "color");

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
	shader->loc[GLNVG_LOC_VIEWSIZE] = glGetUniformLocation(shader->prog, "viewSize");
	shader->loc[GLNVG_LOC_SCISSORMAT] = glGetUniformLocation(shader->prog, "scissorMat");
	shader->loc[GLNVG_LOC_SCISSOREXT] = glGetUniformLocation(shader->prog, "scissorExt");
	shader->loc[GLNVG_LOC_SCISSORSCALE] = glGetUniformLocation(shader->prog, "scissorScale");
	shader->loc[GLNVG_LOC_PAINTMAT] = glGetUniformLocation(shader->prog, "paintMat");
	shader->loc[GLNVG_LOC_EXTENT] = glGetUniformLocation(shader->prog, "extent");
	shader->loc[GLNVG_LOC_RADIUS] = glGetUniformLocation(shader->prog, "radius");
	shader->loc[GLNVG_LOC_FEATHER] = glGetUniformLocation(shader->prog, "feather");
	shader->loc[GLNVG_LOC_INNERCOL] = glGetUniformLocation(shader->prog, "innerCol");
	shader->loc[GLNVG_LOC_OUTERCOL] = glGetUniformLocation(shader->prog, "outerCol");
	shader->loc[GLNVG_LOC_STROKEMULT] = glGetUniformLocation(shader->prog, "strokeMult");
	shader->loc[GLNVG_LOC_TEX] = glGetUniformLocation(shader->prog, "tex");
	shader->loc[GLNVG_LOC_TEXTYPE] = glGetUniformLocation(shader->prog, "texType");
	shader->loc[GLNVG_LOC_TYPE] = glGetUniformLocation(shader->prog, "type");
}

static int glnvg__renderCreate(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;

	static const char* fillVertShader =
#ifdef NANOVG_GLES2
		"#version 100\n"
		"precision mediump float;\n"
#endif
		"uniform vec2 viewSize;\n"
		"attribute vec2 vertex;\n"
		"attribute vec2 tcoord;\n"
		"attribute vec4 color;\n"
		"varying vec2 ftcoord;\n"
		"varying vec4 fcolor;\n"
		"varying vec2 fpos;\n"
		"void main(void) {\n"
		"	ftcoord = tcoord;\n"
		"	fcolor = color;\n"
		"	fpos = vertex;\n"
		"	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"
		"}\n";

	static const char* fillFragShaderEdgeAA = 
#ifdef NANOVG_GLES2
		"#version 100\n"
		"precision mediump float;\n"
#endif
		"uniform mat3 scissorMat;\n"
		"uniform vec2 scissorExt;\n"
		"uniform vec2 scissorScale;\n"
		"uniform mat3 paintMat;\n"
		"uniform vec2 extent;\n"
		"uniform float radius;\n"
		"uniform float feather;\n"
		"uniform vec4 innerCol;\n"
		"uniform vec4 outerCol;\n"
		"uniform float strokeMult;\n"
		"uniform sampler2D tex;\n"
		"uniform int texType;\n"
		"uniform int type;\n"
		"varying vec2 ftcoord;\n"
		"varying vec4 fcolor;\n"
		"varying vec2 fpos;\n"
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
		"\n"
		"// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.\n"
		"float strokeMask() {\n"
		"	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * ftcoord.y;\n"
		"}\n"
		"\n"
		"void main(void) {\n"
		"	if (type == 0) {\n"
		"		float scissor = scissorMask(fpos);\n"
		"		float strokeAlpha = strokeMask();\n"
		"		// Calculate gradient color using box gradient\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
		"		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
		"		vec4 color = mix(innerCol,outerCol,d);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		gl_FragColor = color;\n"
		"	} else if (type == 1) {\n"
		"		float scissor = scissorMask(fpos);\n"
		"		float strokeAlpha = strokeMask();\n"
		"		// Calculate color fron texture\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
		"		vec4 color = texture2D(tex, pt);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		gl_FragColor = color;\n"
		"	} else if (type == 2) {\n"
		"		gl_FragColor = vec4(1,1,1,1);\n"
		"	} else if (type == 3) {\n"
		"		vec4 color = texture2D(tex, ftcoord);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		gl_FragColor = color * fcolor;\n"
		"	}\n"
		"}\n";

	static const char* fillFragShader = 
#ifdef NANOVG_GLES2
		"#version 100\n"
		"precision mediump float;\n"
#endif
		"uniform mat3 scissorMat;\n"
		"uniform vec2 scissorExt;\n"
		"uniform vec2 scissorScale;\n"
		"uniform mat3 paintMat;\n"
		"uniform vec2 extent;\n"
		"uniform float radius;\n"
		"uniform float feather;\n"
		"uniform vec4 innerCol;\n"
		"uniform vec4 outerCol;\n"
		"uniform float strokeMult;\n"
		"uniform sampler2D tex;\n"
		"uniform int texType;\n"
		"uniform int type;\n"
		"varying vec2 ftcoord;\n"
		"varying vec4 fcolor;\n"
		"varying vec2 fpos;\n"
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
		"\n"
		"void main(void) {\n"
		"	if (type == 0) {\n"
		"		float scissor = scissorMask(fpos);\n"
		"		// Calculate gradient color using box gradient\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
		"		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
		"		vec4 color = mix(innerCol,outerCol,d);\n"
		"		// Combine alpha\n"
		"		color.w *= scissor;\n"
		"		gl_FragColor = color;\n"
		"	} else if (type == 1) {\n"
		"		float scissor = scissorMask(fpos);\n"
		"		// Calculate color fron texture\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
		"		vec4 color = texture2D(tex, pt);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		// Combine alpha\n"
		"		color.w *= scissor;\n"
		"		gl_FragColor = color;\n"
		"	} else if (type == 2) {\n"
		"		gl_FragColor = vec4(1,1,1,1);\n"
		"	} else if (type == 3) {\n"
		"		vec4 color = texture2D(tex, ftcoord);\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		gl_FragColor = color * fcolor;\n"
		"	}\n"
		"}\n";

	glnvg__checkError("init");

	if (gl->edgeAntiAlias) {
		if (glnvg__createShader(&gl->shader, "shader", fillVertShader, fillFragShaderEdgeAA) == 0)
			return 0;
	} else {
		if (glnvg__createShader(&gl->shader, "shader", fillVertShader, fillFragShader) == 0)
			return 0;
	}

	glnvg__checkError("uniform locations");
	glnvg__getUniforms(&gl->shader);

	// Create dynamic vertex array
	glGenBuffers(1, &gl->vertBuf);

	glnvg__checkError("done");

	return 1;
}

static int glnvg__renderCreateTexture(void* uptr, int type, int w, int h, const unsigned char* data)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__allocTexture(gl);
	if (tex == NULL) return 0;
	glGenTextures(1, &tex->tex);
	tex->width = w;
	tex->height = h;
	tex->type = type;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	if (type == NVG_TEXTURE_RGBA)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

	if (tex == NULL) return 0;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#ifdef NANOVG_GLES2
	// No support for all of unpack, need to update a whole row at a time.
	if (tex->type == NVG_TEXTURE_RGBA)
		data += y*tex->width*4;
	else
		data += y*tex->width;
	x = 0;
	w = tex->width;
#else
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, y);
#endif

	if (tex->type == NVG_TEXTURE_RGBA)
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

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

static void glnvg__xformToMat3x3(float* m3, float* t)
{
	m3[0] = t[0];
	m3[1] = t[1];
	m3[2] = 0.0f;
	m3[3] = t[2];
	m3[4] = t[3];
	m3[5] = 0.0f;
	m3[6] = t[4];
	m3[7] = t[5];
	m3[8] = 1.0f;
}

static int glnvg__setupPaint(struct GLNVGcontext* gl, struct NVGpaint* paint, struct NVGscissor* scissor, float width, float fringe)
{
	struct NVGcolor innerCol;
	struct NVGcolor  outerCol;
	struct GLNVGtexture* tex = NULL;
	float invxform[6], paintMat[9], scissorMat[9];
	float scissorx = 0, scissory = 0;
	float scissorsx = 0, scissorsy = 0;

	innerCol = paint->innerColor;
	outerCol = paint->outerColor;

	glnvg__xformInverse(invxform, paint->xform);
	glnvg__xformToMat3x3(paintMat, invxform);

	if (scissor->extent[0] < 0.5f || scissor->extent[1] < 0.5f) {
		memset(scissorMat, 0, sizeof(scissorMat));
		scissorx = 1.0f;
		scissory = 1.0f;
		scissorsx = 1.0f;
		scissorsy = 1.0f;
	} else {
		glnvg__xformInverse(invxform, scissor->xform);
		glnvg__xformToMat3x3(scissorMat, invxform);
		scissorx = scissor->extent[0];
		scissory = scissor->extent[1];
		scissorsx = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]) / fringe;
		scissorsy = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]) / fringe;
	}

	if (paint->image != 0) {
		tex = glnvg__findTexture(gl, paint->image);
		if (tex == NULL) return 0;
		glUseProgram(gl->shader.prog);
		glUniform1i(gl->shader.loc[GLNVG_LOC_TYPE], NSVG_SHADER_FILLIMG);
		glUniform2f(gl->shader.loc[GLNVG_LOC_VIEWSIZE], gl->viewWidth, gl->viewHeight);
		glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_SCISSORMAT], 1, GL_FALSE, scissorMat);
		glUniform2f(gl->shader.loc[GLNVG_LOC_SCISSOREXT], scissorx, scissory);
		glUniform2f(gl->shader.loc[GLNVG_LOC_SCISSORSCALE], scissorsx, scissorsy);
		glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_PAINTMAT], 1, GL_FALSE, paintMat);
		glUniform2f(gl->shader.loc[GLNVG_LOC_EXTENT], paint->extent[0], paint->extent[1]);
		glUniform1f(gl->shader.loc[GLNVG_LOC_STROKEMULT], (width*0.5f + fringe*0.5f)/fringe);
		glUniform1i(gl->shader.loc[GLNVG_LOC_TEX], 0);
		glUniform1i(gl->shader.loc[GLNVG_LOC_TEXTYPE], tex->type == NVG_TEXTURE_RGBA ? 0 : 1);
		glnvg__checkError("tex paint loc");
		glBindTexture(GL_TEXTURE_2D, tex->tex);
		glnvg__checkError("tex paint tex");
	} else {
		glUseProgram(gl->shader.prog);
		glUniform1i(gl->shader.loc[GLNVG_LOC_TYPE], NSVG_SHADER_FILLGRAD);
		glUniform2f(gl->shader.loc[GLNVG_LOC_VIEWSIZE], gl->viewWidth, gl->viewHeight);
		glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_SCISSORMAT], 1, GL_FALSE, scissorMat);
		glUniform2f(gl->shader.loc[GLNVG_LOC_SCISSOREXT], scissorx, scissory);
		glUniform2f(gl->shader.loc[GLNVG_LOC_SCISSORSCALE], scissorsx, scissorsy);
		glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_PAINTMAT], 1, GL_FALSE, paintMat);
		glUniform2f(gl->shader.loc[GLNVG_LOC_EXTENT], paint->extent[0], paint->extent[1]);
		glUniform1f(gl->shader.loc[GLNVG_LOC_RADIUS], paint->radius);
		glUniform1f(gl->shader.loc[GLNVG_LOC_FEATHER], paint->feather);
		glUniform4fv(gl->shader.loc[GLNVG_LOC_INNERCOL], 1, innerCol.rgba);
		glUniform4fv(gl->shader.loc[GLNVG_LOC_OUTERCOL], 1, outerCol.rgba);
		glUniform1f(gl->shader.loc[GLNVG_LOC_STROKEMULT], (width*0.5f + fringe*0.5f)/fringe);
		glnvg__checkError("grad paint loc");
	}
	return 1;
}

static void glnvg__renderViewport(void* uptr, int width, int height, int alphaBlend)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	gl->viewWidth = (float)width;
	gl->viewHeight = (float)height;

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	if (alphaBlend == NVG_PREMULTIPLIED_ALPHA)
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void glnvg__renderFlush(void* uptr, int alphaBlend)
{
//	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	NVG_NOTUSED(uptr);
	NVG_NOTUSED(alphaBlend);
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

static void glnvg__uploadPaths(const struct NVGpath* paths, int npaths)
{
	const struct NVGpath* path;
	int i, n = 0;
	for (i = 0; i < npaths; i++) {
		path = &paths[i];
		if (path->nfill > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, n*sizeof(struct NVGvertex), path->nfill * sizeof(struct NVGvertex), &path->fill[0].x);
			n += path->nfill;
		}
		if (path->nstroke > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, n*sizeof(struct NVGvertex), path->nstroke * sizeof(struct NVGvertex), &path->stroke[0].x);
			n += path->nstroke;
		}
	}
}

static void glnvg__renderFill(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
							  const float* bounds, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	const struct NVGpath* path;
	int i, n, offset, maxCount;

	if (gl->shader.prog == 0)
		return;

	maxCount = glnvg__maxVertCount(paths, npaths);
	glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
	glBufferData(GL_ARRAY_BUFFER, maxCount * sizeof(struct NVGvertex), NULL, GL_STREAM_DRAW);
	glnvg__uploadPaths(paths, npaths);

	if (npaths == 1 && paths[0].convex) {

		glEnable(GL_CULL_FACE);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glnvg__setupPaint(gl, paint, scissor, fringe, fringe);

		glDisable(GL_CULL_FACE);
		n = 0;
		for (i = 0; i < npaths; i++) {
			path = &paths[i];
			offset = n * sizeof(struct NVGvertex);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)offset);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(offset + 2*sizeof(float)));
			glDrawArrays(GL_TRIANGLE_FAN, 0, path->nfill);
			n += path->nfill + path->nstroke;
		}

		glEnable(GL_CULL_FACE);

		if (gl->edgeAntiAlias) {
			// Draw fringes
			n = 0;
			for (i = 0; i < npaths; i++) {
				path = &paths[i];
				offset = (n + path->nfill) * sizeof(struct NVGvertex);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)offset);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(offset + 2*sizeof(float)));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, path->nstroke);
				n += path->nfill + path->nstroke;
			}
		}

		glUseProgram(0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

	} else {
		float quad[6*2] = {
			bounds[0], bounds[3], bounds[2], bounds[3], bounds[2], bounds[1],
			bounds[0], bounds[3], bounds[2], bounds[1], bounds[0], bounds[1],
		};

		glEnable(GL_CULL_FACE);

		glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);

		// Draw shapes
		glDisable(GL_BLEND);
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		glStencilFunc(GL_ALWAYS, 0, ~0U);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		glUseProgram(gl->shader.prog);
		glUniform1i(gl->shader.loc[GLNVG_LOC_TYPE], NSVG_SHADER_SIMPLE);
		glUniform2f(gl->shader.loc[GLNVG_LOC_VIEWSIZE], gl->viewWidth, gl->viewHeight);
		glnvg__checkError("fill solid loc");

		glEnableVertexAttribArray(0);

		glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
		glDisable(GL_CULL_FACE);
		n = 0;
		for (i = 0; i < npaths; i++) {
			path = &paths[i];
			offset = n * sizeof(struct NVGvertex);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)offset);
			glDrawArrays(GL_TRIANGLE_FAN, 0, path->nfill);
			n += path->nfill + path->nstroke;
		}

		glEnable(GL_CULL_FACE);

		// Draw aliased off-pixels
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_BLEND);

		glEnableVertexAttribArray(1);
		glnvg__setupPaint(gl, paint, scissor, fringe, fringe);

		if (gl->edgeAntiAlias) {
			glStencilFunc(GL_EQUAL, 0x00, 0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			// Draw fringes
			n = 0;
			for (i = 0; i < npaths; i++) {
				path = &paths[i];
				offset = (n + path->nfill) * sizeof(struct NVGvertex);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)offset);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(offset + 2*sizeof(float)));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, path->nstroke);
				n += path->nfill + path->nstroke;
			}
		}

		// Draw fill
		glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
		glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);

		glDisableVertexAttribArray(1);

		glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * 2*sizeof(float), quad);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (const GLvoid*)0);
		glVertexAttrib2f(1, 0.5f, 1.0f);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUseProgram(0);

		glDisableVertexAttribArray(0);

		glDisable(GL_STENCIL_TEST);
	}
}

static void glnvg__renderStroke(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
								float width, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	const struct NVGpath* path;
	int i, n, offset, maxCount;

	if (gl->shader.prog == 0)
		return;

	glnvg__setupPaint(gl, paint, scissor, width, fringe);

	glEnable(GL_CULL_FACE);

	maxCount = glnvg__maxVertCount(paths, npaths);
	glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
	glBufferData(GL_ARRAY_BUFFER, maxCount * sizeof(struct NVGvertex), NULL, GL_STREAM_DRAW);
	glnvg__uploadPaths(paths, npaths);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Draw Strokes
	n = 0;
	for (i = 0; i < npaths; i++) {
		path = &paths[i];
		offset = (n + path->nfill) * sizeof(struct NVGvertex);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)offset);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(offset + 2*sizeof(float)));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, path->nstroke);
		n += path->nfill + path->nstroke;
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glUseProgram(0);
}

static void glnvg__renderTriangles(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
								   const struct NVGvertex* verts, int nverts)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, paint->image);
	struct NVGcolor color;
	NVG_NOTUSED(scissor);

	if (gl->shader.prog == 0)
		return;

	if (tex != NULL) {
		glBindTexture(GL_TEXTURE_2D, tex->tex);
	}

	glUseProgram(gl->shader.prog);
	glUniform1i(gl->shader.loc[GLNVG_LOC_TYPE], NSVG_SHADER_IMG);
	glUniform2f(gl->shader.loc[GLNVG_LOC_VIEWSIZE], gl->viewWidth, gl->viewHeight);
	glUniform1i(gl->shader.loc[GLNVG_LOC_TEX], 0);
	glUniform1i(gl->shader.loc[GLNVG_LOC_TEXTYPE], (tex != NULL && tex->type == NVG_TEXTURE_RGBA) ? 0 : 1);
	glnvg__checkError("tris solid img loc");

	glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
	glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(struct NVGvertex), verts, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(2 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	color = paint->innerColor;
	glVertexAttrib4fv(2, color.rgba);

	glDrawArrays(GL_TRIANGLES, 0, nverts);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

static void glnvg__renderDelete(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int i;
	if (gl == NULL) return;

	glnvg__deleteShader(&gl->shader);

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].tex != 0)
			glDeleteTextures(1, &gl->textures[i].tex);
	}
	free(gl->textures);

	free(gl);
}


#ifdef NANOVG_GLES2
struct NVGcontext* nvgCreateGLES2(int atlasw, int atlash, int edgeaa)
#else
struct NVGcontext* nvgCreateGL2(int atlasw, int atlash, int edgeaa)
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

#ifdef NANOVG_GLES2
void nvgDeleteGLES2(struct NVGcontext* ctx)
#else
void nvgDeleteGL2(struct NVGcontext* ctx)
#endif
{
	nvgDeleteInternal(ctx);
}

#endif
