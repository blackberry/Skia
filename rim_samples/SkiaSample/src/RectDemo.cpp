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

// Adapted from Skia/samplecode/SampleBox.cpp

#include "RectDemo.h"

RectDemo::RectDemo(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Rectangle Demo";
}

void RectDemo::draw()
{
    int boxCount = (int)GetAnimScalar(SkIntToScalar(360)/24,SkIntToScalar(512));
    boxCount -= 256;
    boxCount = abs(boxCount);
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);

    SkRect  r;
    SkScalar x,y;
    x = 10;
    y = 10;
    const int increment = 4;

    canvas->save();
    r.set(x, y, x + SkIntToScalar(100+boxCount/2), y + SkIntToScalar(100));
    for (int i = 0; i < boxCount; i+=increment) {
        canvas->translate(increment, increment);
        paint.setColor(0xFF000000 + i * 0x00010000);
        canvas->drawRect(r, paint);
    }
    paint.setTextAlign(SkPaint::kCenter_Align);
    canvas->drawText("Filled Path", strlen("Filled Path"), 60+boxCount/4, 140, paint);
    canvas->restore();

    SkPaint paint2;
    paint2.setStyle(SkPaint::kStroke_Style);
    canvas->save();
    canvas->translate(256, 0);
    r.set(x, y, x + SkIntToScalar(100+boxCount/2), y + SkIntToScalar(100));
    for (int i = 0; i < boxCount; i+=increment) {
        canvas->translate(increment, increment);
        paint2.setColor(0xFF000000 + i * 0x00000100);
        canvas->drawRect(r, paint2);
    }
    paint2.setTextAlign(SkPaint::kCenter_Align);
    canvas->drawText("Stroked Path", strlen("Stroked Path"), 60+boxCount/4, 140, paint2);
    canvas->restore();

    SkPaint paint3;
    paint3.setStyle(SkPaint::kStrokeAndFill_Style);
    paint3.setStrokeJoin(SkPaint::kBevel_Join);
    paint3.setStrokeWidth(16);
    canvas->save();
    canvas->translate(512, 0);
    r.set(x+8, y+8, x-8 + SkIntToScalar(100+boxCount/2), y-8 + SkIntToScalar(100));
    for (int i = 0; i < boxCount; i+=increment) {
        canvas->translate(increment, increment);
        paint3.setColor(0xFF000000 + i * 0x00000001);
        canvas->drawRect(r, paint3);
    }
    paint3.setStrokeWidth(0);
    paint3.setTextAlign(SkPaint::kCenter_Align);
    canvas->drawText("Stroked and Filled Path", strlen("Stroked and Filled Path"), 60+boxCount/4, 140, paint3);
    canvas->restore();
}
