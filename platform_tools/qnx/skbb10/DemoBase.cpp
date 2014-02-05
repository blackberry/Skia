/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "DemoBase.h"
#include "BasicDrawing.h"
#include "PaintStyles.h"
#include "PaintStrokeCap.h"
#include "PaintStrokeJoin.h"
#include "PathFillType.h"
#include "PathEffects.h"
#include "BitmapShader.h"
#include "BitmapSprite.h"
#include "GradientShaders.h"
#include "ArcDemo.h"
#include "CircleDemo.h"
#include "ClipDemo.h"
#include "LayerDemo.h"
#include "RectDemo.h"
#include "TextOnPathDemo.h"
#include "TextDemo.h"
#include "Picture.h"
#include "XferDemo.h"
#include "Rects.h"
#include "SkTypeface.h"
#include "BigBlur.h"
#include "strokerects.cpp"
#include <time.h>

std::vector<DemoBase*> DemoFactory::s_demos;

DemoBase::DemoBase(SkCanvas *c, int w, int h)
    : canvas(c)
    , width(w)
    , height(h)
    , name("Untitled")
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

void DemoFactory::createDemos(SkCanvas *canvas, int width, int height)
{
    destroyDemos();

    s_demos.push_back(new StrokeRect(canvas, width, height));
    s_demos.push_back(new CircleDemo(canvas, width, height));
    s_demos.push_back(new LayerDemo(canvas, width, height));
    s_demos.push_back(new BasicDrawing(canvas, width, height));
    s_demos.push_back(new PaintStrokeCap(canvas, width, height));
    s_demos.push_back(new PaintStrokeJoin(canvas, width, height));
    s_demos.push_back(new PathFillType(canvas, width, height));
    s_demos.push_back(new PathEffects(canvas, width, height));
    s_demos.push_back(new BitmapShader(canvas, width, height));
    s_demos.push_back(new ImageRotoBlitter(canvas, width, height));
    s_demos.push_back(new GradientShaders(canvas, width, height));
    s_demos.push_back(new ArcDemo(canvas, width, height));
    s_demos.push_back(new RectDemo(canvas, width, height));
    s_demos.push_back(new ClipDemo(canvas, width, height));
    s_demos.push_back(new PaintStyles(canvas, width, height));
    s_demos.push_back(new XferDemo(canvas, width, height));
    s_demos.push_back(new RectSpamDemo(canvas, width, height));
    s_demos.push_back(new TextOnPathDemo(canvas, width, height));
    s_demos.push_back(new TextDemo(canvas, width, height));
    s_demos.push_back(new TextBench(canvas, width, height));
    s_demos.push_back(new BigBlur(canvas, width, height));
    s_demos.push_back(new Picture(canvas, width, height));
    s_demos.push_back(new BitmapSprite(canvas, width, height));
}

void DemoFactory::destroyDemos()
{
    std::vector<DemoBase*>::iterator it = s_demos.begin();
    std::vector<DemoBase*>::iterator last = s_demos.end();
    for (; it != last; ++it)
        delete (*it);

    s_demos.clear();
}

DemoBase* DemoFactory::getDemo(size_t index)
{
    return (index < s_demos.size()) ? s_demos[index] : 0;
}

size_t DemoFactory::demoCount()
{
    return s_demos.size();
}
