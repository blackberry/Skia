/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleText.cpp

#include "TextDemo.h"
#include "SkTypeface.h"

#include <stdio.h>

static const char* text = "Hamburgefons";
static const size_t length = 12;

static const char* alphabet[] = { "AaBbCcDdEeFfGgHhIiJjKkLlMm",
                                  "NnOoPpQqRrSsTtUuVvWwXxYyZz",
                                  "1234567890.`~!@#$%^&*()-+=" };
static const size_t alphaLength = 26;

TextDemo::TextDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
    , maxLineCount(24)
{
    name = "Text Demo";
    pts = new SkPoint*[maxLineCount];

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    paint.setTypeface(pTypeface);
    paint.setAntiAlias(true);
    paint.setSubpixelText(true);

    SkScalar y = SkIntToScalar(0);
    for (int i=1; i<=maxLineCount; i++) {
        paint.setTextSize(SkIntToScalar(i * 3));
        y += paint.getFontSpacing();
        SkScalar xpos = SkIntToScalar(100);
        pts[i] = new SkPoint[7];
        for (size_t j=0; j < 7; j++) {
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
    char lineNumber[8];
    sprintf(lineNumber, "%d.", static_cast<int>(paint.getTextSize()));

    canvas->drawText(lineNumber, strlen(lineNumber), SkIntToScalar(0), y, paint);
    canvas->drawPosText(text, 7, pts[index], paint);
    canvas->drawText(text, length, SkIntToScalar(600), y, paint);
}

void TextDemo::draw()
{
    SkAutoCanvasRestore restore(canvas, false);

    paint.setStyle(SkPaint::kFill_Style);
    SkScalar y = SkIntToScalar(0);
    for (int i = 1; i <= maxLineCount; i++) {
        paint.setTextSize(SkIntToScalar(i * 3));
        y += paint.getFontSpacing();
        DrawTheText(canvas, y, i, paint);
    }
    for (int i = 0; i < 3; i++) {
        int textSize = i * 3 + 40 + maxLineCount;
        paint.setTextSize(SkIntToScalar(textSize));
        y += paint.getFontSpacing();
        canvas->drawText(alphabet[i], alphaLength, 20, y, paint);
    }

    paint.setStyle(SkPaint::kStroke_Style);
    for (int i = 0; i < 3; i++) {
        paint.setStrokeWidth(i * 2);
        paint.setTextSize(SkIntToScalar(i + 40 + maxLineCount));
        y += paint.getFontSpacing();
        canvas->drawText(alphabet[i], alphaLength, 20, y, paint);
    }

    paint.setStyle(SkPaint::kStrokeAndFill_Style);
    for (int i = 0; i < 3; i++) {
        paint.setStrokeWidth(i * 2);
        paint.setTextSize(SkIntToScalar(i + 40 + maxLineCount));
        y += paint.getFontSpacing();
        canvas->drawText(alphabet[i], alphaLength, 20, y, paint);
    }
}


TextBench::TextBench(SkCanvas* canvas, int width, int height)
    : DemoBase(canvas, width, height)
{
    name = "Wall of Hamburgefame";

    fText.set("Hamburgefons");

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    paint.setTypeface(pTypeface);
    paint.setAntiAlias(true);
    paint.setTextSize(27);
    paint.setColor(0xFF242424);
}

void TextBench::draw()
{
    const SkIPoint dim = SkIPoint::Make(width, height - 80);
    SkRandom rand;

    const SkScalar x0 = SkIntToScalar(-50);
    const SkScalar y0 = SkIntToScalar(-10);

    for (int i = 0; i < 800; i++) {
        SkScalar x = x0 + rand.nextUScalar1() * dim.fX;
        SkScalar y = y0 + rand.nextUScalar1() * dim.fY;
        canvas->drawText(fText.c_str(), fText.size(), x, y, paint);
    }
}
