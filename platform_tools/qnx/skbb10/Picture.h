/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PICTURE_H_
#define PICTURE_H_

#include "DemoBase.h"
#include "SkPicture.h"
#include "SkStream.h"

class Picture : public DemoBase {
public:
    Picture(SkCanvas *c, int w, int h);
    ~Picture() {}

    virtual void draw();

private:
    bool lazyInit();
    SkPicture* picture;
};


#endif /* PATHFILLTYPE_H_ */
