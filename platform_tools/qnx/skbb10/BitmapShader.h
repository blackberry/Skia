/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BITMAPSHADER_H_
#define BITMAPSHADER_H_

#include "DemoBase.h"

class SkShader;

class BitmapShader : public DemoBase {
public:
    BitmapShader(SkCanvas *c, int w, int h);
    ~BitmapShader();

    virtual void draw();

private:
    void drawWithShader(SkShader *shader, const char *label);

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;
    SkShader *clampShader;
    SkShader *tileShader;
    SkShader *mirrorShader;
    SkRect rect;
    SkBitmap *bitmap;

    static const char *tileModes[];
};

class ImageRotoBlitter : public DemoBase {
public:
    ImageRotoBlitter(SkCanvas *c, int w, int h);
    ~ImageRotoBlitter();

    virtual void draw();

private:
    enum { TileCount = 4, };
    SkBitmap tiles[TileCount];
    SkPaint imagePaint;
    SkRect src, dst;
};


#endif /* BITMAPSHADER_H_ */
