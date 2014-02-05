/**
 * Copyright 2014 BlackBerry Limited.
 *
 * Skia qnx entry point.
 */

#include "SkApplication.h"
#include "SkEvent.h"
#include "SkWindow.h"
#include "SkTypes.h"

#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <bps/button.h>

int main(int argc, char** argv)
{
    screen_context_t ctx;
    screen_create_context(&ctx, 0);

    bps_initialize();

    if (BPS_SUCCESS != screen_request_events(ctx)
        || BPS_SUCCESS != navigator_request_events(0)
        || BPS_SUCCESS != button_request_events(0)) {
        SkDebugf("Couldn't initialize bps for skia_qnx.\n");
    } else {
        SkOSWindow* window = create_sk_window((void*)ctx, argc, argv);

        while (SkEvent::ProcessEvent());
        application_init();

        SkDebugf("Enter event loop.\n");
        window->loop();
        SkDebugf("Exiting event loop. Destroying window.\n");
        delete window;
    }

    screen_stop_events(ctx);
    navigator_stop_events(0);
    button_stop_events(0);

    SkDebugf("Destroying screen context and exiting.\n");
    screen_destroy_context(ctx);
    ctx = 0;

    application_term();
    return 0;
}
