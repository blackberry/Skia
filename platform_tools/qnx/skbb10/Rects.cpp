/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleBox.cpp

#include "Rects.h"

RectSpamDemo::RectSpamDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
    , boxW(w / (boxSize + padding))
    , maxBoxes(boxW * ((h - 80) / (boxSize + padding)))
{
    name = "Rect Spam Demo";

    paintSolid.setAntiAlias(true);
    paintSolid.setStyle(SkPaint::kFill_Style);
    paintSolid.setColor(SK_ColorBLUE);
    paintTrans.setAntiAlias(true);
    paintTrans.setStyle(SkPaint::kFill_Style);
    paintTrans.setColor(SK_ColorBLUE);
    paintTrans.setAlpha(127);
}

void RectSpamDemo::draw()
{
    SkRect r = SkRect::MakeWH(boxSize, boxSize);
    SkRect r2 = SkRect::MakeXYWH(boxSize + padding, 0, boxSize, boxSize);

    canvas->save();

    for (int i = 0; i < maxBoxes - 1; i += 2) {
        canvas->translate(padding + (padding + boxSize) * (i % boxW), padding + (padding + boxSize) * (i / boxW));

        canvas->drawRect(r, paintSolid);
        canvas->drawRect(r2, paintTrans);

        canvas->translate(-(padding + (padding + boxSize) * (i % boxW)), -(padding + (padding + boxSize) * (i / boxW)));
    }

    canvas->restore();
}
