/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Precompile.h"

#include "GrContextFactory.h"
#include "SkTypes.h"
#include "Shaders.h"
#include "ShadersCache.h"

namespace Gentl {

void precompileShaders()
{
    GrContextFactory factory;
    GrContext* context = factory.get(GrContextFactory::kNative_GLContextType);

    if (!context) {
        SkDebugf("Making GLES2 context current failed.");
        return;
    }
    // Call into Gentl to get the job done.
    ShadersCache::instance()->getShadersForGpu(context->getGpu())->compileAndStore();
}

} // namespace Gentl
