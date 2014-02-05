/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleCircle.cpp

#include "CircleDemo.h"
const SkScalar CircleDemo::ANIM_DX(SK_Scalar1 / 67);
const SkScalar CircleDemo::ANIM_DY(SK_Scalar1 / 29);
const SkScalar CircleDemo::ANIM_RAD(SK_Scalar1 / 19);

CircleDemo::CircleDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Circle Demo";
    fDX = fDY = fRAD = 0;
}

static void make_poly(SkPath* path, int n) {
    if (n <= 0) {
        return;
    }
    path->incReserve(n + 1);
    path->moveTo(SK_Scalar1, 0);
    SkScalar step = SK_ScalarPI * 2 / n;
    SkScalar angle = 0;
    for (int i = 1; i < n; i++) {
        angle += step;
        SkScalar s = sin(angle);
        SkScalar c = cos(angle);
        path->lineTo(c, s);
    }
    path->close();
}

void CircleDemo::draw()
{
    int sides = 3 + GetAnimScalar(SkIntToScalar(360)/128, SkIntToScalar(24));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorBLUE);
    paint.setStrokeWidth(0.02);
    for (int n = 220; n > 10; n -= 10) {
        if (sides > 19) {
            paint.setColor( SK_ColorRED);
            canvas->drawCircle(750, 250, n, paint);
        } else {
            SkMatrix matrix;
            matrix.setScale(SkIntToScalar(n), SkIntToScalar(n));
            matrix.postTranslate(SkIntToScalar(750), SkIntToScalar(250));
            canvas->save();
            canvas->concat(matrix);
            SkPath path;
            make_poly(&path, sides);
            canvas->drawPath(path, paint);
            canvas->restore();
        }
    }
    paint.setColor(SK_ColorBLUE);
    SkMatrix matrix;
    matrix.setScale(SkIntToScalar(100), SkIntToScalar(100));
    matrix.postTranslate(SkIntToScalar(200), SkIntToScalar(200));
    canvas->save();
    canvas->concat(matrix);
    for (int n = 3; n < sides; n++) {
        SkAutoCanvasRestore acr(canvas, true);
        canvas->rotate(SkIntToScalar(10) * (n - 3));
        canvas->translate(-SK_Scalar1, 0);
        if (n > 19) {
            canvas->drawCircle(0, 0, 1, paint);
        } else {
            SkPath path;
            make_poly(&path, n);
            canvas->drawPath(path, paint);
        }
    }
    canvas->restore();
}


