/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleBox.cpp

#include "XferDemo.h"

#include "SkTypeface.h"

XferDemo::XferDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
    , boxW(w / (boxSize + padding))
{
    name = "Xfer Demo";

    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(32);
    textPaint.setAntiAlias(true);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    unsigned i = 0;

    //0 clear
    underColors[i] = SK_ColorRED;
    overColors[i] = SK_ColorGREEN;
    expectedColors[i] = 0x00000000;
    ++i;

    //1 src
    underColors[i] = SK_ColorRED;
    overColors[i] = SK_ColorGREEN;
    expectedColors[i] = SK_ColorGREEN;
    ++i;

    //2 dst
    underColors[i] = SK_ColorGREEN;
    overColors[i] = SK_ColorRED;
    expectedColors[i] = SK_ColorGREEN;
    ++i;

    //3 src over
    underColors[i] = SK_ColorBLUE;
    overColors[i] = 0x7FFFFFFF & SK_ColorRED;
    expectedColors[i] = 0xFF7F007F;
    ++i;

    //4 dst over
    underColors[i] = 0x7FFFFFFF & SK_ColorBLUE;
    overColors[i] = SK_ColorRED;
    expectedColors[i] = 0xFF7F007F;
    ++i;

    //5 src in
    underColors[i] = SK_ColorRED;
    overColors[i] = 0x7FFFFFFF & SK_ColorGREEN;
    expectedColors[i] = 0xFF007F00;
    ++i;

    //6 dst in
    underColors[i] = 0x7FFFFFFF & SK_ColorGREEN;
    overColors[i] = SK_ColorRED;
    expectedColors[i] = 0xFF007F00;
    ++i;

    //7 src out
    underColors[i] = 0x7FFFFFFF & SK_ColorRED;
    overColors[i] = SK_ColorGREEN;
    expectedColors[i] = 0xFF007F00;
    ++i;

    //8 dst out
    underColors[i] = SK_ColorGREEN;
    overColors[i] = 0x7FFFFFFF & SK_ColorRED;
    expectedColors[i] = 0xFF007F00;
    ++i;

    //9 src atop
    underColors[i] = 0x7F7F0000;
    overColors[i] = 0x7FFFFFFF & SK_ColorGREEN;
    expectedColors[i] = 0x7F3F7F00;
    ++i;

    //10 dst atop
    underColors[i] = 0x7FFFFFFF & SK_ColorGREEN;
    overColors[i] = 0x7F7F0000;
    expectedColors[i] = 0x7F3F7F00;
    ++i;

    //xor
    underColors[i] = 0x7FFFFFFF & SK_ColorBLUE;
    overColors[i] = 0x7FFFFFFF & SK_ColorRED;
    expectedColors[i] = 0x7F7F007F;
    ++i;

    //plus
    underColors[i] = SK_ColorRED;
    overColors[i] = 0xFFFFFFFF & SK_ColorBLUE;
    expectedColors[i] = SK_ColorMAGENTA;
    ++i;
}

void XferDemo::draw()
{

    SkRect  r = SkRect::MakeWH(boxSize, boxSize);
    int expectedOffset = ((numBox + boxW - 1) / boxW + 1) * (r.height() + padding);
    canvas->save();


    for (int i = 0; i < numBox; ++i) {
        canvas->translate(padding + (padding + r.width()) * (i % boxW), padding + (padding + r.height()) * (i / boxW));

        paint.setXfermodeMode(SkXfermode::kSrc_Mode);
        paint.setColor(underColors[i]);
        canvas->drawRect(r, paint);

        canvas->translate(0, expectedOffset);
        paint.setColor(expectedColors[i]);
        canvas->drawRect(r, paint);
        canvas->translate(0, -expectedOffset);

        paint.setXfermodeMode(static_cast<SkXfermode::Mode>(i));
        paint.setColor(overColors[i]);
        canvas->drawRect(r, paint);


        canvas->translate(-(padding + (padding + r.width()) * (i % boxW)), -(padding + (padding + r.height()) * (i / boxW)));
    }

    canvas->drawText("Actual", strlen("Actual"), width / 2, expectedOffset - r.height() + textPaint.getTextSize(),textPaint);
    canvas->drawText("Expected", strlen("Expected"), width / 2, 2 * expectedOffset - r.height() + textPaint.getTextSize(), textPaint);

    canvas->restore();
}
