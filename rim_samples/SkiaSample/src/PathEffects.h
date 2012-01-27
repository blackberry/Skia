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

#ifndef PATHEFFECTS_H_
#define PATHEFFECTS_H_

#include "DemoBase.h"

class SkPathEffect;

class PathEffects : public DemoBase {
public:
    PathEffects(SkCanvas *c, int w, int h);
    ~PathEffects();

    virtual void draw();

private:
    void drawPath(SkPathEffect *effect, const char *label);

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;
    SkPath starPath;
    SkPathEffect* dashEffect;
    SkPathEffect* cornerEffect;
    SkPathEffect* discreteEffect;

    static const SkScalar hSpacing = 30;
    static const SkScalar colWidth = 300;

    static const char *effects[];
};


#endif /* PATHEFFECTS_H_ */
