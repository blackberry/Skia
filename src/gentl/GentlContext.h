/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlContext_h
#define GentlContext_h

#include "InstructionSet.h"
#include "Layers.h"
#include "SkCanvas.h"
#include "SkPoint.h"
#include "SkShader.h"
#include "SkMaskFilter.h"

#include <stdlib.h>
#include <vector>

class SkPaint;
class GrContext;
class GrTexture;
class SkClipStack;

namespace Gentl {

class DisplayList;
class GentlTextState;
struct Glyph;
class SolidColorGen;
class ArcGen;

class GentlContext : public SkNoncopyable {
public:
    GentlContext(GrContext*, const SkISize&);
    ~GentlContext();

    void clear(SkColor);
    void clearRect(const SkRect&, const SkPaint&);

    void flush(DisplayList*);

    bool setupText(GrContext* grContext, const SkDeviceProperties&, const SkPaint&);
    void paintText(const char*, size_t byteLength, const SkPoint&, const SkPaint&);
    void paintPosText(const char*, size_t byteLength, const SkScalar pos[], SkScalar constY, int scalarsPerPos, const SkPaint&);

    void paintPath(const SkPath&, const SkPaint&);
    void paintRect(const SkRect&, const SkPaint&);
    void paintRRect(const SkRRect&, const SkPaint&);
    void paintBitmap(const SkRect& dst, const SkRect& src, const SkBitmap&, const SkPaint&);

    void pushLayer();
    void popLayer(float opacity = 1.0f, const SkSize& blurVector = SkSize::Make(0, 0), float blur = 0);
    void popLayer(float opacity, const SkSize& blurVector, float blur, SkRect dirtyRect, SkXfermode::Mode mode, bool offsetDraw);
    void popLayer(float opacity, const SkRect& dirtyRect, SkXfermode::Mode mode, bool offsetDraw);

    void setMatrix(const SkMatrix&);
    SkMatrix& matrix() { return fMatrix; }
    const SkMatrix& matrix() const { return fMatrix; }

    void setAlpha(float alpha);
    float alpha() const { return fAlpha; }

    void setXfermode(const SkPaint&);
    void setXfermodeMode(SkXfermode::Mode);
    SkXfermode::Mode xferMode() const;

    void setClipFromClipStack(const SkClipStack&, const SkIPoint& origin);
    void resetClip();
    inline Clip& currentClip() { return fClip; }

    const SkISize& size() const;

    InstructionSet* instructions();
    TransparencyLayer* topLayer();

    bool isDrawingEnabled() const { return fDrawingEnabled; }
    SkMaskFilter::BlurType getBlurInfo(const SkPaint& skPaint, SkMaskFilter::BlurInfo& blurInfo) const;

    void addContext(GentlContext&);
    void addFillShaderRect(const SkRect&, const SkPaint&);
    void addFillRect(const SkRect&, const SkPaint&);
    void addStrokeRect(const SkRect&, const SkPaint&);
    void addFillRRect(const SkRRect&, const SkPaint&);
    void addStrokeRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint&);
    void addSolidQuad(SolidColorGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3, const SkPoint& p4);
    void addSmoothQuad(ArcGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3, const SkPoint& p4);
    void addSolidArc(SolidColorGen& gen, const SkPoint& outerCenter, const SkPoint& outerRadius, const SkPoint& innerCenter, const SkPoint& innerRadius);
    void addSmoothArc(ArcGen& gen, const SkPoint& outerCenter, const SkPoint& outerRadius, const SkPoint& innerCenter, const SkPoint& innerRadius, const SkPaint& paint);
    void addRound(SolidColorGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3);
    void addVertices(SkPoint[], unsigned count, SkCanvas::VertexMode, const SkPaint&); // Vertex data can be modified.

private:

    void clip(const SkRect&, bool transform = true);
    void clipOut(const SkRect&, bool transform = true);
    void clipRoundedRect(const SkRRect&, bool transform = true);
    void clipOutRoundedRect(const SkRRect&, bool transform = true);
    void addDrawLine(const SkIPoint&, const SkIPoint&, const SkPaint&, float width);

    bool hasVisibleFill(const SkPaint& paint);
    bool hasVisibleStroke(const SkPaint& paint);
    inline float strokeWidthRespectingHairline(const SkPaint&) const;

    void updateDrawingEnabledState();
    void updateStrokeVisibleState();
    void updateStrokeEnabledState();

    void clipRoundedCorner(const SkPoint& clipa, const SkPoint& clipb, const SkPoint& center, const SkPoint& edge, bool transform);
    void clipOutRoundedCorner(const SkPoint& clipa, const SkPoint& clipb, const SkPoint& center, const SkPoint& edge, bool transform);

    void paintGlyph(const SkGlyph&, const SkPoint&, SkGlyphCache*, const SkPaint&);

private:
    GrContext* fGrContext;
    const SkISize fSize;
    InstructionSet fInstructions;
    GentlTextState* fText;
    std::vector<TransparencyLayer> fLayers;

    bool fDrawingEnabled;

    float fAlpha;
    SkXfermode::Mode fXfermode;
    SkMatrix fMatrix;

    Clip fClip;
    int32_t fClipGenId;

    enum BackingWatermarks {
        LowCommandWatermark = 256,
        LowDataWatermark = 256 * 1024,
        HighDataWatermark = 1024 * 1024,
    };

    enum {
        DistanceFieldTextSize = 32,
        MaximumSupportedTextSize = 96,
    };
};

} // namespace Gentl

#endif // GentlContext_h
