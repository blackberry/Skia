/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Clip.h"
#include "Generators.h"

#include "SkTypes.h"
#include <algorithm>
#include <float.h>
#include "SkPoint.h"

using namespace std;

namespace Gentl {

const char* dlcCommand(DisplayListCommand command)
{
    switch (command.id()) {
    case Clear: return "Clear";
    case SetAlpha: return "SetAlpha";
    case SetXferMode: return "SetXferMode";
    case NoOp: return "NoOp";

    case DrawColor: return "DrawColor";
    case DrawOpaqueColor: return "DrawOpaqueColor";
    case DrawBitmapGlyph: return "DrawBitmapGlyph";
    case DrawEdgeGlyph: return "DrawEdgeGlyph";
    case DrawBlurredGlyph: return "DrawBlurredGlyph";
    case DrawStrokedEdgeGlyph: return "DrawStrokedEdgeGlyph";
    case DrawArc: return "DrawArc";
    case DrawSkShader: return "DrawSkShader";
    case BeginTransparencyLayer: return "BeginTransparencyLayer";
    case EndTransparencyLayer: return "EndTransparencyLayer";
    case DrawCurrentTexture: return "DrawCurrentTexture";
    case DrawTexture: return "DrawTexture";
    }
    return "UnknownCommand";
}

char dlcVariant(DisplayListCommand command)
{
    switch (command.variant()) {
    case DefaultVariant: return ' ';
    case InnerRoundedVariant: return 'I';
    case OuterRoundedVariant: return 'O';
    case BothRoundedVariant: return 'B';
    }
    return '?';
}

char dlcTriangles(DisplayListCommand command)
{
    switch (command.getTriangleMode()) {
    case GR_GL_FALSE:
    case GR_GL_TRIANGLES: return ' ';
    case GR_GL_TRIANGLE_FAN: return 'F';
    case GR_GL_TRIANGLE_STRIP: return 'S';
    }
    return '?';
}

Clip::Clip()
: fRectDefined(false)
, fFilter(PassAll)
{
    fInnerRect.setEmpty();
}

void Clip::updateRect(const SkRect& rect)
{
    // Note that the rect.width and rect.height might be negative. For instance
    // reflection operation request. However the resulting rect is expected to
    // be all positive since it has to be on the screen.clipRect
    // Yet it is important to remember that having it always positive may end
    // up in some tricky numerical artifacts. I.e. zooming into some acute
    // triangle right near the border of the screen. TBD
    // However if negatives are allowed we may end up with even nastier
    // problems because of the GPU HW limitiations. So better do it in a safe,
    // "positive" way always.
    SkRect clipRect = SkRect::MakeLTRB(std::max(0.0f, rect.left()),
                                       std::max(0.0f, rect.top()),
                                       std::max(0.0f, rect.right()),
                                       std::max(0.0f, rect.bottom()));
    clipRect.sort();
    if (clipRect.width() <= FLT_EPSILON || clipRect.height() <= FLT_EPSILON)
        clipRect.setEmpty();

    if (!fRectDefined) {
        if (clipRect.isEmpty())
            return;
        fInnerRect = clipRect;
    } else if (clipRect.right() - FLT_EPSILON <= fInnerRect.left()
            || clipRect.bottom() - FLT_EPSILON <= fInnerRect.top()
            || clipRect.left() + FLT_EPSILON >= fInnerRect.right()
            || clipRect.top() + FLT_EPSILON >= fInnerRect.bottom()) {
        fInnerRect.setEmpty();
    } else {
        fInnerRect.intersect(clipRect);
    }

    SkASSERT(0 <= fInnerRect.left());
    SkASSERT(0 <= fInnerRect.top());
    SkASSERT(0 <= fInnerRect.right());
    SkASSERT(0 <= fInnerRect.bottom());

    fRectDefined = true;
}

void Clip::subtractQuad(const SkPoint pts[4])
{
    fFilter = SlowPass;
    const Plane planes[] = { Plane(pts[0], pts[1]),
                             Plane(pts[1], pts[2]),
                             Plane(pts[2], pts[3]),
                             Plane(pts[3], pts[0]) };

    fOut.push_back(ClipOutQuad(planes));
}

void Clip::addRect(const SkRect& rect)
{
    switch (fFilter) {
    case PassAll:
    case PassRect:
        fFilter = PassRect;
        break;
    case SlowPass:
        break;
    case PassNone:
        return;
    }

    if (rect.isEmpty()) {
        fFilter = PassNone;
        return;
    }

    SkPoint quad[4];
    rect.toQuad(quad);
    addPolygonInternal(quad, 4);
    if (fFilter == PassRect)
        updateRect(rect);
}

void Clip::addPolygon(const SkPoint* pts, int npts)
{
    SkASSERT(npts >= 3);

    switch (fFilter) {
    case PassAll:
    case PassRect:
    case SlowPass:
        fFilter = SlowPass;
        break;
    case PassNone:
        return;
    }

    addPolygonInternal(pts, npts);
}

void Clip::addPolygonInternal(const SkPoint* pts, int npts)
{
    float area = 0;
    for (int i = 0, j = 1; i < npts; ++i, j = (j + 1) % npts) {
        area += pts[i].x()*pts[j].y() - pts[j].x()*pts[i].y();
        if (FloatUtils::isNEq(pts[i], pts[j]))
            fPolygon.push_back(pts[i]);
    }

    if (area > 0) {
        for (int i = 0; i < npts; ++i) {
            Plane plane(pts[(i + 1) % npts], pts[i]);
            if (!plane.isNearZero())
                fPlanes.push_back(plane);
        }
    } else {
        for (int i = 0; i < npts; ++i) {
            Plane plane(pts[i], pts[(i + 1) % npts]);
            if (!plane.isNearZero())
                fPlanes.push_back(plane);
        }
    }
}

void Clip::addRoundedCorner(const SkPoint& clipa, const SkPoint& clipb, const SkPoint& center, const SkPoint& edge)
{
    fFilter = SlowPass;
    fRounded.push_back(Corner(Plane(clipa, clipb), center, edge));
}

void Clip::subtractRoundedCorner(const SkPoint& clipa, const SkPoint& clipb, const SkPoint& center, const SkPoint& edge)
{
    fFilter = SlowPass;
    if (fOut.back().roundedAmount >= 4) {
        SK_CRASH();
    }

    fOut.back().rounded[fOut.back().roundedAmount++] = Corner(Plane(clipa, clipb), center, edge);
}

void Clip::clearRoundedCorner()
{
    fRounded.clear();
}

// These are ax+by+c=0 for the implicit equation of the line.
// Given two points (x0, y0) and (x1, y1)
// We get a system of two equation with three unknowns (a,b,c)
// a * x0 + b * y0 + c = 0
// a * x1 + b * y1 + c = 0
// Or
// a * (x0 - x1) = b * (y1 - y0)
// Since there are three unknowns in a system of two equations we can
// arbitrary pick one of them and derive other two.
// Lets pick: a = y1 - y0
// Then: b = x0 - x1
// And c is derived from
// (y1 - y0) * x0 + (x0 - x1) * y0 + c = 0
// or (y1 - y0) * x1 + (x0 - x1) * y1 + c = 0
void Clip::Plane::ctor(float x0, float y0, float x1, float y1)
{
    fA = y1 - y0;
    fB = x0 - x1;
    fC = y0*x1 - x0*y1;
}

bool Clip::Plane::operator==(const Clip::Plane& rhs) const
{
    return this == &rhs
        || (FloatUtils::isEqual(this->fA, rhs.fA)
         && FloatUtils::isEqual(this->fB, rhs.fB)
         && FloatUtils::isEqual(this->fC, rhs.fC));
}

static Clip::Vertex lerp(const Clip::Vertex& a, const Clip::Vertex& b, float t)
{
    return Clip::Vertex(t * (b.x() - a.x()) + a.x(),
                        t * (b.y() - a.y()) + a.y(),
                        t * (b.u() - a.u()) + a.u(),
                        t * (b.v() - a.v()) + a.v(),
                        t * (b.ou() - a.ou()) + a.ou(),
                        t * (b.ov() - a.ov()) + a.ov(),
                        t * (b.iu() - a.iu()) + a.iu(),
                        t * (b.iv() - a.iv()) + a.iv());
}

inline float Clip::Plane::intersect(const Vertex& v1, const Vertex& v2) const
{
    return -(fA * v1.x() + fB * v1.y() + fC) / (fA * (v2.x() - v1.x()) + fB * (v2.y() - v1.y()));
}

inline float Clip::Plane::getSide(const Vertex& v) const
{
    return fA * v.x() + fB * v.y() + fC;
}

/* Since this function may return a referrence to `tri` if there is no interesction it is only safe to modify triangles
 * in the outside portion of the results which actually reside in `storage` */
const Clip::Triangle* Clip::Plane::intersect(const Triangle& tri, Triangle storage[3], int& inside, int& outside) const
{
    inside = outside = 0;

    const float s0 = getSide(tri.v[0]);
    const float s1 = getSide(tri.v[1]);
    const float s2 = getSide(tri.v[2]);

    if (s0 <= FLT_EPSILON && s1 <= FLT_EPSILON && s2 <= FLT_EPSILON) {
        inside = 0;
        outside = 1;
        storage[0] = tri;
        return storage;
    }
    if (s0 >= -FLT_EPSILON && s1 >= -FLT_EPSILON && s2 >= -FLT_EPSILON) {
        inside = 1;
        outside = 1;
        return &tri;
    }

    // check if the triangle is entirely on the clip plane
    if (FloatUtils::isNearZero(s0) && FloatUtils::isNearZero(s1) && FloatUtils::isNearZero(s2)) {
        inside = 0;
        outside = 0;
        return &tri;
    }

    float s[] = { s0, s1, s2, s0, s1 };
    const Vertex v[] = { tri.v[0], tri.v[1], tri.v[2], tri.v[0], tri.v[1] };

    // Either one vertex is outside and two inside or the other way around
    // or one is on the line. Find the one that's not like the others.
    for (int i = 1; i <= 3; ++i) {
        if (FloatUtils::isNearZero(s[i])) {
            if (FloatUtils::isNearZero(s[i-1])) {
                outside = 1;
                inside = (s[i+1] < 0) ? 0 /* not inside */ : 1 /* inside */;
                return &tri;
            } else if (FloatUtils::isNearZero(s[i+1])) {
                outside = 1;
                inside = (s[i-1] < 0) ? 0 /* not inside */ : 1 /* inside */;
                return &tri;
            } else {
                const float t = intersect(v[i-1], v[i+1]);
                if (s[i+1] > 0) {
                    inside = 1;
                    outside = 2;
                    new (&storage[0]) Triangle(v[i], v[i+1], lerp(v[i-1], v[i+1], t), tri.variant);
                    new (&storage[1]) Triangle(v[i], lerp(v[i-1], v[i+1], t), v[i-1], tri.variant);
                } else {
                    inside = 1;
                    outside = 2;
                    new (&storage[0]) Triangle(v[i], lerp(v[i-1], v[i+1], t), v[i-1], tri.variant);
                    new (&storage[1]) Triangle(v[i], v[i+1], lerp(v[i-1], v[i+1], t), tri.variant);
                }
            }
            return storage;
        }
    }
    for (int i = 1; i <= 3; ++i) {
        if ((s[i] > FLT_EPSILON && s[i-1] <= FLT_EPSILON && s[i+1] <= FLT_EPSILON)
            || (s[i] < -FLT_EPSILON && s[i-1] >= -FLT_EPSILON && s[i+1] >= -FLT_EPSILON)) {
            const float t1 = intersect(v[i], v[i-1]);
            const float t2 = intersect(v[i], v[i+1]);
            const Vertex lerp1 = lerp(v[i], v[i-1], t1);
            const Vertex lerp2 = lerp(v[i], v[i+1], t2);
            if (s[i] > 0) {
                inside = 1;
                outside = 3;
                new (&storage[0]) Triangle(lerp1, v[i], lerp2, tri.variant);
                new (&storage[1]) Triangle(v[i-1], lerp1, lerp2, tri.variant);
                new (&storage[2]) Triangle(v[i-1], lerp2, v[i+1], tri.variant);
            } else {
                inside = 2;
                outside = 3;
                new (&storage[2]) Triangle(lerp1, v[i], lerp2, tri.variant);
                new (&storage[0]) Triangle(v[i-1], lerp1, lerp2, tri.variant);
                new (&storage[1]) Triangle(v[i-1], lerp2, v[i+1], tri.variant);
            }
            return storage;
        }
    }
    return storage;
}

SkRect Clip::enclosingRect(const Clip::Triangle& t)
{
    SkRect enclosingRect = SkRect::MakeXYWH(t.v[0].position.x(), t.v[0].position.y(), 0, 0);
    enclosingRect.growToInclude(&t.v[1].position, 1);
    enclosingRect.growToInclude(&t.v[2].position, 1);

    // The width and height cannot be negative, but they could be zero.
    if (FloatUtils::isNearZero(enclosingRect.width()) || FloatUtils::isNearZero(enclosingRect.height()))
        enclosingRect.setEmpty();

    return enclosingRect;
}

bool Clip::isRectInClip(const SkRect& rect, const SkISize& size) const
{
    // This is really just a quick reject. It can return false even when the
    // rect would pass through wholly unclipped.
    switch (fFilter) {
    case PassAll:
        return true;
    case PassRect: {
        SkRect displayRect = SkRect::MakeWH(size.width(), size.height());
        if (displayRect == fInnerRect) {
            SkRect smaller(rect);
            smaller.inset(5, 5); //check if a small overdraw is present.
            return fInnerRect.contains(smaller);
        }
        return fInnerRect.contains(rect);
    }
    case SlowPass:
    case PassNone:
        break;
    }

    return false;
}

    /*  Vertecies for quads are passed the following fashion:
     *
     *  0--------------1
     *  |              |
     *  |              |
     *  |              |
     *  |              |
     *  3--------------2
     *
     *  Unfortunately this makes it impossible to create quads from strips that contain degenerate triangles.
     *  It is up to the clipping code to swap the order before it gets into the inserter.
     *  The desired order is:
     *
     *  0--------------1
     *  |              |
     *  |              |
     *  |              |
     *  |              |
     *  2--------------3
     *
     */

void Clip::clipQuad(const SkPoint pts[4], Inserter& inserter)
{
    clipQuad(Vertex(pts[0].x(), pts[0].y(), 0, 0),
             Vertex(pts[1].x(), pts[1].y(), 1, 0),
             Vertex(pts[2].x(), pts[2].y(), 1, 1),
             Vertex(pts[3].x(), pts[3].y(), 0, 1), inserter);
}

void Clip::clipQuad(const Vertex& pt0, const Vertex& pt1, const Vertex& pt2, const Vertex& pt3, Inserter& inserter)
{
    switch (fFilter) {
    case PassAll:
        inserter.appendQuad(pt0, pt1, pt3, pt2, DefaultVariant);
        break;
    case PassRect:
        // Fall through to SlowPass if the quad is not completely contained by our innerRect.
        if (fInnerRect.contains(pt0.x(), pt0.y()) && fInnerRect.contains(pt1.x(), pt1.y()) && fInnerRect.contains(pt2.x(), pt2.y()) && fInnerRect.contains(pt3.x(), pt3.y())) {
            inserter.appendQuad(pt0, pt1, pt3, pt2, DefaultVariant);
            break;
        }
    case SlowPass:
        // Since we know it's going to be the slow pass, we can jump right to clipTrianglePlane.
        clipTrianglePlane(Triangle(pt0, pt1, pt3), 0, inserter);
        clipTrianglePlane(Triangle(pt3, pt1, pt2), 0, inserter);
    case PassNone:
        break;
    }
}

void Clip::clipTriangle(const SkPoint pts[3], Inserter& inserter)
{
    clipTriangle(Triangle(Vertex(pts[0].x(), pts[0].y()),
                          Vertex(pts[1].x(), pts[1].y()),
                          Vertex(pts[2].x(), pts[2].y())), inserter);
}

void Clip::clipTriangle(const Triangle& t, Inserter& inserter)
{
    switch (fFilter) {
    case PassAll:
        inserter.appendTriangle(t);
        break;
    case PassRect:
        // Quick accept if inside the inner rect, or fall through to SlowPass.
        if (t.isInside(fInnerRect)) {
            inserter.appendTriangle(t);
            break;
        }
    case SlowPass:
        clipTrianglePlane(t, 0, inserter);
        break;
    case PassNone:
        break;
    }
}

void Clip::clipTrianglePlane(const Triangle& t, unsigned p, Inserter& inserter)
{
    if (p >= fPlanes.size()) {
        clipTriangleRounded(t, 0, inserter);
        return;
    }

    Triangle storage[3];
    int inside, outside;
    const Triangle* result = fPlanes[p].intersect(t, storage, inside, outside);
    for (int i = 0; i < inside; ++i)
        clipTrianglePlane(result[i], p+1, inserter);
}

void Clip::clipTriangleRounded(const Triangle& t, unsigned r, Inserter& inserter)
{
    if (r >= fRounded.size()) {
        clipOutTriangle(t, inserter);
        return;
    }

    Triangle storage[3];
    int inside, outside;
    const Triangle* result = fRounded[r].plane.intersect(t, storage, inside, outside);
    for (int i = 0; i < inside; ++i)
        clipTriangleRounded(result[i], r+1, inserter);
    for (int i = inside; i < outside; ++i) {
        for (int j = 0; j < 3; ++j) {
            // It is safe to write to outside triangles, they are located in `storage` and not some random place on the stack like the inside triangle may be.
            const_cast<Triangle*>(result)[i].v[j].outer.set((result[i].v[j].x() - fRounded[r].center.x()) / (fRounded[r].edge.x() - fRounded[r].center.x()),
                                                            (result[i].v[j].y() - fRounded[r].center.y()) / (fRounded[r].edge.y() - fRounded[r].center.y()));
        }
        // Again, this is safe for outside triangles.
        const_cast<Triangle*>(result)[i].variant = static_cast<CommandVariant>(result[i].variant | OuterRoundedVariant);
        clipOutTriangle(result[i], inserter);
    }
}

void Clip::clipOutTriangle(const Triangle& t, Inserter& inserter)
{
    if (fOut.empty()) {
        inserter.appendTriangle(t);
        return;
    }

    Triangle storage[3];
    int inside, outside;
    cot_clipped.clear();
    cot_clipped.push_back(t);
    for (unsigned int i = 0; i < fOut.size(); ++i) {
        std::vector<Triangle> currentResult;
        const Plane* planes = fOut[i].planes;
        for (int j = 0; j < 4; ++j) {
            cot_insideTris.clear();
            cot_outsideTris.clear();
            for (std::vector<Triangle>::iterator k = cot_clipped.begin(); k != cot_clipped.end(); ++k) {
                const Triangle* result = planes[j].intersect(*k, storage, inside, outside);
                for (int l = 0; l < inside; ++l)
                    cot_insideTris.push_back(result[l]);
                for (int l = inside; l < outside; ++l)
                    cot_outsideTris.push_back(result[l]);
            }
            currentResult.insert(currentResult.end(), cot_insideTris.begin(), cot_insideTris.end());
            cot_clipped.swap(cot_outsideTris);
        }

        for (unsigned int j = 0; j < cot_clipped.size(); ++j)
            clipOutTriangleRounded(cot_clipped[j], i, 0, currentResult);
        cot_clipped.swap(currentResult);
    }
    for (unsigned int i = 0; i < cot_clipped.size(); ++i)
        inserter.appendTriangle(cot_clipped[i]);
}

void Clip::clipOutTriangleRounded(const Triangle& t, unsigned o, unsigned r, std::vector<Triangle>& resultVector)
{
    if (r >= fOut[o].roundedAmount)
        return;

    Triangle storage[3];
    int inside, outside;
    const Triangle* result = fOut[o].rounded[r].plane.intersect(t, storage, inside, outside);

    for (int i = inside; i < outside; ++i) {
        for (int j = 0; j < 3; ++j) {
            // It is safe to write to outside triangles, they are located in `storage` and not some random place on the stack like the inside triangle may be.
            const_cast<Triangle*>(result)[i].v[j].inner.set((result[i].v[j].x() - fOut[o].rounded[r].center.x()) / (fOut[o].rounded[r].edge.x() - fOut[o].rounded[r].center.x()),
                                                            (result[i].v[j].y() - fOut[o].rounded[r].center.y()) / (fOut[o].rounded[r].edge.y() - fOut[o].rounded[r].center.y()));
        }
        // Again, this is safe for outside triangles.
        const_cast<Triangle*>(result)[i].variant = static_cast<CommandVariant>(result[i].variant | InnerRoundedVariant);
        resultVector.push_back(result[i]);
    }

    for (int i = 0; i < inside; ++i)
        clipOutTriangleRounded(result[i], o, r+1, resultVector);
}

} // namespace Gentl
