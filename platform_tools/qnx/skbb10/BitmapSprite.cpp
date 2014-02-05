/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "BitmapSprite.h"

#include "SkBitmap.h"
#include "SkGradientShader.h"
#include "SkImageDecoder.h"
#include <stdio.h>


bool BitmapSprite::decode(SkBitmap* bitmap) {
    char path[4000];
    sprintf(path, "%s/../app/native/assets/sprite.png", getenv("HOME"));
    fDecoded = SkImageDecoder::DecodeFile(path, bitmap);
    return fDecoded;
}

BitmapSprite::BitmapSprite(SkCanvas *c, int w, int h)
    :DemoBase(c, w, h)
    , fBitmap(new SkBitmap())
    , fDecoded(false)
{
    SkPoint pts[] = {SkPoint::Make(10,28), SkPoint::Make(10,48)};
    SkColor colors[] = {SkColorSetARGBInline(0xFF,0xF8,0xF8,0xF8), SkColorSetARGBInline(0xFF,0xDD,0xDD,0xDD)};
    fShaders[0] = SkGradientShader::CreateLinear(pts, colors, NULL, 2, SkShader::kClamp_TileMode);

    if (!decode(fBitmap))
        return;
    fShaders[1] = SkShader::CreateBitmapShader(*fBitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    SkMatrix mat;
    mat.setIdentity();
    mat.setTranslateY(-338);
    fShaders[1]->setLocalMatrix(mat);

    fShaders[2] = SkShader::CreateBitmapShader(*fBitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    SkMatrix mat2;
    mat2.setIdentity();
    fShaders[2]->setLocalMatrix(mat2);

}

BitmapSprite::~BitmapSprite()
{
    delete fBitmap;
    for (int i=0; i<3; i++) {
        fShaders[i]->unref();
    }
}

void BitmapSprite::draw()
{

    SkPaint paint3;
    paint3.setStyle(SkPaint::kFill_Style);
    paint3.setColor(SkColorSetARGBInline(0xFF,0x00,0x00,0x00));
    paint3.setShader(fShaders[0]);
    SkRRect rounded;
    SkVector radii[] = {SkVector::Make(1,1), SkVector::Make(1,1), SkVector::Make(1,1), SkVector::Make(1,1)};
    rounded.setRectRadii(SkRect::MakeLTRB(10,28,55,49), radii);
    canvas->drawRRect(rounded, paint3);

    if (!fDecoded && !decode(fBitmap))
        return;
    canvas->save();

    SkRect repeatRect = SkRect::MakeXYWH(50, 50, 450, 150);
    SkPaint paint1;
    paint1.setShader(fShaders[1]);
    canvas->drawRect(repeatRect, paint1);

    SkRect repeatRect2 = SkRect::MakeXYWH(58, 338, 42, 30);
    SkPaint paint2;
    paint2.setShader(fShaders[2]);
    canvas->drawRect(repeatRect2, paint2);

    canvas->restore();

}
