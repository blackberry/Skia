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
#include <screen/screen.h>
#include <EGL/egl.h>

// To use hardware-accelerated Skia, set USE_SKIA_OPENGL to 1
// To use software Skia, set USE_SKIA_OPENGL to 0
#define USE_SKIA_OPENGL 0

#if USE_SKIA_OPENGL
#include <GLES2/gl2.h>
#include "GrContext.h"
#include "SkGpuDevice.h"
#include "SkGpuDeviceFactory.h"
#include "SkGpuCanvas.h"
#else
#include "SkBitmap.h"
#include "SkCanvas.h"
#endif
#include "bbutil.h"


/*************************** BEGIN USER HEADERS ******************************/
#include "DemoBase.h"
/**************************** END USER HEADERS *******************************/


static float width, height;
static screen_context_t screen_cxt;

#if USE_SKIA_OPENGL
static SkGpuCanvas *canvas;
static SkDevice *device;
static SkDeviceFactory *factory;
#else
static int currentSurface = 0;
static SkBitmap drawingSurface[2];
static SkCanvas *canvas;
#endif


/******************** BEGIN USER VARIABLE DECLARATIONS ***********************/
static DemoBase **demos;
static int currentIndex;
/********************* END USER VARIABLE DECLARATIONS ************************/


int init() {
    EGLint surface_width, surface_height;

    // Get size of the drawing surface
#if USE_SKIA_OPENGL
    EGLint err = eglGetError();
    if (err != 0x3000) {
        fprintf(stderr, "Unable to query EGL surface dimensions\n");
        return EXIT_FAILURE;
    }

    //Query width and height of the window surface created by utility code
    eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &surface_width);
    eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &surface_height);
#else
    surface_width = 1024;
    surface_height = 600;
#endif

    // Set up the appropriate Skia canvas
#if USE_SKIA_OPENGL
    GrContext *context = GrContext::Create(kOpenGL_Shaders_GrEngine, 0);
    canvas = new SkGpuCanvas( context, SkGpuDevice::Current3DApiRenderTarget());
    factory = new SkGpuDeviceFactory(context, SkGpuDevice::Current3DApiRenderTarget());
    device = factory->newDevice(canvas, SkBitmap::kARGB_8888_Config, surface_width, surface_height, true, false);

    canvas->setDevice(device);
#else
    drawingSurface[0].setConfig(SkBitmap::kARGB_8888_Config, surface_width, surface_height, surface_width*4);
    drawingSurface[1].setConfig(SkBitmap::kARGB_8888_Config, surface_width, surface_height, surface_width*4);
    void *pixels;
    int stride;
    if (bbutil_get_native_ptr(&pixels, &stride, 0) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    drawingSurface[0].setPixels(pixels);
    if (bbutil_get_native_ptr(&pixels, &stride, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    drawingSurface[1].setPixels(pixels);
    canvas = new SkCanvas(drawingSurface[0]);
#endif
    width = (float) surface_width;
    height = (float) surface_height;


/************************** BEGIN USER SETUP CODE ****************************/
    demos = DemoFactory::createAllDemos(canvas, width, height);
    currentIndex = 0;
/*************************** END USER SETUP CODE *****************************/


#if USE_SKIA_OPENGL
    glViewport(0, 0, (int) width, (int) height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0, 1.0, 1.0, 1.0);
#endif

    return EXIT_SUCCESS;
}

void render() {
#if USE_SKIA_OPENGL
    bbutil_clear();
#endif


/************************** BEGIN USER RENDER CODE ***************************/
    // Clear the entire canvas to a solid colour
    SkPaint clearColor;
    clearColor.setARGB(255,0xdd,0xdd,0xdd);
    SkRect fullScreen;
    fullScreen.set(0,0,width,height);
    canvas->drawRect(fullScreen, clearColor);

    // Draw the current demo
    if (demos[currentIndex])
        demos[currentIndex]->onDraw();
/**************************** END USER RENDER CODE ***************************/


    // Draw the contents of the canvas to the screen
#if USE_SKIA_OPENGL
    device->flush();
    bbutil_swap(0);
#else
    bbutil_swap(currentSurface);
    currentSurface = 1 - currentSurface;
    canvas->setBitmapDevice(drawingSurface[currentSurface]);
#endif
}

int main(int argc, char **argv) {
    int rc;
#if USE_SKIA_OPENGL
    RENDERING_API api = GL_ES_2;
#else
    RENDERING_API api = NATIVE;
#endif

    //Create a screen context that will be used to create an EGL surface to to receive libscreen events
    screen_create_context(&screen_cxt, 0);

    //Initialize BPS library
    bps_initialize();

    //Use utility code to initialize EGL for 2D rendering with GL ES 1.1
    if (EXIT_SUCCESS != bbutil_init(screen_cxt, api)) {
        fprintf(stderr, "Unable to initialize rendering API\n");
        screen_destroy_context(screen_cxt);
        return 0;
    }

    //Initialize app data
    if (EXIT_SUCCESS != init()) {
        fprintf(stderr, "Unable to initialize app logic\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    //Signal BPS library that navigator orientation is to be locked
    if (BPS_SUCCESS != navigator_rotation_lock(true)) {
        fprintf(stderr, "navigator_rotation_lock failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    //Signal BPS library that navigator and screen events will be requested
    if (BPS_SUCCESS != screen_request_events(screen_cxt)) {
        fprintf(stderr, "screen_request_events failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    if (BPS_SUCCESS != navigator_request_events(0)) {
        fprintf(stderr, "navigator_request_events failed\n");
        bbutil_terminate();
        screen_destroy_context(screen_cxt);
        return 0;
    }

    static int counter = 0;
    for (;;) {
        //Request and process BPS next available event
        bps_event_t *event = NULL;
        rc = bps_get_event(&event, 0);
        assert(rc == BPS_SUCCESS);
        if (event) {
            if (bps_event_get_domain(event) == navigator_get_domain()) {
                if (bps_event_get_code(event) == NAVIGATOR_EXIT) {
                    break;
                }
            } else if (bps_event_get_domain(event) == screen_get_domain()) {
                int type;
                int screenPos[2];
                screen_event_t se = screen_event_get_event(event);
                screen_get_event_property_iv(se, SCREEN_PROPERTY_TYPE, &type);
                screen_get_event_property_iv(se, SCREEN_PROPERTY_POSITION, screenPos);

/********************** BEGIN USER EVENT HANDLING CODE ***********************/
                if (type == SCREEN_EVENT_MTOUCH_RELEASE) {
                    if (screenPos[1] > height - 100) {
                        if (screenPos[0] > width - 80) {
                            currentIndex++;
                            if (currentIndex > DemoFactory::demoCount() - 1)
                                currentIndex = 0;
                        } else if (screenPos[0] < 80) {
                            currentIndex--;
                            if (currentIndex < 0)
                                currentIndex = DemoFactory::demoCount() - 1;
                        }
                    }
                }
/*********************** END USER EVENT HANDLING CODE ************************/
            }
        }

        render();
    }

    delete canvas;
#if USE_SKIA_OPENGL
    delete device;
    delete factory;
#endif

    DemoFactory::deleteDemos(demos);

    //Stop requesting events from libscreen
    screen_stop_events(screen_cxt);

    //Shut down BPS library for this process
    bps_shutdown();

    //Use utility code to terminate EGL setup
    bbutil_terminate();

    //Destroy libscreen context
    screen_destroy_context(screen_cxt);
    return 0;
}
