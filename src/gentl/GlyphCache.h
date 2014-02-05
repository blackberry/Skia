/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Private header. Don't include this in WebKit.
#ifndef GlyphCache_h
#define GlyphCache_h

#include "GrResource.h"
#include "SkRect.h"
#include "SkTypes.h"

#include <map>
#include <list>

class GrTexture;
struct SkGlyph;
class SkGlyphCache;

namespace Gentl {

class GlyphPage;

struct Glyph {
    Glyph(uint16_t inId, uint16_t inSize, const SkRect& inBox, const SkVector& inAdvance)
        : id(inId)
        , size(inSize)
        , box(inBox)
        , advance(inAdvance)
        , texcoords(SkRect::MakeWH(0,0))
        , page(0)
    { }

    uint16_t id; // The glyph id.
    uint16_t size; // Height of source SkGlyph.

    SkRect box; // Origin and dimensions of glyph within bitmap.
    SkVector advance; // Delta to the start of the next glyph

    SkRect texcoords;
    GlyphPage* page;
};

// GlyphPage is a single GPU texture filled with glyphs. They are filled in
// lines from top to bottom. There is no update operation, eviction code
// throws away whole pages at a time.
class GlyphPage : public SkNoncopyable {
public:
    GlyphPage(GrContext* grContext);
    ~GlyphPage();
    GrTexture* texture();

    Glyph* add(Glyph* glyph, const SkGlyph&);

    // Update the GPU copy of the glyph data
    void update();
    static unsigned size();
    bool hasData() const { return fData; }

private:
    enum { PageWidth = 512, PageHeight = 512, };

#if SK_DISTANCEFIELD_FONTS
    enum { DistanceFieldPadding = 4, };
#endif

    // The current insertion point is at (fX, fTop). The current line is
    // fBottom-fTop high. Once fBottom > SIZE the cache is full.
    int fX, fTop, fBottom;
    bool fDirty;

    // Host data
    unsigned char* fData;
    GrTexture* fTexture;
};

class GlyphCache : public GrResource {
public:
    static GlyphCache* getGlyphCacheForGpu(GrGpu*);
    Glyph* get(const SkGlyph&, SkGlyphCache*, uint32_t fontHash, GrContext* grContext);
    void update();

private:
    GlyphCache(GrGpu*);

    virtual size_t sizeInBytes() const { return 0; }
    virtual void onRelease();
    virtual void onAbandon();

    Glyph* add(const SkGlyph&, SkGlyphCache*, uint32_t fontHash, GrContext* grContext);
    Glyph* find(uint32_t fontId, const SkGlyph& skGlyph);

    enum {
        MaximumGlyphPageCount = 32,
        TargetGlyphPageCount = 24,
    };

    bool packPages();
    void deletePage(GlyphPage*);

    typedef std::map<uint64_t, Glyph*> GlyphCacheMap;
    static uint64_t computeKey(uint32_t fontId, uint16_t glyphId, uint16_t height);
    GlyphCacheMap fGlyphs;

    typedef std::list<GlyphPage*> PageList;
    PageList fPages;
};

} // namespace Gentl

#endif // GlyphCache_h
