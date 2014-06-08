#ifndef gl_utils_h
#define gl_utils_h


struct NVGLUframebuffer {
    struct NVGcontext* ctx;
    GLuint fbo;
    GLuint rbo;
    GLuint texture;
    int image;
};

int nvgluCreateFramebuffer(struct NVGcontext* ctx, struct NVGLUframebuffer* fb, int w, int h);
void nvgluDeleteFramebuffer(struct NVGcontext* ctx, struct NVGLUframebuffer* fb);


#endif /* gl_utils_h */

#ifdef NANOVG_GL_IMPLEMENTATION

int nvgluCreateFramebuffer(struct NVGcontext* ctx, struct NVGLUframebuffer* fb, int w, int h) {
	fb->image = nvgCreateImageRGBA(ctx, w, h, NULL);
	fb->texture = nvglImageHandle(ctx, fb->image);
	nvglImageFlags(ctx, fb->image, NVGL_TEXTURE_FLIP_Y);

	/* frame buffer object */
	glGenFramebuffers(1, &fb->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

	/* render buffer object */
	glGenRenderbuffers(1, &fb->rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, fb->rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

	/* combine all */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		nvgluDeleteFramebuffer(ctx, fb);
		return 0;
	}
	return 1;
}

void nvgluDeleteFramebuffer(struct NVGcontext* ctx, struct NVGLUframebuffer* fb) {
	if (fb->fbo != 0)
		glDeleteFramebuffers(1, &fb->fbo);
	if (fb->rbo != 0)
		glDeleteRenderbuffers(1, &fb->rbo);
	if (fb->image >= 0)
		nvgDeleteImage(ctx, fb->image);
	fb->fbo = 0;
	fb->rbo = 0;
	fb->texture = 0;
	fb->image = -1;
}


#endif /* NANOVG_GL_IMPLEMENTATION */
/* vim: set ft=c nu noet ts=4 sw=4 : */
