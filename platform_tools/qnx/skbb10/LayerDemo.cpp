/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleArc.cpp

#include "LayerDemo.h"
#include <sstream>
#include <string>

#include "SkGradientShader.h"

LayerDemo::LayerDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Layer Demo";
}

void LayerDemo::draw()
{
    SkPaint layer;
    layer.setAlpha(0x40);

    SkPaint green, blue, bg;
    green.setColor(SK_ColorGREEN);
    blue.setColor(SK_ColorBLUE);
    {
        SkPoint pts[2];
        pts[0].set(0, 0);
        pts[1].set(50, 50);
        SkColor colors[2];
        colors[0] = 0xFFDCDCDC;
        colors[1] = 0xFFAAAAAA;
        bg.setShader(SkGradientShader::CreateLinear(pts, colors, NULL, 2, SkShader::kMirror_TileMode));
    }

    SkRect big = SkRect::MakeLTRB(320, 240, 480, 480);

    canvas->save();
    canvas->translate(200, 0);

    canvas->drawPaint(bg);

    canvas->saveLayer(&big, &layer);
    canvas->drawRect(big, green);

    SkRect small = SkRect::MakeLTRB(360, 280, 460, 520);
    canvas->saveLayer(&small, &layer);
    canvas->drawRect(small, blue);
    canvas->restore();

    canvas->restore();
    canvas->restore();

    SkPaint paint;
    paint.setColor(SK_ColorGREEN);
    paint.setAlpha(0x80);

    canvas->save();
    SkRect rect = SkRect::MakeXYWH(0, 0, 250, 250);
    for (int i = 0; i < 4; ++i) {
        canvas->translate(50, 20);
        canvas->drawOval(rect, paint);
    }
    canvas->restore();

    canvas->save();
    canvas->translate(0, 400);

    SkRect layerRect = SkRect::MakeXYWH(0, 0, 750, 750);
    canvas->saveLayer(&layerRect, &paint);

    paint.setAlpha(0xFF);

    for (int i = 0; i < 4; ++i) {
        canvas->translate(50, 20);
        canvas->drawOval(rect, paint);
    }
    canvas->restore();

    canvas->restore();
}
