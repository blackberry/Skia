/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef ARCDEMO_H_
#define ARCDEMO_H_

#include "DemoBase.h"

class ArcDemo : public DemoBase {
public:
    ArcDemo(SkCanvas *c, int w, int h);
    ~ArcDemo() {}

    virtual void draw();
};


#endif /* ARCDEMO_H_ */
