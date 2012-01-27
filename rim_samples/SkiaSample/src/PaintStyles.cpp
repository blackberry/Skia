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

#include "PaintStyles.h"
#include "SkTypeface.h"

const char *PaintStyles::shapeNames[] = {"Rectangle", /*"Oval",*/ "Arc", "Path", "Lines"};
const char *PaintStyles::paintStyles[] = {"Stroke", "Fill", "Both"};

PaintStyles::PaintStyles(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Paint Styles Demo";

    // Set up a paint for drawing shapes
    shapePaint.setAntiAlias(true);

    // Set up another paint to be used for drawing text labels
    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up a rectangle
    rect.set(0, 0, colWidth, rowHeight);

    // Set up a path
    linPath.moveTo(0, rowHeight);
    linPath.rLineTo(colWidth, 0);
    linPath.rLineTo(-1*colWidth, -100);
    linPath.close();
}

void PaintStyles::drawHLabels() {
    // Save state
    canvas->save();

    // Position and draw text labels
    canvas->translate(90, 0);
    for (unsigned int i=0; i < sizeof(shapeNames) / sizeof(shapeNames[0]); i++) {
        canvas->drawText(shapeNames[i], strlen(shapeNames[i]), colWidth / 2, 0, textPaint);
        canvas->translate(hSpacing + colWidth, 0);
    }

    // Restore state
    canvas->restore();
}

void PaintStyles::drawVLabels() {
    // Save state
    canvas->save();

    // Save text alignment
    SkPaint::Align align = textPaint.getTextAlign();
    // Position and draw text labels
    canvas->translate(0, 22);
    textPaint.setTextAlign(SkPaint::kLeft_Align);
    for (unsigned int i=0; i < sizeof(paintStyles) / sizeof(paintStyles[0]); i++) {
        canvas->drawText(paintStyles[i], strlen(paintStyles[i]), 0, rowHeight / 2, textPaint);
        canvas->translate(0, vSpacing + rowHeight);
    }
    // Restore text alignment
    textPaint.setTextAlign(align);

    // Restore state
    canvas->restore();
}

void PaintStyles::drawRow() {
    // Save state
    canvas->save();

    // Draw a few shapes
    canvas->drawRect(rect, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    canvas->drawArc(rect, -40, -280, true, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    canvas->drawPath(linPath, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    canvas->drawLine(0, 0, 0, rowHeight, shapePaint);
    canvas->drawLine(0, rowHeight, colWidth, rowHeight, shapePaint);
    canvas->drawLine(colWidth, rowHeight, 0, 0, shapePaint);

    // Restore state
    canvas->restore();
}

void PaintStyles::draw()
{
    // Save state
    canvas->save();

    // Draw the column and row labels
    canvas->translate(10, 20);
    drawHLabels();
    drawVLabels();
    canvas->translate(90, 20);

    // Set paint style to kStroke_Style and draw
    shapePaint.setColor(SK_ColorRED);
    shapePaint.setStyle(SkPaint::kStroke_Style);
    shapePaint.setStrokeWidth(5);
    drawRow();
    canvas->translate(0, vSpacing + rowHeight);

    // Set paint style to kFill_Style and draw
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setStyle(SkPaint::kFill_Style);
    drawRow();
    canvas->translate(0, vSpacing + rowHeight);

    // Combine the two paint styles together
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setStyle(SkPaint::kFill_Style);
    drawRow();
    shapePaint.setColor(SK_ColorRED);
    shapePaint.setStyle(SkPaint::kStroke_Style);
    drawRow();

    // Restore state
    canvas->restore();

}
