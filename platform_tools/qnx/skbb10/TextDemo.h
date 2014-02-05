/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TEXTDEMO_H_
#define TEXTDEMO_H_

#include "DemoBase.h"
#include "SkCanvas.h"
#include "SkFontHost.h"
#include "SkPaint.h"
#include "SkRandom.h"
#include "SkString.h"

class TextDemo: public DemoBase {
public:
    TextDemo(SkCanvas *c, int w, int h);
    ~TextDemo();

    virtual void draw();
private:
    void DrawTheText(SkCanvas* canvas, SkScalar y, int index, const SkPaint& paint);

    SkPoint **pts;
    int maxLineCount;
    SkPaint paint;
};

class TextBench : public DemoBase {
public:
    TextBench(SkCanvas* canvas, int width, int height);
    ~TextBench() { }

    virtual void draw();

private:
    SkPaint paint;
    SkString fText;
    SkString fName;
};


#endif /* TEXTDEMO_H_ */
