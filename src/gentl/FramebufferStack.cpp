/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "FramebufferStack.h"

#include "GrGpu.h"
#include "gl/GrGpuGL.h"
#include "SkTypes.h"

namespace Gentl {

FramebufferStack::FramebufferStack(GrGpu* gpu, const SkMatrix44& view)
    : fGpu(gpu)
    , fGlInterface(static_cast<GrGpuGL*>(gpu)->glInterface())
    , fPreviousFramebufferBinding(0)
{
    fOffsets.push(view);
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &fPreviousFramebufferBinding);
}

FramebufferStack::~FramebufferStack()
{
    for (; !fLayers.empty(); fLayers.pop()) {
        if (fLayers.top()) {
            fLayers.top()->unref(); // not needed any more, can be reused by BeginTransparencyLayer, for instance
        }
    }
}

void FramebufferStack::push(const SkISize& size, const SkMatrix44& view, GrContext* grContext)
{
    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit | kNoStencil_GrTextureFlagBit;
    desc.fConfig = kRGBA_8888_GrPixelConfig;
    desc.fWidth = size.width();
    desc.fHeight = size.height();

    GrTexture* texture = fGpu->createTexture(desc, 0, 0);

    fLayers.push(texture); // Has to be added even if NULL because of corresponding EndTransparencyLayer
    fOffsets.push(view);

    if (!texture) {
        SkDebugf("Can't allocate texture for layer.\n");
        return;
    }

    fGlInterface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, texture->asRenderTarget()->getRenderTargetHandle());

    // Clearing a framebuffer immediately is the oldest optimization in the book.
    fGlInterface->fFunctions.fClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    fGlInterface->fFunctions.fClear(GR_GL_COLOR_BUFFER_BIT);
}

FramebufferStack::RestoreInfo FramebufferStack::pop()
{
    if (fLayers.empty()) {
        RestoreInfo restoreInfo(0, SkMatrix44(SkMatrix44::kIdentity_Constructor));
        return restoreInfo;
    }

    fOffsets.pop();
    RestoreInfo restoreInfo(fLayers.top(), fOffsets.top());
    fLayers.pop();

    if (!restoreInfo.texture) {
        SkDebugf("Could not pop FBO. It was never allocated.\n");
        return restoreInfo;
    }

    SkASSERT(1 == restoreInfo.texture->getRefCnt());
    fGlInterface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, fLayers.empty() ? fPreviousFramebufferBinding : fLayers.top()->asRenderTarget()->getRenderTargetHandle());

    // Return the texture to the caller with a single refCount. When finished with
    // it the caller must unref it, returning it to the cache.
    return restoreInfo;
}

} // namespace Gentl
