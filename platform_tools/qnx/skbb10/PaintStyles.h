/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PAINTSTYLES_H_
#define PAINTSTYLES_H_

#include "DemoBase.h"

class PaintStyles : public DemoBase {
public:
    PaintStyles(SkCanvas *c, int w, int h);
    ~PaintStyles() {}

    virtual void draw();

private:
    void drawHLabels();
    void drawVLabels();
    void drawRow();

    SkPaint shapePaint;
    SkPaint textPaint;
    SkRect rect;
    SkPath linPath;

    static const SkScalar hSpacing = 50;
    static const SkScalar vSpacing = 50;
    static const SkScalar colWidth = 150;
    static const SkScalar rowHeight = 120;

    static const char *shapeNames[];
    static const char *paintStyles[];
};


#endif /* PAINTSTYLES_H_ */
