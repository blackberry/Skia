/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keycodes.h>
#include <screen/screen.h>
#include <assert.h>
#include <bps/accelerometer.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <bps/orientation.h>
#include <bps/button.h>
#include <screen/screen.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#ifndef USE_SKIA_SOFTWARE
#define USE_SKIA_SOFTWARE 0
#else
#undef USE_SKIA_OPENGL
#endif

#ifndef USE_SKIA_OPENGL
// To use hardware-accelerated Skia, set USE_SKIA_OPENGL to 1
// To use software Skia, set USE_SKIA_OPENGL to 0
#define USE_SKIA_OPENGL 0
#endif

#include "SkCanvas.h"
#include "SkBitmapDevice.h"
#include "SkDevice.h"
#if !USE_SKIA_SOFTWARE
#include "GrContextFactory.h"
#include "GrContext.h"
#include "gl/GrGLDefines.h"
#include "gl/GrGLInterface.h"
#include "gl/GrGLUtil.h"
#include "SkGpuDevice.h"

#if !USE_SKIA_OPENGL
#include "GentlDevice.h"
#include "GentlTrace.h"
#include "GrContext.h"
#include "Precompile.h"
#endif
#endif
#include "bbutil.h"

/*************************** BEGIN USER HEADERS ******************************/
#include "DemoBase.h"
/**************************** END USER HEADERS *******************************/

static int width, height;
static screen_context_t screen_cxt;

static SkCanvas *canvas;
static SkBaseDevice *device;

/******************** BEGIN USER VARIABLE DECLARATIONS ***********************/
static int currentDemo;
/********************* END USER VARIABLE DECLARATIONS ************************/


int init() {
    // Make a quick and dirty window to get the screen dimensions and set
    // all the static values. In the future GentL ought to work like this.
    screen_window_t temp_win;
    screen_create_window(&temp_win, screen_cxt);
    int size[2];
    screen_get_window_property_iv(temp_win, SCREEN_PROPERTY_SIZE, size);
    screen_destroy_window(temp_win);
    width = size[0];
    height = size[1];

#if USE_SKIA_SOFTWARE
    // Create a software device. This is the simplest path.
    void* data;
    int stride;
    bbutil_get_native_ptr(&data, &stride, 0);

    SkBitmap* bitmap = new SkBitmap;
    bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height, stride);
    bitmap->setPixels(data);
    device = new SkBitmapDevice(*bitmap);

#else
    // Create a GrContext and RenderTarget to make a SkGpu or Gentl device.
    // Get size of the drawing surface
    EGLint err = eglGetError();
    if (err != 0x3000) {
        SkDebugf("Unable to query EGL surface dimensions\n");
        return EXIT_FAILURE;
    }

    //Query width and height of the window surface created by utility code
    eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &width);
    eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &height);

    // Set up the appropriate Skia canvas
    const GrGLInterface* interface = GrGLCreateNativeInterface();
    GrContext *context = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)interface);

    GrBackendRenderTargetDesc desc;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fConfig = kSkia8888_GrPixelConfig;
    desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
    desc.fSampleCnt = 2;
    desc.fStencilBits = 8;

    GrGLint buffer;
    GR_GL_GetIntegerv(interface, GR_GL_FRAMEBUFFER_BINDING, &buffer);
    desc.fRenderTargetHandle = buffer;
    GrRenderTarget* renderTarget = context->wrapBackendRenderTarget(desc);

#if USE_SKIA_OPENGL
    device = SkGpuDevice::Create(renderTarget);
#else
    device = GentlDevice::Create(renderTarget);
#endif
#endif
    canvas = new SkCanvas(device);


/************************** BEGIN USER SETUP CODE ****************************/
    DemoFactory::createDemos(canvas, width, height);
    currentDemo = 0;
/*************************** END USER SETUP CODE *****************************/

#if !USE_SKIA_SOFTWARE
    interface->fFunctions.fViewport(0, 0, width, height);
    interface->fFunctions.fBindFramebuffer(GL_FRAMEBUFFER, 0);
    interface->fFunctions.fClearColor(1.0, 1.0, 1.0, 1.0);
#endif
    return EXIT_SUCCESS;
}

void render() {
    // Clear the entire canvas to a solid colour
    SkPaint clearColor;
    clearColor.setColor(0xFFCDCDCD); // Cyan is gross. Gainsboro is lovely.
    canvas->clear(clearColor.getColor());

    // Draw the current demo
    if (DemoFactory::getDemo(currentDemo))
        DemoFactory::getDemo(currentDemo)->onDraw();

    canvas->flush();
    bbutil_swap(0);
}

void nextDemo() {
    currentDemo = (currentDemo + 1) % DemoFactory::demoCount();
}

void prevDemo() {
    currentDemo = currentDemo ? currentDemo - 1 : DemoFactory::demoCount() - 1;
}

void handle_screen_event(bps_event_t* event) {
    int type;
    int screenPos[2];
    screen_event_t se = screen_event_get_event(event);
    screen_get_event_property_iv(se, SCREEN_PROPERTY_TYPE, &type);
    screen_get_event_property_iv(se, SCREEN_PROPERTY_POSITION, screenPos);
    if (type == SCREEN_EVENT_MTOUCH_RELEASE) {
        if (screenPos[1] > height * 3 / 4) {
            if (screenPos[0] > width / 4) {
                nextDemo();
            } else if (screenPos[0] < width * 3 / 4) {
                prevDemo();
            }
        }
    }
}

void handle_button_event(bps_event_t* event) {
    if (bps_event_get_code(event) != BUTTON_DOWN)
        return;

    switch (button_event_get_button(event)) {
    case BUTTON_MINUS:
        nextDemo();
        break;
    case BUTTON_PLUS:
        prevDemo();
        break;
    case BUTTON_POWER:
    case BUTTON_PLAYPAUSE:
        break;
    }
}

int main(int argc, char **argv) {
    //Create a screen context that will be used to create an EGL surface to to receive libscreen events
    screen_create_context(&screen_cxt, 0);

    //Initialize BPS library
    bps_initialize();
#if !USE_SKIA_OPENGL && !USE_SKIA_SOFTWARE
    Gentl::precompileShaders();
#endif

#if USE_SKIA_SOFTWARE
    RENDERING_API api = NATIVE;
#else
    RENDERING_API api = GL_ES_2;
#endif
    //Use utility code to initialize EGL for 2D rendering with GL ES 1.1
    if (EXIT_SUCCESS != bbutil_init(screen_cxt, api)) {
        SkDebugf("Unable to initialize rendering API\n");
        screen_destroy_context(screen_cxt);
        return 0;
    }
    //Initialize app data
    if (EXIT_SUCCESS != init()) {
        SkDebugf("Unable to initialize app logic\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    //Signal BPS library that navigator and screen events will be requested
    if (BPS_SUCCESS != screen_request_events(screen_cxt)) {
        SkDebugf("screen_request_events failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    if (BPS_SUCCESS != navigator_request_events(0)) {
        SkDebugf("navigator_request_events failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    if (BPS_SUCCESS != button_request_events(0)) {
        SkDebugf("button_request_events failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    for (;;) {
        //Request and process BPS next available event
        bps_event_t *event = NULL;
        bps_get_event(&event, 0);
        if (event) {
            if (bps_event_get_domain(event) == navigator_get_domain()) {
                if (bps_event_get_code(event) == NAVIGATOR_EXIT) {
                    break;
                }
            }
/********************** BEGIN USER EVENT HANDLING CODE ***********************/
            else if (bps_event_get_domain(event) == screen_get_domain()) {
                handle_screen_event(event);
            }
            else if (bps_event_get_domain(event) == button_get_domain()) {
                handle_button_event(event);
            }
/*********************** END USER EVENT HANDLING CODE ************************/
        }

        render();
    }

    delete canvas;
    delete device;

    DemoFactory::destroyDemos();

    screen_stop_events(screen_cxt);
    button_stop_events(0);

    //Shut down BPS library for this process
    bps_shutdown();

    bbutil_terminate();

    //Destroy libscreen context
    screen_destroy_context(screen_cxt);

    SkDebugf("%s exiting with return code 0.", *argv);
    return 0;
}
