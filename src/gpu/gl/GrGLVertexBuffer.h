
/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#ifndef GrGLVertexBuffer_DEFINED
#define GrGLVertexBuffer_DEFINED

#include "../GrVertexBuffer.h"
#include "gl/GrGLInterface.h"

class GrGpuGL;

class GrGLVertexBuffer : public GrVertexBuffer {

public:
    virtual ~GrGLVertexBuffer() { this->release(); }
    // overrides of GrVertexBuffer
    virtual void* lock();
    virtual void* lockPtr() const;
    virtual void unlock();
    virtual bool isLocked() const;
    virtual bool updateData(const void* src, size_t srcSizeInBytes);

protected:
    GrGLVertexBuffer(GrGpuGL* gpu,
                     void* ptr,
                     size_t sizeInBytes,
                     bool dynamic);

    // overrides of GrResource
    virtual void onAbandon();
    virtual void onRelease();

private:
    void* fPtr;
    bool  fLocked;

    friend class GrGpuGL;

    typedef GrVertexBuffer INHERITED;
};

#endif
