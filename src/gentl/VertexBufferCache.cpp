/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "VertexBufferCache.h"

#include "SkTypes.h"
#include "gl/GrGLInterface.h"
#include "gl/GrGLVertexBuffer.h"
#include "gl/GrGpuGL.h"

#include <limits>

namespace Gentl {

VertexBufferCache* VertexBufferCache::instance()
{
    static VertexBufferCache instance;
    return &instance;
}

VertexBufferCache::VertexBufferCache()
    : fCacheSize(0)
    , fQuadEBO(0)
{
}

GrGLVertexBuffer* VertexBufferCache::createFromCache(unsigned minSize, GrGpuGL* gpu)
{
    // Look in our cache for the first VBO that is at least minSize. But if it's
    // more than twice as big as we need we just create a new one anyway.
    VboMap::iterator it = fCache.lower_bound(minSize);
    if (it == fCache.end() || it->second->sizeInBytes() > minSize * 2)
        return static_cast<GrGLVertexBuffer*>(gpu->createVertexBuffer(minSize, true));

    if (!it->second->isValid()) {
        it->second->unref();
        fCache.erase(it);
        return static_cast<GrGLVertexBuffer*>(gpu->createVertexBuffer(minSize, true));
    }

    GrGLVertexBuffer* vbo = it->second;
    fCacheSize -= vbo->sizeInBytes();
    fCache.erase(it);
    return vbo;
}

void VertexBufferCache::recycleThreadSafe(GrGLVertexBuffer* vbo)
{
    if (!vbo)
        return;

    // Just throw away excessively large VBOs to avoid wasting cache space.
    if (vbo->sizeInBytes() > MaxVertexBufferSize || !vbo->isValid()) {
        vbo->unref();
        return;
    }

    VertexBufferCache* cache = VertexBufferCache::instance();
    cache->fCacheSize += vbo->sizeInBytes();
    cache->fCache.insert(std::make_pair(vbo->sizeInBytes(), vbo));

    // Reducing the cache removes every other element which will reduce it
    // by almost half. Typically it goes from 10MB to 5.5MB afterward.
    while (cache->fCacheSize > MaxVertexBufferCacheSize)
        cache->reduce();
}

void VertexBufferCache::reduce()
{
    VboMap::iterator it = fCache.begin();
    VboMap::iterator last = fCache.end();

    // Iterate through the map and erase on every other element.
    // This preserves the overall distribution of small and large VBOs.
    for (int i = 0; it != last; ++i) {
        if (i % 2) {
            fCacheSize -= it->second->sizeInBytes();
            it->second->unref();
            fCache.erase(it++);
        } else {
            ++it;
        }
    }
}

void VertexBufferCache::bindQuadEBO(GrGpuGL* gpu)
{
    if (!fQuadEBO || !fQuadEBO->isValid()) {
        // We don't use GrGpu::getQuadIndexBuffer() because it only allocates 24576 elements and we need more, lots more.
        unsigned short eboData[std::numeric_limits<unsigned short>::max()];
        for (int i = 5, j = 0; i < std::numeric_limits<unsigned short>::max(); i += 6, j += 4) {
            eboData[i - 5] = j;
            eboData[i - 4] = j + 1;
            eboData[i - 3] = j + 2;
            eboData[i - 2] = j + 2;
            eboData[i - 1] = j + 1;
            eboData[i - 0] = j + 3;
        }
        fQuadEBO = static_cast<GrGLIndexBuffer*>(gpu->createIndexBuffer(std::numeric_limits<unsigned short>::max() * sizeof(unsigned), false));
        fQuadEBO->updateData(eboData, std::numeric_limits<unsigned short>::max() * sizeof(unsigned short));
    }
    gpu->glInterface()->fFunctions.fBindBuffer(GR_GL_ELEMENT_ARRAY_BUFFER, fQuadEBO->bufferID());
}

} // namespace Gentl
