/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ShadersCache.h"

namespace Gentl {

CachedShaders::CachedShaders(GrGpu* gpu, ShadersCache* shadersCache)
    : Shaders(gpu)
    , fShadersCache(shadersCache)
{ }

void CachedShaders::onRelease()
{
    fShadersCache->removeShaders(getGpu());
    Shaders::onRelease();
}

void CachedShaders::onAbandon()
{
    Shaders::onAbandon();
}

ShadersCache::ShadersCache()
{ }

ShadersCache* ShadersCache::instance()
{
    static ShadersCache* s_cache = 0;
    if (!s_cache) {
        s_cache = new ShadersCache();
    }
    return s_cache;
}

Shaders* ShadersCache::getShadersForGpu(GrGpu* gpu)
{
    std::map<GrGpu*, Shaders*>::iterator it = fGpuToShaders.find(gpu);
    if (it != fGpuToShaders.end())
        return it->second;

    Shaders* shaders = new CachedShaders(gpu, this);
    fGpuToShaders[gpu] = shaders;
    return shaders;
}

void ShadersCache::removeShaders(GrGpu* gpu)
{
    bool didRemove = fGpuToShaders.erase(gpu);
    sk_ignore_unused_variable(didRemove);
    SkASSERT(didRemove);
}

}
