/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "BigBlur.h"

#include "SkBlurMask.h"
#include "SkBlurMaskFilter.h"

BigBlur::BigBlur(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Big Blur";
}

BigBlur::~BigBlur()
{ }

void BigBlur::draw()
{
    SkPaint paint;
    canvas->save();
    paint.setColor(SK_ColorBLUE);
    paint.setAlpha(128);
    SkMaskFilter* mf = SkBlurMaskFilter::Create(
        SkBlurMaskFilter::kNormal_BlurStyle,
        SkBlurMask::ConvertRadiusToSigma(SkIntToScalar(128)),
        SkBlurMaskFilter::kHighQuality_BlurFlag);
    paint.setMaskFilter(mf)->unref();
    canvas->translate(200, 200);
    canvas->drawCircle(100, 100, 200, paint);
    canvas->restore();
}
