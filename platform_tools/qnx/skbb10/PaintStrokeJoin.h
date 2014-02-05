/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PAINTSTROKEJOIN_H_
#define PAINTSTROKEJOIN_H_

#include "DemoBase.h"

class PaintStrokeJoin : public DemoBase {
public:
    PaintStrokeJoin(SkCanvas *c, int w, int h);
    ~PaintStrokeJoin() {}

    virtual void draw();

private:
    void drawHLabels();
    void drawVLabels();
    void drawRow();

    SkPaint shapePaint;
    SkPaint textPaint;
    SkRect rect;
    SkPath linPath;

    static const SkScalar hSpacing = 80;
    static const SkScalar vSpacing = 50;
    static const SkScalar colWidth = 150;
    static const SkScalar rowHeight = 120;

    static const char *shapeNames[];
    static const char *paintStrokeJoins[];
};


#endif /* PAINTSTROKEJOIN_H_ */
