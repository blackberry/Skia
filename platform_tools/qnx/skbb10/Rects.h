/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef RECTSDEMO_H_
#define RECTSDEMO_H_

#include "DemoBase.h"

class RectSpamDemo : public DemoBase {
public:
    RectSpamDemo(SkCanvas *c, int w, int h);
    ~RectSpamDemo() {}

    virtual void draw();
private:
    static const int boxSize = 7;
    static const int padding = 1;

    const int boxW;
    const int maxBoxes;
    int currentBoxes;

    SkPaint paintSolid;
    SkPaint paintTrans;
};

#endif /* RECTSDEMO_H_ */
