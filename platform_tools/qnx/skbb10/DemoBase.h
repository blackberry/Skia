/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DEMOBASE_H_
#define DEMOBASE_H_

#include "SkCanvas.h"
#include "SkPaint.h"

#include <vector>

class DemoBase {
public:
    DemoBase(SkCanvas *c, int w, int h);
    virtual ~DemoBase() {}

    SkScalar GetAnimScalar(SkScalar speed, SkScalar period);

    void onDraw();

    void drawLabel();
    void drawArrows();

    virtual void draw() = 0;

protected:
    SkCanvas *canvas;
    int width;
    int height;
    const char *name;
    SkPaint labelPaint;
    SkPaint arrowPaint;

    unsigned long long animTime;
    unsigned long long prevAnimTime;
};

class DemoFactory {
public:
    static void createDemos(SkCanvas *canvas, int width, int height);
    static void destroyDemos();

    static DemoBase* getDemo(size_t);
    static size_t demoCount();

private:
    static void deleteDemos(DemoBase** demos);
    static DemoBase *createDemo(int index, SkCanvas *canvas, int width, int height);
    static std::vector<DemoBase*> s_demos;
};

#endif /* DEMOBASE_H_ */
