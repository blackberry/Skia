/*
 * Copyright 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkOSWindow_Qnx.h"

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColor.h"
#include "SkEvent.h"
#include "SkKey.h"
#include "SkWindow.h"

#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <bps/button.h>
#include <screen/screen.h>
#include <sys/keycodes.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <unistd.h>

// egl state
struct SkQnxEgl {
    SkQnxEgl() { memset(this, 0, sizeof(*this)); }
    EGLConfig fConfig;
    EGLConfig fContext;
    EGLDisplay fDisplay;
    EGLSurface fSurface;
};

// libscreen state
struct SkQnxScreen {
    SkQnxScreen() { memset(this, 0, sizeof(*this)); }
    screen_context_t fContext;
    screen_window_t fWindow;
};

SkOSWindow::SkOSWindow(void* ctx)
    : INHERITED()
    , fEgl(new SkQnxEgl)
    , fScreen(new SkQnxScreen)
    , fMSAASampleCount(0)
{
    fScreen->fContext = reinterpret_cast<screen_context_t>(ctx);
    initWindow();
    resizeSkWindowToNativeWindow();
}

SkOSWindow::~SkOSWindow() {
    delete fEgl;
    fEgl = 0;

    delete fScreen;
    fScreen = 0;

    closeWindow();
}

bool SkOSWindow::attach(SkBackEndTypes backend, int msaaSampleCount, AttachmentInfo* info) {
    if (fEgl->fSurface == EGL_NO_SURFACE || fEgl->fContext == EGL_NO_CONTEXT) {
        detach();
    }

    if (backend != kNativeGL_BackEndType)
        return false;

    if (info) {
        info->fSampleCount = 2;
        info->fStencilBits = 8;
    }

    if (!setupEglWindow())
        return false;

    int rc;
    rc = eglMakeCurrent(fEgl->fDisplay, fEgl->fSurface, fEgl->fSurface, fEgl->fContext);
    if (rc != EGL_TRUE) {
        SkDebugf("Could not make egl current on this thread.\n");
        return false;
    }

    glViewport(0, 0, width(), height());
    glClearColor(1, 0, 1, 1); // Nice debug magenta that we ought to never see.
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    return true;
}

void SkOSWindow::detach() {

}

void SkOSWindow::closeWindow() {

    if (fEgl->fDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(fEgl->fDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (fEgl->fSurface != EGL_NO_SURFACE) {
            eglDestroySurface(fEgl->fDisplay, fEgl->fSurface);
            fEgl->fSurface = EGL_NO_SURFACE;
        }
        if (fEgl->fContext != EGL_NO_CONTEXT) {
            eglDestroyContext(fEgl->fDisplay, fEgl->fContext);
            fEgl->fContext = EGL_NO_CONTEXT;
        }
    }

    if (fEgl->fDisplay != EGL_NO_DISPLAY) {
        eglTerminate(fEgl->fDisplay);
        fEgl->fDisplay = EGL_NO_DISPLAY;
        eglReleaseThread();
    }

    if (fScreen->fWindow) {
        screen_destroy_window(fScreen->fWindow);
        fScreen->fWindow = 0;
    }
}

bool SkOSWindow::initWindow() {
    if (fEgl->fDisplay)
        return true;

    fEgl->fDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (fEgl->fDisplay == EGL_NO_DISPLAY) {
        SkDebugf("Failed to get egl display.");
        return false;
    }

    int rc;
    rc = eglInitialize(fEgl->fDisplay, NULL, NULL);
    if (rc != EGL_TRUE) {
        SkDebugf("eglInitialize failed.");
        return false;
    }

    rc = eglBindAPI(EGL_OPENGL_ES_API);
    if (rc != EGL_TRUE) {
        SkDebugf("Could not bind to openGLES api.\n");
        return false;
    }

    EGLint attributes[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_BLUE_SIZE,       8,
        EGL_GREEN_SIZE,      8,
        EGL_ALPHA_SIZE,      8,
        EGL_STENCIL_SIZE,    8,
        EGL_NONE };

    EGLint possibleConfigurations = 0;
    eglChooseConfig(fEgl->fDisplay, attributes, &fEgl->fConfig, 1, &possibleConfigurations);
    if (!fEgl->fConfig) {
        SkDebugf("Could not choose egl config. There were %d possible configurations.\n", possibleConfigurations);
        return false;
    }

    return true;
}

void SkOSWindow::resizeSkWindowToNativeWindow()
{
    // There are a couple ways to do this. The easiest is to use the environment.
    const char* strWidth = getenv("WIDTH");
    const char* strHeight = getenv("HEIGHT");
    if (strWidth && strHeight) {
        resize(atoi(strWidth), atoi(strHeight));
        return;
    }

    // The second way is to create a throwaway window.
    int size[2];
    screen_window_t window;
    screen_create_window(&window, fScreen->fContext);
    screen_get_window_property_iv(window, SCREEN_PROPERTY_SIZE, size);
    screen_destroy_window(window);
    resize(size[0], size[1]);
}

bool SkOSWindow::setupEglWindow()
{
    if (fEgl->fSurface != EGL_NO_SURFACE)
        return true;

    SkDebugf("SkOSWindow::setupEglWindow for dispay %d.\n", fEgl->fDisplay);

    int rc;
    EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    fEgl->fContext = eglCreateContext(fEgl->fDisplay, fEgl->fConfig, EGL_NO_CONTEXT, attributes);

    if (fEgl->fContext == EGL_NO_CONTEXT) {
        SkDebugf("Could create egl context.");
        return false;
    }

    rc = setupScreenWindowForEgl();
    if (!rc) {
        SkDebugf("Could not create screen window for egl.\n");
        return false;
    }

    fEgl->fSurface = eglCreateWindowSurface(fEgl->fDisplay, fEgl->fConfig, fScreen->fWindow, NULL);
    if (fEgl->fSurface == EGL_NO_SURFACE) {
        SkDebugf("Could not create egl window surface.\n");
        return false;
    }

    return true;
}

bool SkOSWindow::setupScreenWindowForEgl() {
    return setupScreenWindow(SCREEN_FORMAT_RGBX8888, SCREEN_USAGE_OPENGL_ES2, 2, 0);
}

bool SkOSWindow::setupScreenWindowForNative() {
    return setupScreenWindow(SCREEN_FORMAT_RGBA8888, SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE, 1, SCREEN_PRE_MULTIPLIED_ALPHA);
}

bool SkOSWindow::setupScreenWindow(int format, int usage, int nbuffers, int alphaMode) {
    int rc;
    if (!fScreen->fWindow) {
        rc = screen_create_window(&fScreen->fWindow, fScreen->fContext);
        if (rc) {
            SkDebugf("Could not create screen window.\n");
            return false;
        }

        rc = screen_set_window_property_iv(fScreen->fWindow, SCREEN_PROPERTY_FORMAT, &format);
        if (rc) {
            SkDebugf("Could not set window format.\n");
            return false;
        }

        rc = screen_set_window_property_iv(fScreen->fWindow, SCREEN_PROPERTY_USAGE, &usage);
        if (rc) {
            SkDebugf("Could not set window usage.\n");
            return false;
        }

        if (alphaMode) {
            rc = screen_set_window_property_iv(fScreen->fWindow, SCREEN_PROPERTY_ALPHA_MODE, &alphaMode);
            if (rc) {
                SkDebugf("Could not set window alpha mode.\n");
                return false;
            }
        }
    }

    screen_destroy_window_buffers(fScreen->fWindow);
    rc = screen_create_window_buffers(fScreen->fWindow, nbuffers);
    if (rc) {
        SkDebugf("Could not create window buffers.\n");
        return false;
    }

    return true;
}

void SkOSWindow::loop() {
    for (;;) {
        SkEvent::ServiceQueueTimer();
        bool moreToDo = SkEvent::ProcessEvent();

        if (moreToDo)
            continue;

        if (isDirty()) {
            update(0);
        }

        //Request and process BPS next available event
        bps_event_t* event = 0;
        bps_get_event(&event, 0);
        if (!event)
            continue;

        if (bps_event_get_domain(event) == navigator_get_domain()) {
            if (bps_event_get_code(event) == NAVIGATOR_EXIT) {
                break;
            }
        }
        else if (bps_event_get_domain(event) == screen_get_domain()) {
             handleScreenEvent(event);
        }
        else if (bps_event_get_domain(event) == button_get_domain()) {
             handleButtonEvent(event);
        } else {
            SkDebugf("Received unknown event.\n");
        }
    }
}

SkKey qnxButtonToSkKey(int button) {
    switch (button) {
    case BUTTON_POWER:
    case BUTTON_PLAYPAUSE:
        break;
    case BUTTON_PLUS:
        return kRight_SkKey;
    case BUTTON_MINUS:
        return kLeft_SkKey;
    }

    return kNONE_SkKey;
}

void SkOSWindow::handleButtonEvent(bps_event_t* event) {
    int button = button_event_get_button(event);
    bool pressed = bps_event_get_code(event) == BUTTON_DOWN;

    // Hack Play/Pause to d to switch devices.
    if (pressed && button == BUTTON_PLAYPAUSE) {
        handleChar('d');
        return;
    }

    SkKey key = qnxButtonToSkKey(button);
    if (pressed)
        handleKey(key);
    else
        handleKeyUp(key);
}

void SkOSWindow::handleScreenEvent(bps_event_t* bpsEvent) {
    int type;
    SkIPoint position;
    screen_event_t event = screen_event_get_event(bpsEvent);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, &position.fX);

    switch (type) {
    case SCREEN_EVENT_KEYBOARD: {
        int cap, flags, mods, scan, sym;
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &cap);
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_FLAGS, &flags);
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_MODIFIERS, &mods);
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SCAN, &scan);
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SYM, &sym);

        SkUnichar character = 0;

        // Really only interested in valid unicode chars here.
        if (flags & KEY_SYM_VALID)
            character = sym;
        else
            break;

        // We don't care at all about the physical key events yet.
        if (flags & KEY_REPEAT || flags & KEY_DOWN)
            handleChar(character);
        break;
    }
    }
}

void SkOSWindow::present() {
    SkASSERT(fEgl->fDisplay != EGL_NO_DISPLAY);
    eglSwapBuffers(fEgl->fDisplay, fEgl->fSurface);
}

void SkOSWindow::onSetTitle(const char title[]) {
    SkDebugf("Title set to '%s'.\n", title);
}

bool SkOSWindow::onEvent(const SkEvent&) {
    return false;
}

void SkOSWindow::onHandleInval(const SkIRect&) {
}

bool SkOSWindow::onHandleChar(SkUnichar) {
    return false;
}

bool SkOSWindow::onHandleKey(SkKey) {
    return false;
}

bool SkOSWindow::onHandleKeyUp(SkKey) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void SkEvent::SignalNonEmptyQueue() {
}

void SkEvent::SignalQueueTimer(SkMSec delay) {
}
