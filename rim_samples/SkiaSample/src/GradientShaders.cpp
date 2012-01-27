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

#include "GradientShaders.h"
#include "SkGradientShader.h"
#include "SkTypeface.h"

const char *GradientShaders::gradients[] = {"Linear", "Radial", "Two Point Radial", "Sweep"};
const char *GradientShaders::tileModes[] = {"Clamp", "Repeat", "Mirror"};

GradientShaders::GradientShaders(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Gradient Shaders Demo";

    diagonal = 0.5 * sqrt(colWidth*colWidth + rowHeight*rowHeight);

    // Set up outline and text paints
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(4);
    outlinePaint.setColor(SK_ColorBLACK);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up linear gradient shaders
    {
        SkPoint pts[2];
        pts[0].set(colWidth/3, rowHeight/2);
        pts[1].set(2*colWidth/3, rowHeight/2);
        SkColor colors[3];
        colors[0] = SK_ColorBLUE;
        colors[1] = SK_ColorWHITE;
        colors[2] = SK_ColorRED;
        linearShader[0] = SkGradientShader::CreateLinear(pts, colors, NULL, 3, SkShader::kClamp_TileMode);
        linearShader[1] = SkGradientShader::CreateLinear(pts, colors, NULL, 3, SkShader::kRepeat_TileMode);
        linearShader[2] = SkGradientShader::CreateLinear(pts, colors, NULL, 3, SkShader::kMirror_TileMode);
    }
    // Set up radial gradient shaders
    {
        SkPoint center;
        center.set(colWidth/2, rowHeight/2);
        SkColor colors[3];
        colors[0] = SK_ColorBLUE;
        colors[1] = SK_ColorWHITE;
        colors[2] = SK_ColorRED;
        radialShader[0] = SkGradientShader::CreateRadial(center, diagonal/3+1, colors, NULL, 3, SkShader::kClamp_TileMode);
        radialShader[1] = SkGradientShader::CreateRadial(center, diagonal/3+1, colors, NULL, 3, SkShader::kRepeat_TileMode);
        radialShader[2] = SkGradientShader::CreateRadial(center, diagonal/3+1, colors, NULL, 3, SkShader::kMirror_TileMode);
    }
    // Set up two point radial gradient shaders
    {
        SkPoint pts[2];
        pts[0].set(colWidth/4+10, rowHeight/4+10);
        pts[1].set(colWidth/2-10, rowHeight/2-10);
        SkColor colors[3];
        colors[0] = SK_ColorBLUE;
        colors[1] = SK_ColorWHITE;
        colors[2] = SK_ColorRED;
        radial2PtShader[0] = SkGradientShader::CreateTwoPointRadial(pts[0], 10, pts[1], diagonal/4+11, colors, NULL, 3, SkShader::kClamp_TileMode);
        radial2PtShader[1] = SkGradientShader::CreateTwoPointRadial(pts[0], 10, pts[1], diagonal/4+11, colors, NULL, 3, SkShader::kRepeat_TileMode);
        radial2PtShader[2] = SkGradientShader::CreateTwoPointRadial(pts[0], 10, pts[1], diagonal/4+11, colors, NULL, 3, SkShader::kMirror_TileMode);
    }
    // Set up sweep gradient shader
    {
        SkColor colors[3];
        colors[0] = SK_ColorBLUE;
        colors[1] = SK_ColorWHITE;
        colors[2] = SK_ColorRED;
        sweepShader = SkGradientShader::CreateSweep(colWidth/2, rowHeight/2, colors, NULL, 3);
    }
}

GradientShaders::~GradientShaders()
{
    // Clean up shaders
    for (int i=0; i<3; i++) {
        linearShader[i]->unref();
        radialShader[i]->unref();
        radial2PtShader[i]->unref();
    }
    sweepShader->unref();
}

void GradientShaders::drawHLabels() {
    // Save state
    canvas->save();

    // Position and draw text labels
    canvas->translate(90, 0);
    for (unsigned int i=0; i < sizeof(gradients) / sizeof(gradients[0]); i++) {
        canvas->drawText(gradients[i], strlen(gradients[i]), colWidth / 2, 0, textPaint);
        canvas->translate(hSpacing + colWidth, 0);
    }

    // Restore state
    canvas->restore();
}

void GradientShaders::drawVLabels() {
    // Save state
    canvas->save();
    canvas->translate(0, 22);

    // Position and draw text labels
    SkPaint::Align align = textPaint.getTextAlign();
    textPaint.setTextAlign(SkPaint::kLeft_Align);
    for (unsigned int i=0; i < sizeof(tileModes) / sizeof(tileModes[0]); i++) {
        canvas->drawText(tileModes[i], strlen(tileModes[i]), 0, rowHeight / 2, textPaint);
        canvas->translate(0, vSpacing + rowHeight);
    }
    textPaint.setTextAlign(align);

    // Restore state
    canvas->restore();
}

void GradientShaders::drawWithShader(SkShader *shader, const char *label, int size)
{
    if (!shader)
        return;

    // Set up shader paint
    shapePaint.setStyle(SkPaint::kFill_Style);
    shapePaint.setAntiAlias(true);
    shapePaint.setShader(shader);

    // Fill the rectangle using the shader and then draw its outline
    SkRect rect;
    rect.set(0, 0, size, size);
    canvas->drawRect(rect, shapePaint);
    canvas->drawRect(rect, outlinePaint);
}

void GradientShaders::drawRow(int index)
{
    // Save state
    canvas->save();

    // Draw using a linear shader
    drawWithShader(linearShader[index], "Linear", rowHeight);
    canvas->translate(colWidth + hSpacing, 0);

    // Draw using a radial shader
    drawWithShader(radialShader[index], "Radial", rowHeight);
    canvas->translate(colWidth + hSpacing, 0);

    // Draw using a two point radial shader
    drawWithShader(radial2PtShader[index], "Two Point Radial", rowHeight);

    // Tile modes are not applicable to a sweep gradient shader, so only draw it for the first row
    if (index == 0) {
        canvas->translate(colWidth + hSpacing, 0);
        drawWithShader(sweepShader, "Sweep", rowHeight);
    }

    // Restore state
    canvas->restore();
}

void GradientShaders::draw()
{
    // Save state
    canvas->save();

    canvas->translate(10, 20);
    drawHLabels();
    drawVLabels();

    // Draw row with kClamp_TileMode
    canvas->translate(90, 20);
    drawRow(0);

    // Draw row with kRepeat_TileMode
    canvas->translate(0, rowHeight + vSpacing);
    drawRow(1);

    // Draw row with kMirror_TileMode
    canvas->translate(0, rowHeight + vSpacing);
    drawRow(2);

    // Restore state
    canvas->restore();
}
