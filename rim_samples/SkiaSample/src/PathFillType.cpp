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

#include "PathFillType.h"
#include "SkTypeface.h"

PathFillType::PathFillType(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Path Fill Type Demo";

    // Set up paints
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setStyle(SkPaint::kFill_Style);
    shapePaint.setAntiAlias(true);

    outlinePaint.setColor(SK_ColorBLACK);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setAntiAlias(true);
    outlinePaint.setStrokeWidth(2);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kLeft_Align);
    pTypeface->unref();

    // Make a donut by combining two circles in with the same winding direction
    donutPath.addCircle(120, 120, 100, SkPath::kCW_Direction);
    donutPath.addCircle(120, 120, 40, SkPath::kCW_Direction);

    // Set up a clipping rect
    clipRect.set(0, 0, colWidth, rowHeight);
}

void PathFillType::drawPath(SkPath::FillType type, const char *label) {
    // Draw label
    canvas->drawText(label, strlen(label), 0, 130, textPaint);

    canvas->translate(120, 0);

    // Set the fill type
    donutPath.setFillType(type);

    // Save state before setting the clipping rect
    canvas->save();
    // Set a clipping rectangle and draw the path
    canvas->clipRect(clipRect);
    canvas->drawPath(donutPath, shapePaint);
    // Restore state
    canvas->restore();

    // Draw an outline of the clipping rect
    canvas->drawRect(clipRect, outlinePaint);
}

void PathFillType::draw()
{
    // Save state
    canvas->save();

    canvas->translate(20, 20);

    canvas->save();

    // Draw path with fill rule of kEvenOdd_FillType
    drawPath(SkPath::kEvenOdd_FillType, "EvenOdd");
    canvas->translate(hSpacing + colWidth, 0);

    // Draw path with fill rule of kInverseEvenOdd_FillType
    drawPath(SkPath::kInverseEvenOdd_FillType, "InvEvenOdd");

    // Go to next row
    canvas->restore();
    canvas->save();
    canvas->translate(0, vSpacing + rowHeight);

    // Draw path with fill rule of kWinding_FillType
    drawPath(SkPath::kWinding_FillType, "Winding");
    canvas->translate(hSpacing + colWidth, 0);

    // Draw path with fill rule of kInverseWinding_FillType
    drawPath(SkPath::kInverseWinding_FillType, "InvWinding");

    canvas->restore();

    // Restore state
    canvas->restore();
}
