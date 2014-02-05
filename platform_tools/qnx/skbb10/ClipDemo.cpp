/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Adapted from Skia/samplecode/SampleBox.cpp

#include "ClipDemo.h"
#include "SkRRect.h"

ClipDemo::ClipDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Clipping Demo";
}

static void stress_clip_rect(SkCanvas* canvas, const SkRect& rect, const SkPaint& paint)
{
    for (int i = rect.left(); i < rect.right(); i += 4)
        canvas->drawRect(SkRect::MakeXYWH(i, rect.y(), 2, rect.height()), paint);
}

void ClipDemo::draw()
{
    SkPaint rectPaint;
    rectPaint.setAntiAlias(true);
    rectPaint.setStyle(SkPaint::kFill_Style);
    rectPaint.setColor(SK_ColorRED);

    SkPaint textPaint = rectPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(24);

    SkRect drawRect = SkRect::MakeXYWH(0, 0, 300, 300);
    SkRect clipRect = SkRect::MakeXYWH(80, 100, 140, 100);
    SkRect clipBigRect = SkRect::MakeXYWH(40, 50, 220, 200);
    SkRRect clipRRect;
    clipRRect.setRectXY(clipRect, 40, 40);
    const char* msg;

    canvas->save();

    canvas->save();
    canvas->clipRect(drawRect, SkRegion::kDifference_Op);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Completely Clipped out.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(0, 360);

    canvas->save();
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Unclipped.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(450, -360);

    canvas->save();
    canvas->clipRect(clipRect);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clipped to a small rect.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(0, 360);

    canvas->save();
    canvas->clipRect(clipRect, SkRegion::kDifference_Op);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clipped out a small rect.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(-450, 360);

    canvas->save();
    canvas->clipRect(clipBigRect);
    canvas->clipRect(clipRect, SkRegion::kDifference_Op);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clipped in and out.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(0, 360);

    canvas->save();
    canvas->translate(-60, -60);
    canvas->clipRect(clipRect, SkRegion::kDifference_Op);
    canvas->translate(120, 120);
    canvas->clipRect(clipRect, SkRegion::kDifference_Op);
    canvas->translate(-60, -60);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clipped out two rects.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(425, -360);

    canvas->save();
    canvas->clipRRect(clipRRect);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clip to a rounded rect.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->translate(0, 360);

    canvas->save();
    canvas->clipRRect(clipRRect, SkRegion::kDifference_Op);
    stress_clip_rect(canvas, drawRect, rectPaint);
    canvas->restore();
    msg = "Clipped out a rounded rect.";
    canvas->drawText(msg, strlen(msg), 20, 320, textPaint);

    canvas->restore();
}
