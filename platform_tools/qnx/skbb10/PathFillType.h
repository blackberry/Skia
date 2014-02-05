/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PATHFILLTYPE_H_
#define PATHFILLTYPE_H_

#include "DemoBase.h"

class PathFillType : public DemoBase {
public:
    PathFillType(SkCanvas *c, int w, int h);
    ~PathFillType() {}

    virtual void draw();

private:
    void drawPath(SkPath::FillType type, const char *label);

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;

    SkRect clipRect;
    SkPath donutPath;

    static const SkScalar hSpacing = 100;
    static const SkScalar vSpacing = 20;
    static const SkScalar colWidth = 240;
    static const SkScalar rowHeight = 240;
};


#endif /* PATHFILLTYPE_H_ */
