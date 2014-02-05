/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PATHEFFECTS_H_
#define PATHEFFECTS_H_

#include "DemoBase.h"

class SkPathEffect;

class PathEffects : public DemoBase {
public:
    PathEffects(SkCanvas *c, int w, int h);
    ~PathEffects();

    virtual void draw();

private:
    void drawPath(SkPathEffect *effect, const char *label);

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;
    SkPath starPath;
    SkPathEffect* dashEffect;
    SkPathEffect* cornerEffect;
    SkPathEffect* discreteEffect;

    static const SkScalar hSpacing = 30;
    static const SkScalar colWidth = 300;

    static const char *effects[];
};


#endif /* PATHEFFECTS_H_ */
