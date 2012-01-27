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

// Adapted from Skia/samplecode/SampleText.cpp

#include "TextDemo.h"
#include "SkTypeface.h"

static const char* text = "Hamburgefons";
static const size_t length = 12;

TextDemo::TextDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
    , maxLineCount(19)
{
    name = "Text Demo";
    pts = new SkPoint*[maxLineCount];

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    paint.setTypeface(pTypeface);
    paint.setAntiAlias(true);

    SkScalar y = SkIntToScalar(0);
    for (int i=0; i<=maxLineCount; i++) {
        paint.setTextSize(SkIntToScalar(i+9));
        y += paint.getFontSpacing();
        SkScalar xpos = SkIntToScalar(20);
        pts[i] = new SkPoint[length];
        for (size_t j=0; j < length; j++) {
            pts[i][j].set(xpos, y);
            xpos += paint.getTextSize();
        }
    }
}

TextDemo::~TextDemo()
{
    for (int i=0; i<maxLineCount; i++) {
        delete [] pts[i];
    }
    delete [] pts;
}

void TextDemo::DrawTheText(SkCanvas* canvas, SkScalar y, int index, const SkPaint& paint) {
    SkPaint p(paint);

    canvas->drawPosText(text, length, pts[index], paint);

    p.setSubpixelText(true);
    canvas->drawText(text, length, SkIntToScalar(380), y, p);
}

void TextDemo::draw()
{
    SkAutoCanvasRestore restore(canvas, false);
    int lineCount = GetAnimScalar(SkIntToScalar(360)/80,
                               SkIntToScalar(maxLineCount*2-1));
    if (lineCount > maxLineCount)
        lineCount = (maxLineCount*2-1) - lineCount;

    SkScalar y = SkIntToScalar(0);
    for (int i = 9; i <= lineCount+9; i++) {
        paint.setTextSize(SkIntToScalar(i));
        y += paint.getFontSpacing();
        DrawTheText(canvas, y, i-9, paint);
    }
}

