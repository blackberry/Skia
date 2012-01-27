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

#include "PathEffects.h"
#include "SkTypeface.h"
#include "SkCornerPathEffect.h"
#include "SkDashPathEffect.h"
#include "SkDiscretePathEffect.h"

const char *PathEffects::effects[] = {"Dashed Effect", "Corner Effect", "Discrete Effect"};

// Generate a star path with the specified number of arms, with center at (centerX, centerY),
// small radius r and large radius R
static void createStar(SkPath &path, int arms, SkScalar centerX, SkScalar centerY, SkScalar r, SkScalar R)
{
    SkScalar angleIncrement = SK_ScalarPI / SkIntToScalar(arms);
    SkScalar currentAngle = SK_ScalarPI / SkIntToScalar(2);
    SkPoint p;

    for (int i = 0; i < 2 * arms; i++)
    {
        SkScalar radius = (i % 2) == 0 ? r : R;
        p.set(centerX + cos(currentAngle) * radius, centerY + sin(currentAngle) * radius);
        if (i == 0)
            path.moveTo(p.x(), p.y());
        else
            path.lineTo(p.x(), p.y());

        currentAngle += angleIncrement;
    }
    path.close();
}

PathEffects::PathEffects(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Path Effects Demo";

    // Set up paints
    shapePaint.setColor(SK_ColorBLUE);
    shapePaint.setStrokeWidth(4);
    shapePaint.setAntiAlias(true);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up star path
    createStar(starPath, 7, colWidth/2, colWidth/2, 80, colWidth/2);

    // Create a dash path effect
    SkScalar interval[2];
    interval[0] = 8;
    interval[1] = 4;
    dashEffect = new SkDashPathEffect(interval, 2, 0);

    // Create a corner path effect
    cornerEffect = new SkCornerPathEffect(15);

    // Create a discrete path effect
    discreteEffect = new SkDiscretePathEffect(10, 5);
}

PathEffects::~PathEffects()
{
    // Clean up effects
    dashEffect->unref();;
    cornerEffect->unref();;
    discreteEffect->unref();;
}

void PathEffects::drawPath(SkPathEffect *effect, const char *label) {
    // Draw label
    canvas->drawText(label, strlen(label), colWidth/2, colWidth + 20, textPaint);

    // Draw path with effect
    shapePaint.setPathEffect(effect);
    canvas->drawPath(starPath, shapePaint);
    shapePaint.setPathEffect(NULL);
}

void PathEffects::draw()
{
    // Save state
    canvas->save();
    canvas->translate(30, 40);

    // Draw path using the three effects
    shapePaint.setStyle(SkPaint::kFill_Style);
    drawPath(cornerEffect, effects[1]);
    canvas->translate(colWidth + hSpacing, 0);

    shapePaint.setStyle(SkPaint::kStroke_Style);
    drawPath(dashEffect, effects[0]);
    canvas->translate(colWidth + hSpacing, 0);

    shapePaint.setStyle(SkPaint::kFill_Style);
    drawPath(discreteEffect, effects[2]);

    // Restore state
    canvas->restore();
}
