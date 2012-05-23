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

#ifndef TEXTDEMO_H_
#define TEXTDEMO_H_

#include "DemoBase.h"
#include "SkPaint.h"

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

#endif /* TEXTDEMO_H_ */
