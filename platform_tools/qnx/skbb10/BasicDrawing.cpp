/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "BasicDrawing.h"
#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkTypeface.h"
#include "SkDrawLooper.h"
#include "SkBlurDrawLooper.h"

const char *BasicDrawing::textStrings[] = {"Rectangle", "Rounded Rectangle", "Oval", "Circle",
                                            "Arc #1", "Arc #2", "Linear path", "Cubic path",
                                            "Lines", "Text on path", "Bitmap",
                                            "This is some text drawn along the cubic path we created earlier. So cool!"};

BasicDrawing::BasicDrawing(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Basic Drawing Demo";

    // Set up a simple blue Paint which we will use for drawing all of the shapes
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setAntiAlias(true);

    linePaint.setColor(SK_ColorBLUE);
    linePaint.setStrokeWidth(1);
    linePaint.setStyle(SkPaint::kStroke_Style);

    // Set up another paint to be used for drawing text labels
    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setAntiAlias(true);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up a rectangle which we will use for drawing a number of basic shapes
    rect.set(0, 0, colWidth, rowHeight);

    // Set up a simple path forming a triangle
    linPath.moveTo(0, rowHeight);
    linPath.rLineTo(150, 0);
    linPath.rLineTo(-150, -100);
    linPath.close();

    // Set up a path using bezier segments in the shape of a heart
    cubicPath.moveTo(61, 36);
    cubicPath.cubicTo(52, -2, 1, -1, 0, 46);
    cubicPath.cubicTo(4, 71, 11, 102, 69, 120);
    cubicPath.cubicTo(110, 102, 118, 71, 121, 46);
    cubicPath.cubicTo(120, -1, 69, -2, 61, 36);
    cubicPath.close();

    bitmap.setConfig(SkBitmap::kARGB_8888_Config, 128, 128);
    bitmap.allocPixels();
    SkCanvas tempCanvas(bitmap);
    SkPaint bitmapPaint;
    bitmapPaint.setColor(SK_ColorRED);
    tempCanvas.drawOval(SkRect::MakeWH(128, 128), bitmapPaint);
    bitmapPaint.setColor(SK_ColorBLUE);
    tempCanvas.drawRoundRect(SkRect::MakeXYWH(32, 32, 64, 64), 24, 12, bitmapPaint);
}

BasicDrawing::~BasicDrawing()
{
}

void BasicDrawing::draw()
{
    SkBlurDrawLooper blurLooper(8, 8, 8, SK_ColorRED, SkBlurDrawLooper::kAll_BlurFlag);

    // Save state
    canvas->save();

    canvas->translate(50, 20);
    canvas->save();

    // Draw a rectangle
    shapePaint.setLooper(&blurLooper);
    canvas->drawRect(rect, shapePaint);

    canvas->drawText(textStrings[0], strlen(textStrings[0]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw a rounded rectangle
    canvas->drawRoundRect(rect, 15, 15, shapePaint);
    canvas->drawText(textStrings[1], strlen(textStrings[1]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw an oval enclosed in the rectangle we created
    canvas->drawOval(rect, shapePaint);
    canvas->drawText(textStrings[2], strlen(textStrings[2]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw a circle such that it aligns with the vertical midpoint of the rectangle
    SkScalar radius = rowHeight / 2;
    canvas->drawCircle(radius, radius, radius, shapePaint);
    canvas->drawText(textStrings[3], strlen(textStrings[3]), radius, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    shapePaint.setLooper(0);

    canvas->drawLine(colWidth, 0, colWidth, rowHeight, linePaint);
    canvas->drawLine(0, 0, colWidth, rowHeight, linePaint);
    canvas->drawLine(0, rowHeight, colWidth, 0, linePaint);
    canvas->drawLine(colWidth / 2, 0, colWidth / 2, rowHeight, linePaint);
    canvas->drawText(textStrings[8], strlen(textStrings[8]), colWidth / 2, rowHeight + 25, textPaint);

    // Move to the next row
    canvas->restore();
    canvas->translate(0, vSpacing + rowHeight);
    canvas->save();

    // Draw an arc from 0 to 225 degrees, and don't include the center when filling
    canvas->drawArc(rect, 0, 225, false, shapePaint);
    canvas->drawText(textStrings[4], strlen(textStrings[4]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw the same arc, but this time do include the center when filling
    canvas->drawArc(rect, 0, 225, true, shapePaint);
    canvas->drawText(textStrings[5], strlen(textStrings[5]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw a simple path
    canvas->drawPath(linPath, shapePaint);
    canvas->drawText(textStrings[6], strlen(textStrings[6]), colWidth / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw a more complicated path
    SkRect pathBounds = cubicPath.getBounds();
    canvas->drawPath(cubicPath, shapePaint);
    canvas->drawText(textStrings[7], strlen(textStrings[7]), pathBounds.width() / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + pathBounds.width(), 0);

    // Draw a few lines
    canvas->drawLine(colWidth, 0, colWidth, rowHeight, shapePaint);
    canvas->drawLine(0, 0, colWidth, rowHeight, shapePaint);
    canvas->drawLine(0, rowHeight, colWidth, 0, shapePaint);
    canvas->drawLine(colWidth / 2, 0, colWidth / 2, rowHeight, shapePaint);
    canvas->drawText(textStrings[8], strlen(textStrings[8]), colWidth / 2, rowHeight + 25, textPaint);

    shapePaint.setLooper(0);

    // Move to the next row
    canvas->restore();
    canvas->translate(0, vSpacing + rowHeight);
    canvas->save();

    // Draw some text on a path
    canvas->drawTextOnPath(textStrings[11], strlen(textStrings[11]), cubicPath, NULL, shapePaint);
    canvas->drawText(textStrings[9], strlen(textStrings[9]), pathBounds.width() / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + pathBounds.width(), 0);

    canvas->drawBitmapRect(bitmap, 0, SkRect::MakeWH(128, 128));
    canvas->drawText(textStrings[10], strlen(textStrings[10]), bitmap.width() / 2, rowHeight + 25, textPaint);

    canvas->restore();

    // Restore state
    canvas->restore();
}
