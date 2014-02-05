/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlGraphicsLayers_h
#define GentlGraphicsLayers_h

#include "SkRect.h"

namespace Gentl {

class TransparencyLayer {
public:
    TransparencyLayer();
    TransparencyLayer(int command);

    int command() const { return fCommand; }
    void adjustCommand(int delta) { fCommand += delta; }

    SkRect dirtyRect() const { return fDirtyRect; }
    void dirty(const SkRect&);

private:
    int fCommand;
    SkRect fDirtyRect;
};

} // namespace Gentl

#endif
