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

#ifndef PAINTSTROKEJOIN_H_
#define PAINTSTROKEJOIN_H_

#include "DemoBase.h"

class PaintStrokeJoin : public DemoBase {
public:
    PaintStrokeJoin(SkCanvas *c, int w, int h);
    ~PaintStrokeJoin() {}

    virtual void draw();

private:
    void drawHLabels();
    void drawVLabels();
    void drawRow();

    SkPaint shapePaint;
    SkPaint textPaint;
    SkRect rect;
    SkPath linPath;

    static const SkScalar hSpacing = 80;
    static const SkScalar vSpacing = 50;
    static const SkScalar colWidth = 150;
    static const SkScalar rowHeight = 120;

    static const char *shapeNames[];
    static const char *paintStrokeJoins[];
};


#endif /* PAINTSTROKEJOIN_H_ */
