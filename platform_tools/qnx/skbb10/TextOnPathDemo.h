/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TEXTONPATHDEMO_H_
#define TEXTONPATHDEMO_H_

#include "DemoBase.h"
#include "SkPaint.h"

class TextOnPathDemo: public DemoBase {
public:
    TextOnPathDemo(SkCanvas *c, int w, int h);
    ~TextOnPathDemo();

    virtual void draw();
private:
    SkPath      fPath;
    SkScalar    fHOffset;
};

#endif /* TEXTONPATHDEMO_H_ */
