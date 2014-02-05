/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VertexBufferCache_h
#define VertexBufferCache_h

#include <map>

struct GrGLInterface;
class GrGLIndexBuffer;
class GrGLVertexBuffer;
class GrGpuGL;

namespace Gentl {

class VertexBufferCache {
public:
    static VertexBufferCache* instance();
    GrGLVertexBuffer* createFromCache(unsigned minSize, GrGpuGL*);
    void reduce();
    void bindQuadEBO(GrGpuGL*);

    // Thread safe.
    static void recycleThreadSafe(GrGLVertexBuffer*);

private:
    enum {
        MaxVertexBufferSize = 1024 * 1024,
        MaxVertexBufferCacheSize = 10 * 1024 * 1024,
    };

    VertexBufferCache();
    typedef std::multimap<unsigned, GrGLVertexBuffer*> VboMap;
    VboMap fCache;
    unsigned fCacheSize;
    GrGLIndexBuffer* fQuadEBO;
};

} // namespace Gentl

#endif
