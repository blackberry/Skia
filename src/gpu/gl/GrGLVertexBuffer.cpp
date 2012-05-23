
/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#include "GrGLVertexBuffer.h"
#include "GrGpuGL.h"

#define GPUGL static_cast<GrGpuGL*>(getGpu())

#define GL_CALL(X) GR_GL_CALL(GPUGL->glInterface(), X)

GrGLVertexBuffer::GrGLVertexBuffer(GrGpuGL* gpu,
                                   void* ptr,
                                   size_t sizeInBytes,
                                   bool dynamic)
    : INHERITED(gpu, sizeInBytes, dynamic)
    , fPtr(ptr)
    , fLocked(false) {
}

void GrGLVertexBuffer::onRelease() {
    // make sure we've not been abandoned
    if (fPtr) {
        GPUGL->notifyVertexBufferDelete(this);
        free(fPtr);
        fPtr = 0;
        fLocked = false;
    }
}

void GrGLVertexBuffer::onAbandon() {
    fPtr = 0;
    fLocked = false;
}

void* GrGLVertexBuffer::lock() {
    fLocked = true;
    return fPtr;
}

void* GrGLVertexBuffer::lockPtr() const {
    return fPtr;
}

void GrGLVertexBuffer::unlock() {
    fLocked = false;
}

bool GrGLVertexBuffer::isLocked() const {
    return fLocked;
}

bool GrGLVertexBuffer::updateData(const void* src, size_t srcSizeInBytes) {
    if (srcSizeInBytes > this->sizeInBytes()) {
        return false;
    }
    memcpy(fPtr, src, srcSizeInBytes);
    return true;
}

