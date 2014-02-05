/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef Clip_h
#define Clip_h

#include "GentlFloatUtils.h"

#include "gl/GrGLDefines.h"
#include "gl/GrGLFunctions.h"

#include <vector>

class SkCanvas;
class SkRegion;

namespace Gentl {

class Inserter;

// Between two uint32 commands
// - difference in ID means two completely different commands with different
//   execution sequence and (probably) different shaders.
// - difference in Flags, only, exactly the same shaders and very similar
//   execution sequence and Yet some of the execution parameters are different.
// - difference in Variant, only, means exactly the similar execution sequence
//   with exactly the same parameters. Yet the shaders are different.
//
// If there are no differences, two commands can be glued together. This
// simply adds the geometry of the second command to the first without
// requiring another DisplayListCommand.
//
// The table below summurizes the concept.
//
/*******************************************************************************
                ||           |           |           ||
 difference     ||    ID     |   Flags   |  Variants ||         Comments
 in command's   ||           |           |           ||
                ||           |           |           ||
********************************************************************************
                ||           |           |           ||
 execution      || different |  similar  |  similar  ||
 sequence       ||           |           |           ||
                ||           |           |           ||
--------------------------------------------------------------------------------
                ||           |           |           || Generally used to set up
 parameters     ||   same    | different |    same   || texture filtering
                ||           |           |           || parameters.
--------------------------------------------------------------------------------
                ||           |           |           || Different shaders
 shaders        || different |   same    |  similar  || generally require
                ||           |           |           || different arguments.
*******************************************************************************/

enum CommandId {
    Clear                      = 0x00000010,
    SetAlpha                   = 0x00000020,
    SetXferMode                = 0x00000030,
    NoOp                       = 0x00000040,

    DrawColor                  = 0x00001010,
    DrawOpaqueColor            = 0x00001020,
    DrawBitmapGlyph            = 0x00001030,
    DrawBlurredGlyph           = 0x00001040,
    DrawEdgeGlyph              = 0x00001050,
    DrawStrokedEdgeGlyph       = 0x00001060,
    DrawArc                    = 0x00001070,
    DrawSkShader               = 0x00001080,
    DrawTexture                = 0x00001090,

    BeginTransparencyLayer     = 0x00002010,
    EndTransparencyLayer       = 0x00002020,
    DrawCurrentTexture         = 0x00002030,
};

// It is critical that the CommandVariant occupy the least significant bits
// because it is used as an index into many NumberOfVariant sized arrays.
enum CommandVariant {
    DefaultVariant             = 0x00000000,
    InnerRoundedVariant        = 0x00000001,
    OuterRoundedVariant        = 0x00000002,
    BothRoundedVariant         = 0x00000003,
};

// This shift is done so that it could be part of the command mask.
static const unsigned GlVertexModeShift = 16;

enum CommandTriangleMode {
    SoupMode                   = GR_GL_TRIANGLES      << GlVertexModeShift,
    FanMode                    = GR_GL_TRIANGLE_FAN   << GlVertexModeShift,
    StripMode                  = GR_GL_TRIANGLE_STRIP << GlVertexModeShift,
};

static const unsigned SkipToLayerShift = 20;

enum CommandSkip {
    SkipToLayer                = 0x00100000,
};

static const unsigned NumberOfVariants = 4;

class DisplayListCommand {
public:
    // This class must be implicitly convertable to and from unsigned int.
    DisplayListCommand(unsigned data) : fData(data) { }
    operator unsigned() const { return fData; }

    CommandId           id()            const { return static_cast<CommandId>          (fData & IdMask); };
    CommandVariant      variant()       const { return static_cast<CommandVariant>     (fData & VariantMask); };
    CommandTriangleMode triangleMode()  const { return static_cast<CommandTriangleMode>(fData & TriangleModeMask); };
    unsigned            reserved()      const { return fData & ReservedMask; };

    GrGLint getTriangleMode() const { return (fData & TriangleModeMask) >> GlVertexModeShift; }

private:
    enum CommandMask {
        IdMask                     = 0x0000FFF0,
        VariantMask                = 0x0000000F,
        TriangleModeMask           = 0x000F0000,
        SkipMask                   = 0x00F00000,
        ReservedMask               = 0xFF000000,
    };

    unsigned fData;
};

const char* dlcCommand(DisplayListCommand);
char dlcVariant(DisplayListCommand);
char dlcTriangles(DisplayListCommand);

class Clip {
public:
    Clip();
    struct Triangle;

    struct Vertex {
        Vertex(float x, float y) { position.set(x, y); }
        Vertex(float x, float y, float u, float v) { position.set(x, y); texcoords.set(u, v); }
        Vertex(float x, float y, float u, float v, float ou, float ov, float iu, float iv)
            { position.set(x, y); texcoords.set(u, v); outer.set(ou, ov); inner.set(iu, iv); }

        inline float x() const { return position.x(); }
        inline float y() const { return position.y(); }
        inline float u() const { return texcoords.x(); }
        inline float v() const { return texcoords.y(); }
        inline float iu() const { return inner.x(); }
        inline float iv() const { return inner.y(); }
        inline float ou() const { return outer.x(); }
        inline float ov() const { return outer.y(); }

        SkPoint position, texcoords, outer, inner;

    private:
        Vertex() { }
        friend struct Triangle; // Only allow the default ctor internally.
    };

    struct Triangle {
        Triangle() { }
        Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, CommandVariant inVariant = DefaultVariant)
        {
            v[0] = v0;
            v[1] = v1;
            v[2] = v2;
            variant = inVariant;
        }
        Vertex v[3];
        CommandVariant variant;

        bool isInside(const SkRect& rect) const {
            const float xMin = rect.x();
            const float xMax = rect.right();
            const float yMin = rect.y();
            const float yMax = rect.bottom();
            return ((xMin <= v[0].x()) && (xMin <= v[1].x()) &&
                    (xMin <= v[2].x()) && (xMax >= v[0].x()) &&
                    (xMax >= v[1].x()) && (xMax >= v[2].x()) &&
                    (yMin <= v[0].y()) && (yMin <= v[1].y()) &&
                    (yMin <= v[2].y()) && (yMax >= v[0].y()) &&
                    (yMax >= v[1].y()) && (yMax >= v[2].y()));
       }
    };

    void clipQuad(const SkPoint pts[4], Inserter&);
    void clipQuad(const Vertex&, const Vertex&, const Vertex&, const Vertex&, Inserter&);
    void clipTriangle(const SkPoint pts[3], Inserter&);
    void clipTriangle(const Triangle& t, Inserter&);

    void subtractQuad(const SkPoint pts[4]);
    void addRect(const SkRect&);
    void addPolygon(const SkPoint* pts, int npts);
    void addRoundedCorner(const SkPoint& clipa, const SkPoint& clipb,
                          const SkPoint& center, const SkPoint& edge);
    void subtractRoundedCorner(const SkPoint& clipa, const SkPoint& clipb,
                               const SkPoint& center, const SkPoint& edge);
    void clearRoundedCorner();

    static SkRect enclosingRect(const Triangle&);

    bool isRectInClip(const SkRect& rect, const SkISize&) const;
    bool canCreateRoundedVariants() const { return fRounded.size() || fOut.size(); }

private:

    enum Filter {
        PassAll,
        PassRect,
        SlowPass,
        PassNone,
    };

    void addPolygonInternal(const SkPoint* pts, int npts);

    class Plane {
    public:
        Plane()
            : fA(0)
            , fB(0)
            , fC(0)
        { }

        Plane(float x0, float y0, float x1, float y1) { ctor(x0, y0, x1, y1); }
        Plane(const SkPoint& p0, const SkPoint& p1) { ctor(p0.x(), p0.y(), p1.x(), p1.y()); }

        float intersect(const Vertex& v1, const Vertex& v2) const;
        const Triangle* intersect(const Triangle& t, Triangle storage[3], int& inside, int& outside) const;

        bool operator==(const Clip::Plane& rhs) const;
        // Note that default operator= is used implicitly.
        bool isNearZero() { return FloatUtils::isNearZero(fA) && FloatUtils::isNearZero(fB); }

    private:
        void ctor(float x0, float y0, float x1, float y1);
        float getSide(const Vertex& v) const;
        void intersect(const Triangle& tri, Triangle result[3],
                       const float s[5], const bool snz[5], const Vertex v[5],
                       const int i, int& inside, int& outside) const;
        void intersect(const Triangle& tri, Triangle result[3],
                       const bool sgz, const Vertex v[5],
                       const int i, int& inside, int& outside) const;

        // These are ax+by+c=0 for the implicit equation of the line.
        float fA, fB, fC;
    };

    struct Corner {
        Corner() {
            center.set(0, 0);
            edge.set(0, 0);
        }
        Corner(const Plane& p, const SkPoint& c, const SkPoint& e) : plane(p), center(c), edge(e) { }
        Plane plane;
        SkPoint center, edge;
    };

    struct ClipOutQuad { // Right now we need rectangle only
        ClipOutQuad(const Plane plns[4])
            : roundedAmount(0)
        {
            planes[0] = plns[0];
            planes[1] = plns[1];
            planes[2] = plns[2];
            planes[3] = plns[3];
        }

        Plane planes[4];
        Corner rounded[4]; // Note that the correspondece might be lost. TBD
        std::size_t roundedAmount; // Up to 4. Not all corners might be rounded.
    };

    void updateRect(const SkRect& rect);

    void clipOutTriangle(const Triangle& t, Inserter&);
    void clipTrianglePlane(const Triangle& t, unsigned p, Inserter&);
    void clipTriangleRounded(const Triangle& t, unsigned r, Inserter&);
    void clipOutTriangleRounded(const Triangle& t, unsigned o, unsigned r, std::vector<Triangle>& result);

    bool fRectDefined;
    Filter fFilter;
    SkRect fInnerRect; // Keeps track of the largest interior rect of the clip.
    std::vector<Plane> fPlanes;
    std::vector<Corner> fRounded;
    std::vector<ClipOutQuad> fOut;
    std::vector<SkPoint> fPolygon;

    // caches to avoid reallocations
    std::vector<Triangle> ct_insideTris, ct_outsideTris;
    std::vector<Triangle> cot_clipped, cot_insideTris, cot_outsideTris;
};

} // namespace Gentl

#endif
