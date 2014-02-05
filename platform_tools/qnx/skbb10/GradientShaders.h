/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GRADIENTSHADERS_H_
#define GRADIENTSHADERS_H_

#include "DemoBase.h"

class SkShader;

class GradientShaders : public DemoBase {
public:
    GradientShaders(SkCanvas *c, int w, int h);
    ~GradientShaders();

    virtual void draw();

private:
    void drawWithShader(SkShader *shader, const char *label, int size);
    void drawRow(int index);
    void drawHLabels();
    void drawVLabels();

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;
    SkShader *linearShader[3];
    SkShader *radialShader[3];
    SkShader *radial2PtShader[3];
    SkShader *sweepShader;

    static const SkScalar hSpacing = 50;
    static const SkScalar vSpacing = 50;
    static const SkScalar colWidth = 120;
    static const SkScalar rowHeight = 120;
    SkScalar diagonal;

    static const char *gradients[];
    static const char *tileModes[];
};


#endif /* GRADIENTSHADERS_H_ */
