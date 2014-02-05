/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <SkTypes.h>

#include <process.h>
#include <pthread.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/slog2.h>
#include <sys/cdefs.h>
#include <stdarg.h>
#include <stdint.h>

#define EVENTLOG_MAX_LENGTH 2048

/*
 * http://www.qnx.com/developers/docs/6.3.0SP3/neutrino/lib_ref/p/__progname.html
 */
extern char *__progname;

enum LogMode { Auto = 0, Stdio, Slog2 };
static LogMode s_logMode = Auto;
static uint16_t s_applicationCode = 0;
static char s_applicationName[256] = "";

void setupApplicationLogging();

void SkDebugf(const char* format, ...)
{
    if (s_logMode == Auto) {
        if (!strcmp(getenv("HOME"), "/var")) {
            s_logMode = Stdio;
        } else {
            s_logMode = Slog2;
        }
    }

    switch (s_logMode) {
    case Auto: {
        // This should have been set to Stdio or Slog
        SkASSERT(false);
    }
    case Stdio: {
        va_list args;
        va_start(args, format);
        vfprintf(stdout, format, args);
        va_end(args);
        break;
    }
    case Slog2: {
        if (!*s_applicationName)
            setupApplicationLogging();

        va_list args;
        va_start(args, format);
        vslog2f(0, s_applicationCode, SLOG2_INFO, format, args);
        va_end(args);
        break;
    }
    }
}

void setupApplicationLogging()
{
    static bool setupComplete = false;
    if (setupComplete)
        return;

    setupComplete = true;

    // Reset the log buffers in case this is after a fork. (If there were no log buffers, this does
    // nothing.)
    int rc = slog2_reset();
    if (rc < 0) {
        fprintf(stderr, "Error resetting slog2 buffer!\n");
        return;
    }

    // Set up default slog2 buffer
    slog2_buffer_set_config_t bufferConfigSet;
    bufferConfigSet.buffer_set_name = s_applicationName;
    bufferConfigSet.num_buffers = 1;
    bufferConfigSet.verbosity_level = SLOG2_INFO;
    bufferConfigSet.buffer_config[0].buffer_name = "skbb10";
    bufferConfigSet.buffer_config[0].num_pages = 8;
    slog2_buffer_t bufferHandle;

    for (int i = 0; i < 16; ++i) {
        sprintf(s_applicationName, "%s_%02d", __progname, i);
        rc = slog2_register(&bufferConfigSet, &bufferHandle, 0);
        if (rc >= 0)
            break;
    }

    slog2_set_default_buffer(bufferHandle);

    SkDebugf("slog2 buffers created for %s", s_applicationName);
}


