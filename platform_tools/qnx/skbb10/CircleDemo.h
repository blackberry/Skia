/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CIRCLEDEMO_H_
#define CIRCLEDEMO_H_

#include "DemoBase.h"

class CircleDemo: public DemoBase {
public:
    CircleDemo(SkCanvas *c, int w, int h);
    ~CircleDemo() {}

    virtual void draw();
private:
//    void circle(SkCanvas* canvas, int width, bool aa);
//    void drawSix(SkCanvas* canvas, SkScalar dx, SkScalar dy);

    static const SkScalar ANIM_DX;
    static const SkScalar ANIM_DY;
    static const SkScalar ANIM_RAD;
    SkScalar fDX, fDY, fRAD;
};

#endif /* CIRCLEDEMO_H_ */
