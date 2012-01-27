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

#include "DemoBase.h"
#include "BasicDrawing.h"
#include "PaintStyles.h"
#include "PaintStrokeCap.h"
#include "PaintStrokeJoin.h"
#include "PathFillType.h"
#include "PathEffects.h"
#include "BitmapShader.h"
#include "GradientShaders.h"
#include "ArcDemo.h"
#include "CircleDemo.h"
#include "RectDemo.h"
#include "TextOnPathDemo.h"
#include "TextDemo.h"
#include "SkTypeface.h"
#include <time.h>

DemoBase::DemoBase(SkCanvas *c, int w, int h)
    : canvas(c)
    , width(w)
    , height(h)
{
    // Set up paints
    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    labelPaint.setTypeface(pTypeface);
    labelPaint.setColor(SK_ColorGRAY);
    labelPaint.setTextSize(40);
    labelPaint.setAntiAlias(true);
    labelPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    arrowPaint.setStyle(SkPaint::kFill_Style);
    arrowPaint.setColor(SK_ColorGRAY);
}

SkScalar DemoBase::GetAnimScalar(SkScalar speed, SkScalar period) {
    // since gAnimTime can be up to 32 bits, we can't convert it to a float
    // or we'll lose the low bits. Hence we use doubles for the intermediate
    // calculations
    double seconds = (double)animTime / 1000.0;
    double value = SkScalarToDouble(speed) * seconds;
    if (period) {
        value = ::fmod(value, SkScalarToDouble(period));
    }
    return SkDoubleToScalar(value);
}

// convert the timespec into milliseconds
static unsigned long long currentTimeInMSec()
{
    struct timespec times;
    clock_gettime(CLOCK_REALTIME, &times);

    return times.tv_sec*1000 + times.tv_nsec/1000000;
}

void DemoBase::onDraw()
{
    prevAnimTime = animTime;
    animTime = currentTimeInMSec();

    draw();
    drawLabel();
    drawArrows();
}

void DemoBase::drawLabel()
{
    canvas->drawText(name, strlen(name), width/2, height-30, labelPaint);
}

void DemoBase::drawArrows()
{
    SkPath path;
    path.moveTo(10, height-40);
    path.lineTo(40, height-70);
    path.lineTo(40, height-10);
    path.lineTo(10, height-40);

    path.moveTo(width-10, height-40);
    path.lineTo(width-40, height-70);
    path.lineTo(width-40, height-10);
    path.lineTo(width-10, height-40);

    canvas->drawPath(path, arrowPaint);
}

const unsigned DemoFactory::m_demoCount = 13;

DemoBase **DemoFactory::createAllDemos(SkCanvas *canvas, int width, int height)
{
    DemoBase **demos = new DemoBase *[DemoFactory::demoCount()];
    for (int i=0; i<DemoFactory::demoCount(); i++)
        demos[i] = DemoFactory::createDemo(i, canvas, width, height);
    return demos;
}

DemoBase *DemoFactory::createDemo(int index, SkCanvas *canvas, int width, int height)
{
    switch (index) {
    case 0:
        return new BasicDrawing(canvas, width, height);
    case 1:
        return new PaintStyles(canvas, width, height);
    case 2:
        return new PaintStrokeCap(canvas, width, height);
    case 3:
        return new PaintStrokeJoin(canvas, width, height);
    case 4:
        return new PathFillType(canvas, width, height);
    case 5:
        return new PathEffects(canvas, width, height);
    case 6:
        return new BitmapShader(canvas, width, height);
    case 7:
        return new GradientShaders(canvas, width, height);
    case 8:
        return new ArcDemo(canvas, width, height);
    case 9:
        return new RectDemo(canvas, width, height);
    case 10:
        return new CircleDemo(canvas, width, height);
    case 11:
        return new TextOnPathDemo(canvas, width, height);
    case 12:
        return new TextDemo(canvas, width, height);
    default:
        fprintf(stderr, "Requested non-existent demo at index %d\n", index);
        return 0;
    }
}

void DemoFactory::deleteDemos(DemoBase **demos)
{
    for (int i=0; i<DemoFactory::demoCount(); i++)
        delete demos[i];
    delete [] demos;
}
