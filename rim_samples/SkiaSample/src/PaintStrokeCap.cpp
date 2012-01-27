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

#include "PaintStrokeCap.h"
#include "SkTypeface.h"

const char *PaintStrokeCap::shapeNames[] = {"Lines", "Open Path", "Closed Path"};
const char *PaintStrokeCap::paintStrokeCaps[] = {"Butt", "Round", "Square"};

PaintStrokeCap::PaintStrokeCap(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Paint Stroke Cap Demo";

    // Create paints
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setStyle(SkPaint::kStroke_Style);
    shapePaint.setStrokeWidth(20);
    shapePaint.setAntiAlias(true);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();
}

void PaintStrokeCap::drawHLabels() {
    // Save state
    canvas->save();

    // Position and draw text labels
    canvas->translate(90, 0); // FIXME
    for (unsigned int i=0; i < sizeof(shapeNames) / sizeof(shapeNames[0]); i++) {
        canvas->drawText(shapeNames[i], strlen(shapeNames[i]), colWidth / 2, 0, textPaint);
        canvas->translate(hSpacing + colWidth, 0);
    }

    // Restore state
    canvas->restore();
}

void PaintStrokeCap::drawVLabels() {
    // Save state
    canvas->save();

    // Save text alignment
    SkPaint::Align align = textPaint.getTextAlign();
    // Position and draw text labels
    canvas->translate(0, 22);
    textPaint.setTextAlign(SkPaint::kLeft_Align);
    for (unsigned int i=0; i < sizeof(paintStrokeCaps) / sizeof(paintStrokeCaps[0]); i++) {
        canvas->drawText(paintStrokeCaps[i], strlen(paintStrokeCaps[i]), 0, rowHeight / 2, textPaint);
        canvas->translate(0, vSpacing + rowHeight);
    }
    // Restore text alignment
    textPaint.setTextAlign(align);

    // Restore state
    canvas->restore();
}

void PaintStrokeCap::drawRow() {
    // Save the current transformation matrix
    canvas->save();

    // Draw some lines
    canvas->drawLine(0, 20, colWidth, 20, shapePaint);
    canvas->drawLine(colWidth, 20, 0, rowHeight, shapePaint);
    canvas->drawLine(0, rowHeight, colWidth, rowHeight, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Draw an open path
    SkPath linPath;
    linPath.moveTo(0, 20);
    linPath.rLineTo(colWidth, 0);
    linPath.rLineTo(-1*colWidth, 100);
    linPath.rLineTo(colWidth, 0);
    canvas->drawPath(linPath, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Close the path, and draw it again
    linPath.close();
    canvas->drawPath(linPath, shapePaint);
    canvas->translate(hSpacing + colWidth, 0);

    // Restore the transformation matrix we saved earlier
    canvas->restore();
}

void PaintStrokeCap::draw()
{
    // Save state
    canvas->save();

    // Draw the column and row labels
    canvas->translate(10, 20);
    drawHLabels();
    drawVLabels();
    canvas->translate(90, 20);

    canvas->save();

    // Set paint stroke cap to kButt_Cap and draw
    shapePaint.setStrokeCap(SkPaint::kButt_Cap);
    drawRow();
    canvas->translate(0, vSpacing + rowHeight);

    //Set paint stroke cap to kRound_Cap and draw
    shapePaint.setStrokeCap(SkPaint::kRound_Cap);
    drawRow();
    canvas->translate(0, vSpacing + rowHeight);

    // Set paint stroke cap to kSquare_Cap and draw
    shapePaint.setStrokeCap(SkPaint::kSquare_Cap);
    drawRow();

    canvas->restore();

    // Restore state
    canvas->restore();
}
