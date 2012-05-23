
/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#include "GrGLIndexBuffer.h"
#include "GrGpuGL.h"

#define GPUGL static_cast<GrGpuGL*>(getGpu())

#define GL_CALL(X) GR_GL_CALL(GPUGL->glInterface(), X)

GrGLIndexBuffer::GrGLIndexBuffer(GrGpuGL* gpu,
                                 void* ptr,
                                 size_t sizeInBytes,
                                 bool dynamic)
    : INHERITED(gpu, sizeInBytes, dynamic)
    , fPtr(ptr)
    , fLocked(false) {

}

void GrGLIndexBuffer::onRelease() {
    // make sure we've not been abandoned
    if (fPtr) {
        free(fPtr);
        GPUGL->notifyIndexBufferDelete(this);
        fPtr = 0;
        fLocked = false;
    }
}

void GrGLIndexBuffer::onAbandon() {
    fPtr = 0;
    fLocked = false;
}

void* GrGLIndexBuffer::lock() {
    fLocked = true;
    return fPtr;
}

void* GrGLIndexBuffer::lockPtr() const {
    return fPtr;
}

void GrGLIndexBuffer::unlock() {
    fLocked = false;
}

bool GrGLIndexBuffer::isLocked() const {
    return fLocked;
}

bool GrGLIndexBuffer::updateData(const void* src, size_t srcSizeInBytes) {
    if (srcSizeInBytes > this->sizeInBytes()) {
        return false;
    }
    memcpy(fPtr, src, srcSizeInBytes);
    return true;
}

