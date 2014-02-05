/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LAYERDEMO_H_
#define LAYERDEMO_H_

#include "DemoBase.h"

class LayerDemo : public DemoBase {
public:
    LayerDemo(SkCanvas *c, int w, int h);
    ~LayerDemo() {}

    virtual void draw();
};


#endif /* LAYERDEMO_H_ */
