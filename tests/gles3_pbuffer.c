#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <stdbool.h>
#include <assert.h>
#include "nanovg.h"

#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include "test_registory.h"
#include "image_compare.h"

static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_STENCIL_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE};

#define BUFFER_WIDTH 1000
#define BUFFER_HEIGHT 600
static const EGLint pbufferAttribs[] = {
    EGL_WIDTH, BUFFER_WIDTH,
    EGL_HEIGHT, BUFFER_HEIGHT,
    EGL_NONE,
};

static const EGLint contextAttribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE,
};

int main(int argc, char *argv[])
{
    int test_result = true;
    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major, minor;
    bool ret;

    ret = eglInitialize(eglDpy, &major, &minor);
    assert(ret);

    printf("EGL version %d.%d\n", major, minor);

    EGLint numConfigs;
    EGLConfig eglCfg;

    ret = eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
    assert(ret);
    assert(numConfigs > 0);

    EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg,
                                                 pbufferAttribs);

    assert(EGL_NO_SURFACE != eglSurf);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT,
                                         contextAttribs);

    assert(EGL_NO_CONTEXT != eglCtx);

    ret = eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
    assert(ret != EGL_FALSE);

    printf("GL_VERSION:%s\n", glGetString(GL_VERSION));
    printf("GL_VENDOR:%s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER:%s\n", glGetString(GL_RENDERER));
    printf("GL_SHADING_LANGUAGE_VERSION:%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    int i = 0;
    for (; i < sizeof(DRAW_TEST_CASES) / sizeof(DRAW_TEST_CASES[0]); ++i)
    {
        const char *test_name = DRAW_TEST_CASES[i].name;
        draw_test_function_type test_function = DRAW_TEST_CASES[i].function;

        NVGcontext *vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);

        int winWidth = 1000;
        int winHeight = 600;

        // Calculate pixel ration for hi-dpi devices.
        float pxRatio = (float)BUFFER_WIDTH / (float)winWidth;

        // Update and render
        glViewport(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        test_function(vg, winWidth, winHeight, pxRatio);

        char outputpath[512];
        snprintf(outputpath, 512, "results/%s-%s.png", RENDERER_NAME, test_name);
        saveScreenShot(BUFFER_WIDTH, BUFFER_HEIGHT, false, outputpath);

        char expectedpath[512];
        snprintf(expectedpath, 512, "expected/%s-%s.png", RENDERER_NAME, test_name);
        if (!imageCompare(outputpath, expectedpath, 5))
        {
            test_result = false;
            printf("%s-%s is failure\n", RENDERER_NAME, test_name);
        }
        glEnable(GL_DEPTH_TEST);

        nvgDeleteGLES3(vg);
    }

    eglTerminate(eglDpy);
    return test_result ? 0 : -1;
}