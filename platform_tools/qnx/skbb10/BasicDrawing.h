/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BASICDRAWING_H_
#define BASICDRAWING_H_

#include "DemoBase.h"

class Bitmap;

class BasicDrawing : public DemoBase {
public:
    BasicDrawing(SkCanvas *c, int w, int h);
    ~BasicDrawing();

    virtual void draw();

private:
    SkPaint shapePaint;
    SkPaint textPaint;
    SkPaint linePaint;
    SkRect rect;
    SkPath linPath;
    SkPath cubicPath;
    SkBitmap bitmap;
    bool result;

    static const SkScalar hSpacing = 50;
    static const SkScalar vSpacing = 50;
    static const SkScalar rowHeight = 120;
    static const SkScalar colWidth = 150;

    static const char *textStrings[];
};


#endif /* BASICDRAWING_H_ */
