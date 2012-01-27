/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keycodes.h>
#include <time.h>

#include "bbutil.h"

EGLDisplay egl_disp;
EGLSurface egl_surf;

static EGLConfig egl_conf;
static EGLContext egl_ctx;

static screen_context_t screen_ctx;
static screen_window_t screen_win;
static const int nbuffers = 2;
static screen_buffer_t screen_native_buffers[nbuffers];

static bool use_native = false;

static void
bbutil_egl_perror(const char *msg) {
    static const char *errmsg[] = {
        "function succeeded",
        "EGL is not initialized, or could not be initialized, for the specified display",
        "cannot access a requested resource",
        "failed to allocate resources for the requested operation",
        "an unrecognized attribute or attribute value was passed in an attribute list",
        "an EGLConfig argument does not name a valid EGLConfig",
        "an EGLContext argument does not name a valid EGLContext",
        "the current surface of the calling thread is no longer valid",
        "an EGLDisplay argument does not name a valid EGLDisplay",
        "arguments are inconsistent",
        "an EGLNativePixmapType argument does not refer to a valid native pixmap",
        "an EGLNativeWindowType argument does not refer to a valid native window",
        "one or more argument values are invalid",
        "an EGLSurface argument does not name a valid surface configured for rendering",
        "a power management event has occurred",
    };

    fprintf(stderr, "%s: %s\n", msg, errmsg[eglGetError() - EGL_SUCCESS]);
}
EGLConfig bbutil_choose_config(EGLDisplay egl_disp, enum RENDERING_API api) {
    EGLConfig egl_conf = (EGLConfig)0;
    EGLConfig *egl_configs;
    EGLint egl_num_configs;
    EGLint val;
    EGLBoolean rc;
    EGLint i;

    rc = eglGetConfigs(egl_disp, NULL, 0, &egl_num_configs);
    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglGetConfigs");
        return egl_conf;
    }
    if (egl_num_configs == 0) {
        fprintf(stderr, "eglGetConfigs: could not find a configuration\n");
        return egl_conf;
    }

    egl_configs = new EGLConfig[egl_num_configs];
    if (egl_configs == NULL) {
        fprintf(stderr, "could not allocate memory for %d EGL configs\n", egl_num_configs);
        return egl_conf;
    }

    rc = eglGetConfigs(egl_disp, egl_configs,
        egl_num_configs, &egl_num_configs);
    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglGetConfigs");
        free(egl_configs);
        return egl_conf;
    }

    for (i = 0; i < egl_num_configs; i++) {
        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_SURFACE_TYPE, &val);
        if (!(val & EGL_WINDOW_BIT)) {
            continue;
        }

        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_RENDERABLE_TYPE, &val);
        if (!(val & api)) {
            continue;
        }

        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_DEPTH_SIZE, &val);
        if ((api & (GL_ES_1|GL_ES_2)) && (val == 0)) {
            continue;
        }

        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_RED_SIZE, &val);
        if (val != 8) {
            continue;
        }
        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_GREEN_SIZE, &val);
        if (val != 8) {
            continue;
        }

        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_BLUE_SIZE, &val);
        if (val != 8) {
            continue;
        }

        eglGetConfigAttrib(egl_disp, egl_configs[i], EGL_BUFFER_SIZE, &val);
        if (val != 32) {
            continue;
        }

        egl_conf = egl_configs[i];
        break;
    }

    delete []egl_configs;

    if (egl_conf == (EGLConfig)0) {
        fprintf(stderr, "bbutil_choose_config: could not find a matching configuration\n");
    }

    return egl_conf;
}

int
bbutil_init_egl(screen_context_t ctx, enum RENDERING_API api) {
    int usage;
    int format = SCREEN_FORMAT_RGBX8888;
    int nbuffers = 2;
    EGLint interval = 1;
    int rc;
    EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    if (api == GL_ES_1) {
        usage = SCREEN_USAGE_OPENGL_ES1;
    } else if (api == GL_ES_2) {
        usage = SCREEN_USAGE_OPENGL_ES2;
    } else if (api == VG) {
        usage = SCREEN_USAGE_OPENVG;
    } else {
        fprintf(stderr, "invalid api setting\n");
        return EXIT_FAILURE;
    }

    //Simple egl initialization
    screen_ctx = ctx;

    egl_disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_disp == EGL_NO_DISPLAY) {
        bbutil_egl_perror("eglGetDisplay");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = eglInitialize(egl_disp, NULL, NULL);
    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglInitialize");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    if ((api == GL_ES_1) || (api == GL_ES_2)) {
        rc = eglBindAPI(EGL_OPENGL_ES_API);
    } else if (api == VG) {
        rc = eglBindAPI(EGL_OPENVG_API);
    }

    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglBindApi");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    egl_conf = bbutil_choose_config(egl_disp, api);
    if (egl_conf == (EGLConfig)0) {
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    if (api == GL_ES_2) {
        egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, attributes);
    } else {
        egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, NULL);
    }

    if (egl_ctx == EGL_NO_CONTEXT) {
        bbutil_egl_perror("eglCreateContext");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_create_window(&screen_win, screen_ctx);
    if (rc) {
        perror("screen_create_window");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT)");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    if (rc) {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_USAGE)");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_create_window_buffers(screen_win, nbuffers);
    if (rc) {
        perror("screen_create_window_buffers");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    egl_surf = eglCreateWindowSurface(egl_disp, egl_conf, screen_win, NULL);
    if (egl_surf == EGL_NO_SURFACE) {
        bbutil_egl_perror("eglCreateWindowSurface");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = eglMakeCurrent(egl_disp, egl_surf, egl_surf, egl_ctx);
    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglMakeCurrent");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = eglSwapInterval(egl_disp, interval);
    if (rc != EGL_TRUE) {
        bbutil_egl_perror("eglSwapInterval");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#ifndef USE_GLES2
int
bbutil_init_gl2d() {
    EGLint surface_width, surface_height;

    if ((egl_disp == EGL_NO_DISPLAY) || (egl_surf == EGL_NO_SURFACE) ){
        return EXIT_FAILURE;
    }

    eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &surface_width);
    eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &surface_height);

       glShadeModel(GL_SMOOTH);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glViewport(0, 0, surface_width, surface_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrthof(0.0f, (float)(surface_width) / (float)(surface_height), 0.0f, 1.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return EXIT_SUCCESS;
}
#else
int
bbutil_init_gl2d() {
    EGLint surface_width, surface_height;

    if ((egl_disp == EGL_NO_DISPLAY) || (egl_surf == EGL_NO_SURFACE) ){
        return EXIT_FAILURE;
    }

    eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &surface_width);
    eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &surface_height);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glViewport(0, 0, surface_width, surface_height);

    return EXIT_SUCCESS;
}
#endif

int
bbutil_get_native_ptr(void **ptr, int *stride, int index) {
    if (!use_native)
        return EXIT_FAILURE;

    int rc = screen_get_window_property_pv(screen_win,
            SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&screen_native_buffers);
    if (rc) {
        perror("screen_get_window_property_pv(SCREEN_PROPERTY_RENDER_BUFFERS)");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_get_buffer_property_pv(screen_native_buffers[index], SCREEN_PROPERTY_POINTER, ptr);
    if (rc) {
        perror("screen_get_buffer_property_pv(SCREEN_PROPERTY_POINTER)");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Got %d buffer ptr %08x\n", index, *ptr);

    rc = screen_get_buffer_property_iv(screen_native_buffers[index], SCREEN_PROPERTY_STRIDE, stride);
    if (rc) {
        perror("screen_get_buffer_property_iv(SCREEN_PROPERTY_STRIDE)");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
bbutil_init_native(screen_context_t ctx) {
    int format = SCREEN_FORMAT_RGBX8888;
    int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;
    int rc;

    screen_ctx = ctx;

    rc = screen_create_window(&screen_win, screen_ctx);
    if (rc) {
        perror("screen_create_window");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT)");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    if (rc) {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_USAGE)");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    rc = screen_create_window_buffers(screen_win, nbuffers);
    if (rc) {
        perror("screen_create_window_buffers");
        bbutil_terminate();
        return EXIT_FAILURE;
    }

    use_native = true;
    return EXIT_SUCCESS;
}

int
bbutil_init(screen_context_t ctx, enum RENDERING_API api) {
    if (api == NATIVE) {
        return bbutil_init_native(ctx);
    } else {

        if (EXIT_SUCCESS != bbutil_init_egl(ctx, api)) {
            return EXIT_FAILURE;
        }

    #ifdef USE_GLES2
        if ((GL_ES_2 == api) && (EXIT_SUCCESS != bbutil_init_gl2d())) {
    #else
        if ((GL_ES_1 == api) && (EXIT_SUCCESS != bbutil_init_gl2d())) {
    #endif
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
}

void
bbutil_terminate() {
    //Typical EGL cleanup
    if (egl_disp != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surf != EGL_NO_SURFACE) {
            eglDestroySurface(egl_disp, egl_surf);
            egl_surf = EGL_NO_SURFACE;
        }
        if (egl_ctx != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_disp, egl_ctx);
            egl_ctx = EGL_NO_CONTEXT;
        }
        if (screen_win != NULL) {
            screen_destroy_window(screen_win);
            screen_win = NULL;
        }
        eglTerminate(egl_disp);
        egl_disp = EGL_NO_DISPLAY;
    }
    eglReleaseThread();
}

void
bbutil_swap(int index) {
    if (use_native) {
        static const int rect[4] = {0, 0, 1024, 600};
        screen_post_window(screen_win, screen_native_buffers[index], 1, rect, 0);
    } else {
        int rc = eglSwapBuffers(egl_disp, egl_surf);
        if (rc != EGL_TRUE) {
            bbutil_egl_perror("eglSwapBuffers");
        }
    }
}

void
bbutil_clear() {
    if (!use_native)
        glClear(GL_COLOR_BUFFER_BIT);
}
