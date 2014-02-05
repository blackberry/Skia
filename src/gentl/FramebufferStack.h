/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FramebufferStack_h
#define FramebufferStack_h

#include "SkMatrix44.h"
#include <stack>

class GrContext;
class GrGLInterface;
class GrGpu;
class GrTexture;

namespace Gentl {

class FramebufferStack {
public:
    struct RestoreInfo {
        GrTexture* texture;
        SkMatrix44 view;
        RestoreInfo(GrTexture* inTexture, SkMatrix44 inView) : texture(inTexture), view(inView) {}
    };

    FramebufferStack(GrGpu*, const SkMatrix44& view);
    ~FramebufferStack();

    void push(const SkISize& size, const SkMatrix44& view, GrContext* grContext);
    RestoreInfo pop();

private:
    std::stack<GrTexture*> fLayers;
    std::stack<SkMatrix44> fOffsets;
    GrGpu* fGpu;
    const GrGLInterface* fGlInterface;
    int32_t fPreviousFramebufferBinding;
};


} // namespace Gentl

#endif // FramebufferStack_h
