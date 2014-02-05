/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Picture.h"

#include "SkForceLinking.h"

__SK_FORCE_IMAGE_DECODER_LINKING;

Picture::Picture(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
    , picture(0)
{
    SkDebugf("linking_forced:%d", linking_forced);
    name = "SkPicture Demo";
}

bool Picture::lazyInit() {
    if (!picture) {
        char path[4000];
        sprintf(path, "%s/../app/native/assets/layer_0.skp", getenv("HOME"));
        picture = SkPicture::CreateFromStream(new SkFILEStream(path));
    }
    return picture;
}

void Picture::draw()
{
    if (!lazyInit()) {
        return;
    }
    // Save state
    canvas->save();

    picture->draw(canvas);

    // Restore state
    canvas->restore();
}
