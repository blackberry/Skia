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

#ifndef DEMOBASE_H_
#define DEMOBASE_H_

#include "SkCanvas.h"
#include "SkPaint.h"

class DemoBase {
public:
    DemoBase(SkCanvas *c, int w, int h);
    virtual ~DemoBase() {}

    SkScalar GetAnimScalar(SkScalar speed, SkScalar period);

    void onDraw();

    void drawLabel();
    void drawArrows();

    virtual void draw() = 0;

protected:
    SkCanvas *canvas;
    int width;
    int height;
    const char *name;
    SkPaint labelPaint;
    SkPaint arrowPaint;

    unsigned long long animTime;
    unsigned long long prevAnimTime;
};

class DemoFactory {
public:
    static DemoBase **createAllDemos(SkCanvas *canvas, int width, int height);
    static void deleteDemos(DemoBase** demos);
    static DemoBase *createDemo(int index, SkCanvas *canvas, int width, int height);
    static int demoCount() { return m_demoCount; }
private:
    const static unsigned m_demoCount;
};

#endif /* DEMOBASE_H_ */
