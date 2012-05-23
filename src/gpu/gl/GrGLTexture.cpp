
/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "GrGLTexture.h"

#include "GrGpuGL.h"
#include "GrContext.h"

#define GPUGL static_cast<GrGpuGL*>(getGpu())

#define GL_CALL(X) GR_GL_CALL(GPUGL->glInterface(), X)

const GrGLenum* GrGLTexture::WrapMode2GLWrap() {
    static const GrGLenum repeatModes[] = {
        GR_GL_CLAMP_TO_EDGE,
        GR_GL_REPEAT,
        GR_GL_MIRRORED_REPEAT
    };
    return repeatModes;
};

void GrGLTexture::init(GrGpuGL* gpu,
                       const Desc& textureDesc,
                       const GrGLRenderTarget::Desc* rtDesc,
                       GrEGLImage* eglImage) {

    GrAssert(0 != textureDesc.fTextureID);

    fTexParams.invalidate();
    fTexParamsTimestamp = GrGpu::kExpiredTimestamp;
    fTexIDObj           = new GrGLTexID(GPUGL->glInterface(),
                                        textureDesc.fTextureID,
                                        textureDesc.fOwnsID);
    fOrientation        = textureDesc.fOrientation;
    fEglImage           = eglImage;

    if (NULL != rtDesc) {
        // we render to the top left
        GrGLIRect vp;
        vp.fLeft   = 0;
        vp.fWidth  = textureDesc.fWidth;
        vp.fBottom = 0;
        vp.fHeight = textureDesc.fHeight;

        fRenderTarget = new GrGLRenderTarget(gpu, *rtDesc, vp, fTexIDObj, this);
    }
}

GrGLTexture::GrGLTexture(GrGpuGL* gpu,
                         const Desc& textureDesc,
                         GrEGLImage* eglImage)
    : INHERITED(gpu,
                textureDesc.fWidth,
                textureDesc.fHeight,
                textureDesc.fConfig) {
    this->init(gpu, textureDesc, NULL, eglImage);
}

GrGLTexture::GrGLTexture(GrGpuGL* gpu,
                         const Desc& textureDesc,
                         const GrGLRenderTarget::Desc& rtDesc)
    : INHERITED(gpu,
                textureDesc.fWidth,
                textureDesc.fHeight,
                textureDesc.fConfig) {
    this->init(gpu, textureDesc, &rtDesc, NULL);
}

void GrGLTexture::onRelease() {
    INHERITED::onRelease();
    GPUGL->notifyTextureDelete(this);
    if (NULL != fTexIDObj) {
        fTexIDObj->unref();
        fTexIDObj = NULL;
    }
    if (NULL != fEglImage) {
        fEglImage->unref();
        fEglImage = NULL;
    }
}

void GrGLTexture::onAbandon() {
    INHERITED::onAbandon();
    if (NULL != fTexIDObj) {
        fTexIDObj->abandon();
    }
}

intptr_t GrGLTexture::getTextureHandle() const {
    return fTexIDObj->id();
}

