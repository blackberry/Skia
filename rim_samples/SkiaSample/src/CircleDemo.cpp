/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

void CircleDemo::circle(SkCanvas* canvas, int width, bool aa) {
    SkPaint paint;

    paint.setAntiAlias(aa);
    if (width < 0) {
        paint.setStyle(SkPaint::kFill_Style);
    } else {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(width));
    }
    canvas->drawCircle(0, 0, SkIntToScalar(9) + fRAD, paint);
}

void CircleDemo::drawSix(SkCanvas* canvas, SkScalar dx, SkScalar dy) {
    for (int width = -1; width <= 1; width++) {
        canvas->save();
        circle(canvas, width, false);
        canvas->translate(0, dy);
        circle(canvas, width, true);
        canvas->restore();
        canvas->translate(dx, 0);
    }
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

static void rotate(SkCanvas* canvas, SkScalar angle, SkScalar px, SkScalar py) {
    canvas->translate(-px, -py);
    canvas->rotate(angle);
    canvas->translate(px, py);
}

void CircleDemo::draw()
{
    int sides = 3 + GetAnimScalar(SkIntToScalar(360)/128, SkIntToScalar(24));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorBLUE);
    for (int n = 220; n > 10; n -= 10) {
        if (sides > 19)
            canvas->drawCircle(750, 250, n, paint);
        else {
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
    SkMatrix matrix;
    matrix.setScale(SkIntToScalar(100), SkIntToScalar(100));
    matrix.postTranslate(SkIntToScalar(200), SkIntToScalar(200));
    canvas->save();
    canvas->concat(matrix);
    for (int n = 3; n < sides; n++) {
        if (n > 19) {
            SkAutoCanvasRestore acr(canvas, true);
            canvas->rotate(SkIntToScalar(10) * (n - 3));
            canvas->translate(-SK_Scalar1, 0);
            canvas->drawCircle(0, 0, 1, paint);
        } else {
            SkPath path;
            make_poly(&path, n);
            SkAutoCanvasRestore acr(canvas, true);
            canvas->rotate(SkIntToScalar(10) * (n - 3));
            canvas->translate(-SK_Scalar1, 0);
            canvas->drawPath(path, paint);
        }
    }
    canvas->restore();
}


