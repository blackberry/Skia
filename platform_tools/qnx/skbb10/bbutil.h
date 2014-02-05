/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _UTILITY_H_INCLUDED
#define _UTILITY_H_INCLUDED

#include <EGL/egl.h>
#ifndef USE_GLES2
#include <GLES/gl.h>
#else
#include <GLES2/gl2.h>
#endif
#include <screen/screen.h>
#include <sys/platform.h>

extern EGLDisplay egl_disp;
extern EGLSurface egl_surf;

enum RENDERING_API {NATIVE = 0, GL_ES_1 = EGL_OPENGL_ES_BIT, GL_ES_2 = EGL_OPENGL_ES2_BIT, VG = EGL_OPENVG_BIT};

/**
 * Initializes EGL, GL and loads a default font
 *
 * \param libscreen context that will be used for EGL setup
 * \return EXIT_SUCCESS if initialization succeeded otherwise EXIT_FAILURE
 */
int bbutil_init(screen_context_t ctx, enum RENDERING_API api);

int bbutil_init_native(screen_context_t ctx);

int bbutil_get_native_ptr(void **ptr, int *stride, int index);
/**
 * Initializes EGL
 *
 * \param libscreen context that will be used for EGL setup
 * \return EXIT_SUCCESS if initialization succeeded otherwise EXIT_FAILURE
 */
int bbutil_init_egl(screen_context_t ctx, enum RENDERING_API api);

/**
 * Initializes GL 1.1 for simple 2D rendering. GL2 initialization will be added at a later point.
 *
 * \return EXIT_SUCCESS if initialization succeeded otherwise EXIT_FAILURE
 */
int bbutil_init_gl2d();

/**
 * Terminates EGL
 */
void bbutil_terminate();

/**
 * Swaps default bbutil window surface to the screen
 */
void bbutil_swap(int index);

/**
 * Clears the screen of any existing text.
 * NOTE: must be called after a successful return from bbutil_init() or bbutil_init_egl() call
 */
void bbutil_clear();

#endif
