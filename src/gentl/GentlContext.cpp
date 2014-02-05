/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GentlContext.h"

#include "Generators.h"
#include "GentlCommands.h"
#include "GentlPath.h"
#include "GlyphCache.h"
#include "GrContext.h"

#include "SkGlyphCache.h"
#include "SkBlurMaskFilter.h"
#include "SkCanvas.h"
#include "SkClipStack.h"
#include "SkColorPriv.h"
#include "SkDashPathEffect.h"
#include "SkDrawProcs.h"
#include "SkGlyph.h"
#include "SkGr.h"
#include "SkLayerDrawLooper.h"
#include "SkMaskFilter.h"
#include "SkShader.h"
#include "SkPaint.h"
#include "SkTypeface.h"
#include "SkTypes.h"

namespace Gentl {

class SkAutoCachedTexture : public ::SkNoncopyable {
public:
    SkAutoCachedTexture()
        : fTexture(NULL) {
    }

    SkAutoCachedTexture(GrContext* context,
                        const SkBitmap& bitmap,
                        const GrTextureParams* params,
                        GrTexture** texture)
        : fTexture(NULL) {
        SkASSERT(NULL != texture);
        *texture = this->set(context, bitmap, params);
    }

    ~SkAutoCachedTexture() {
        if (NULL != fTexture) {
            GrUnlockAndUnrefCachedBitmapTexture(fTexture);
        }
    }

    GrTexture* set(GrContext* context,
                   const SkBitmap& bitmap,
                   const GrTextureParams* params) {
        if (NULL != fTexture) {
            GrUnlockAndUnrefCachedBitmapTexture(fTexture);
            fTexture = NULL;
        }
        GrTexture* result = (GrTexture*)bitmap.getTexture();
        if (NULL == result) {
            // Cannot return the native texture so look it up in our cache
            fTexture = GrLockAndRefCachedBitmapTexture(context, bitmap, params);
            result = fTexture;
        }
        return result;
    }

private:
    GrTexture*   fTexture;
};

class GentlTextState {
public:
    GlyphCache* glyphCache;
    GrContext* grContext;
    const SkDeviceProperties* deviceProperties;
    SkPaint paint;
    float ratio;
    float insideCutoff;
    float outsideCutoff;
    float halfStroke;
    ABGR32Premultiplied color;  // Calculated once for a run of characters.
};

GentlContext::GentlContext(GrContext* context, const SkISize& size)
    : fGrContext(context)
    , fSize(size)
    , fInstructions()
    , fText(0)
    , fDrawingEnabled(true)
    , fAlpha(1.0f)
    , fXfermode(SkXfermode::kSrcOver_Mode)
    , fMatrix()
    , fClip()
    , fClipGenId(0)
{
}

GentlContext::~GentlContext()
{
    delete fText;
    fText = 0;
}

void GentlContext::clear(SkColor color)
{
    fInstructions.unrefAndClear();
    fLayers.clear();

    updateDrawingEnabledState();

    ClearUniforms uniforms(SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f, SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f);
    fInstructions.push(Clear);
    fInstructions.pushUniforms(uniforms);
}

void GentlContext::pushLayer()
{
    if (!isDrawingEnabled()) {
        // We'll insert a fake layer for layer consistency purposes, but no
        // commands or surfaces as drawing will be disabled the whole time.
        fLayers.push_back(TransparencyLayer());
        updateDrawingEnabledState(); // dependent on fLayers.top()
        return;
    }

    fLayers.push_back(TransparencyLayer(fInstructions.commandSize()));
    fInstructions.push(BeginTransparencyLayer);
    // Make space for the uniforms that get set in popLayer
    BeginTransparencyLayerUniforms uniforms;
    memset(&uniforms, 0, sizeof(BeginTransparencyLayerUniforms));
    fInstructions.pushUniforms(uniforms);
}
void GentlContext::popLayer(float opacity, const SkRect& inDirtyRect, SkXfermode::Mode mode, bool offsetDraw)
{
    SkRect backDirty = fLayers.back().dirtyRect();
    popLayer(opacity, SkSize::Make(0, 0), 0, backDirty.isEmpty() ? backDirty : inDirtyRect, mode, offsetDraw);
}

void GentlContext::popLayer(float opacity, const SkSize& blurVector, float blur)
{
    popLayer(opacity, blurVector, blur, fLayers.back().dirtyRect(), fXfermode, true);
}

void GentlContext::popLayer(float opacity, const SkSize& blurVector, float blur, SkRect dirtyRect, SkXfermode::Mode mode, bool offsetDraw)
{
    if (!fLayers.size()) // no begin?!
        return;

    if (fLayers.back().command() == -1) {
        // Invisible (no-op) transparency layer, pop and proceed without trying to draw it.
        fLayers.pop_back();
        updateDrawingEnabledState(); // dependent on fLayers.top()
        return;
    }

    unsigned pushCommandIndex = fLayers.back().command();

    // Pop the TL stack before we draw so that the layer below is properly dirtied by the operation.
    fLayers.pop_back();

    // If the layer actually has no drawing commands in it we can just NoOp the BeginTransparencyLayer
    // now and skip all the work of creating an offscreen buffer and blending it afterward.
    if (dirtyRect.isEmpty()) {
        SkASSERT(fInstructions[pushCommandIndex].command == BeginTransparencyLayer);
        if (fInstructions[pushCommandIndex].command == BeginTransparencyLayer) {
            fInstructions.makeNoOp(pushCommandIndex);
        }
        return;
    }

    fInstructions.push(EndTransparencyLayer);

    SkPoint quad[4];
    dirtyRect.outset(blur, blur);
    dirtyRect.toQuad(quad);

    BeginTransparencyLayerUniforms uniforms;
    if (offsetDraw) {
        uniforms.offset.set(dirtyRect.x(), dirtyRect.y());
    } else {
        uniforms.offset.set(0, 0);
    }
    uniforms.size.set(ceil(dirtyRect.width()), ceil(dirtyRect.height()));

    Clip clip;
    if (FloatUtils::isGreaterZero(blur)) {
        SkASSERT(FloatUtils::isGreaterZero(blurVector.width()) || FloatUtils::isGreaterZero(blurVector.height()));
        DrawCurrentTextureGen gen(this, opacity, SkSize::Make(dirtyRect.width(), dirtyRect.height()), blur, blurVector, mode);
        clip.clipQuad(quad, gen);
    } else {
        DrawCurrentTextureGen gen(this, opacity, mode);
        clip.clipQuad(quad, gen);
    }
    fInstructions.setUniforms<BeginTransparencyLayerUniforms>(uniforms, pushCommandIndex);
}

SkMaskFilter::BlurType GentlContext::getBlurInfo(const SkPaint& skPaint, SkMaskFilter::BlurInfo& blurInfo) const
{
    if (skPaint.getMaskFilter())
        return skPaint.getMaskFilter()->asABlur(&blurInfo);
    else
        return SkMaskFilter::kNone_BlurType;
}

void GentlContext::setMatrix(const SkMatrix& matrix)
{
    fMatrix = matrix;
}

void GentlContext::setAlpha(float alpha)
{
    if (fAlpha != alpha) {
        fAlpha = alpha;
        fInstructions.push(SetAlpha);
        fInstructions.pushUniforms(SetAlphaUniforms(alpha));
    }

    updateDrawingEnabledState(); // dependent on alpha
}

void GentlContext::setXfermodeMode(SkXfermode::Mode mode)
{
    if (fXfermode != mode) {
        fXfermode = mode;
        fInstructions.push(SetXferMode);
        fInstructions.pushUniforms(SetXferModeUniforms(mode));
    }

    // All three dependent on SkXfermode::Mode.
    updateDrawingEnabledState();
}

void GentlContext::setXfermode(const SkPaint& paint)
{
    SkXfermode* xfer = paint.getXfermode();
    if (!xfer) {
        setXfermodeMode(SkXfermode::kSrcOver_Mode);
        return;
    }

    SkXfermode::Mode mode;
    if (!xfer->asMode(&mode))
        return;

    setXfermodeMode(mode);
}

SkXfermode::Mode GentlContext::xferMode() const
{
    return fXfermode;
}

const SkISize& GentlContext::size() const
{
    return fSize;
}

InstructionSet* GentlContext::instructions()
{
    return &fInstructions;
}

TransparencyLayer* GentlContext::topLayer()
{
    return fLayers.size() ? &fLayers.back() : 0;
}

void GentlContext::setClipFromClipStack(const SkClipStack& clipStack, const SkIPoint& origin)
{
    if (clipStack.getTopmostGenID() == fClipGenId)
        return;

    fClipGenId = clipStack.getTopmostGenID();
    resetClip();
    SkMatrix matrix;
    matrix.setIdentity();
    if (origin.x() || origin.y())
        matrix.setTranslate(-origin.x(), -origin.y());
    SkClipStack::Iter clipIter(clipStack, SkClipStack::Iter::kBottom_IterStart);
    while (const SkClipStack::Element* element = clipIter.next()) {
        switch (element->getType()) {
        case SkClipStack::Element::kRect_Type: {
            const SkRect& origRect = element->getRect();
            SkRect rect;
            matrix.mapRect(&rect, origRect);
            SkRegion::Op op = element->getOp();
            if (op != SkRegion::kIntersect_Op)
                clipOut(rect, false);
            else
                clip(rect, false);
            break;
        }
        case SkClipStack::Element::kRRect_Type: {
             const SkRRect& origRRect = element->getRRect();
             SkRRect rrect;
             origRRect.transform(matrix, &rrect);
             SkRegion::Op op = element->getOp();
             if (op != SkRegion::kIntersect_Op)
                 clipOutRoundedRect(rrect, false);
             else
                 clipRoundedRect(rrect, false);
             break;
        }
        case SkClipStack::Element::kPath_Type: {
            if (SkRegion::kIntersect_Op == element->getOp() && element->getPath().isConvex()) {
                SkPaint dummy;
                SkPath devPath(element->getPath());
                devPath.transform(matrix);
                const GentlPath path(devPath);
                std::vector<std::vector<float> > items;
                SkMatrix ident;
                ident.setIdentity();
                // instead of passing the origin translation matrix which could get applied multiple times
                // we pass the identity matrix since we don't otherwise want to transform the passed in path
                GentlPath::collectForTesselator(ident, path, items, dummy);
                for (size_t i = 0; i < items.size(); ++i) {
                    std::vector<SkPoint> points;
                    for (size_t j = 0; j + 1 < items.at(i).size(); j += 2) {
                        points.push_back(SkPoint::Make(items.at(i).at(j), items.at(i).at(j+1)));
                    }
                    fClip.addPolygon(&points[0], points.size());
                }
            }
            break;
        }
        case SkClipStack::Element::kEmpty_Type: {
            resetClip();
            break;
        }
        }
    }
}

void GentlContext::clip(const SkRect& r, bool transform)
{
    if (transform) {
        if (fMatrix.rectStaysRect()) {
            SkRect transformed;
            fMatrix.mapRect(&transformed, r);
            fClip.addRect(transformed);
        } else {
            SkPoint quad[4];
            fMatrix.mapRectToQuad(quad, r);
            fClip.addPolygon(quad, 4);
        }
    } else {
        fClip.addRect(r);
    }
}

void GentlContext::clipOut(const SkRect& r, bool transform)
{
    if (r.isEmpty())
        return;

    SkPoint quad[4];
    if (transform) {
        fMatrix.mapRectToQuad(quad, r);
    } else {
        r.toQuad(quad);
    }

    fClip.subtractQuad(quad);
}

void GentlContext::clipRoundedRect(const SkRRect& r, bool transform)
{
    // TODO: This can be wrong if we by two rounded rects where the second
    // isn't a strict subset of the first we need to figure out which rounded
    // clips apply, rather than throwing the first ones away wholesale
    fClip.clearRoundedCorner();
    clip(r.rect(), transform);

    SkVector corner = r.radii(SkRRect::kUpperLeft_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipRoundedCorner(SkPoint::Make(r.rect().x() + corner.x(), r.rect().y()),
                          SkPoint::Make(r.rect().x(), r.rect().y() + corner.y()),
                          SkPoint::Make(r.rect().x() + corner.x(), r.rect().y() + corner.y()),
                          SkPoint::Make(r.rect().x(), r.rect().y()), transform);

    corner = r.radii(SkRRect::kUpperRight_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipRoundedCorner(SkPoint::Make(r.rect().right(), r.rect().y() + corner.y()),
                          SkPoint::Make(r.rect().right() - corner.x(), r.rect().y()),
                          SkPoint::Make(r.rect().right() - corner.x(), r.rect().y() + corner.y()),
                          SkPoint::Make(r.rect().right(), r.rect().y()), transform);

    corner = r.radii(SkRRect::kLowerRight_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipRoundedCorner(SkPoint::Make(r.rect().right() - corner.x(), r.rect().bottom()),
                          SkPoint::Make(r.rect().right(), r.rect().bottom() - corner.y()),
                          SkPoint::Make(r.rect().right() - corner.x(), r.rect().bottom() - corner.y()),
                          SkPoint::Make(r.rect().right(), r.rect().bottom()), transform);

    corner = r.radii(SkRRect::kLowerLeft_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipRoundedCorner(SkPoint::Make(r.rect().x(), r.rect().bottom() - corner.y()),
                          SkPoint::Make(r.rect().x() + corner.x(), r.rect().bottom()),
                          SkPoint::Make(r.rect().x() + corner.x(), r.rect().bottom() - corner.y()),
                          SkPoint::Make(r.rect().x(), r.rect().bottom()), transform);
}

void GentlContext::clipOutRoundedRect(const SkRRect& r, bool transform)
{
    clipOut(r.rect(), transform);

    SkVector corner = r.radii(SkRRect::kUpperLeft_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipOutRoundedCorner(SkPoint::Make(r.rect().x() + corner.x(), r.rect().y()),
                             SkPoint::Make(r.rect().x(), r.rect().y() + corner.y()),
                             SkPoint::Make(r.rect().x() + corner.x(), r.rect().y() + corner.y()),
                             SkPoint::Make(r.rect().x(), r.rect().y()), transform);

    corner = r.radii(SkRRect::kUpperRight_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipOutRoundedCorner(SkPoint::Make(r.rect().right(), r.rect().y() + corner.y()),
                             SkPoint::Make(r.rect().right() - corner.x(), r.rect().y()),
                             SkPoint::Make(r.rect().right() - corner.x(), r.rect().y() + corner.y()),
                             SkPoint::Make(r.rect().right(), r.rect().y()), transform);

    corner = r.radii(SkRRect::kLowerRight_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipOutRoundedCorner(SkPoint::Make(r.rect().right() - corner.x(), r.rect().bottom()),
                             SkPoint::Make(r.rect().right(), r.rect().bottom() - corner.y()),
                             SkPoint::Make(r.rect().right() - corner.x(), r.rect().bottom() - corner.y()),
                             SkPoint::Make(r.rect().right(), r.rect().bottom()), transform);

    corner = r.radii(SkRRect::kLowerLeft_Corner);
    if (corner.x() > 0 && corner.y() > 0)
        clipOutRoundedCorner(SkPoint::Make(r.rect().x(), r.rect().bottom() - corner.y()),
                             SkPoint::Make(r.rect().x() + corner.x(), r.rect().bottom()),
                             SkPoint::Make(r.rect().x() + corner.x(), r.rect().bottom() - corner.y()),
                             SkPoint::Make(r.rect().x(), r.rect().bottom()), transform);
}

void GentlContext::resetClip()
{
    fClip = Clip();
}

void GentlContext::paintRect(const SkRect& rect, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    SkRect sorted(rect);
    sorted.sort();

    if (hasVisibleFill(paint)) {
        if (paint.getShader()) {
            addFillShaderRect(sorted, paint);
        } else {
            addFillRect(sorted, paint);
        }
    }

    if (hasVisibleStroke(paint))
        addStrokeRect(sorted, paint);
}

void GentlContext::paintRRect(const SkRRect& rRect, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    if (hasVisibleFill(paint)) {
        if (paint.getShader()) {
            const Clip oldClip = fClip;
            clipRoundedRect(rRect);
            addFillShaderRect(rRect.rect(), paint);
            fClip = oldClip;
        } else {
            addFillRRect(rRect, paint);
        }
    }

    if (hasVisibleStroke(paint)) {
        float halfStroke = strokeWidthRespectingHairline(paint) / 2;
        SkRRect inner, outer;
        rRect.inset(halfStroke, halfStroke, &inner);
        rRect.outset(halfStroke, halfStroke, &outer);
        // If the stroke width is large enough the radius can be set to zero
        // But if we started as an oval we want to stay an oval.
        if (rRect.isOval())
            inner.setOval(inner.rect());
        addStrokeRRect(outer, inner, paint);
    }
}

void GentlContext::addFillShaderRect(const SkRect& rect, const SkPaint& paint)
{
    SkShader* shader = paint.getShader();
    if (!shader) {
        SkDebugf("Cannot fill shader rect without SkShader.\n");
        SkASSERT(false);
        return;
    }

    setAlpha(paint.getAlpha() / 255.f);

    SkBitmap bitmap;
    SkMatrix gradientMatrix;
    gradientMatrix.setIdentity();
    SkShader::TileMode tileModes[2];
    const SkShader::BitmapType type = shader->asABitmap(&bitmap, &gradientMatrix, tileModes);
    GrTexture* texture;
    SkAutoCachedTexture act(fGrContext, bitmap, 0, &texture);

    if (!texture) {
        SkDebugf("Could not extract texture from SkShader.\n");
        return;
    }

    SkPoint texcoords[4];
    SkPoint quad[4];
    fMatrix.mapRectToQuad(quad, rect);

    switch (type) {
    case SkShader::kNone_BitmapType:
        SkDebugf("Shader cannot be represented as a bitmap. Fail.\n");
        break;
    case SkShader::kDefault_BitmapType: {
        SkMatrix uvTransform;
        if (!shader->getLocalMatrix().invert(&uvTransform)) {
            SkDebugf("Could not invert local matrix. Cannot draw SkShader.\n");
            return;
        }

        uvTransform.postScale(1.f / texture->width(), 1.f / texture->height());
        uvTransform.mapRectToQuad(texcoords, rect);
        break;
    }
    case SkShader::kRadial_BitmapType:
    case SkShader::kSweep_BitmapType:
    case SkShader::kTwoPointRadial_BitmapType:
    case SkShader::kTwoPointConical_BitmapType:
    case SkShader::kLinear_BitmapType:
        rect.toQuad(texcoords);
        break;
    }

    const Clip::Vertex v1(quad[0].x(), quad[0].y(), texcoords[0].x(), texcoords[0].y());
    const Clip::Vertex v2(quad[1].x(), quad[1].y(), texcoords[1].x(), texcoords[1].y());
    const Clip::Vertex v3(quad[2].x(), quad[2].y(), texcoords[2].x(), texcoords[2].y());
    const Clip::Vertex v4(quad[3].x(), quad[3].y(), texcoords[3].x(), texcoords[3].y());

    SkShader::GradientInfo gInfo;
    gInfo.fColorCount = 0;
    SkShader::GradientType gType = shader->asAGradient(&gInfo);
    SkShaderGen gen(this, texture, paint.getFilterLevel(), gradientMatrix, tileModes, gType, gInfo);
    fClip.clipQuad(v1, v2, v3, v4, gen);
}

/**
 * This illustrates what the points for the blur mean.

     1---2-------------8---5
     |   |             |   |
     4---3-------------7---6
     |   |             |   |
     |   |             |   |
     |   |             |   |
     |   |             |   |
     |   |             |   |
     |   |             |   |
     |   |             |   |
    14--15------------11---12
     |   |             |   |
    13--16------------10---9
 */
void GentlContext::addFillRect(const SkRect& rect, const SkPaint& paint)
{
    SkMaskFilter::BlurInfo blurInfo;
    if (getBlurInfo(paint, blurInfo) != SkMaskFilter::kNone_BlurType) {
        bool ignoreTransforms = blurInfo.fIgnoreTransform;
        if (FloatUtils::isGreaterZero(blurInfo.fRadius)) {
            // FIXME: when using blur together with gradient/pattern something special needs to be done, but no test seen for this yet.
            float sb = blurInfo.fRadius * 2;
            SkPoint blur = SkPoint::Make(sb, sb);
            SkRect outerRect = rect;

            if (ignoreTransforms) {
                float xScale = !FloatUtils::isNearZero(fMatrix.getScaleX()) ? fMatrix.getScaleX() : fabs(fMatrix.getSkewY());
                float yScale = !FloatUtils::isNearZero(fMatrix.getScaleY()) ? fMatrix.getScaleY() : fabs(fMatrix.getSkewX());
                if (!FloatUtils::isNearZero(xScale) && !FloatUtils::isNearZero(xScale))
                    blur.set((1.0f / xScale) * blur.x(), (1.0f / yScale) * blur.y());
            }

            outerRect.outset(blur.x() / 2, blur.y() / 2);

            SkPoint pts[16] = { SkPoint::Make(outerRect.left(), outerRect.top()),
                                SkPoint::Make(outerRect.left() + blur.x(), outerRect.top()),
                                SkPoint::Make(outerRect.left() + blur.x(), outerRect.top() + blur.y()),
                                SkPoint::Make(outerRect.left(), outerRect.top() + blur.y()),
                                SkPoint::Make(outerRect.right(), outerRect.top()),
                                SkPoint::Make(outerRect.right(), outerRect.top() + blur.y()),
                                SkPoint::Make(outerRect.right() - blur.x(), outerRect.top() + blur.y()),
                                SkPoint::Make(outerRect.right() - blur.x(), outerRect.top()),
                                SkPoint::Make(outerRect.right(), outerRect.bottom()),
                                SkPoint::Make(outerRect.right() - blur.x(), outerRect.bottom()),
                                SkPoint::Make(outerRect.right() - blur.x(), outerRect.bottom() - blur.y()),
                                SkPoint::Make(outerRect.right(), outerRect.bottom() - blur.y()),
                                SkPoint::Make(outerRect.left(), outerRect.bottom()),
                                SkPoint::Make(outerRect.left(), outerRect.bottom() - blur.y()),
                                SkPoint::Make(outerRect.left() + blur.x(), outerRect.bottom() - blur.y()),
                                SkPoint::Make(outerRect.left() + blur.x(), outerRect.bottom()) };

            SkPoint tpts[16];
            fMatrix.mapPoints(tpts, pts, 16);

            // we nine slice the blur
            // Top Left
            Clip::Vertex v1(tpts[0].x(), tpts[0].y(), 1, 1);
            Clip::Vertex v2(tpts[1].x(), tpts[1].y(), 1, 0);
            Clip::Vertex v3(tpts[2].x(), tpts[2].y(), 0, 0);
            Clip::Vertex v4(tpts[3].x(), tpts[3].y(), 0, 1);
            // Top Right
            Clip::Vertex v5(tpts[4].x(), tpts[4].y(), 1, 1);
            Clip::Vertex v6(tpts[5].x(), tpts[5].y(), 0, 1);
            Clip::Vertex v7(tpts[6].x(), tpts[6].y(), 0, 0);
            Clip::Vertex v8(tpts[7].x(), tpts[7].y(), 1, 0);
            // Bottom Right
            Clip::Vertex v9(tpts[8].x(), tpts[8].y(), 1, 1);
            Clip::Vertex v10(tpts[9].x(), tpts[9].y(), 1, 0);
            Clip::Vertex v11(tpts[10].x(), tpts[10].y(), 0, 0);
            Clip::Vertex v12(tpts[11].x(), tpts[11].y(), 0, 1);
            // Bottom Left
            Clip::Vertex v13(tpts[12].x(), tpts[12].y(), 1, 1);
            Clip::Vertex v14(tpts[13].x(), tpts[13].y(), 0, 1);
            Clip::Vertex v15(tpts[14].x(), tpts[14].y(), 0, 0);
            Clip::Vertex v16(tpts[15].x(), tpts[15].y(), 1, 0);

            ArcGen gen(this, SkPaintToABGR32Premultiplied(paint), 0);
            gen.setInsertionTime(Inserter::Deferred);

            bool shouldClip = blur.x() > rect.width() || blur.y() > rect.height();
            const Clip oldClip = fClip;

            SkSize size = SkSize::Make(outerRect.width()/2, outerRect.height()/2);
            if (shouldClip) {
                clip(SkRect::MakeXYWH(outerRect.left(), outerRect.top(), size.width(), size.height()));
            }
            fClip.clipQuad(v1, v2, v3, v4, gen);
            if (shouldClip)
                fClip = oldClip;
            if (pts[1].x() < pts[7].x() && pts[1].y() < pts[2].y()) {
                if (FloatUtils::isLessZero(pts[14].y() - pts[2].y()))
                    clip(SkRect::MakeXYWH(pts[1].x(), pts[1].y(), pts[7].x() - pts[1].x(), size.height()));

                fClip.clipQuad(v3, v2, v8, v7, gen);
                fClip = oldClip;
            }

            if (shouldClip) {
                clip(SkRect::MakeXYWH(outerRect.right() - size.width(), outerRect.top(), size.width(), size.height()));
            }

            fClip.clipQuad(v8, v5, v6, v7, gen);
            if (shouldClip)
                fClip = oldClip;
            if (pts[6].x() < pts[5].x() && pts[6].y() < pts[10].y()) {
                if (FloatUtils::isLessZero(pts[6].x() - pts[2].x()))
                    clip(SkRect::MakeXYWH(pts[5].x() - size.width(), pts[6].y(), size.width(), pts[10].y() - pts[6].y()));

                fClip.clipQuad(v7, v6, v12, v11, gen);
                fClip = oldClip;
            }

            if (shouldClip) {
                clip(SkRect::MakeXYWH(outerRect.right() - size.width(), outerRect.bottom() - size.height(), size.width(), size.height()));
            }

            fClip.clipQuad(v12, v9, v10, v11, gen);
            if (shouldClip)
                fClip = oldClip;
            if (pts[14].x() < pts[10].x() && pts[14].y() < pts[15].y()) {
                if (FloatUtils::isLessZero(pts[14].y() - pts[2].y()))
                    clip(SkRect::MakeXYWH(pts[14].x(), pts[15].y() - size.height(), pts[10].x() - pts[14].x(), size.height()));

                fClip.clipQuad(v11, v10, v16, v15, gen);
                fClip = oldClip;
            }

            if (shouldClip) {
                clip(SkRect::MakeXYWH(outerRect.left(), outerRect.bottom() - size.height(), size.width(), size.height()));
            }
            fClip.clipQuad(v16, v13, v14, v15, gen);
            if (shouldClip)
                fClip = oldClip;
            if (pts[3].x() < pts[2].x() && pts[3].y() < pts[13].y()) {
                if (FloatUtils::isLessZero(pts[6].x() - pts[2].x()))
                    clip(SkRect::MakeXYWH(pts[3].x(), pts[3].y(), size.width(), pts[13].y() - pts[3].y()));

                fClip.clipQuad(v15, v14, v4, v3, gen);
                fClip = oldClip;
            }

            if (FloatUtils::isGreaterZero(pts[14].y() - pts[2].y()) && FloatUtils::isGreaterZero(pts[6].x() - pts[2].x())) {
                SkPoint pts2[4] = { tpts[2], tpts[6], tpts[10], tpts[14] };
                SolidColorGen solidGen(this, SkPaintToABGR32Premultiplied(paint));
                fClip.clipQuad(pts2, solidGen);
            }
        } else {
            if (paint.getShader() && paint.getShader()->asAGradient(0) != SkShader::kNone_GradientType) {
                pushLayer();
                SkPoint quad[4];
                fMatrix.mapRectToQuad(quad, rect);
                SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
                fClip.clipQuad(quad, gen);
                popLayer();
            } else {
                SkPoint quad[4];
                fMatrix.mapRectToQuad(quad, rect);
                SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
                fClip.clipQuad(quad, gen);
            }
        }
    } else {
        SkPoint quad[4];
        fMatrix.mapRectToQuad(quad, rect);
        SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
        fClip.clipQuad(quad, gen);
    }
}

void GentlContext::addStrokeRect(const SkRect& r, const SkPaint& paint)
{
    // strokerect has special rules for CSS when the rect is degenerate:
    // if width==0 && height==0, do nothing
    // if width==0 || height==0, then just draw line for the other dimension
    const bool validW = r.width() > 0;
    const bool validH = r.height() > 0;
    const float lw = strokeWidthRespectingHairline(paint);
    const float lw2 = lw / 2.0f;

    if (FloatUtils::isNearZero(lw2))
        return;

    if (validW && validH) {
        if (paint.getStrokeJoin() != SkPaint::kMiter_Join || paint.getPathEffect() || paint.getShader()) {
            GentlPath path;
            path.moveTo(SkPoint::Make(r.left(), r.top()));
            path.lineTo(SkPoint::Make(r.right(), r.top()));
            path.lineTo(SkPoint::Make(r.right(), r.bottom()));
            path.lineTo(SkPoint::Make(r.left(), r.bottom()));
            path.closeSubpath();
            path.stroke(this, paint);
        } else {
            SkRRect inner, outer;
            outer.setRect(r);
            outer.outset(lw2, lw2);
            outer.outset(-lw, -lw, &inner);
            SkRect innerSorted(inner.rect());
            innerSorted.sort();
            inner.setRect(innerSorted);
            addStrokeRRect(outer, inner, paint);
        }
    } else if (validW || validH) {
        // we are expected to respect the lineJoin, so we can't just call
        // drawLine -- we have to create a path that doubles back on itself.
        if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
            SkRect rect = r;
            rect.outset(lw2, lw2);
            SkRRect rRect;
            rRect.setRectXY(rect, lw2, lw2);
            addFillRRect(rRect, paint);
        } else {
            addDrawLine(SkIPoint::Make(r.x(), r.y()), SkIPoint::Make(r.right(), r.bottom()), paint, lw);
        }
    }
}

void GentlContext::addFillRRect(const SkRRect& rr, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    if (!hasVisibleFill(paint))
        return;

    if (rr.isRect()) {
        SkRect sorted(rr.rect());
        sorted.sort();
        addFillRect(sorted, paint);
        return;
    }

    SkMaskFilter::BlurInfo blurInfo;
    SkMaskFilter::BlurType blurType(getBlurInfo(paint, blurInfo));
    if (blurType != SkMaskFilter::kNone_BlurType && FloatUtils::isGreaterZero(blurInfo.fRadius)) {
        const float halfBlur = blurInfo.fRadius;
        const float blur = halfBlur * 2;
        const float minimum = std::min(rr.rect().width(), rr.rect().height());
        SkRRect outer(rr);
        outer.outset(halfBlur, halfBlur);

        SkRRect inner(outer);
        inner.inset(blur, blur);
        unsigned alpha = SkColorGetA(paint.getColor());
        if (inner.rect().width() < 0 && inner.rect().height() < 0) {
            const float delta = blur - minimum;
            if (FloatUtils::isGreaterZero(delta)) {
                // since the blur is larger than the shape we can keep drawing the shape as it is and just reduce alpha.
                alpha = (unsigned)(alpha * (minimum / (blur + delta)));

            }
            if (inner.rect().width() < 0) {
                inner.setRect(SkRect::MakeLTRB((inner.rect().left() + inner.rect().right()) / 2, inner.rect().top(), (inner.rect().left() + inner.rect().right()) / 2, inner.rect().bottom()));
            }
            if (inner.rect().height() < 0) {
                inner.setRect(SkRect::MakeLTRB(inner.rect().left(), (inner.rect().top() + inner.rect().bottom()) / 2, inner.rect().right(), (inner.rect().top() + inner.rect().bottom()) / 2));
            }
        }

        const Clip oldClip = fClip;
        if (blurType == SkMaskFilter::kOuter_BlurType || blurType == SkMaskFilter::kSolid_BlurType) {
            clipOutRoundedRect(rr);
        } else if (blurType == SkMaskFilter::kInner_BlurType) {
            clipRoundedRect(rr);
        }

        SkPoint points[4] = {
            SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kUpperLeft_Corner).x(), inner.rect().y() + inner.radii(SkRRect::kUpperLeft_Corner).y()),
            SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kUpperRight_Corner).x(), inner.rect().y() + inner.radii(SkRRect::kUpperRight_Corner).y()),
            SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kLowerRight_Corner).x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerRight_Corner).y()),
            SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerLeft_Corner).y())
        };

        SkPoint quad1Point1(SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kUpperRight_Corner).x(), inner.rect().y()));
        SkPoint quad1Point2(SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kUpperLeft_Corner).x(), inner.rect().y()));

        SkPoint quad2Point1(SkPoint::Make(inner.rect().right(), inner.rect().bottom() - inner.radii(SkRRect::kLowerRight_Corner).y()));
        SkPoint quad2Point2(SkPoint::Make(inner.rect().right(), inner.rect().y() + inner.radii(SkRRect::kUpperRight_Corner).y()));

        SkPoint quad3Point1(SkPoint::Make(inner.rect().x(), inner.rect().y() + inner.radii(SkRRect::kUpperLeft_Corner).y()));
        SkPoint quad3Point2(SkPoint::Make(inner.rect().x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerLeft_Corner).y()));

        SkPoint quad4Point1(SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.rect().bottom()));
        SkPoint quad4Point2(SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kLowerRight_Corner).x(), inner.rect().bottom()));

        if (alpha) {
            SkPaint alphaPaint(paint);
            alphaPaint.setAlpha(alpha);
            ArcGen gen(this, SkPaintToABGR32Premultiplied(alphaPaint), false);

            addSmoothQuad(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kUpperLeft_Corner).x(), outer.rect().y()),
                               SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kUpperRight_Corner).x(), outer.rect().y()),
                               quad1Point1,
                               quad1Point2);
            addSmoothQuad(gen, SkPoint::Make(outer.rect().right(), outer.rect().y() + outer.radii(SkRRect::kUpperRight_Corner).y()),
                               SkPoint::Make(outer.rect().right(), outer.rect().bottom() - outer.radii(SkRRect::kLowerRight_Corner).y()),
                               quad2Point1,
                               quad2Point2);
            addSmoothQuad(gen, SkPoint::Make(outer.rect().x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerLeft_Corner).y()),
                               SkPoint::Make(outer.rect().x(), outer.rect().y() + outer.radii(SkRRect::kUpperLeft_Corner).y()),
                               quad3Point1,
                               quad3Point2);
            addSmoothQuad(gen, SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kLowerRight_Corner).x(), outer.rect().bottom()),
                               SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.rect().bottom()),
                               quad4Point1,
                               quad4Point2);


            addSmoothArc(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kUpperLeft_Corner).x(), outer.rect().y() + outer.radii(SkRRect::kUpperLeft_Corner).y()),
                              SkPoint::Make(-outer.radii(SkRRect::kUpperLeft_Corner).x(), -outer.radii(SkRRect::kUpperLeft_Corner).y()),
                              points[0],
                              SkPoint::Make(-inner.radii(SkRRect::kUpperLeft_Corner).x(), -inner.radii(SkRRect::kUpperLeft_Corner).y()), paint);
            addSmoothArc(gen, SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kUpperRight_Corner).x(), outer.rect().y() + outer.radii(SkRRect::kUpperRight_Corner).y()),
                              SkPoint::Make(outer.radii(SkRRect::kUpperRight_Corner).x(), -outer.radii(SkRRect::kUpperRight_Corner).y()),
                              points[1],
                              SkPoint::Make(inner.radii(SkRRect::kUpperRight_Corner).x(), -inner.radii(SkRRect::kUpperRight_Corner).y()), paint);
            addSmoothArc(gen, SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kLowerRight_Corner).x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerRight_Corner).y()),
                              SkPoint::Make(outer.radii(SkRRect::kLowerRight_Corner).x(), outer.radii(SkRRect::kLowerRight_Corner).y()),
                              points[2],
                              SkPoint::Make(inner.radii(SkRRect::kLowerRight_Corner).x(), inner.radii(SkRRect::kLowerRight_Corner).y()), paint);
            addSmoothArc(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerLeft_Corner).y()),
                              SkPoint::Make(-outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.radii(SkRRect::kLowerLeft_Corner).y()),
                              points[3],
                              SkPoint::Make(-inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.radii(SkRRect::kLowerLeft_Corner).y()), paint);
        }

        if (blurType == SkMaskFilter::kOuter_BlurType || blurType == SkMaskFilter::kInner_BlurType || blurType == SkMaskFilter::kSolid_BlurType) {
            fClip = oldClip;
        }
        /**
         * This illustrates what the points for the solid quads drawn.

         +-----------------------------+
         |       |             |       |
         |     Q1P1___________Q1P2     |
         |       |             |       |
         |       |  SolidQuad1 |       |
         +-Q3P2-P0------------P1--Q2P2-+
         |   |   |             |   |   |
         |   |   |             |   |   |
         |   |SQ3|             |SQ2|   |
         |   |   |   points[]  |   |   |
         |   |   |    Quad     |   |   |
         |   |   |             |   |   |
         |   |   |             |   |   |
         +-Q3P1-P3------------P2--Q2P1-+
         |       |  SolidQuad4 |       |
         |     Q4P1__________Q4P2      |
         |       |             |       |
         |       |             |       |
         +-----------------------------+
         */
        SolidColorGen solid(this, SkPaintToABGR32Premultiplied(paint));
        SkPoint quadPoints[] = { quad1Point1, quad1Point2, quad2Point1, quad2Point2, quad3Point1, quad3Point2, quad4Point1, quad4Point2 };
        fMatrix.mapPoints(points, 4);
        fMatrix.mapPoints(quadPoints, 8);
        SkPoint solidQuad1[] = { quadPoints[1], quadPoints[0], points[1], points[0] };
        SkPoint solidQuad2[] = { quadPoints[3], quadPoints[2], points[2], points[1] };
        SkPoint solidQuad3[] = { quadPoints[5], quadPoints[4], points[0], points[3] };
        SkPoint solidQuad4[] = { quadPoints[7], quadPoints[6], points[3], points[2] };
        fClip.clipQuad(points, solid);
        fClip.clipQuad(solidQuad1, solid);
        fClip.clipQuad(solidQuad2, solid);
        fClip.clipQuad(solidQuad3, solid);
        fClip.clipQuad(solidQuad4, solid);
        if (blurType == SkMaskFilter::kSolid_BlurType) {
            SkPaint noBlur(paint);
            noBlur.setMaskFilter(0);
            addFillRRect(rr, noBlur);
        }
    } else {
        // find the end points of the arcs we're about to draw.
        const SkPoint tl1 = SkPoint::Make(rr.rect().left() + rr.radii(SkRRect::kUpperLeft_Corner).x(), rr.rect().top());
        const SkPoint tl2 = SkPoint::Make(rr.rect().left(), rr.rect().top() + rr.radii(SkRRect::kUpperLeft_Corner).y());
        const SkPoint tr1 = SkPoint::Make(rr.rect().right() - rr.radii(SkRRect::kUpperRight_Corner).x(), rr.rect().top());
        const SkPoint tr2 = SkPoint::Make(rr.rect().right(), rr.rect().top() + rr.radii(SkRRect::kUpperRight_Corner).y());
        const SkPoint bl1 = SkPoint::Make(rr.rect().left() + rr.radii(SkRRect::kLowerLeft_Corner).x(), rr.rect().bottom());
        const SkPoint bl2 = SkPoint::Make(rr.rect().left(), rr.rect().bottom() - rr.radii(SkRRect::kLowerLeft_Corner).y());
        const SkPoint br1 = SkPoint::Make(rr.rect().right() - rr.radii(SkRRect::kLowerRight_Corner).x(), rr.rect().bottom());
        const SkPoint br2 = SkPoint::Make(rr.rect().right(), rr.rect().bottom() - rr.radii(SkRRect::kLowerRight_Corner).y());

        // fill the space between the arcs with a triangle fan.
        SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
        addSolidQuad(gen, tl1, tr1, tr2, br2);
        addSolidQuad(gen, tl1, br2, br1, bl1);
        addSolidQuad(gen, tl1, bl1, bl2, tl2);

        SkPoint quad[4];
        rr.rect().toQuad(quad);

        addRound(gen, tl1, tl2, quad[0]);
        addRound(gen, tr1, tr2, quad[1]);
        addRound(gen, br1, br2, quad[2]);
        addRound(gen, bl1, bl2, quad[3]);
    }
}

void GentlContext::addStrokeRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint& paint)
{
    SkMaskFilter::BlurInfo blurInfo;
    if (getBlurInfo(paint, blurInfo) != SkMaskFilter::kNone_BlurType && FloatUtils::isGreaterZero(blurInfo.fRadius)) {
        SkPoint blur = SkPoint::Make(blurInfo.fRadius * 2, blurInfo.fRadius * 2);

        SkRect outerRect = inner.rect();
        outerRect.outset(blur.x() / 2, blur.y() / 2);
        // see addFillRect for a reference to what the points 1 through 16 represent.
        SkPoint pts[16] = { SkPoint::Make(outerRect.left(), outerRect.top()),
                            SkPoint::Make(outerRect.left() + blur.x(), outerRect.top()),
                            SkPoint::Make(outerRect.left() + blur.x(), outerRect.top() + blur.y()),
                            SkPoint::Make(outerRect.left(), outerRect.top() + blur.y()),
                            SkPoint::Make(outerRect.right(), outerRect.top()),
                            SkPoint::Make(outerRect.right(), outerRect.top() + blur.y()),
                            SkPoint::Make(outerRect.right() - blur.x(), outerRect.top() + blur.y()),
                            SkPoint::Make(outerRect.right() - blur.x(), outerRect.top()),
                            SkPoint::Make(outerRect.right(), outerRect.top()),
                            SkPoint::Make(outerRect.right() - blur.x(), outerRect.bottom()),
                            SkPoint::Make(outerRect.right() - blur.x(), outerRect.bottom() - blur.y()),
                            SkPoint::Make(outerRect.right(), outerRect.bottom() - blur.y()),
                            SkPoint::Make(outerRect.left(), outerRect.bottom()),
                            SkPoint::Make(outerRect.left(), outerRect.bottom() - blur.y()),
                            SkPoint::Make(outerRect.left() + blur.x(), outerRect.bottom() - blur.y()),
                            SkPoint::Make(outerRect.left() + blur.x(), outerRect.bottom()) };

        SkPoint tpts[16];
        fMatrix.mapPoints(tpts, pts, 16);

        // we nine slice the blur
        // Top Left
        Clip::Vertex v1(tpts[0].x(), tpts[0].y(), 1, 1);
        Clip::Vertex v2(tpts[1].x(), tpts[1].y(), 1, 0);
        Clip::Vertex v3(tpts[2].x(), tpts[2].y(), 0, 0);
        Clip::Vertex v4(tpts[3].x(), tpts[3].y(), 0, 1);
        // Top Right
        Clip::Vertex v5(tpts[4].x(), tpts[4].y(), 1, 1);
        Clip::Vertex v6(tpts[5].x(), tpts[5].y(), 0, 1);
        Clip::Vertex v7(tpts[6].x(), tpts[6].y(), 0, 0);
        Clip::Vertex v8(tpts[7].x(), tpts[7].y(), 1, 0);
        // Bottom Right
        Clip::Vertex v9(tpts[8].x(), tpts[8].y(), 1, 1);
        Clip::Vertex v10(tpts[9].x(), tpts[9].y(), 1, 0);
        Clip::Vertex v11(tpts[10].x(), tpts[10].y(), 0, 0);
        Clip::Vertex v12(tpts[11].x(), tpts[11].y(), 0, 1);
        // Bottom Left
        Clip::Vertex v13(tpts[12].x(), tpts[12].y(), 1, 1);
        Clip::Vertex v14(tpts[13].x(), tpts[13].y(), 0, 1);
        Clip::Vertex v15(tpts[14].x(), tpts[14].y(), 0, 0);
        Clip::Vertex v16(tpts[15].x(), tpts[15].y(), 1, 0);

        ArcGen gen(this, SkPaintToABGR32Premultiplied(paint), 1.0);
        gen.setInsertionTime(Inserter::Deferred);

        bool shouldClip = blur.x() > inner.rect().width() || blur.y() > inner.rect().height();
        const Clip oldClip = fClip;

        SkSize size = SkSize::Make(outerRect.width()/2, outerRect.height()/2);
        if (shouldClip) {
            clip(SkRect::MakeXYWH(outerRect.left(), outerRect.top(), size.width(), size.height()));
        }
        fClip.clipQuad(v1, v2, v3, v4, gen);
        if (shouldClip)
            fClip = oldClip;
        if (pts[1].x() < pts[7].x() && pts[1].y() < pts[2].y()) {
            if (FloatUtils::isLessZero(pts[14].y() - pts[2].y()))
                clip(SkRect::MakeXYWH(pts[1].x(), pts[1].y(), pts[7].x() - pts[1].x(), size.height()));

            fClip.clipQuad(v3, v2, v8, v7, gen);
            fClip = oldClip;
        }

        if (shouldClip) {
            clip(SkRect::MakeXYWH(outerRect.right() - size.width(), outerRect.top(), size.width(), size.height()));
        }
        fClip.clipQuad(v8, v5, v6, v7, gen);
        if (shouldClip)
            fClip = oldClip;
        if (pts[6].x() < pts[5].x() && pts[6].y() < pts[10].y()) {
            if (FloatUtils::isLessZero(pts[6].x() - pts[2].x()))
                clip(SkRect::MakeXYWH(pts[5].x() - size.width(), pts[6].y(), size.width(), pts[10].y() - pts[6].y()));

            fClip.clipQuad(v7, v6, v12, v11, gen);
            fClip = oldClip;
        }

        if (shouldClip) {
            clip(SkRect::MakeXYWH(outerRect.right() - size.width(), outerRect.bottom() - size.height(), size.width(), size.height()));
        }
        fClip.clipQuad(v12, v9, v10, v11, gen);
        if (shouldClip)
            fClip = oldClip;
        if (pts[14].x() < pts[10].x() && pts[14].y() < pts[15].y()) {
            if (FloatUtils::isLessZero(pts[14].y() - pts[2].y()))
                clip(SkRect::MakeXYWH(pts[14].x(), pts[15].y() - size.height(), pts[10].x() - pts[14].x(), size.height()));

            fClip.clipQuad(v11, v10, v16, v15, gen);
            fClip = oldClip;
        }

        if (shouldClip) {
            clip(SkRect::MakeXYWH(outerRect.left(), outerRect.bottom() - size.height(), size.width(), size.height()));
        }
        fClip.clipQuad(v16, v13, v14, v15, gen);
        if (shouldClip)
            fClip = oldClip;
        if (pts[3].x() < pts[2].x() && pts[3].y() < pts[13].y()) {
            if (FloatUtils::isLessZero(pts[6].x() - pts[2].x()))
                clip(SkRect::MakeXYWH(pts[3].x(), pts[3].y(), size.width(), pts[13].y() - pts[3].y()));

            fClip.clipQuad(v15, v14, v4, v3, gen);
            fClip = oldClip;
        }

        // we need to fill in the outside of the inner blur if there's a gap between our blur and the bounding box of the rect.
        // left quad
        SkPoint outerPts[8] = { SkPoint::Make(outer.rect().left(), outer.rect().top()),
                                SkPoint::Make(outerRect.left(), outerRect.top()),
                                SkPoint::Make(outerRect.left(), outerRect.bottom()),
                                SkPoint::Make(outer.rect().left(), outer.rect().bottom()),
                                // right quad
                                SkPoint::Make(outerRect.right(), outerRect.top()),
                                SkPoint::Make(outer.rect().right(), outer.rect().top()),
                                SkPoint::Make(outer.rect().right(), outer.rect().bottom()),
                                SkPoint::Make(outerRect.right(), outerRect.bottom()) };
        fMatrix.mapPoints(outerPts, 8);

        const SkPoint topPts[4] = { outerPts[0], outerPts[6], outerPts[4], outerPts[1] };
        const SkPoint bottomPts[4] = { outerPts[2], outerPts[7], outerPts[6], outerPts[3] };

        SolidColorGen solid(this, SkPaintToABGR32Premultiplied(paint));
        fClip.clipQuad(&outerPts[0], solid);
        fClip.clipQuad(topPts, solid);
        fClip.clipQuad(&outerPts[4], solid);
        fClip.clipQuad(bottomPts, solid);
    } else {
        SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
        addSolidQuad(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kUpperLeft_Corner).x(), outer.rect().y()),
                          SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kUpperRight_Corner).x(), outer.rect().y()),
                          SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kUpperRight_Corner).x(), inner.rect().y()),
                          SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kUpperLeft_Corner).x(), inner.rect().y()));
        addSolidQuad(gen, SkPoint::Make(outer.rect().right(), outer.rect().y() + outer.radii(SkRRect::kUpperRight_Corner).y()),
                          SkPoint::Make(outer.rect().right(), outer.rect().bottom() - outer.radii(SkRRect::kLowerRight_Corner).y()),
                          SkPoint::Make(inner.rect().right(), inner.rect().bottom() - inner.radii(SkRRect::kLowerRight_Corner).y()),
                          SkPoint::Make(inner.rect().right(), inner.rect().y() + inner.radii(SkRRect::kUpperRight_Corner).y()));
        addSolidQuad(gen, SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.rect().bottom()),
                          SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kLowerRight_Corner).x(), inner.rect().bottom()),
                          SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kLowerRight_Corner).x(), outer.rect().bottom()),
                          SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.rect().bottom()));
        addSolidQuad(gen, SkPoint::Make(inner.rect().x(), inner.rect().y() + inner.radii(SkRRect::kUpperLeft_Corner).y()),
                          SkPoint::Make(inner.rect().x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerLeft_Corner).y()),
                          SkPoint::Make(outer.rect().x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerLeft_Corner).y()),
                          SkPoint::Make(outer.rect().x(), outer.rect().y() + outer.radii(SkRRect::kUpperLeft_Corner).y()));

        addSolidArc(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kUpperLeft_Corner).x(), outer.rect().y() + outer.radii(SkRRect::kUpperLeft_Corner).y()),
                         SkPoint::Make(-outer.radii(SkRRect::kUpperLeft_Corner).x(), -outer.radii(SkRRect::kUpperLeft_Corner).y()),
                         SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kUpperLeft_Corner).x(), inner.rect().y() + inner.radii(SkRRect::kUpperLeft_Corner).y()),
                         SkPoint::Make(-inner.radii(SkRRect::kUpperLeft_Corner).x(), -inner.radii(SkRRect::kUpperLeft_Corner).y()));
        addSolidArc(gen, SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kUpperRight_Corner).x(), outer.rect().y() + outer.radii(SkRRect::kUpperRight_Corner).y()),
                         SkPoint::Make(outer.radii(SkRRect::kUpperRight_Corner).x(), -outer.radii(SkRRect::kUpperRight_Corner).y()),
                         SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kUpperRight_Corner).x(), inner.rect().y() + inner.radii(SkRRect::kUpperRight_Corner).y()),
                         SkPoint::Make(inner.radii(SkRRect::kUpperRight_Corner).x(), -inner.radii(SkRRect::kUpperRight_Corner).y()));
        addSolidArc(gen, SkPoint::Make(outer.rect().x() + outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerLeft_Corner).y()),
                         SkPoint::Make(-outer.radii(SkRRect::kLowerLeft_Corner).x(), outer.radii(SkRRect::kLowerLeft_Corner).y()),
                         SkPoint::Make(inner.rect().x() + inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerLeft_Corner).y()),
                         SkPoint::Make(-inner.radii(SkRRect::kLowerLeft_Corner).x(), inner.radii(SkRRect::kLowerLeft_Corner).y()));
        addSolidArc(gen, SkPoint::Make(outer.rect().right() - outer.radii(SkRRect::kLowerRight_Corner).x(), outer.rect().bottom() - outer.radii(SkRRect::kLowerRight_Corner).y()),
                         SkPoint::Make(outer.radii(SkRRect::kLowerRight_Corner).x(), outer.radii(SkRRect::kLowerRight_Corner).y()),
                         SkPoint::Make(inner.rect().right() - inner.radii(SkRRect::kLowerRight_Corner).x(), inner.rect().bottom() - inner.radii(SkRRect::kLowerRight_Corner).y()),
                         SkPoint::Make(inner.radii(SkRRect::kLowerRight_Corner).x(), inner.radii(SkRRect::kLowerRight_Corner).y()));
    }
}

void GentlContext::addDrawLine(const SkIPoint& point1, const SkIPoint& point2, const SkPaint& paint, float w)
{
    if (!isDrawingEnabled() || !hasVisibleStroke(paint))
        return;

    // If we have special dashes just create a Path. It would simplify the code a lot
    // to always do this, but we need to investigate what the runtime cost would be.
    if (paint.getPathEffect()) {
        GentlPath path;
        path.moveTo(SkPoint::Make(point1.x(), point1.y()));
        path.lineTo(SkPoint::Make(point2.x(), point2.y()));
        path.stroke(this, paint);
        return;
    }

    const SkPoint normVec = SkPoint::Make(point2.x() - point1.x(), point2.y() - point1.y());
    const float length = std::sqrt(normVec.x() * normVec.x() + normVec.y() * normVec.y());
    if (length < FLT_EPSILON)
        return;

    SkPoint perpVec = SkPoint::Make(-normVec.y() / length, normVec.x() / length);

    // Skia + Webkit trick, copy & past + some rotation adjustment.
    // For odd widths, we add in 0.5 to the appropriate x/y so that the float arithmetic
    // works out.  For example, with a border width of 3, WebKit will pass us (y1+y2)/2, e.g.,
    // (50+53)/2 = 103/2 = 51 when we want 51.5.  It is always true that an even width gave
    // us a perfect position, but an odd width gave us a position that is off by exactly 0.5.
    float dx = 0, dy = 0;
    if (static_cast<int>(w) % 2) { //odd
        dx = 0.5 * normVec.x() / length;
        dy = 0.5 * normVec.y() / length;
    }

    const float w2 = w / 2.f;
    SkPoint quad[4] = { SkPoint::Make(point1.x() + perpVec.x() * w2 - dx, point1.y() + perpVec.y() * w2 - dy),
                        SkPoint::Make(point2.x() + perpVec.x() * w2 - dx, point2.y() + perpVec.y() * w2 - dy),
                        SkPoint::Make(point2.x() - perpVec.x() * w2 - dx, point2.y() - perpVec.y() * w2 - dy),
                        SkPoint::Make(point1.x() - perpVec.x() * w2 - dx, point1.y() - perpVec.y() * w2 - dy)};
    fMatrix.mapPoints(quad, 4);
    SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));
    fClip.clipQuad(quad, gen);
}

void GentlContext::paintPath(const SkPath& skPath, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    const GentlPath path(skPath);

    if (hasVisibleFill(paint)) {
        path.fill(this, paint);
    }

    if (hasVisibleStroke(paint)) {
        SkPaint hairline(paint);
        hairline.setStrokeWidth(strokeWidthRespectingHairline(paint));
        path.stroke(this, hairline);
    }
}

void GentlContext::clearRect(const SkRect& rect, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    SkXfermode::Mode oldXferMode = fXfermode;
    setXfermodeMode(SkXfermode::kClear_Mode);
    addFillRect(rect, paint);
    fXfermode = oldXferMode;
}

void GentlContext::paintBitmap(const SkRect& dst, const SkRect& src, const SkBitmap& bitmap, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    GrTexture* texture = 0;
    SkAutoCachedTexture act(fGrContext, bitmap, 0, &texture);
    if (!texture) {
        SkDebugf("Could not convert source bitmap into GrTexture.\n");
        return;
    }

    SkMaskFilter::BlurInfo blurInfo;
    bool doBlur = getBlurInfo(paint, blurInfo) != SkMaskFilter::kNone_BlurType;
    if (doBlur) {
        pushLayer();
        pushLayer();
    }

    SkPoint quad[4];
    fMatrix.mapRectToQuad(quad, dst);

    TextureGen gen(this, texture, paint.getFilterLevel());
    fClip.clipQuad(Clip::Vertex(quad[0].x(), quad[0].y(), src.left() /  texture->width(), src.top() /    texture->height()),
                   Clip::Vertex(quad[1].x(), quad[1].y(), src.right() / texture->width(), src.top() /    texture->height()),
                   Clip::Vertex(quad[2].x(), quad[2].y(), src.right() / texture->width(), src.bottom() / texture->height()),
                   Clip::Vertex(quad[3].x(), quad[3].y(), src.left() /  texture->width(), src.bottom() / texture->height()),
                   gen);

    if (doBlur) {
        popLayer(1.0f, SkSize::Make(1.0f, 0), blurInfo.fRadius);
        popLayer(1.0f, SkSize::Make(0, 1.0f), blurInfo.fRadius);
    }
}

// Glyph Specific /////////////////////////////////////////////////////////////

bool GentlContext::setupText(GrContext* grContext, const SkDeviceProperties& deviceProperties, const SkPaint& paint)
{
    if (paint.getTextSize() > MaximumSupportedTextSize)
        return false;

    if (!fText) {
        fText = new GentlTextState();
        fText->grContext = grContext;
        fText->glyphCache = GlyphCache::getGlyphCacheForGpu(grContext->getGpu());
        fText->deviceProperties = &deviceProperties;
        fText->ratio = 1.0f;
        fText->insideCutoff = 0.0f;
        fText->outsideCutoff = 0.0f;
        fText->halfStroke = 1.0f;
    }

    fText->paint = paint;
    fText->color = SkPaintToABGR32Premultiplied(paint);

#if SK_DISTANCEFIELD_FONTS
    // TODO: Investigate where these numbers should really come from in an edtaa world.
    static const float inside = 0.1f;
    static const float outside = -0.1f;

    fText->ratio = paint.getTextSize() / DistanceFieldTextSize;
    fText->insideCutoff = inside / fText->ratio;
    fText->outsideCutoff = outside / fText->ratio;
    fText->halfStroke = strokeWidthRespectingHairline(fText->paint) * 0.5 * (fText->insideCutoff - fText->outsideCutoff) / fText->ratio;
    fText->paint.setTextSize(DistanceFieldTextSize);
    fText->paint.setLCDRenderText(false);
    fText->paint.setAutohinted(false);
    fText->paint.setSubpixelText(false);
#endif

    return true;
}

void GentlContext::paintGlyph(const SkGlyph& skGlyph, const SkPoint& pt, SkGlyphCache* skGlyphCache, const SkPaint& paint)
{
    const uint32_t fontHash = SkTypeface::UniqueID(paint.getTypeface());
    const Glyph* glyph = fText->glyphCache->get(skGlyph, skGlyphCache, fontHash, fText->grContext);
    if (!glyph)
        return;

    // Transform the glyph box into a quad with our matrix after applying the ratio
    // from the distance field size to the desired font height. Then clip the quad.
    SkRect rect = SkRect::MakeXYWH(glyph->box.x() * fText->ratio + pt.x(),
                                   glyph->box.y() * fText->ratio + pt.y(),
                                   glyph->box.width() * fText->ratio,
                                   glyph->box.height() * fText->ratio);
    SkPoint quad[4];
    fMatrix.mapRectToQuad(quad, rect);

    Clip::Vertex v[] = { Clip::Vertex(quad[0].x(), quad[0].y(), glyph->texcoords.left(), glyph->texcoords.top()),
                         Clip::Vertex(quad[1].x(), quad[1].y(), glyph->texcoords.right(), glyph->texcoords.top()),
                         Clip::Vertex(quad[2].x(), quad[2].y(), glyph->texcoords.right(), glyph->texcoords.bottom()),
                         Clip::Vertex(quad[3].x(), quad[3].y(), glyph->texcoords.left(), glyph->texcoords.bottom()) };

#if SK_DISTANCEFIELD_FONTS
    //TODO: calculate the proper cut offs.
    if (fText->paint.getStyle() == SkPaint::kFill_Style) {
        EdgeGlyphGen gen(this, glyph->page->texture(), fText->color, fText->insideCutoff, fText->outsideCutoff);
        fClip.clipQuad(v[0], v[1], v[2], v[3], gen);
    } else {
        const ABGR32Premultiplied fillColor = paint.getStyle() == SkPaint::kStrokeAndFill_Style ? fText->color : 0;
        StrokedEdgeGlyphGen gen(this, glyph->page->texture(), fillColor, fText->color, fText->halfStroke, fText->insideCutoff, fText->outsideCutoff);
        fClip.clipQuad(v[0], v[1], v[2], v[3], gen);
    }
#else
    BitmapGlyphGen gen(this, glyph->page->texture(), fText->color);
    fClip.clipQuad(v[0], v[1], v[2], v[3], gen);
#endif
}

void GentlContext::paintText(const char* text, size_t byteLength, const SkPoint& point, const SkPaint& paint)
{
    SkASSERT(byteLength == 0 || text != NULL);
    if (!text || !byteLength)
        return;

    float x = point.x();
    float y = point.y();

    SkDrawCacheProc  glyphCacheProc = fText->paint.getDrawCacheProc();
    SkAutoGlyphCache autoCache(fText->paint, fText->deviceProperties, NULL);
    SkGlyphCache*    cache = autoCache.getCache();

    // need to measure first
    // TODO - generate positions and pre-load cache as well?
    const char* stop = text + byteLength;
    if (paint.getTextAlign() != SkPaint::kLeft_Align) {
        SkFixed    stopX = 0;
        SkFixed    stopY = 0;

        const char* textPtr = text;
        while (textPtr < stop) {
            // don't need x, y here, since all subpixel variants will have the
            // same advance
            const SkGlyph& glyph = glyphCacheProc(cache, &textPtr, 0, 0);

            stopX += glyph.fAdvanceX;
            stopY += glyph.fAdvanceY;
        }
        SkASSERT(textPtr == stop);

        SkScalar alignX = SkFixedToScalar(stopX) * fText->ratio;
        SkScalar alignY = SkFixedToScalar(stopY) * fText->ratio;

        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            alignX = SkScalarHalf(alignX);
            alignY = SkScalarHalf(alignY);
        }

        x -= alignX;
        y -= alignY;
    }

    SkFixed fx = SkScalarToFixed(x);
    SkFixed fy = SkScalarToFixed(y);
    SkFixed fixedScale = SkScalarToFixed(fText->ratio);
    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, fx, fy);

        if (glyph.fWidth) {
            paintGlyph(glyph, SkPoint::Make(SkFixedToScalar(fx), SkFixedToScalar(fy)), cache, paint);
        }

        fx += SkFixedMul_portable(glyph.fAdvanceX, fixedScale);
        fy += SkFixedMul_portable(glyph.fAdvanceY, fixedScale);
    }
}

void GentlContext::paintPosText(const char* text, size_t byteLength, const SkScalar pos[], SkScalar constY, int scalarsPerPosition, const SkPaint& paint)
{
    SkASSERT(byteLength == 0 || text != NULL);
    SkASSERT(1 == scalarsPerPosition || 2 == scalarsPerPosition);
    if (!text || !byteLength)
        return;

    SkDrawCacheProc  glyphCacheProc = fText->paint.getDrawCacheProc();
    SkAutoGlyphCache autoCache(fText->paint, fText->deviceProperties, NULL);
    SkGlyphCache*    cache = autoCache.getCache();

    const char*        stop = text + byteLength;

    if (SkPaint::kLeft_Align == paint.getTextAlign()) {
        while (text < stop) {
            // the last 2 parameters are ignored
            const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

            if (glyph.fWidth) {
                SkScalar x = pos[0];
                SkScalar y = scalarsPerPosition == 1 ? constY : pos[1];

                paintGlyph(glyph, SkPoint::Make(x, y), cache, paint);
            }
            pos += scalarsPerPosition;
        }
    } else {
        int alignShift = SkPaint::kCenter_Align == paint.getTextAlign() ? 1 : 0;
        while (text < stop) {
            // the last 2 parameters are ignored
            const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

            if (glyph.fWidth) {
                SkScalar x = pos[0] - SkFixedToFloat(glyph.fAdvanceX >> alignShift);
                SkScalar y = (scalarsPerPosition == 1 ? constY : pos[1]) - SkFixedToFloat(glyph.fAdvanceY >> alignShift);

                paintGlyph(glyph, SkPoint::Make(x, y), cache, paint);
            }
            pos += scalarsPerPosition;
        }
    }
}

// Gentl Internal /////////////////////////////////////////////////////////////

void GentlContext::flush(DisplayList* displayList)
{
    if (fInstructions.isEmpty())
        return;

    if (((fInstructions.commandSize() > LowCommandWatermark && fInstructions.dataBytes() > LowDataWatermark)
        || fInstructions.dataBytes() > HighDataWatermark)) {
        displayList->setCurrentBacking(AlphaBacking);
    }

    // Check if all transparency layers have been ended and if not, discard
    // the unfinished ones.
    if (!fLayers.empty() && fLayers[0].command() != -1)
        fInstructions.unrefAndResize(fLayers[0].command());

    fLayers.clear();

    // If there are still commands in the front display list (it hasn't been drawn into
    // the backing surface) we need to append the current batch to what's already there.
    if (!displayList->instructions().isEmpty())
        displayList->instructions().transferFrom(fInstructions);
    else
        fInstructions.swapWith(displayList->instructions()); // efficient

    updateDrawingEnabledState(); // dependent on fLayers.top(), among others
}

bool GentlContext::hasVisibleFill(const SkPaint& paint)
{
    return (paint.getStyle() == SkPaint::kFill_Style || paint.getStyle() == SkPaint::kStrokeAndFill_Style)
           && (paint.getAlpha() || !SkXfermode::IsMode(paint.getXfermode(), SkXfermode::kSrcOver_Mode));
}

bool GentlContext::hasVisibleStroke(const SkPaint& paint)
{
    return (paint.getStyle() == SkPaint::kStrokeAndFill_Style || paint.getStyle() == SkPaint::kStroke_Style)
           && (paint.getAlpha() || !SkXfermode::IsMode(paint.getXfermode(), SkXfermode::kSrcOver_Mode));
}

float GentlContext::strokeWidthRespectingHairline(const SkPaint& paint) const
{
    if (paint.getStrokeWidth())
        return paint.getStrokeWidth();

    SkMatrix inverse;
    if (!fMatrix.isIdentity() && fMatrix.invert(&inverse)) {
        return inverse.mapRadius(1);
    }
    return 1;
}

void GentlContext::updateDrawingEnabledState()
{
    if ((!fLayers.empty() && fLayers.back().command() == -1)
        || (fXfermode == SkXfermode::kSrcOver_Mode && FloatUtils::isNearZero(fAlpha)))
        fDrawingEnabled = false;
    else
        fDrawingEnabled = true;
}

void GentlContext::addSolidQuad(SolidColorGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3, const SkPoint& p4)
{
    SkPoint quad[4] = { p1, p2, p3, p4 };
    fMatrix.mapPoints(quad, 4);
    fClip.clipQuad(quad, gen); // The texture coordinates will not matter for a SolidColorGen.
}

void GentlContext::addSmoothQuad(ArcGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3, const SkPoint& p4)
{
    SkPoint quad[4] = { p1, p2, p3, p4 };
    fMatrix.mapPoints(quad, 4);

    Clip::Vertex v1(quad[0].x(), quad[0].y(), 1, 0);
    Clip::Vertex v2(quad[1].x(), quad[1].y(), 1, 0);
    Clip::Vertex v3(quad[2].x(), quad[2].y(), 0, 0);
    Clip::Vertex v4(quad[3].x(), quad[3].y(), 0, 0);

    fClip.clipQuad(v1, v2, v3, v4, gen);
}

void GentlContext::addSolidArc(SolidColorGen& gen, const SkPoint& outerCenter, const SkPoint& outerRadius, const SkPoint& innerCenter, const SkPoint& innerRadius)
{
    // This is about as sophisticated as gentl gets. We use all of the vertex texture coordinates to
    // determine the appropriate variant for this SolidColorGen.
    gen.setInsertionTime(Inserter::Deferred);
    SkPoint transformedPoints[5] = { SkPoint::Make(outerCenter.x() + outerRadius.x(), outerCenter.y() + outerRadius.y()),
                                     SkPoint::Make(outerCenter.x() + outerRadius.x(), outerCenter.y()),
                                     SkPoint::Make(innerCenter.x() + innerRadius.x(), innerCenter.y()),
                                     SkPoint::Make(innerCenter.x(), innerCenter.y() + innerRadius.y()),
                                     SkPoint::Make(outerCenter.x(), outerCenter.y() + outerRadius.y()) };
    fMatrix.mapPoints(transformedPoints, 5);

    Clip::Vertex v1(transformedPoints[0].x(), transformedPoints[0].y(), 0, 0, 1, 1, outerCenter.x()+outerRadius.x()-innerCenter.x(), outerCenter.y()+outerRadius.y()-innerCenter.y());
    Clip::Vertex v2(transformedPoints[1].x(), transformedPoints[1].y(), 0, 0, 1, 0, outerCenter.x()+outerRadius.x()-innerCenter.x(), outerCenter.y()-innerCenter.y());
    Clip::Vertex v3(transformedPoints[2].x(), transformedPoints[2].y(), 0, 0, 0, 0, 1, 0);
    Clip::Vertex v4(transformedPoints[3].x(), transformedPoints[3].y(), 0, 0, 0, 0, 0, 1);
    Clip::Vertex v5(transformedPoints[4].x(), transformedPoints[4].y(), 0, 0, 0, 1, outerCenter.x()-innerCenter.x(), outerCenter.y()+outerRadius.y()-innerCenter.y());

    const float absInnerX = fabs(innerRadius.x());
    const float absInnerY = fabs(innerRadius.y());
    const float absOuterX = fabs(outerRadius.x());
    const float absOuterY = fabs(outerRadius.y());
    if (absInnerX > 0.5) {
        v1.inner.fX /= innerRadius.x();
        v2.inner.fX /= innerRadius.x();
        v5.inner.fX /= innerRadius.x();
    } else {
        v1.inner.fX = v2.inner.fX = v5.inner.fX = 3.0;
        v3.inner.fX = v4.inner.fX = 2.0;
    }
    if (absInnerY > 0.5) {
        v1.inner.fY /= innerRadius.y();
        v2.inner.fY /= innerRadius.y();
        v5.inner.fY /= innerRadius.y();
    } else {
        v1.inner.fY = v2.inner.fY = v5.inner.fY = 3.0;
        v3.inner.fY = v4.inner.fY = 2.0;
    }
    if (absOuterX > 0.5) {
        v3.outer.fX = (innerCenter.x() + innerRadius.x() - outerCenter.x()) / outerRadius.x();
        v4.outer.fX = (innerCenter.x() - outerCenter.x()) / outerRadius.x();
    } else {
        v1.outer.fX = 0.5;
    }
    if (absOuterY > 0.5) {
        v3.outer.fY = (innerCenter.y() - outerCenter.y()) / outerRadius.y();
        v4.outer.fY = (innerCenter.y() + innerRadius.y() - outerCenter.y()) / outerRadius.y();
    } else {
        v1.outer.fY = 0.5;
    }

    CommandVariant variant = DefaultVariant;
    if (absOuterX > 0.5 || absOuterY > 0.5)
        variant = static_cast<CommandVariant>(variant | OuterRoundedVariant);
    if (absInnerX > 0.5 || absInnerY > 0.5)
        variant = static_cast<CommandVariant>(variant | InnerRoundedVariant);

    if (outerRadius.x() * outerRadius.y() < 0 || innerRadius.x() * innerRadius.y() < 0) {
        if (absOuterX > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v2, v3, variant), gen);
        if (absInnerX > 0.5 || absInnerY > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v3, v4, variant), gen);
        if (absOuterY > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v4, v5, variant), gen);
    } else {
        if (absOuterX > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v5, v4, variant), gen);
        if (absInnerX > 0.5 || absInnerY > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v4, v3, variant), gen);
        if (absOuterY > 0.5)
            fClip.clipTriangle(Clip::Triangle(v1, v3, v2, variant), gen);
    }
}

void GentlContext::addSmoothArc(ArcGen& gen, const SkPoint& outerCenter, const SkPoint& outerRadius, const SkPoint& innerCenter, const SkPoint& innerRadius, const SkPaint& paint)
{
    if (!isDrawingEnabled())
        return;

    if (!hasVisibleFill(paint))
        return;

    SkPoint transformedPoints[4] = { SkPoint::Make(outerCenter.x() + outerRadius.x(), outerCenter.y()),
                                     SkPoint::Make(innerCenter.x() + innerRadius.x(), innerCenter.y()),
                                     SkPoint::Make(innerCenter.x(), innerCenter.y() + innerRadius.y()),
                                     SkPoint::Make(outerCenter.x(), outerCenter.y() + outerRadius.y()) };
    fMatrix.mapPoints(transformedPoints, 4);

    const SkPoint& p1 = transformedPoints[0];
    const SkPoint& p2 = transformedPoints[1];
    const SkPoint& p3 = transformedPoints[2];
    const SkPoint& p4 = transformedPoints[3];
    // This checks the area of the triangle formed by points 1,2,4 and if it's negative we need to flip
    // some of our points for the triangles we form so that they have the correct winding.
    // We can't choose  p2 and p3 to form our triangle since these points can be coincident.
    bool reverse = ((p2.x() - p1.x()) * (p4.y() - p1.y()) - (p4.x() - p1.x()) * (p2.y() - p1.y())) < 0;

    gen.setInsertionTime(Inserter::Deferred);
    static const int sliverCount = 22;
    static const float sliverDegree = M_PI / (2 * sliverCount);
    if (!FloatUtils::isNearZero(innerRadius.x()) || !FloatUtils::isNearZero(innerRadius.y())) {
        // draw slivers
        // let a be x and b be y radius
        std::vector<SkPoint> inners;
        SkPoint lastInner = p2;
        SkPoint lastOuter = p1;
        inners.push_back(lastInner);
        for (int i = 1; i < sliverCount; ++i) {
            SkPoint innerPoint;
            fMatrix.mapXY(innerCenter.x() + innerRadius.x() * cos(sliverDegree * i), innerCenter.y() + innerRadius.y() * sin(sliverDegree * i), &innerPoint);
            SkPoint outerPoint;
            fMatrix.mapXY(outerCenter.x() + outerRadius.x() * cos(sliverDegree * i), outerCenter.y() + outerRadius.y() * sin(sliverDegree * i), &outerPoint);
            const Clip::Vertex v1(lastOuter.x(), lastOuter.y(), 1, 0);
            const Clip::Vertex v2(lastInner.x(), lastInner.y(), 0, 0);
            const Clip::Vertex v3(innerPoint.x(), innerPoint.y(), 0, 0);
            const Clip::Vertex v4(outerPoint.x(), outerPoint.y(), 1, 0);
            if (reverse) {
                fClip.clipQuad(v4, v3, v2, v1, gen);
            } else {
                fClip.clipQuad(v1, v2, v3, v4, gen);
            }
            lastInner = innerPoint;
            lastOuter = outerPoint;
            inners.push_back(lastInner);
        }
        const Clip::Vertex v1(lastOuter.x(), lastOuter.y(), 1, 0);
        const Clip::Vertex v2(lastInner.x(), lastInner.y(), 0, 0);
        const Clip::Vertex v3(p3.x(), p3.y(), 0, 0);
        const Clip::Vertex v4(p4.x(), p4.y(), 1, 0);
        if (reverse) {
            fClip.clipQuad(v4, v3, v2, v1, gen);
        } else {
            fClip.clipQuad(v1, v2, v3, v4, gen);
        }

        // TODO: Investigate whether we can do this faster using addVertices
        SkPoint inner(innerCenter);
        fMatrix.mapPoints(&inner, 1);
        inners.push_back(p3);
        const Clip::Vertex center(inner.x(), inner.y(), 0, 0);
        for (size_t i = 0; i + 1 < inners.size(); ++i) {
            const Clip::Vertex v1(inners[i].x(), inners[i].y(), 0, 0);
            const Clip::Vertex v2(inners[i + 1].x(), inners[i + 1].y(), 0, 0);
            fClip.clipTriangle(Clip::Triangle(reverse ? v1: v2, reverse? v2 : v1, center, DefaultVariant), gen);
        }
        return;
    }

    const Clip::Vertex center(p2.x(), p2.y(), 0, 0);
    SkPoint lastOuter = p1;
    for  (int i = 1; i < sliverCount; ++i) {
        SkPoint outerPoint;
        fMatrix.mapXY(outerCenter.x() + outerRadius.x() * cos(sliverDegree * i), outerCenter.y() + outerRadius.y() * sin(sliverDegree * i), &outerPoint);
        const Clip::Vertex v1(lastOuter.x(), lastOuter.y(), 1, 0);
        const Clip::Vertex v2(outerPoint.x(), outerPoint.y(), 1, 0);
        fClip.clipTriangle(Clip::Triangle(reverse ? v1: v2, reverse? v2 : v1, center, DefaultVariant), gen);
        lastOuter = outerPoint;
    }
    const Clip::Vertex v2(p4.x(), p4.y(), 1, 0);
    const Clip::Vertex v1(lastOuter.x(), lastOuter.y(), 1, 0);
    fClip.clipTriangle(Clip::Triangle(reverse ? v1: v2, reverse? v2 : v1, center, DefaultVariant), gen);
}

void GentlContext::addRound(SolidColorGen& gen, const SkPoint& p1, const SkPoint& p2, const SkPoint& p3)
{
    SkPoint transformedPoints[3] = {p1,p2,p3};
    fMatrix.mapPoints(transformedPoints, 3);

    Clip::Vertex v1(transformedPoints[0].x(), transformedPoints[0].y(), 0, 0, 0, 1, 0, 0);
    Clip::Vertex v2(transformedPoints[1].x(), transformedPoints[1].y(), 0, 0, 1, 0, 0, 0);
    Clip::Vertex v3(transformedPoints[2].x(), transformedPoints[2].y(), 0, 0, 1, 1, 0, 0);

    if (((p2.x() - p1.x()) * (p3.y() - p1.y()) - (p3.x() - p1.x()) * (p2.y() - p1.y())) < 0)
        std::swap(v2, v3);
    CommandVariant variant = static_cast<CommandVariant>(DefaultVariant | OuterRoundedVariant);
    fClip.clipTriangle(Clip::Triangle(v1, v2, v3, variant), gen);
}

void GentlContext::addVertices(SkPoint vertices[], unsigned count, SkCanvas::VertexMode mode, const SkPaint& paint)
{
    SolidColorGen gen(this, SkPaintToABGR32Premultiplied(paint));

    // We are allowed to smash the input array so we transform the points in place here.
    fMatrix.mapPoints(vertices, count);

    switch (mode) {
    case SkCanvas::kTriangles_VertexMode: {
        for (unsigned i = 0; i + 2 < count; i += 3) {
            fClip.clipTriangle(&vertices[i], gen);
        }
        break;
    }
    case SkCanvas::kTriangleStrip_VertexMode: {
        // For a strip we can create a quad out of every four vertices moving
        // forward two vertices at a time until we run out. Then check for 1 more.
        unsigned i;
        SkPoint quad[4];
        for (i = 0; i + 3 < count; i += 2) {
            memcpy(quad, &vertices[i], 4 * sizeof(SkPoint));
            std::swap(quad[2], quad[3]);
            fClip.clipQuad(quad, gen);
        }
        if (i + 2 < count) {
            fClip.clipTriangle(&vertices[i], gen);
        }
        break;
    }
    case SkCanvas::kTriangleFan_VertexMode: {
        unsigned i;
        SkPoint quad[4];
        quad[0] = vertices[0];
        for (i = 1; i + 2 < count; i += 2) {
            memcpy(&quad[1], &vertices[i], 3 * sizeof(SkPoint)); // Put the outer vertices into the quad.
            fClip.clipQuad(quad, gen);
        }
        if (i + 1 < count) {
            memcpy(&quad[1], &vertices[i], 2 * sizeof(SkPoint));
            fClip.clipTriangle(quad, gen); // Only using the first 3 points of quad.
        }
        break;
    }
    }
}

void GentlContext::addContext(GentlContext& other)
{
    SkASSERT(!other.fLayers.size()); // No open layers in the other context.
    fInstructions.transferFrom(other.fInstructions);
}

void GentlContext::clipRoundedCorner(const SkPoint& clipa, const SkPoint& clipb,
                                                       const SkPoint& center, const SkPoint& edge, bool transform)
{
    SkPoint transformedPoints[4] = { clipa, clipb, center, edge };
    if (transform)
        fMatrix.mapPoints(transformedPoints, 4);

    fClip.addRoundedCorner(transformedPoints[0],
                           transformedPoints[1],
                           transformedPoints[2],
                           transformedPoints[3]);
}

void GentlContext::clipOutRoundedCorner(const SkPoint& clipa, const SkPoint& clipb,
                                                          const SkPoint& center, const SkPoint& edge, bool transform)
{
    SkPoint transformedPoints[4] = { clipa, clipb, center, edge };
    if (transform)
        fMatrix.mapPoints(transformedPoints, 4);

    fClip.subtractRoundedCorner(transformedPoints[0],
                                transformedPoints[1],
                                transformedPoints[2],
                                transformedPoints[3]);
}


} // namespace Gentl
