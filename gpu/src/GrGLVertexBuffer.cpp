/*
    Copyright 2011 Google Inc.
    Copyright (C) 2011 Research In Motion Limited. All rights reserved.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */


#include "GrGLVertexBuffer.h"
#include "GrGpuGL.h"

#define GPUGL static_cast<GrGpuGL*>(getGpu())

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
    if (srcSizeInBytes > size()) {
        return false;
    }
    memcpy(fPtr, src, srcSizeInBytes);
    return true;
}

bool GrGLVertexBuffer::updateSubData(const void* src,
                                     size_t srcSizeInBytes,
                                     size_t offset) {
    if (srcSizeInBytes + offset > size()) {
        return false;
    }
    memcpy((char*)fPtr + offset, src, srcSizeInBytes);
    return true;
}

