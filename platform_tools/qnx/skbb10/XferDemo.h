/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef XFERDEMO_H_
#define XFERDEMO_H_

#include "DemoBase.h"

class XferDemo : public DemoBase {
public:
    XferDemo(SkCanvas *c, int w, int h);
    ~XferDemo() {}

    virtual void draw();
private:
    static const int numBox = 13;
    static const int boxSize = 90;
    static const int padding = 10;

    const int boxW;
    SkColor underColors[numBox];
    SkColor overColors[numBox];
    SkColor expectedColors[numBox];

    SkPaint paint;
    SkPaint textPaint;
};

#endif /* XFERDEMO_H_ */
