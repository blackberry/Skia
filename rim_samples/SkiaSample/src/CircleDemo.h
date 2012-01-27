/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CIRCLEDEMO_H_
#define CIRCLEDEMO_H_

#include "DemoBase.h"

class CircleDemo: public DemoBase {
public:
    CircleDemo(SkCanvas *c, int w, int h);
    ~CircleDemo() {}

    virtual void draw();
private:
    void circle(SkCanvas* canvas, int width, bool aa);
    void drawSix(SkCanvas* canvas, SkScalar dx, SkScalar dy);

    static const SkScalar ANIM_DX;
    static const SkScalar ANIM_DY;
    static const SkScalar ANIM_RAD;
    SkScalar fDX, fDY, fRAD;
};

#endif /* CIRCLEDEMO_H_ */
