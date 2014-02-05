/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PAINTSTROKECAP_H_
#define PAINTSTROKECAP_H_

#include "DemoBase.h"
#include "SkBlurDrawLooper.h"

class PaintStrokeCap : public DemoBase {
public:
    PaintStrokeCap(SkCanvas *c, int w, int h);
    ~PaintStrokeCap() {}

    virtual void draw();

private:
    void drawHLabels();
    void drawVLabels();
    void drawRow();

    SkBlurDrawLooper looper;
    SkPaint shapePaint;
    SkPaint textPaint;

    static const SkScalar hSpacing = 120;
    static const SkScalar vSpacing = 50;
    static const SkScalar rowHeight = 120;
    static const SkScalar colWidth = 150;

    static const char *shapeNames[];
    static const char *paintStrokeCaps[];
};


#endif /* PAINTSTROKECAP_H_ */
