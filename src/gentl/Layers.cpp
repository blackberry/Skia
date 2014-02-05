/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Layers.h"

namespace Gentl {

TransparencyLayer::TransparencyLayer()
    : fCommand(-1)
{
    fDirtyRect.setEmpty();
}

TransparencyLayer::TransparencyLayer(int command)
    : fCommand(command)
{
    fDirtyRect.setEmpty();
}

void TransparencyLayer::dirty(const SkRect& rect)
{
    fDirtyRect.join(rect);
}

} // namespace Gentl
