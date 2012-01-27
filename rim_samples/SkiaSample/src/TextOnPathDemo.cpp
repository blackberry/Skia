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

// Adapted from Skia/samplecode/SampleTextOnPath.cpp

#include "TextOnPathDemo.h"
#include "SkTypeface.h"
#include "SkPathMeasure.h"

static const char* text = "Hamburgefons";
static const size_t length = 12;

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
}

void TextOnPathDemo::draw()
{
    canvas->save();
    SkPaint paint;

    paint.setAntiAlias(true);
    paint.setTextSize(SkIntToScalar(50));

    paint.setColor(SK_ColorGREEN);
    paint.setStyle(SkPaint::kStroke_Style);

    canvas->translate(0, SkIntToScalar(100));
    test_textpathmatrix(canvas,
            GetAnimScalar(SkIntToScalar(360)/24, SkIntToScalar(200)));
    canvas->restore();
}

