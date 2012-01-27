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

// Adapted from Skia/samplecode/SampleArc.cpp

#include "ArcDemo.h"
#include <sstream>
#include <string>

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

    std::string str;
    std::stringstream ss(str);
    ss << start << ", " << sweep;
    canvas->drawText(str.c_str(), str.size(), rect.centerX(),
                     rect.fBottom + paint.getTextSize() * 5/4, paint);
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
