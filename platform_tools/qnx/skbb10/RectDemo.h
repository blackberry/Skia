/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef RECTDEMO_H_
#define RECTDEMO_H_

#include "DemoBase.h"

class RectDemo: public DemoBase {
public:
    RectDemo(SkCanvas *c, int w, int h);
    ~RectDemo() {}

    virtual void draw();
};

#endif /* RECTDEMO_H_ */
