/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "gl/SkNativeGLContext.h"
#include "screen/screen.h"
#include "SkTypes.h"

SkNativeGLContext::AutoContextRestore::AutoContextRestore() {
    fOldEGLContext = eglGetCurrentContext();
    fOldDisplay = eglGetCurrentDisplay();
    fOldSurface = eglGetCurrentSurface(EGL_DRAW);

}

SkNativeGLContext::AutoContextRestore::~AutoContextRestore() {
    if (NULL != fOldDisplay) {
        eglMakeCurrent(fOldDisplay, fOldSurface, fOldSurface, fOldEGLContext);
    }
}

///////////////////////////////////////////////////////////////////////////////

SkNativeGLContext::SkNativeGLContext()
    : fContext(EGL_NO_CONTEXT)
    , fDisplay(EGL_NO_DISPLAY)
    , fSurface(EGL_NO_SURFACE) {
}

SkNativeGLContext::~SkNativeGLContext() {
    SkGLContextHelper::deleteBuffers();
    this->destroyGLContext();
}

void SkNativeGLContext::destroyGLContext() {
    if (fDisplay) {
        eglMakeCurrent(fDisplay, 0, 0, 0);

        if (fContext) {
            eglDestroyContext(fDisplay, fContext);
            fContext = EGL_NO_CONTEXT;
        }

        if (fSurface) {
            eglDestroySurface(fDisplay, fSurface);
            fSurface = EGL_NO_SURFACE;
        }

        screen_destroy_window(fScreenWin);
        screen_destroy_context(fScreenCtx);
        //TODO should we close the display?
        fDisplay = EGL_NO_DISPLAY;
    }
}

const GrGLInterface* SkNativeGLContext::createGLContext() {
    fDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean success = eglInitialize(fDisplay, &majorVersion, &minorVersion);
    EGLint eglErrorCode = eglGetError();
    int format = SCREEN_FORMAT_RGBX8888;
    int nbuffers = 2;
    int usage = SCREEN_USAGE_OPENGL_ES2;

    if (success == EGL_FALSE) {
        SkDebugf("error initializing EGL -- code %d\n", eglErrorCode);
    }
    EGLint numConfigs;
    static const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLConfig surfaceConfig;
    success = eglChooseConfig(fDisplay, configAttribs, &surfaceConfig, 1, &numConfigs);
    eglErrorCode = eglGetError();
    if (success == EGL_FALSE) {
        SkDebugf("error choosing display -- code %d\n", eglErrorCode);
    }
    static const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    fContext = eglCreateContext(fDisplay, surfaceConfig, NULL, contextAttribs);
    eglErrorCode = eglGetError();
    if (eglErrorCode != EGL_SUCCESS) {
        SkDebugf("error creating context-- code %d\n", eglErrorCode);
    }

    screen_create_context(&fScreenCtx, 0);
    int rc = screen_create_window(&fScreenWin, fScreenCtx);
    if (rc) {
        SkDebugf("screen_create_window");
        this->destroyGLContext();
        return NULL;
    }

    rc = screen_set_window_property_iv(fScreenWin, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        SkDebugf("screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT)");
        this->destroyGLContext();
        return NULL;
    }

    rc = screen_set_window_property_iv(fScreenWin, SCREEN_PROPERTY_USAGE, &usage);
    if (rc) {
        SkDebugf("screen_set_window_property_iv(SCREEN_PROPERTY_USAGE)");
        this->destroyGLContext();
        return NULL;
    }

    rc = screen_create_window_buffers(fScreenWin, nbuffers);
    if (rc) {
        SkDebugf("screen_create_window_buffers");
        this->destroyGLContext();
        return NULL;
    }
    fSurface = eglCreateWindowSurface(fDisplay, surfaceConfig, fScreenWin, NULL);

    eglMakeCurrent(fDisplay, fSurface, fSurface, fContext);

    const GrGLInterface* interface = GrGLCreateNativeInterface();
    if (!interface) {
        SkDebugf("Failed to create gl interface");
        this->destroyGLContext();
        return NULL;
    }
    return interface;
}

void SkNativeGLContext::makeCurrent() const {
    if (!eglMakeCurrent(fDisplay, fSurface, fSurface, fContext)) {
        SkDebugf("Could not set the context.\n");
    }
}

void SkNativeGLContext::swapBuffers() const {
    if (!eglSwapBuffers(fDisplay, fSurface)) {
        SkDebugf("Could not complete eglSwapBuffers.\n");
    }
}
