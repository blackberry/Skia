/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "BuffChain.h"

namespace Gentl {

BuffChunkCache* BuffChunkCache::instance()
{
    static BuffChunkCache instance;
    return &instance;
}

}
