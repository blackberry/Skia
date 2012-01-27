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

#ifndef PATHFILLTYPE_H_
#define PATHFILLTYPE_H_

#include "DemoBase.h"

class PathFillType : public DemoBase {
public:
    PathFillType(SkCanvas *c, int w, int h);
    ~PathFillType() {}

    virtual void draw();

private:
    void drawPath(SkPath::FillType type, const char *label);

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;

    SkRect clipRect;
    SkPath donutPath;

    static const SkScalar hSpacing = 100;
    static const SkScalar vSpacing = 20;
    static const SkScalar colWidth = 240;
    static const SkScalar rowHeight = 240;
};


#endif /* PATHFILLTYPE_H_ */
