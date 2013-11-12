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
#ifndef GLSTASH_H
#define GLSTASH_H

int glstInit(struct FONSparams* params);
unsigned int glstRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLSTASH_IMPLEMENTATION

struct GLSTcontext {
	GLuint tex;
	int width, height;
};

static int glst__renderCreate(void* userPtr, int width, int height)
{
	struct GLSTcontext* gl = (struct GLSTcontext*)userPtr;
	glGenTextures(1, &gl->tex);
	if (!gl->tex) return 0;
	gl->width = width;
	gl->height = width;
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	return 1;
}

static void glst__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
	struct GLSTcontext* gl = (struct GLSTcontext*)userPtr;
	if (gl->tex == 0) return;
	int w = rect[2] - rect[0];
	int h = rect[3] - rect[1];
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, gl->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_ALPHA,GL_UNSIGNED_BYTE, data);
}

static void glst__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
	struct GLSTcontext* gl = (struct GLSTcontext*)userPtr;
	if (gl->tex == 0) return;
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(2, GL_FLOAT, sizeof(float)*2, verts);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, tcoords);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(unsigned int), colors);

	glDrawArrays(GL_TRIANGLES, 0, nverts);

	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void glst__renderDelete(void* userPtr)
{
	struct GLSTcontext* gl = (struct GLSTcontext*)userPtr;
	if (gl->tex)
		glDeleteTextures(1, &gl->tex);
	gl->tex = 0;
	free(gl);
}


int glstInit(struct FONSparams* params)
{
	struct GLSTcontext* gl = (struct GLSTcontext*)malloc(sizeof(struct GLSTcontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(struct GLSTcontext));

	params->renderCreate = glst__renderCreate;
	params->renderUpdate = glst__renderUpdate;
	params->renderDraw = glst__renderDraw; 
	params->renderDelete = glst__renderDelete;
	params->userPtr = gl;

	return 1;

error:
	if (gl != NULL) free(gl);
	return 0;
}

unsigned int glstRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (r) | (g << 8) | (b << 16) | (a << 24);
}

#endif
