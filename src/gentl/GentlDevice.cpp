/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GentlDevice.h"
#include "DisplayList.h"
#include "GentlContext.h"
#include "GentlPath.h"
#include "GentlTrace.h"
#include "SkColor.h"
#include "SkDraw.h"
#include "SkBitmap.h"
#include "SkColor.h"
#include "SkCanvas.h"
#include "SkDraw.h"
#include "SkGlyph.h"
#include "SkGlyphCache.h"
#include "SkMatrix44.h"
#include "SkPathMeasure.h"
#include "SkRRect.h"
#include "SkGr.h"
#include "GrTypes.h"
#include "gl/GrGpuGL.h"

using namespace Gentl;

#define PREPARE_DRAW_OR_RETURN(draw, paint) if (!prepareDraw(draw, paint)) {\
                                                return;\
                                            }

#define PREPARE_DRAW_ALPHA_OR_RETURN(draw, paint, alpha) if (!prepareDraw(draw, paint, alpha)) {\
                                                             return;\
                                                         }

GentlDevice* GentlDevice::Create(GrSurface* surface)
{
    SkASSERT(NULL != surface);
    if (NULL == surface->asRenderTarget() || NULL == surface->getContext()) {
        return NULL;
    }
    return SkNEW_ARGS(GentlDevice, (surface->getContext(), surface->asRenderTarget()));
}

static SkBitmap::Config grConfig2skConfig(GrPixelConfig config, bool* isOpaque) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
            *isOpaque = false;
            return SkBitmap::kA8_Config;
        case kRGB_565_GrPixelConfig:
            *isOpaque = true;
            return SkBitmap::kRGB_565_Config;
        case kRGBA_4444_GrPixelConfig:
            *isOpaque = false;
            return SkBitmap::kARGB_4444_Config;
        case kSkia8888_GrPixelConfig:
            // we don't currently have a way of knowing whether
            // a 8888 is opaque based on the config.
            *isOpaque = false;
            return SkBitmap::kARGB_8888_Config;
        default:
            *isOpaque = false;
            return SkBitmap::kNo_Config;
    }
}

/*
 * GrRenderTarget does not know its opaqueness, only its config, so we have
 * to make conservative guesses when we return an "equivalent" bitmap.
 */
static SkBitmap make_bitmap(GrContext* context, GrRenderTarget* renderTarget) {
    bool isOpaque;
    SkBitmap::Config config = grConfig2skConfig(renderTarget->config(), &isOpaque);

    SkBitmap bitmap;
    bitmap.setConfig(config, renderTarget->width(), renderTarget->height(), 0,
                     isOpaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType);
    return bitmap;
}

GentlDevice::GentlDevice(GrContext* grContext, GrRenderTarget* renderTarget)
    : SkBitmapDevice(make_bitmap(grContext, renderTarget))
    , fDisplayList(new DisplayList(grContext, SkISize::Make(renderTarget->width(), renderTarget->height()), OnDemandBacking, renderTarget->getRenderTargetHandle() ? Identity : YAxisInverted))
    , fGentlContext(new GentlContext(grContext, SkISize::Make(renderTarget->width(), renderTarget->height())))
    , fGrContext(grContext)
{
    fGrContext->ref();
}

GentlDevice::GentlDevice(GrContext* grContext, SkBitmap::Config config, int width, int height)
    : SkBitmapDevice(config, width, height, false)
    , fDisplayList(new DisplayList(grContext, SkISize::Make(width, height), OnDemandBacking, Identity))
    , fGentlContext(new GentlContext(grContext, SkISize::Make(width, height)))
    , fGrContext(grContext)
{
    fGrContext->ref();
}

GentlDevice::~GentlDevice()
{
    delete fDisplayList;
    fDisplayList = 0;

    delete fGentlContext;
    fGentlContext = 0;

    fGrContext->unref();
    fGrContext = 0;
}

void GentlDevice::clear(SkColor color)
{
    fDisplayList->clear();
    fGentlContext->clear(color);
}

void GentlDevice::drawPaint(const SkDraw& draw, const SkPaint& paint)
{
    SkDraw myDraw(draw);
    myDraw.fMatrix = &SkMatrix::I();

    static const SkClipStack empty;
    myDraw.fClipStack = &empty;

    PREPARE_DRAW_OR_RETURN(myDraw, paint);
    fGentlContext->paintRect(SkRect::MakeWH(width(), height()), paint);
}

void GentlDevice::drawPoints(const SkDraw& draw, SkCanvas::PointMode mode, size_t count, const SkPoint points[], const SkPaint& paint)
{
    SkPath path;
    for (size_t i = 0; i < count; ++i) {
        SkPoint point = points[i];
        switch (mode) {
        case SkCanvas::kLines_PointMode:
            if(i + 1 < count){
                path.moveTo(point);
                path.lineTo(points[i + 1]);
            }
            break;
        case SkCanvas::kPoints_PointMode: {
            SkPaint newPaint(paint);
            newPaint.setStyle(SkPaint::kFill_Style);

            SkScalar width = newPaint.getStrokeWidth();
            SkScalar radius = SkScalarHalf(width);
            SkRect r;
            r.fLeft = points[i].fX - radius;
            r.fTop = points[i].fY - radius;
            r.fRight = r.fLeft + width;
            r.fBottom = r.fTop + width;
            if (newPaint.getStrokeCap() == SkPaint::kRound_Cap) {
                drawOval(draw, r, newPaint);
            } else {
                drawRect(draw, r, newPaint);
            }
            break;
        }
        case SkCanvas::kPolygon_PointMode:
            if (i == 0)
                path.moveTo(point);
            path.lineTo(point);
            break;
        }
    }
    if (mode == SkCanvas::kPoints_PointMode)
        return;
    SkPaint myPaint(paint);
    myPaint.setStyle(SkPaint::kStroke_Style);
    myPaint.setStrokeWidth(std::max(myPaint.getStrokeWidth(), 1.0f));
    drawPath(draw, path, myPaint, 0, false);
}

void GentlDevice::drawRect(const SkDraw& draw, const SkRect& rect, const SkPaint& paint)
{
    PREPARE_DRAW_OR_RETURN(draw, paint);
    fGentlContext->paintRect(rect, paint);
}

void GentlDevice::drawRRect(const SkDraw& draw, const SkRRect& rRect, const SkPaint& paint)
{
    PREPARE_DRAW_OR_RETURN(draw, paint);
    fGentlContext->paintRRect(rRect, paint);
}

void GentlDevice::drawOval(const SkDraw& draw, const SkRect& oval, const SkPaint& paint)
{
    SkRRect rRect;
    rRect.setOval(oval);
    drawRRect(draw, rRect, paint);
}

void GentlDevice::drawPath(const SkDraw& draw, const SkPath& origSrcPath, const SkPaint& paint, const SkMatrix* prePathMatrix, bool pathIsMutable)
{
    PREPARE_DRAW_OR_RETURN(draw, paint);

    SkPath* pathPtr = const_cast<SkPath*>(&origSrcPath);
    SkPath  tmpPath;

    if (prePathMatrix) {
        SkPath* result = pathPtr;

        if (!pathIsMutable) {
            result = &tmpPath;
            pathIsMutable = true;
        }
        // should I push prePathMatrix on our MV stack temporarily, instead
        // of applying it here? See SkDraw.cpp
        pathPtr->transform(*prePathMatrix, result);
        pathPtr = result;
    }

    fGentlContext->paintPath(*pathPtr, paint);
}

void GentlDevice::drawBitmap(const SkDraw& draw, const SkBitmap& bitmap, const SkMatrix& matrix, const SkPaint& paint)
{
    // TODO: Tile as needed.
    SkDraw myDraw(draw);
    SkMatrix myMatrix = *myDraw.fMatrix;
    myMatrix.preConcat(matrix);
    myDraw.fMatrix = &myMatrix;

    const SkRect dstRect = SkRect::MakeWH(bitmap.width(), bitmap.height());

    drawBitmapRect(myDraw, bitmap, 0, dstRect, paint, SkCanvas::kBleed_DrawBitmapRectFlag);
}

void GentlDevice::drawBitmapRect(const SkDraw& draw, const SkBitmap& bitmap, const SkRect* srcRectOrNull, const SkRect& dstRect, const SkPaint& paint, SkCanvas::DrawBitmapRectFlags flags)
{
    PREPARE_DRAW_ALPHA_OR_RETURN(draw, paint, paint.getAlpha() / 255.0f);

    const int maxTextureSize = fGrContext->getMaxRenderTargetSize();
    if (bitmap.width() <= maxTextureSize && bitmap.height() <= maxTextureSize) {
        const SkRect srcRect = srcRectOrNull ? *srcRectOrNull : SkRect::MakeWH(bitmap.width(), bitmap.height());
        fGentlContext->paintBitmap(dstRect, srcRect, bitmap, paint);
    } else {
        // Tile the bitmap.
        const SkRect srcRect = srcRectOrNull ? *srcRectOrNull : SkRect::MakeWH(bitmap.width(), bitmap.height());
        for (int x = srcRect.x(); x < srcRect.width(); x += maxTextureSize) {
        for (int y = srcRect.y(); y < srcRect.height(); y += maxTextureSize) {
            SkIRect tileRect = SkIRect::MakeXYWH(x, y, std::min(maxTextureSize, (int)srcRect.width() - x), std::min(maxTextureSize, (int)srcRect.height() - y));
            SkBitmap tile;
            bitmap.extractSubset(&tile, tileRect);
            float dx = dstRect.x() + tileRect.width() / srcRect.width() * tileRect.x();
            float dy = dstRect.y() + tileRect.height() / srcRect.height() * tileRect.y();
            float dwidth = tileRect.width() / srcRect.width() * dstRect.width();
            float dheight = tileRect.height() / srcRect.height() * dstRect.height();
            SkRect tileDest = SkRect::MakeXYWH(dx, dy, dwidth, dheight);
            fGentlContext->paintBitmap(tileDest, SkRect::MakeWH(tileRect.width(), tileRect.height()), tile, paint);
        }
        }
    }
}

void GentlDevice::drawSprite(const SkDraw& draw, const SkBitmap& bitmap, int x, int y, const SkPaint& paint)
{
    SkDraw myDraw(draw);
    myDraw.fMatrix = &SkMatrix::I();
    const SkRect src = SkRect::MakeWH(bitmap.width(), bitmap.height());

    drawBitmapRect(myDraw, bitmap, &src, SkRect::MakeXYWH(x, y, bitmap.width(), bitmap.height()), paint, SkCanvas::kBleed_DrawBitmapRectFlag);
}

void GentlDevice::drawText(const SkDraw& draw, const void* data, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint)
{
    PREPARE_DRAW_OR_RETURN(draw, paint);

    const char* text = reinterpret_cast<const char*>(data);
    if (fGentlContext->setupText(fGrContext, fLeakyProperties, paint))
        fGentlContext->paintText(text, byteLength, SkPoint::Make(x, y), paint);
    else
        draw.drawText_asPaths(text, byteLength, x, y, paint);
}

void GentlDevice::drawPosText(const SkDraw& draw, const void* data, size_t byteLength, const SkScalar pos[], SkScalar constY, int scalarsPerPos, const SkPaint& paint)
{
    PREPARE_DRAW_OR_RETURN(draw, paint);

    const char* text = reinterpret_cast<const char*>(data);
    if (fGentlContext->setupText(fGrContext, fLeakyProperties, paint))
        fGentlContext->paintPosText(text, byteLength, pos, constY, scalarsPerPos, paint);
    else
        draw.drawPosText_asPaths(text, byteLength, pos, constY, scalarsPerPos, paint);
}

void GentlDevice::drawTextOnPath(const SkDraw& draw, const void* text, size_t len, const SkPath& path, const SkMatrix* matrix, const SkPaint& paint)
{
    SkASSERT(draw.fDevice == this);
    draw.drawTextOnPath((const char*)text, len, path, matrix, paint);
}

void GentlDevice::drawVertices(const SkDraw& draw, SkCanvas::VertexMode, int vertexCount,
        const SkPoint verts[], const SkPoint texs[], const SkColor colors[], SkXfermode* xmode,
        const uint16_t indices[], int indexCount, const SkPaint&)
{
}

void GentlDevice::drawDevice(const SkDraw& draw, SkBaseDevice* device, int x, int y, const SkPaint& paint)
{
    // FIXME: This will explode if ever called for a layer created with General usage because
    // we will have created an SkBitmapDevice, not a GentlDevice. The way SkCanvas works today
    // though this cannot happen so we are safe for now.
    GentlDevice* compatibleDevice = static_cast<GentlDevice*>(device);

    // Balancing the initial layer from onCreateCompatibleDevice();
    // We perform these operations here rather than after the addContext to avoid
    // losing the dirtyRect data that the input device has been accumulating.
    compatibleDevice->fGentlContext->setXfermode(paint);
    SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode;
    if (paint.getXfermode())
        paint.getXfermode()->asMode(&mode);
    compatibleDevice->fGentlContext->popLayer(paint.getAlpha() / 255.0f, SkRect::MakeXYWH(x,y, compatibleDevice->width(), compatibleDevice->height()), mode, false);

    // This addContext call actually pulls the commands from the parameter device into ours, thus
    // smashing all of the commands in the parameter. However we're okay for now because SkCanvas
    // deletes the parameter device right after this method.
    fGentlContext->addContext(*compatibleDevice->fGentlContext);
}

bool GentlDevice::filterTextFlags(const SkPaint&, TextFlags*)
{
    return false;
}

void GentlDevice::flush()
{
    fGentlContext->flush(fDisplayList);

    SkIRect rect = SkIRect::MakeSize(fDisplayList->size());
    fDisplayList->draw(rect, rect);
}

void GentlDevice::onAttachToCanvas(SkCanvas* canvas)
{
}

void GentlDevice::onDetachFromCanvas()
{
}

void GentlDevice::makeRenderTargetCurrent()
{
}

void GentlDevice::writePixels(const SkBitmap& bitmap, int x, int y, SkCanvas::Config8888 config8888) {
    return;
}

bool GentlDevice::onReadPixels(const SkBitmap& bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    const int maxTextureSize = fGrContext->getMaxRenderTargetSize();

    SkASSERT(bitmap.width() <= maxTextureSize);
    if (bitmap.width() > maxTextureSize) {
        return false;
    }

    // Without a flush here the read could be stale or even blank.
    if (fDisplayList->specifiedBacking() == OnDemandBacking)
        fDisplayList->setCurrentBacking(AlphaBacking);
    fGentlContext->flush(fDisplayList);

    SkAutoLockPixels alp(bitmap);

    // TODO: maybe move this code into display list
    int offset = 0;
    while (offset < bitmap.height()) {
        SkMatrix44 view = fDisplayList->getDefaultProjection();
        view.preTranslate(0, -offset, 0);
        fDisplayList->updateBackingSurface(view);
        fDisplayList->readPixels(SkIRect::MakeXYWH(x, y, bitmap.width(), std::min(maxTextureSize, bitmap.height() - offset)), ((unsigned char*)bitmap.getPixels()) + (offset * bitmap.width() * 4), true);
        offset += maxTextureSize;
    }

    return true;
}

bool GentlDevice::canHandleImageFilter(SkImageFilter*)
{
    return false;
}

bool GentlDevice::filterImage(SkImageFilter*, const SkBitmap&, const SkMatrix&, SkBitmap* result, SkIPoint* offset)
{
    return false;
}

SkBaseDevice* GentlDevice::onCreateCompatibleDevice(SkBitmap::Config config, int width, int height, bool isOpaque, Usage usage)
{
    SkBaseDevice* compatibleDevice = 0;
    switch (usage) {
    case SkBaseDevice::kSaveLayer_Usage: {
        // Create a compatible GentlDevice and push a layer into it so that it can calculate
        // an accurate dirty rect for its layer blending onto the layer below.
        GentlDevice* gentlDevice = new GentlDevice(fGrContext, config, width, height);
        gentlDevice->fGentlContext->pushLayer();
        compatibleDevice = gentlDevice;
        break;
    }
    case SkBaseDevice::kGeneral_Usage: {
        // To support ImageFilters we provide a BitmapDevice. General usage is specifically
        // used for layers when there is a paint with an ImageFilter supplied.
        SkBitmap* bitmap = new SkBitmap;
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
        bitmap->allocPixels();
        compatibleDevice = new SkBitmapDevice(*bitmap);
        break;
    }
    }

    return compatibleDevice;
}

bool GentlDevice::prepareDraw(const SkDraw& draw, const SkPaint& paint, float alpha) {
    if (draw.fClipStack->getTopmostGenID() == SkClipStack::kEmptyGenID) {
        return false;
    }

    fGentlContext->setMatrix(*draw.fMatrix);
    fGentlContext->setXfermode(paint);
    fGentlContext->setAlpha(alpha);

    fGentlContext->setClipFromClipStack(*draw.fClipStack, getOrigin());

    return true;
}

