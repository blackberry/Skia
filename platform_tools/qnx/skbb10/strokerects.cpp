
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "DemoBase.h"
#include "SkRandom.h"

class StrokeRect : public DemoBase {
public:
    StrokeRect(SkCanvas *c, int w, int h)
        : DemoBase(c,w,h)
    {
        name = "StrokeRects Demo";
    }
    virtual ~StrokeRect() {}

protected:

    void rnd_rect(SkRect* r, SkLCGRandom& rand) {
        SkScalar x = rand.nextUScalar1() * width;
        SkScalar y = rand.nextUScalar1() * height;
        SkScalar w = rand.nextUScalar1() * (width >> 2);
        SkScalar h = rand.nextUScalar1() * (height >> 2);
        SkScalar hoffset = rand.nextSScalar1();
        SkScalar woffset = rand.nextSScalar1();

        r->set(x, y, x + w, y + h);
        r->offset(-w/2 + woffset, -h/2 + hoffset);
    }

    virtual void draw() {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);

        for (int y = 0; y < 1; y++) {
            paint.setAntiAlias(!!y);
            for (int x = 0; x < 1; x++) {
                paint.setStrokeWidth(x * SkIntToScalar(3));

                SkAutoCanvasRestore acr(canvas, true);
                canvas->translate(width * x, height * y);
                canvas->clipRect(SkRect::MakeLTRB(
                        SkIntToScalar(2), SkIntToScalar(2)
                        , width - SkIntToScalar(2), height - SkIntToScalar(2)
                ));

                SkLCGRandom rand;
                for (int i = 0; i < 150; i++) {
                    SkRect r;
                    rnd_rect(&r, rand);
                    canvas->drawRect(r, paint);
                }
            }
        }
    }

private:

};

