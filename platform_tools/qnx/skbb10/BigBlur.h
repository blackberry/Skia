/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BIGBLUR_H_
#define BIGBLUR_H_

#include "DemoBase.h"

class BigBlur : public DemoBase {
public:
    BigBlur(SkCanvas *c, int w, int h);
    virtual ~BigBlur();

    virtual void draw();
};

#endif /* BIGBLUR_H_ */
