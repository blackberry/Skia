/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CLIPDEMO_H_
#define CLIPDEMO_H_

#include "DemoBase.h"

class ClipDemo: public DemoBase {
public:
    ClipDemo(SkCanvas *c, int w, int h);
    ~ClipDemo() {}

    virtual void draw();
};

#endif /* RECTDEMO_H_ */
