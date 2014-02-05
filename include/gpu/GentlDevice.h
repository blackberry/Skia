/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlDevice_DEFINED
#define GentlDevice_DEFINED

#include "SkBitmapDevice.h"

namespace Gentl {
class DisplayList;
class GentlContext;
}

class GrContext;
class GrSurface;

class SK_API GentlDevice: public SkBitmapDevice {

public:
    static GentlDevice* Create(GrSurface* surface);
    GentlDevice(GrContext*, SkBitmap::Config, int width, int height);
    GentlDevice(GrContext*, GrRenderTarget*);

    virtual ~GentlDevice();
    // overrides from SkDevice
    virtual void clear(SkColor color) SK_OVERRIDE;
    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count, const SkPoint[], const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkDraw&, const SkRRect& r, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawOval(const SkDraw&, const SkRect& oval, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkDraw&, const SkPath& path, const SkPaint& paint, const SkMatrix* prePathMatrix, bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap, const SkMatrix&, const SkPaint&) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkDraw&, const SkBitmap&, const SkRect* srcOrNull, const SkRect& dst, const SkPaint& paint, SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE;
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap, int x, int y, const SkPaint& paint);
    virtual void drawText(const SkDraw&, const void* text, size_t len, SkScalar x, SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len, const SkScalar pos[], SkScalar constY, int scalarsPerPos, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len, const SkPath& path, const SkMatrix* matrix, const SkPaint&) SK_OVERRIDE;
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount, const SkPoint verts[], const SkPoint texs[], const SkColor colors[],
            SkXfermode* xmode, const uint16_t indices[], int indexCount, const SkPaint&) SK_OVERRIDE;
    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y, const SkPaint&) SK_OVERRIDE;
    virtual bool filterTextFlags(const SkPaint&, TextFlags*) SK_OVERRIDE;
    virtual void flush();
    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;
    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888 = SkCanvas::kNative_Premul_Config8888) SK_OVERRIDE;
    /**
     * Make's this device's rendertarget current in the underlying 3D API.
     * Also implicitly flushes.
     */
    virtual void makeRenderTargetCurrent();
    virtual bool canHandleImageFilter(SkImageFilter*) SK_OVERRIDE;
    virtual bool filterImage(SkImageFilter*, const SkBitmap&, const SkMatrix&, SkBitmap*, SkIPoint*) SK_OVERRIDE;

    GrContext* grContext() { return fGrContext; }

protected:
    // overrides from SkDevice
    virtual bool onReadPixels(const SkBitmap& bitmap, int x, int y, SkCanvas::Config8888 config8888) SK_OVERRIDE;

private:
    bool prepareDraw(const SkDraw& draw, const SkPaint& paint, float alpha = 1.0f);

    // override from SkDevice
    virtual SkBaseDevice* onCreateCompatibleDevice(SkBitmap::Config config, int width, int height, bool isOpaque, Usage usage) SK_OVERRIDE;

    Gentl::DisplayList* fDisplayList;
    Gentl::GentlContext* fGentlContext;
    GrContext* fGrContext;
};

#endif
