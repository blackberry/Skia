/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BITMAPSPRITE_H_
#define BITMAPSPRITE_H_

#include "DemoBase.h"

class SkShader;
class SkCanvas;

class BitmapSprite: public DemoBase {
public:
    BitmapSprite(SkCanvas *c, int w, int h);
    virtual ~BitmapSprite();

    virtual void draw();
private:
    bool decode(SkBitmap*);
    SkBitmap* fBitmap;
    bool fDecoded;
    SkShader* fShaders[3];
};

#endif /* BITMAPSPRITE_H_ */
