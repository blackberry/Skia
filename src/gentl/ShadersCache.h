/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef ShadersCache_h
#define ShadersCache_h

#include "Shaders.h"

#include <map>

class GrGpu;

namespace Gentl {

class ShadersCache {
public:
    static ShadersCache* instance();
    Shaders* getShadersForGpu(GrGpu*);
    void removeShaders(GrGpu*);

private:
    ShadersCache();

    std::map<GrGpu*, Shaders*> fGpuToShaders;
};

class CachedShaders : public Shaders {

public:
    CachedShaders(GrGpu*, ShadersCache*);
    virtual void onRelease();
    virtual void onAbandon();

private:
    ShadersCache* fShadersCache;
};

} // namespace Gentl Graphics

#endif // ShadersCache_h
