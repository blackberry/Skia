/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleArc.cpp

#include "ArcDemo.h"
#include <sstream>
#include <string>

static unsigned s_bufferSize = 32;

ArcDemo::ArcDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Arc Demo";
}

static void drawRectWithLines(SkCanvas* canvas, const SkRect& r, const SkPaint& p) {
    canvas->drawRect(r, p);
    canvas->drawLine(r.fLeft, r.fTop, r.fRight, r.fBottom, p);
    canvas->drawLine(r.fLeft, r.fBottom, r.fRight, r.fTop, p);
    canvas->drawLine(r.fLeft, r.centerY(), r.fRight, r.centerY(), p);
    canvas->drawLine(r.centerX(), r.fTop, r.centerX(), r.fBottom, p);
}

static void draw_label(SkCanvas* canvas, const SkRect& rect,
                        int start, int sweep) {
    SkPaint paint;

    paint.setAntiAlias(true);
    paint.setTextAlign(SkPaint::kCenter_Align);

    char message[s_bufferSize];
    int len = snprintf(message, s_bufferSize, "%d-%d", start, sweep);
    canvas->drawText(message, len, rect.centerX(), rect.fBottom + paint.getTextSize() * 5/4, paint);
}

static void drawArcs(SkCanvas* canvas) {
    SkPaint paint;
    SkRect  r;
    SkScalar w = SkIntToScalar(75);
    SkScalar h = SkIntToScalar(50);

    r.set(0, 0, w, h);
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);

    canvas->save();
    canvas->translate(SkIntToScalar(10), SkIntToScalar(300));

    paint.setStrokeWidth(SkIntToScalar(1));

    static const int gAngles[] = {
        0, 360,
        0, 45,
        0, -45,
        720, 135,
        -90, 269,
        -90, 270,
        -90, 271,
        -180, -270,
        225, 90
    };

    for (size_t i = 0; i < SK_ARRAY_COUNT(gAngles); i += 2) {
        paint.setColor(SK_ColorBLACK);
        drawRectWithLines(canvas, r, paint);

        paint.setColor(SK_ColorRED);
        canvas->drawArc(r, SkIntToScalar(gAngles[i]),
                        SkIntToScalar(gAngles[i+1]), false, paint);

        draw_label(canvas, r, gAngles[i], gAngles[i+1]);

        canvas->translate(w * 8 / 7, 0);
    }

    canvas->restore();
}

void ArcDemo::draw()
{
    SkScalar fSweep = GetAnimScalar(SkIntToScalar(360)/24,
                                               SkIntToScalar(360));
    SkRect  r;
    SkPaint paint;

    paint.setAntiAlias(true);
    paint.setStrokeWidth(SkIntToScalar(2));
    paint.setStyle(SkPaint::kStroke_Style);

    r.set(0, 0, SkIntToScalar(200), SkIntToScalar(200));
    r.offset(SkIntToScalar(20), SkIntToScalar(20));

    if (false) {
        const SkScalar d = SkIntToScalar(3);
        const SkScalar rad[] = { d, d, d, d, d, d, d, d };
        SkPath path;
        path.addRoundRect(r, rad);
        canvas->drawPath(path, paint);
        return;
    }

    drawRectWithLines(canvas, r, paint);

    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0x800000FF);
    canvas->drawArc(r, 0, fSweep, true, paint);

    paint.setColor(0x800FF000);
    canvas->drawArc(r, 0, fSweep, false, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorRED);
    canvas->drawArc(r, 0, fSweep, true, paint);

    paint.setStrokeWidth(0);
    paint.setColor(SK_ColorBLUE);
    canvas->drawArc(r, 0, fSweep, false, paint);

    drawArcs(canvas);
}
