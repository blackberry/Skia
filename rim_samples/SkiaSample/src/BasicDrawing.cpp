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

#include "BasicDrawing.h"
#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkTypeface.h"

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

    // Read in a bitmap from a png file
    bitmap = new SkBitmap();
    SkImageDecoder::DecodeFile("app/native/icon.png", bitmap);
}

BasicDrawing::~BasicDrawing()
{
    delete bitmap; //FIXME ???
}

void BasicDrawing::draw()
{
    // Save state
    canvas->save();

    canvas->translate(50, 20);
    canvas->save();

    // Draw a rectangle
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

    // Move to the next row
    canvas->restore();
    canvas->translate(0, vSpacing + rowHeight);
    canvas->save();

    // Draw some text on a path
    canvas->drawTextOnPath(textStrings[11], strlen(textStrings[11]), cubicPath, NULL, shapePaint);
    canvas->drawText(textStrings[9], strlen(textStrings[9]), pathBounds.width() / 2, rowHeight + 25, textPaint);
    canvas->translate(hSpacing + pathBounds.width(), 0);

    // Draw a bitmap
    if (bitmap) {
        canvas->drawBitmap(*bitmap, 0, 0, NULL);
        canvas->drawText(textStrings[10], strlen(textStrings[10]), bitmap->width() / 2, rowHeight + 25, textPaint);
    }

    canvas->restore();

    // Restore state
    canvas->restore();
}
