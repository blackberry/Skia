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

#ifndef GRADIENTSHADERS_H_
#define GRADIENTSHADERS_H_

#include "DemoBase.h"

class SkShader;

class GradientShaders : public DemoBase {
public:
    GradientShaders(SkCanvas *c, int w, int h);
    ~GradientShaders();

    virtual void draw();

private:
    void drawWithShader(SkShader *shader, const char *label, int size);
    void drawRow(int index);
    void drawHLabels();
    void drawVLabels();

    SkPaint shapePaint;
    SkPaint outlinePaint;
    SkPaint textPaint;
    SkShader *linearShader[3];
    SkShader *radialShader[3];
    SkShader *radial2PtShader[3];
    SkShader *sweepShader;

    static const SkScalar hSpacing = 50;
    static const SkScalar vSpacing = 50;
    static const SkScalar colWidth = 120;
    static const SkScalar rowHeight = 120;
    SkScalar diagonal;

    static const char *gradients[];
    static const char *tileModes[];
};


#endif /* GRADIENTSHADERS_H_ */
