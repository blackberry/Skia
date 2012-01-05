#include "gm.h"
#include "SkGradientShader.h"

namespace skiagm {

struct GradData {
    int             fCount;
    const SkColor*  fColors;
    const SkScalar* fPos;
};

static const SkColor gColors[] = {
    SK_ColorRED, SK_ColorGREEN, SK_ColorBLUE, SK_ColorWHITE, SK_ColorBLACK
};
static const SkScalar gPos0[] = { 0, SK_Scalar1 };
static const SkScalar gPos1[] = { SK_Scalar1/4, SK_Scalar1*3/4 };
static const SkScalar gPos2[] = {
    0, SK_Scalar1/8, SK_Scalar1/2, SK_Scalar1*7/8, SK_Scalar1
};

static const GradData gGradData[] = {
    { 2, gColors, NULL },
    { 2, gColors, gPos0 },
    { 2, gColors, gPos1 },
    { 5, gColors, NULL },
    { 5, gColors, gPos2 }
};

static SkShader* MakeLinear(const SkPoint pts[2], const GradData& data,
                            SkShader::TileMode tm, SkUnitMapper* mapper) {
    return SkGradientShader::CreateLinear(pts, data.fColors, data.fPos,
                                          data.fCount, tm, mapper);
}

static SkShader* MakeRadial(const SkPoint pts[2], const GradData& data,
                            SkShader::TileMode tm, SkUnitMapper* mapper) {
    SkPoint center;
    center.set(SkScalarAve(pts[0].fX, pts[1].fX),
               SkScalarAve(pts[0].fY, pts[1].fY));
    return SkGradientShader::CreateRadial(center, center.fX, data.fColors,
                                          data.fPos, data.fCount, tm, mapper);
}

static SkShader* MakeSweep(const SkPoint pts[2], const GradData& data,
                           SkShader::TileMode tm, SkUnitMapper* mapper) {
    SkPoint center;
    center.set(SkScalarAve(pts[0].fX, pts[1].fX),
               SkScalarAve(pts[0].fY, pts[1].fY));
    return SkGradientShader::CreateSweep(center.fX, center.fY, data.fColors,
                                         data.fPos, data.fCount, mapper);
}

static SkShader* Make2Radial(const SkPoint pts[2], const GradData& data,
                             SkShader::TileMode tm, SkUnitMapper* mapper) {
    SkPoint center0, center1;
    center0.set(SkScalarAve(pts[0].fX, pts[1].fX),
                SkScalarAve(pts[0].fY, pts[1].fY));
    center1.set(SkScalarInterp(pts[0].fX, pts[1].fX, SkIntToScalar(3)/5),
                SkScalarInterp(pts[0].fY, pts[1].fY, SkIntToScalar(1)/4));
    return SkGradientShader::CreateTwoPointRadial(
                                                  center1, (pts[1].fX - pts[0].fX) / 7,
                                                  center0, (pts[1].fX - pts[0].fX) / 2,
                                                  data.fColors, data.fPos, data.fCount, tm, mapper);
}

typedef SkShader* (*GradMaker)(const SkPoint pts[2], const GradData& data,
                               SkShader::TileMode tm, SkUnitMapper* mapper);
static const GradMaker gGradMakers[] = {
    MakeLinear, MakeRadial, MakeSweep, Make2Radial
};

///////////////////////////////////////////////////////////////////////////////

class GradientsGM : public GM {
public:
	GradientsGM() {}
    
protected:
    SkString onShortName() {
        return SkString("gradients");
    }
    
    virtual SkISize onISize() { return make_isize(640, 510); }
    
    void drawBG(SkCanvas* canvas) {
        canvas->drawColor(0xFFDDDDDD);
    }
    
    virtual void onDraw(SkCanvas* canvas) {
        this->drawBG(canvas);
        
        SkPoint pts[2] = {
            { 0, 0 },
            { SkIntToScalar(100), SkIntToScalar(100) }
        };
        SkShader::TileMode tm = SkShader::kClamp_TileMode;
        SkRect r = { 0, 0, SkIntToScalar(100), SkIntToScalar(100) };
        SkPaint paint;
        paint.setAntiAlias(true);
        
        canvas->translate(SkIntToScalar(20), SkIntToScalar(20));
        for (size_t i = 0; i < SK_ARRAY_COUNT(gGradData); i++) {
            canvas->save();
            for (size_t j = 0; j < SK_ARRAY_COUNT(gGradMakers); j++) {
                SkShader* shader = gGradMakers[j](pts, gGradData[i], tm, NULL);
                paint.setShader(shader);
                canvas->drawRect(r, paint);
                shader->unref();
                canvas->translate(0, SkIntToScalar(120));
            }
            canvas->restore();
            canvas->translate(SkIntToScalar(120), 0);
        }
    }
    
private:
    typedef GM INHERITED;
};

/*
 Inspired by this <canvas> javascript, where we need to detect that we are not
 solving a quadratic equation, but must instead solve a linear (since our X^2
 coefficient is 0)

 ctx.fillStyle = '#f00';
 ctx.fillRect(0, 0, 100, 50);
 
 var g = ctx.createRadialGradient(-80, 25, 70, 0, 25, 150);
 g.addColorStop(0, '#f00');
 g.addColorStop(0.01, '#0f0');
 g.addColorStop(0.99, '#0f0');
 g.addColorStop(1, '#f00');
 ctx.fillStyle = g;
 ctx.fillRect(0, 0, 100, 50);
 */
class GradientsDegenrate2PointGM : public GM {
public:
    GradientsDegenrate2PointGM() {}
    
protected:
    SkString onShortName() {
        return SkString("gradients_degenerate_2pt");
    }
    
	virtual SkISize onISize() { return make_isize(320, 320); }
    
    void drawBG(SkCanvas* canvas) {
        canvas->drawColor(SK_ColorBLUE);
    }
    
    virtual void onDraw(SkCanvas* canvas) {
        this->drawBG(canvas);
        
        SkColor colors[] = { SK_ColorRED, SK_ColorGREEN, SK_ColorGREEN, SK_ColorRED };
        SkScalar pos[] = { 0, SkFloatToScalar(0.01f), SkFloatToScalar(0.99f), SK_Scalar1 };
        SkPoint c0 = { -80, 25 };
        SkScalar r0 = 70;
        SkPoint c1 = { 0, 25 };
        SkScalar r1 = 150;
        SkShader* s = SkGradientShader::CreateTwoPointRadial(c0, r0, c1, r1, colors,
                                                             pos, SK_ARRAY_COUNT(pos),
                                                             SkShader::kClamp_TileMode);
        SkPaint paint;
        paint.setShader(s)->unref();
        canvas->drawPaint(paint);
    }
    
private:
    typedef GM INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new GradientsGM; }
static GMRegistry reg(MyFactory);

static GM* MyFactory2(void*) { return new GradientsDegenrate2PointGM; }
static GMRegistry reg2(MyFactory2);

}

