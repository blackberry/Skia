/*
 * Copyright 2006 The Android Open Source Project
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkOSWindow_Qnx_DEFINED
#define SkOSWindow_Qnx_DEFINED

#include "SkWindow.h"

class SkEvent;
struct bps_event_t;
struct SkQnxScreen;
struct SkQnxEgl;

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void*);
    ~SkOSWindow();

    void loop();

    enum SkBackEndTypes {
        kNone_BackEndType,
        kNativeGL_BackEndType,
    };

    struct AttachmentInfo {
        int fSampleCount;
        int fStencilBits;
    };

    bool attach(SkBackEndTypes attachType, int msaaSampleCount, AttachmentInfo*);
    void detach();
    void present();

    int getMSAASampleCount() const { return fMSAASampleCount; }

protected:
    // Overridden from from SkWindow:
    virtual bool onEvent(const SkEvent&) SK_OVERRIDE;
    virtual void onHandleInval(const SkIRect&) SK_OVERRIDE;
    virtual bool onHandleChar(SkUnichar) SK_OVERRIDE;
    virtual bool onHandleKey(SkKey) SK_OVERRIDE;
    virtual bool onHandleKeyUp(SkKey) SK_OVERRIDE;
    virtual void onSetTitle(const char title[]) SK_OVERRIDE;

private:
    bool initWindow();
    void resizeSkWindowToNativeWindow();
    void closeWindow();

    bool setupEglWindow();

    bool setupScreenWindowForEgl();
    bool setupScreenWindowForNative();
    bool setupScreenWindow(int usage, int format, int nbuffers, int alphaMode);

    void handleButtonEvent(bps_event_t*);
    void handleScreenEvent(bps_event_t*);

    SkQnxEgl* fEgl;
    SkQnxScreen* fScreen;
    int fMSAASampleCount;

    typedef SkWindow INHERITED;
};

#endif
