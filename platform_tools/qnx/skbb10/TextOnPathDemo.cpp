/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleTextOnPath.cpp

#include "TextOnPathDemo.h"
#include "SkTypeface.h"
#include "SkPathMeasure.h"

TextOnPathDemo::TextOnPathDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Text on Path Demo";
    SkRect r;
    r.set(SkIntToScalar(100), SkIntToScalar(100),
          SkIntToScalar(300), SkIntToScalar(300));
    fPath.addOval(r);
    fPath.offset(SkIntToScalar(200), 0);

    fHOffset = SkIntToScalar(50);
}

TextOnPathDemo::~TextOnPathDemo()
{
}

static SkScalar getpathlen(const SkPath& path) {
    SkPathMeasure   meas(path, false);
    return meas.getLength();
}

static void test_textpathmatrix(SkCanvas* canvas, int yOffset) {
    SkPaint paint;
    SkPath  path;
    SkMatrix matrix;

    path.moveTo(SkIntToScalar(200), SkIntToScalar(100));
    path.quadTo(SkIntToScalar(400), SkIntToScalar(-100),
                SkIntToScalar(600), SkIntToScalar(100+yOffset));

    paint.setAntiAlias(true);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setTextSize(SkIntToScalar(48));
    paint.setTextAlign(SkPaint::kRight_Align);
    paint.setAntiAlias(true);

    const char* text = "Reflection";
    size_t      len = strlen(text);
    SkScalar    pathLen = getpathlen(path);

    canvas->drawTextOnPath(text, len, path, NULL, paint);

    paint.setColor(SK_ColorRED);
    matrix.setScale(-SK_Scalar1, SK_Scalar1);
    matrix.postTranslate(pathLen, 0);
    canvas->drawTextOnPath(text, len, path, &matrix, paint);

    paint.setColor(SK_ColorBLUE);
    matrix.setScale(SK_Scalar1, -SK_Scalar1);
    canvas->drawTextOnPath(text, len, path, &matrix, paint);

    paint.setColor(SK_ColorGREEN);
    matrix.setScale(-SK_Scalar1, -SK_Scalar1);
    matrix.postTranslate(pathLen, 0);
    canvas->drawTextOnPath(text, len, path, &matrix, paint);
    SkPaint yellowPaint;
    yellowPaint.setColor(SK_ColorYELLOW);
    yellowPaint.setStrokeWidth(1);
    yellowPaint.setStyle(SkPaint::kStroke_Style);
    canvas->drawPath(path, yellowPaint);
}

void TextOnPathDemo::draw()
{
    canvas->save();

    canvas->translate(0, SkIntToScalar(100));
    test_textpathmatrix(canvas,
            GetAnimScalar(SkIntToScalar(360)/24, SkIntToScalar(200)));
    canvas->restore();
}

