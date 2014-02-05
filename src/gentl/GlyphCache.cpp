/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GlyphCache.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrTexture.h"
#include "SkGlyph.h"
#include "SkGlyphCache.h"

#if SK_DISTANCEFIELD_FONTS
#include "edtaa3.h"
#define DISTANCE_FIELD_RANGE (4.0)
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace Gentl {

static std::map<GrGpu*, GlyphCache*> kGlyphCaches;

GlyphCache* GlyphCache::getGlyphCacheForGpu(GrGpu* gpu)
{
    std::map<GrGpu*, GlyphCache*>::iterator it = kGlyphCaches.find(gpu);
    if (it != kGlyphCaches.end())
        return it->second;

    GlyphCache* cache = new GlyphCache(gpu);
    kGlyphCaches[gpu] = cache;
    return cache;
}

GlyphCache::GlyphCache(GrGpu* gpu)
    : GrResource(gpu, true)
{
}

void GlyphCache::onRelease()
{
    kGlyphCaches.erase(getGpu());
    GrResource::onRelease();
}

void GlyphCache::onAbandon()
{
    kGlyphCaches.erase(getGpu());
    GrResource::onAbandon();
}

void GlyphCache::deletePage(GlyphPage* page)
{
    // Delete and erase all the cached glyphs on the page in question.
    GlyphCacheMap::iterator it = fGlyphs.begin();
    while (it != fGlyphs.end()) {
        if (it->second->page == page) {
            delete it->second;
            fGlyphs.erase(it++);
        } else {
            ++it;
        }
    }

    delete page;
}

bool GlyphCache::packPages()
{
    // Pages which return false from hasData() have transferred ownership of
    // their glyphs to their texture and can be deleted. So too can pages with
    // a refCount of 1; this means the GlyphPage object has the only ref to its
    // texture, which is not used by any display list. This loop increments its
    // iterator inside the loop to leverage the erase(it++) list idiom.
    PageList::iterator it = fPages.begin();
    while (fPages.size() > TargetGlyphPageCount && it != fPages.end()) {
        if (!(*it)->hasData() || (*it)->texture()->getRefCnt() == 1) {
            deletePage(*it);
            fPages.erase(it++);
        } else {
            ++it;
        }
    }

    SkDebugf("GlyphCache::packPages() reduced cache to %d pages, %dKB.\n",
        fPages.size(), fPages.size() * GlyphPage::size() * sizeof(unsigned) / 1024);

    return fPages.size() < MaximumGlyphPageCount;
}

Glyph* GlyphCache::add(const SkGlyph& skGlyph, SkGlyphCache* skGlyphCache, uint32_t fontHash, GrContext* grContext)
{
    SkRect box = SkRect::MakeXYWH(skGlyph.fLeft, skGlyph.fTop, skGlyph.fWidth, skGlyph.fHeight);
    SkVector advance = SkVector::Make(SkFixedToFloat(skGlyph.fAdvanceX), SkFixedToFloat(skGlyph.fAdvanceY));
    Glyph* glyph = new Glyph(skGlyph.getGlyphID(), skGlyph.fHeight, box, advance);
    if (!glyph)
        return 0;

    // We're going to need the actual bytes now to put them in our cache texture.
    skGlyphCache->findImage(skGlyph);

    // If the cache is empty, or the page currently being filled cannot accommodate
    // this glyphId, we need to create a new page. This is a good time to do eviction too.
    if (!fPages.empty() && fPages.back()->add(glyph, skGlyph)) {
        fGlyphs[computeKey(fontHash, glyph->id, skGlyph.fHeight)] = glyph;
        return glyph;
    }

    // Make a new page and try to add the glyphId there
    GlyphPage* page = new GlyphPage(grContext);
    if (!page || !page->add(glyph, skGlyph)) {
        SkDebugf("Can't create page or glyph %d is too large (%d, %d)", skGlyph.getGlyphID(), skGlyph.fWidth, skGlyph.fHeight);
        delete page;
        delete glyph;
        return 0;
    }

    if (fPages.size() >= MaximumGlyphPageCount && !packPages()) {
        SkDebugf("Out of cache memory and page compression failed.");
        delete page;
        delete glyph;
        return 0;
    }

    fPages.push_back(page); // add a new page, note that there is a place for one, at least
    fGlyphs[computeKey(fontHash, glyph->id, skGlyph.fHeight)] = glyph;
    return glyph;
}

Glyph* GlyphCache::find(uint32_t fontId, const SkGlyph& skGlyph)
{
    GlyphCacheMap::iterator it = fGlyphs.find(computeKey(fontId, skGlyph.getGlyphID(), skGlyph.fHeight));
    if (it != fGlyphs.end())
        return it->second;

    return 0;
}

Glyph* GlyphCache::get(const SkGlyph& skGlyph, SkGlyphCache* skGlyphCache, uint32_t fontHash, GrContext* grContext)
{
    Glyph* cachedGlyph = find(fontHash, skGlyph);

    if (!cachedGlyph) {
        cachedGlyph = add(skGlyph, skGlyphCache, fontHash, grContext);
    }

    return cachedGlyph;
}

void GlyphCache::update()
{
    PageList::iterator it = fPages.begin();
    PageList::iterator last = fPages.end();
    for (; it != last; ++it)
        (*it)->update();
}

GlyphPage::GlyphPage(GrContext* grContext)
    : fX(0)
    , fTop(0)
    , fBottom(0)
    , fDirty(0)
    , fData(0)
{
    GrTextureDesc desc;
    desc.fFlags = kNoStencil_GrTextureFlagBit | kDynamicUpdate_GrTextureFlagBit;
    desc.fConfig = kAlpha_8_GrPixelConfig;
    desc.fWidth = PageWidth;
    desc.fHeight = PageHeight;

    fTexture = grContext->getGpu()->createTexture(desc, 0, 0);

    if (fTexture) {
        fData = new unsigned char[size()];
        memset(fData, 0, size());
    }
}

GlyphPage::~GlyphPage()
{
    fTexture->unref();

    delete[] fData;
    fData = 0;
}

GrTexture* GlyphPage::texture()
{
    return fTexture;
}

void GlyphPage::update()
{
    if (!fDirty || !fData)
        return;

    fDirty = false;

    // FIXME: This is decent but we can further improve this by only updating what's
    // changed since the last time, rather than the entire used glyph bitmap.
    fTexture->writePixels(0, 0, PageWidth, std::min<int>(fBottom, PageHeight), kAlpha_8_GrPixelConfig, fData, PageWidth, 0);

    // We can release our copy of the data once we've uploaded the entire page.
    if (fBottom > PageHeight) {
        delete[] fData;
        fData = 0;
    }
}

Glyph* GlyphPage::add(Glyph* glyph, const SkGlyph& skGlyph)
{
    SkASSERT(glyph);

    // Note fData pointer/flag in update(). Note also that GlyphCache, and
    // it's internal GlyphPage, are thread safe.
    if (!fData || fBottom > PageHeight)
        return 0;

#if SK_DISTANCEFIELD_FONTS
    const int dstHeight = skGlyph.fHeight + DistanceFieldPadding * 2;
    const int dstWidth = skGlyph.fWidth + DistanceFieldPadding * 2;

    glyph->box.outset(DistanceFieldPadding, DistanceFieldPadding);
#else
    const int dstHeight = skGlyph.fHeight;
    const int dstWidth = skGlyph.fWidth;
#endif

    if (fX + dstWidth + 2 > PageWidth) {
        fTop = fBottom;
        fX = 0;
        SkASSERT(fX + dstWidth + 2 <= PageWidth);
    }

    if (fTop + dstHeight + 2 > fBottom) {
        fBottom = fTop + dstHeight + 2;
        if (fBottom > PageHeight) // page is full
            return 0; // a bit unoptimal since "new Glyph" may fail.
    }

    fDirty = true;

#if SK_DISTANCEFIELD_FONTS
    const unsigned rowBytes = dstWidth;
    const unsigned bytesPerPixel = 1;

    // alloc storage for distance field glyph
    size_t dfSize = dstWidth * dstHeight * bytesPerPixel;
    SkAutoSMalloc<1024> dfStorage(dfSize);

    // copy glyph into distance field storage
    sk_bzero(dfStorage.get(), dfSize);

    unsigned char* ptr = reinterpret_cast<unsigned char*>(skGlyph.fImage);
    unsigned char* dfPtr = reinterpret_cast<unsigned char*>(dfStorage.get());
    unsigned dfStride = rowBytes;
    unsigned stride = skGlyph.rowBytes();
    dfPtr += DistanceFieldPadding * dfStride;
    dfPtr += DistanceFieldPadding * bytesPerPixel;

    for (int i = 0; i < skGlyph.fHeight; ++i) {
        memcpy(dfPtr, ptr, stride);
        dfPtr += dfStride;
        ptr += stride;
    }

    // generate distance field data
    SkAutoSMalloc<1024> distXStorage(dstWidth * dstHeight * sizeof(short));
    SkAutoSMalloc<1024> distYStorage(dstWidth * dstHeight * sizeof(short));
    SkAutoSMalloc<1024> outerDistStorage(dstWidth * dstHeight * sizeof(double));
    SkAutoSMalloc<1024> innerDistStorage(dstWidth * dstHeight * sizeof(double));
    SkAutoSMalloc<1024> gxStorage(dstWidth * dstHeight * sizeof(double));
    SkAutoSMalloc<1024> gyStorage(dstWidth * dstHeight * sizeof(double));

    short* distX = reinterpret_cast<short*>(distXStorage.get());
    short* distY = reinterpret_cast<short*>(distYStorage.get());
    double* outerDist = reinterpret_cast<double*>(outerDistStorage.get());
    double* innerDist = reinterpret_cast<double*>(innerDistStorage.get());
    double* gx = reinterpret_cast<double*>(gxStorage.get());
    double* gy = reinterpret_cast<double*>(gyStorage.get());

    dfPtr = reinterpret_cast<unsigned char*>(dfStorage.get());
    EDTAA::computegradient(dfPtr, dstWidth, dstHeight, gx, gy);
    EDTAA::edtaa3(dfPtr, gx, gy, dstWidth, dstHeight, distX, distY, outerDist);

    for (int i = 0; i < dstWidth * dstHeight; ++i) {
        *dfPtr = 255 - *dfPtr;
        dfPtr++;
    }

    dfPtr = reinterpret_cast<unsigned char*>(dfStorage.get());
    sk_bzero(gx, sizeof(double) * dstWidth * dstHeight);
    sk_bzero(gy, sizeof(double) * dstWidth * dstHeight);
    EDTAA::computegradient(dfPtr, dstWidth, dstHeight, gx, gy);
    EDTAA::edtaa3(dfPtr, gx, gy, dstWidth, dstHeight, distX, distY, innerDist);
    sk_bzero(dfStorage.get(), dfSize);

    for (int i = 0; i < dstWidth*dstHeight; ++i) {
        unsigned char val;
        double outerval = outerDist[i];
        if (outerval < 0.0) {
            outerval = 0.0;
        }
        double innerval = innerDist[i];
        if (innerval < 0.0) {
            innerval = 0.0;
        }
        double dist = outerval - innerval;
        if (dist <= -DISTANCE_FIELD_RANGE) {
            val = 255;
        } else if (dist > DISTANCE_FIELD_RANGE) {
            val = 0;
        } else {
            val = static_cast<unsigned char>((DISTANCE_FIELD_RANGE-dist)*128.0/DISTANCE_FIELD_RANGE);
        }
        *dfPtr++ = val;
    }

    unsigned char* src = reinterpret_cast<unsigned char*>(dfStorage.get());
#else
    const unsigned rowBytes = skGlyph.rowBytes();
    unsigned char* src = reinterpret_cast<unsigned char*>(skGlyph.fImage);
#endif
    unsigned char* dst = fData + (PageWidth * (fTop + 1)) + fX + 1;

    for (int i = 0; i < dstHeight; ++i, dst += PageWidth, src += rowBytes) {
        memcpy(dst, src, rowBytes);
    }

    static const float invPageWidth = 1.f / PageWidth;
    static const float invPageHeight = 1.f / PageHeight;
    glyph->texcoords.setXYWH((fX + 1) * invPageWidth, (fTop + 1) * invPageHeight,
                             dstWidth * invPageWidth, dstHeight * invPageHeight);
    fX += dstWidth + 2;

    glyph->page = this;
    return glyph;
}

unsigned GlyphPage::size()
{
    return PageWidth * PageHeight;
}

uint64_t GlyphCache::computeKey(uint32_t fontId, uint16_t glyphId, uint16_t height)
{
#if SK_DISTANCEFIELD_FONTS
    return (fontId << 16) | glyphId;
#else
    return (static_cast<uint64_t>(fontId) << 32) | (glyphId << 16) | height;
#endif

}

} // namespace Gentl
