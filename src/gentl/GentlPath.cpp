/*
 * Copyright (C) 2008 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GentlPath.h"

#include "Generators.h"
#include "tess/tesselator.h"

#include "SkCanvas.h"
#include "SkDashPathEffect.h"
#include "SkGradientShader.h"
#include "SkMathPriv.h"
#include "SkMaskFilter.h"
#include "SkPathMeasure.h"
#include "SkShader.h"

namespace Gentl {

static SkPoint* convertPathPoints(SkPoint dst[], const SkPoint src[], int count)
{
    for (int i = 0; i < count; ++i) {
        dst[i].set(SkScalarToFloat(src[i].fX),
                   SkScalarToFloat(src[i].fY));
    }
    return dst;
}

GentlPath::GentlPath()
    : SkPath()
{
    SkPath::setFillType(SkPath::kEvenOdd_FillType);
}

GentlPath::GentlPath(const SkPath& path)
    : SkPath()
{
    SkPath::setFillType(path.getFillType());
    SkPath::RawIter iter(path);
    SkPoint pts[4];
    SkPoint pathPoints[3];

    for (SkPath::Verb verb = iter.next(pts); verb != SkPath::kDone_Verb; verb = iter.next(pts)) {
        switch (verb) {
        case SkPath::kMove_Verb:
            convertPathPoints(pathPoints, &pts[0], 1);
            moveTo(pathPoints[0]);
            break;
        case SkPath::kLine_Verb:
            convertPathPoints(pathPoints, &pts[1], 1);
            lineTo(pathPoints[0]);
            break;
        case SkPath::kQuad_Verb:
            convertPathPoints(pathPoints, &pts[1], 2);
            quadTo(pathPoints[0], pathPoints[1]);
            break;
        case SkPath::kCubic_Verb:
            convertPathPoints(pathPoints, &pts[1], 3);
            cubicTo(pathPoints[0], pathPoints[1], pathPoints[2]);
            break;
        case SkPath::kClose_Verb:
            closeSubpath();
            break;
        case SkPath::kConic_Verb:
            convertPathPoints(pathPoints, &pts[1], 2);
            conicTo(pathPoints[0], pathPoints[1], iter.conicWeight());
        case SkPath::kDone_Verb:
            break;
        }
    }
}

GentlPath::~GentlPath()
{
}

void GentlPath::moveTo(const SkPoint& point)
{
    if (size_t count = countPoints())
        fPathBreaks.push_back(count);

    SkPath::moveTo(point);
}

void GentlPath::closeSubpath()
{
    SkPoint start = SkPath::getPoint(fPathBreaks.size() ? fPathBreaks.at(fPathBreaks.size() - 1) : 0);
    SkPoint last = SkPath::getPoint(countPoints() - 1);
    if (start != last)
        SkPath::lineTo(start);

    SkPath::close();
}

void GentlPath::reset()
{
    SkPath::reset();
    fPathBreaks.clear();
}

static void* stdAlloc( void* userData, unsigned int size )
{
    int *allocated = (int*)userData;
    *allocated += (int)size;
    return malloc( size );
}

static void stdFree( void* userData, void* ptr )
{
    free( ptr );
}

struct TesselationContext {
    TESSalloc ma;
    TESStesselator* tess;
    int allocated;
    TesselationContext(int numPoints)
        : allocated(0)
    {
        memset(&ma, 0, sizeof(ma));
        ma.memalloc = stdAlloc;
        ma.memfree = stdFree;
        ma.extraVertices = numPoints/4 + 1;
        ma.meshVertexBucketSize = numPoints;
        ma.userData = (void*)&allocated;
        tess = tessNewTess(&ma);
    }
    ~TesselationContext()
    {
        tessDeleteTess(tess);
    }
};

static bool isCurved(const SkPath& path)
{
    return path.getSegmentMasks() & ~SkPath::kLine_SegmentMask;
}

static bool addPoint(std::vector<std::vector<float> >& contours, SkPoint p)
{
    const float clipMax = 1e12f;
    float x = p.x();
    // exclude NaN and Inf values
    if (FloatUtils::isNaN(x) || FloatUtils::isInfinity(x))
        return false;
    float y = p.y();
    if (FloatUtils::isNaN(y) || FloatUtils::isInfinity(y))
        return false;
    // clip points to reasonable values since arithmetic on these numbers can introduce NaNs and Infs
    if (abs(x) > clipMax)
        x = copysign(clipMax, x);
    if (abs(y) > clipMax)
        y = copysign(clipMax, y);
    // ensure there is a float vector to add to (could happen if points are skipped in collectForTesselator)
    if (!contours.size())
        contours.push_back(std::vector<float>());
    contours.back().push_back(x);
    contours.back().push_back(y);
    return true;
}

static void collectFillGeometryForTesselator(const SkMatrix& matrix, const SkPath& originalPath, std::vector<std::vector<float> >& contours)
{
    SkPath path;
    originalPath.transform(matrix, &path);
    SkPathMeasure measure(path, false);
    SkPoint point;
    SkPoint tempPoint;
    SkVector tangent;

    SkMatrix inverseMatrix;
    if(matrix.invert(&inverseMatrix)){
        do {
            // A Path is made up of multiple contours. A contour contains segments of lines,
            // quadratics, and cubic, but no gaps. We iterate over each segment individually.
            const int segmentCount = measure.segments().count();
            contours.push_back(std::vector<float>());
            contours.back().reserve(path.countPoints() * 3);
            float currentDistanceAlongSegment = 0;
            for (int i = 0; i < segmentCount; ++i) { // For each segment in each contour.
                SkPathSegment currentSegment = measure.segments()[i];

                if (measure.getPosTan(currentDistanceAlongSegment, &point, &tangent)) {
                    inverseMatrix.mapXY(point.x(), point.y(), &tempPoint);
                    if (!addPoint(contours, tempPoint))
                        continue;
                }
                float chunkSize = currentSegment.fDistance - currentDistanceAlongSegment;
                if (currentSegment.fVerb == SkPath::kQuad_Verb || currentSegment.fVerb == SkPath::kCubic_Verb) {
                    const float numSteps = currentSegment.fCurvedSegments;
                    const float testChunkSize = chunkSize / numSteps;
                    // only use the result if it is large enough to increment the current distance (preventing an infinite loop)
                    if ((currentDistanceAlongSegment + testChunkSize) != currentDistanceAlongSegment)
                        chunkSize = testChunkSize;
                }

                while ((currentDistanceAlongSegment += chunkSize) < currentSegment.fDistance) {
                    if (measure.getPosTan(currentDistanceAlongSegment, &point, &tangent)) {
                        inverseMatrix.mapXY(point.x(), point.y(), &tempPoint);
                        if (!addPoint(contours, tempPoint))
                            break;
                    }
                }
                currentDistanceAlongSegment = currentSegment.fDistance;
                if (segmentCount == i + 1) {
                    if (measure.getPosTan(currentDistanceAlongSegment, &point, &tangent)) {
                        inverseMatrix.mapXY(point.x(), point.y(), &tempPoint);
                        addPoint(contours, tempPoint);
                    }
                }
            }
            if (!contours.back().size())
                contours.pop_back(); // handle case where we didn't add any points
        } while (measure.nextContour());
    }
}

void GentlPath::collectForTesselator(const SkMatrix& matrix, const SkPath& path, std::vector<std::vector<float> >& contours, const SkPaint& skPaint)
{
    const SkPath* newpath = &path;
    SkPath dashedPath;
    if (skPaint.getPathEffect()) {
        SkStrokeRec rec(skPaint);
        skPaint.getPathEffect()->filterPath(&dashedPath, *newpath, &rec, NULL);
        newpath = &dashedPath;
    }
    if (isCurved(*newpath)) {
        collectFillGeometryForTesselator(matrix, *newpath, contours);
        return;
    }
    SkPoint currentPoints[4];
    SkPath::Iter iter(*newpath, false);
    for (SkPath::Verb verb = iter.next(currentPoints);
         verb != SkPath::kDone_Verb;
         verb = iter.next(currentPoints)) {
        switch (verb) {
        case SkPath::kMove_Verb:
            contours.push_back(std::vector<float>());
            if (!addPoint(contours, currentPoints[0]))
                contours.pop_back(); // ensure at least one point exists in each vector; add to the previous one if there is a problem
            break;
        case SkPath::kLine_Verb:
            // iter.next returns 2 points, from and to
            addPoint(contours, currentPoints[1]);
            break;
        case SkPath::kClose_Verb:
            addPoint(contours, currentPoints[0]);
            break;
        default:
            break;
        }
    }
}

void GentlPath::fill(GentlContext* context, const SkPaint& skPaint) const
{
    if (!context->isDrawingEnabled())
        return;

    // if we have less than 3 points then there is nothing to fill.
    if (countPoints() < 3)
        return;

    // use glu library to tesselate if possible...
    TesselationContext ctx(countPoints());
    if (!ctx.tess)
        return;

    std::vector<std::vector<float> > contours;
    contours.reserve(3);
    collectForTesselator(context->matrix(), *this, contours, skPaint);
    std::vector<std::vector<float> >::iterator itr;
    static const size_t stride = sizeof(float) * 2;
    for (itr = contours.begin(); itr != contours.end() ; ++itr)
        tessAddContour(ctx.tess, 2, &itr->at(0), stride, itr->size() / 2);

    static float normal[3] = {0,0,1};
    //   1 if succeed, 0 if failed.
    if (tessTesselate(ctx.tess, SkPath::getFillType() == SkPath::kWinding_FillType ? TESS_WINDING_NONZERO : TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal)) {
        const int numElements = tessGetElementCount(ctx.tess) * 3;
        if (numElements) {
            const float* vertices = tessGetVertices(ctx.tess);
            const int* elements = tessGetElements(ctx.tess);
            std::vector<SkPoint> points;
            points.reserve(numElements);
            for (int i = 0; i < numElements; ++i) {
                points.push_back(SkPoint::Make(vertices[elements[i]*2], vertices[elements[i]*2+1]));
            }

            SkPaint black = skPaint;
            black.setColor(SK_ColorBLACK);
            bool needsTransparencyLayer = skPaint.getAlpha() != SK_AlphaOPAQUE || skPaint.getShader();
            if (needsTransparencyLayer) {
                context->pushLayer();
            }

            SkMaskFilter::BlurInfo blurInfo;
            bool doBlur = context->getBlurInfo(skPaint, blurInfo) != SkMaskFilter::kNone_BlurType;
            if (doBlur) {
                blurInfo.fRadius = context->matrix().mapRadius(blurInfo.fRadius);
                context->pushLayer();
                if (!needsTransparencyLayer)
                    context->pushLayer();
            }

            context->addVertices(&points[0], points.size(), SkCanvas::kTriangles_VertexMode, needsTransparencyLayer ? black : skPaint);

            if (doBlur) {
                context->popLayer(1.0f, SkSize::Make(1.0f, 0), blurInfo.fRadius);
                if (needsTransparencyLayer) {
                    const SkXfermode::Mode oldXferMode = context->xferMode();
                    context->setXfermodeMode(SkXfermode::kSrcIn_Mode);
                    SkPaint boundsPaint(skPaint);
                    boundsPaint.setAlpha(SK_AlphaOPAQUE);
                    context->paintRect(SkPath::getBounds(), boundsPaint);
                    context->setXfermodeMode(oldXferMode);
                }
                context->popLayer(skPaint.getAlpha() / 255.0f, SkSize::Make(0, 1.0f), blurInfo.fRadius);
            } else if (needsTransparencyLayer) {
                const SkXfermode::Mode oldXferMode = context->xferMode();
                context->setXfermodeMode(SkXfermode::kSrcIn_Mode);
                SkPaint boundsPaint(skPaint);
                boundsPaint.setAlpha(SK_AlphaOPAQUE);
                context->paintRect(SkPath::getBounds(), boundsPaint);
                context->setXfermodeMode(oldXferMode);
                context->popLayer(skPaint.getAlpha() / 255.0f);
            }
        }
    }
}

bool GentlPath::drawTrianglesForPath(GentlContext* context, const SkPaint& paint) const
{
    size_t start = 0;

    size_t end = fPathBreaks.size();
    bool success = true;
    size_t i = 0;
    do {
        if (!drawPath(context, start, ((i == end) ? countPoints() : fPathBreaks.at(i)) - 1, paint))
            success = false;
        else if (i < end)
            start = fPathBreaks.at(i);
    } while (success && ++i <= end);
    return success;
}

static SkPoint calculateArcControlPoint(const SkPoint& alpha, const SkPoint& beta, const SkPoint& joint,
                                           const SkPoint& prevNormVec, const SkPoint& normVec)
{
    // Inspiration from http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/NURBS/RB-circles.html
    // for orthogonal lines, the join is the correct control point for the arc (sin(90) = 1). For other angles use
    // the factor w.
    float dot = prevNormVec.dot(normVec);
    // product of squared lengths
    float lengthsSquared = (prevNormVec.x() * prevNormVec.x() + prevNormVec.y() * prevNormVec.y()) * (normVec.x() * normVec.x() + normVec.y() * normVec.y());
    // angle between the previous line segment and the current one
    double angle = acos(dot / sqrt(lengthsSquared));
    // calculate w from angle
    double w = sin(angle);
    SkPoint mid = SkPoint::Make((alpha.x() + beta.x()) / 2.0f, (alpha.y() + beta.y()) / 2.0f); // mid-point on the chord
    return SkPoint::Make(mid.x() + (joint.x() - mid.x()) * w,
                         mid.y() + (joint.y() - mid.y()) * w);
}

void GentlPath::stroke(GentlContext* context, const SkPaint& skPaint) const
{
    const float halfStroke = skPaint.getStrokeWidth() / 2.0f;
    if (countPoints() < 2 || FloatUtils::isNearZero(halfStroke))
        return;

    SkMaskFilter::BlurInfo blurInfo;
    SkPaint opaquePaint(skPaint);
    bool needsTransparencyLayer = skPaint.getAlpha() != SK_AlphaOPAQUE || skPaint.getShader();
    const bool doBlur = context->getBlurInfo(skPaint, blurInfo) != SkMaskFilter::kNone_BlurType;
    if (doBlur) {
        context->pushLayer();
        context->pushLayer();
        opaquePaint.setAlpha(SK_AlphaOPAQUE);
    } else if (needsTransparencyLayer) {
        context->pushLayer();
        opaquePaint.setAlpha(SK_AlphaOPAQUE);
    }
    // Fast path for no effect, non-curving paths.
    const bool needsTesselation = skPaint.getPathEffect() || skPaint.getShader();
    if (!(needsTesselation || isCurved(*this)) && drawTrianglesForPath(context, opaquePaint)) {
        if (doBlur) {
            context->popLayer(1.0f, SkSize::Make(1.0f, 0), blurInfo.fRadius);
            context->popLayer(skPaint.getAlpha() / 255.0f, SkSize::Make(0, 1.0f), blurInfo.fRadius);
        } else if (needsTransparencyLayer) {
            context->popLayer(skPaint.getAlpha() / 255.0f);
        }
        return;
    }

    // Create a vector to store a strip of triangles to hold the tesselated stroke,
    // and a vector of circle origins of strokeThickness diameter for round joins and caps.
    std::vector<SkPoint> roundSegments;
    std::vector<SkPoint> triangleStrip;
    extractStrokeGeometry(context, triangleStrip, roundSegments, skPaint);

    // We need a layer when we need to fill in round joins, or use sophisticated paints.
    // In these cases we draw in a solid black alpha mask, and then SourceIn the paint.
    // We employ an opaque black alpha mask, but the hue is actually irrelevant.
    SkPaint black;
    black.setColor(SK_ColorBLACK);
    paintStrokeGeometry(context, triangleStrip, roundSegments, needsTransparencyLayer || doBlur ? black : skPaint, needsTransparencyLayer);

    if (doBlur) {
        context->popLayer(1.0f, SkSize::Make(1.0f, 0), blurInfo.fRadius);
        if (needsTransparencyLayer) {
            const SkXfermode::Mode oldXferMode = context->xferMode();
            context->setXfermodeMode(SkXfermode::kSrcIn_Mode);

            SkPaint boundsPaint(skPaint);
            boundsPaint.setStyle(SkPaint::kFill_Style);
            boundsPaint.setAlpha(SK_AlphaOPAQUE);
            context->paintRect(getStrokeBounds(context, skPaint), boundsPaint);
            context->setXfermodeMode(oldXferMode);
        }
        context->popLayer(skPaint.getAlpha() / 255.0f, SkSize::Make(0, 1.0f), blurInfo.fRadius);
    } else if (needsTransparencyLayer) {
        const SkXfermode::Mode oldXferMode = context->xferMode();
        context->setXfermodeMode(SkXfermode::kSrcIn_Mode);

        SkPaint boundsPaint(skPaint);
        boundsPaint.setStyle(SkPaint::kFill_Style);
        boundsPaint.setAlpha(SK_AlphaOPAQUE);
        context->paintRect(getStrokeBounds(context, skPaint), boundsPaint);

        context->setXfermodeMode(oldXferMode);
        context->popLayer(skPaint.getAlpha() / 255.0f);
    }
}

void GentlPath::paintStrokeGeometry(GentlContext* context, std::vector<SkPoint>& triangleStrip, const std::vector<SkPoint>& roundSegments, const SkPaint& paint, bool usingTransparencyLayer) const
{
    const float oldAlpha = context->alpha();
    const SkXfermode::Mode oldXferMode = context->xferMode();
    if (usingTransparencyLayer) {
        context->setAlpha(1.0f);
        context->setXfermodeMode(SkXfermode::kSrc_Mode);
    }

    // Draw the circles first as they require rounded variants. Our optimized CompositeCopy
    // will leave white artifacts around them, but they will be harmlessly drawn over by
    // the triangle strip we generated for the path itself.
    SolidColorGen gen(context, SkPaintToABGR32Premultiplied(paint));
    for (unsigned i = 0; i < roundSegments.size(); i += 3)
        context->addRound(gen, roundSegments[i], roundSegments[i + 1], roundSegments[i + 2]);
    SkRect rect = getStrokeBounds(context, paint);
    context->matrix().mapRect(&rect);
    if (context->currentClip().isRectInClip(rect, context->size())) {
        SolidColorGen stripGen(context, SkPaintToABGR32Premultiplied(paint), StripMode);
        context->matrix().mapPoints(&triangleStrip[0], triangleStrip.size());
        stripGen.appendPoints(triangleStrip, rect);
    } else
        context->addVertices(&triangleStrip[0], triangleStrip.size(), SkCanvas::kTriangleStrip_VertexMode, paint);

    if (usingTransparencyLayer) {
        context->setAlpha(oldAlpha);
        context->setXfermodeMode(oldXferMode);
    }
}

void GentlPath::extractStrokeGeometry(const GentlContext* context, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPaint& skPaint) const
{
    const SkPath* path = this;
    SkPath dashedPath;
    if (skPaint.getPathEffect()) {
         SkStrokeRec rec(skPaint);
         skPaint.getPathEffect()->filterPath(&dashedPath, *path, &rec, NULL);
         path = &dashedPath;
    }

    SkPathMeasure measure(*path, false);
    SkPoint point = SkPoint::Make(0, 0);
    SkVector tangent = point;
    SkVector normal = point;
    SkVector previousNormal = point;
    const float halfStroke = skPaint.getStrokeWidth() / 2.0f;
    const SkPaint::Cap lineCap = skPaint.getStrokeCap();

    const SkMatrix& matrix = context->matrix();

    do {
        // A Path is made up of multiple contours. A contour contains segments of lines,
        // quadratics, and cubic, but no gaps. We iterate over each segment individually.
        const int segmentCount = measure.segments().count();
        const bool hasCaps = !measure.isClosed();

        float currentDistanceAlongSegment = 0;
        for (int i = 0; i < segmentCount; ++i) { // For each segment in each contour.
            SkPathSegment currentSegment = measure.segments()[i];

            // For each segment we start slightly ahead to avoid crossed lines if the last
            // tangent from the last segment happened to point the other direction.
            if (measure.getPosTan(currentDistanceAlongSegment + 0.001f, &point, &tangent))
                tangent.scale(halfStroke);
            normal.setOrthog(tangent, SkPoint::kRight_Side);

            // We need to add a couple of degenerate triangles to move the next contour.
            if (!i && triangleStrip.size()) {
                triangleStrip.push_back(triangleStrip.back());
                triangleStrip.push_back(point + normal);
            }

            triangleStrip.push_back(point + normal);
            triangleStrip.push_back(point - normal);

            if (i)
                appendJoin(context, triangleStrip, roundSegments, point, normal, previousNormal, skPaint);
            else if (hasCaps)
                appendCap(lineCap, triangleStrip, roundSegments, point, tangent, true);

            // Compute distance as shown on screen, taking into account the global matrix
            float chunkSize = calculateStepForSegment(measure, currentSegment, point, currentDistanceAlongSegment, matrix);

            do {
                currentDistanceAlongSegment = std::min(currentDistanceAlongSegment + chunkSize, currentSegment.fDistance);
                if (measure.getPosTan(currentDistanceAlongSegment, &point, &tangent))
                    tangent.scale(halfStroke);
                normal.setOrthog(tangent, SkPoint::kRight_Side); // Change from a tangent to a normal vector.
                triangleStrip.push_back(point + normal);
                triangleStrip.push_back(point - normal);
            } while (currentDistanceAlongSegment < currentSegment.fDistance);
            previousNormal = normal;
        }

        if (hasCaps) {
            if (FloatUtils::isGreaterZero(measure.getLength()))
                appendCap(lineCap, triangleStrip, roundSegments, point, tangent, false);
        } else {
            // For a closing join we treat it like connecting back to the beginning.
            if (measure.getPosTan(0.001f, &point, &tangent))
                tangent.scale(halfStroke);
            normal.setOrthog(tangent, SkPoint::kRight_Side);

            triangleStrip.push_back(point + normal);
            triangleStrip.push_back(point - normal);
            appendJoin(context, triangleStrip, roundSegments, point, normal, previousNormal, skPaint);
        }
    } while (measure.nextContour());
}

void GentlPath::appendJoin(const GentlContext* context, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPoint& intersection, const SkPoint& normal, const SkPoint& previousNormal, const SkPaint& paint) const
{
    if (paint.getStrokeJoin() == SkPaint::kBevel_Join)
        return;

    /**
     * Calculate the points of intersection of the outside edges of the stroke.
     * calculate the constant 'c' for the four lines with equation: ax + by + c = 0
     * where 'a' is normal.x() and 'b' is normal.y(), then solve to obtain joints.
     *
     *  joint1
     *    \.____________________________
     *     |
     *     |   intersection
     *     |   +------------------------
     *     |   :
     *     |   :   .____________________
     *     |   :   |\          normal |
     *     |   :   | joint2           v
     *     |   :   |
     *     |   :   | --> previousNormal
     *
     */
    const float sign = previousNormal.cross(normal);
    if (fabs(sign) < 0.03f) {
        if (previousNormal.dot(normal) > 0)
            return;
        // The path is doubling back on itself. Only a RoundJoin is possible;
        // in this instance it is identical to a RoundCap, which is more efficient.
        if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
            const SkPoint tangent = SkPoint::Make(-previousNormal.y(), previousNormal.x());
            appendCap(SkPaint::kRound_Cap, triangleStrip, roundSegments, intersection, tangent, false);
        }
        return;
    }

    // Now we know the path is not doubling back on itself so we can calculate the
    // joints at which the stroke borders intersect, and draw the appropriate join.
    const float constant1 = -1 * previousNormal.dot(intersection + previousNormal);
    const float constant2 = -1 * normal.dot(intersection + normal);
    const float constant3 = -1 * previousNormal.dot(intersection - previousNormal);
    const float constant4 = -1 * normal.dot(intersection - normal);
    SkPoint joint1 = SkPoint::Make((constant2 * previousNormal.y() - constant1 * normal.y()) / sign,
                                   (constant2 * previousNormal.x() - constant1 * normal.x()) / -sign);
    SkPoint joint2 = SkPoint::Make((constant4 * previousNormal.y() - constant3 * normal.y()) / sign,
                                   (constant4 * previousNormal.x() - constant3 * normal.x()) / -sign);

    if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
        if (sign > 0) {
            roundSegments.push_back(intersection + previousNormal);
            roundSegments.push_back(intersection + normal);
            roundSegments.push_back(calculateArcControlPoint(intersection + previousNormal, intersection + normal, joint1, previousNormal, normal));
        } else {
            roundSegments.push_back(intersection - previousNormal);
            roundSegments.push_back(intersection - normal);
            roundSegments.push_back(calculateArcControlPoint(intersection - previousNormal, intersection - normal, joint2, previousNormal, normal));
        }
    } else {
        // We determine if the miter should be drawn by ensuring the miter distance is
        // not too large according to spec. Then depending on the turn we draw the
        // inner or outer miter joint and return the strip to the former state.
        const SkPoint delta = joint1 - intersection;
        const bool drawMiter = paint.getStrokeJoin() == SkPaint::kMiter_Join && sqrt(delta.x() * delta.x() + delta.y() * delta.y()) < paint.getStrokeWidth() / 2.0f * paint.getStrokeMiter();
        if (drawMiter) {
            if (sign > 0) {
                // Right turn.
                triangleStrip.push_back(intersection - normal);
                triangleStrip.push_back(joint2);
                triangleStrip.push_back(intersection - previousNormal);
            } else {
                // Left turn.
                triangleStrip.push_back(intersection + previousNormal);
                triangleStrip.push_back(joint1);
                triangleStrip.push_back(intersection - normal);
            }

            triangleStrip.push_back(intersection + normal);
            triangleStrip.push_back(intersection - normal);
        }
    }
}

void GentlPath::appendCap(const SkPaint::Cap lineCap, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPoint& point, const SkPoint& inTangent, bool isStartCap) const
{
    const SkPoint tangent = isStartCap ? SkPoint::Make(0, 0) - inTangent : inTangent;
    const SkPoint normal = SkPoint::Make(tangent.y(), -tangent.x());
    switch (lineCap) {
    case SkPaint::kSquare_Cap: {
        const SkPoint first = isStartCap ? point + normal : point - normal;
        const SkPoint second = isStartCap ? point - normal : point + normal;
        triangleStrip.push_back(first + tangent);
        triangleStrip.push_back(second + tangent);
        triangleStrip.push_back(triangleStrip.back()); // Degenerate
        triangleStrip.push_back(first);
        triangleStrip.push_back(second);
        break;
    }
    case SkPaint::kRound_Cap: {
        roundSegments.push_back(point + normal);
        roundSegments.push_back(point + tangent);
        roundSegments.push_back(calculateArcControlPoint(point + normal, point + tangent, point + normal + tangent, normal, tangent));
        roundSegments.push_back(point - normal);
        roundSegments.push_back(point + tangent);
        roundSegments.push_back(calculateArcControlPoint(point - normal, point + tangent, point - normal + tangent, normal, SkPoint::Make(0, 0) - tangent));

        triangleStrip.push_back(point + tangent);
        triangleStrip.push_back(triangleStrip.back());
        triangleStrip.push_back(point + normal);
        triangleStrip.push_back(triangleStrip.back());
        triangleStrip.push_back(point - normal);
        break;
    }
    case SkPaint::kButt_Cap:
        break;
    case SkPaint::kCapCount:
        break;
    }
}

SkRect GentlPath::getStrokeBounds(const GentlContext* context, const SkPaint& paint) const
{
    const float halfStroke = paint.getStrokeWidth() / 2.0f;

    // With nothing but bevel joints and butt caps, the stroke will never exceed halfStroke.
    float maxStrokeDistanceFromPath = halfStroke;

    // Round joins and caps can extend up to precisely halfStroke plus a buffer for anti-aliasing.
    // Square caps can extend this further to halfStroke * sqrt(2) ~= 1.414
    // Miter joins can extend significantly further, up to halfStroke * miterLimit.
    // Thus we check for each of these conditions in the reverse order.
    if (paint.getStrokeJoin() == SkPaint::kMiter_Join)
        maxStrokeDistanceFromPath = halfStroke * std::max(1.5f, paint.getStrokeMiter());
    else if (paint.getStrokeCap() == SkPaint::kSquare_Cap)
        maxStrokeDistanceFromPath = halfStroke * 1.5f;
    else if (paint.getStrokeJoin() == SkPaint::kRound_Join || paint.getStrokeCap() == SkPaint::kRound_Cap)
        maxStrokeDistanceFromPath = halfStroke * 1.1f;

    SkRect bounds = SkPath::getBounds();
    bounds.outset(maxStrokeDistanceFromPath, maxStrokeDistanceFromPath);
    return bounds;
}

static bool isJointFar(const SkPoint& joint, const SkPoint& start, const SkPoint& middle, const SkPoint& end, float strokeLength)
{
    float dist1 = (start.x() - middle.x()) * (start.x() - middle.x()) + (start.y() - middle.y()) * (start.y() - middle.y());
    float dist2 = (end.x() - middle.x()) * (end.x() - middle.x()) + (end.y() - middle.y()) * (end.y() - middle.y());
    SkPoint v = SkPoint::Make(joint.x() - middle.x(), joint.y() - middle.y());
    if (dist1 < dist2) {
        SkPoint u = SkPoint::Make(start.x() - middle.x(), start.y() - middle.y());
        float dot = abs(u.dot(v));
        if (dot > dist1 && dot > (strokeLength * strokeLength))
            return true;
    } else {
        SkPoint u = SkPoint::Make(end.x() - middle.x(), end.y() - middle.y());
        float dot = abs(u.dot(v));
        if (dot > dist2 && dot > (strokeLength * strokeLength))
            return true;
    }
    return false;
}

void GentlPath::drawRoundedCap(const SkPath* path, GentlContext* context, size_t pos, bool startCap, SolidColorGen& gen, float halfStroke)
{
    SkPoint fromFloat = path->getPoint(pos);
    SkPoint toFloat = path->getPoint(startCap ? pos + 1 : pos - 1);
    const SkMatrix oldMatrix = context->matrix();
    SkMatrix rotated = oldMatrix;
    const double angle = fmod(atan2(toFloat.y() - fromFloat.y(), toFloat.x() - fromFloat.x()) / M_PI * 180.0, 360.0);
    rotated.preRotate(angle, fromFloat.x(), fromFloat.y());
    context->setMatrix(rotated);
    context->addSolidArc(gen, fromFloat, SkPoint::Make(-halfStroke, -halfStroke), fromFloat, SkPoint::Make(0, 0));
    context->addSolidArc(gen, fromFloat, SkPoint::Make(-halfStroke, halfStroke), fromFloat, SkPoint::Make(0, 0));
    context->setMatrix(oldMatrix);
}

float GentlPath::calculateStepForSegment(SkPathMeasure& measure, const SkPathSegment& currentSegment, const SkPoint& startPoint, float currentDistanceAlongSegment, const SkMatrix& matrix)
{
    float length = currentSegment.fDistance - currentDistanceAlongSegment;
    // For straight segments the chunk is the length of the segment, for curving
    // segments we choose the larger of 1% of its length, or a quarter of the total
    // curve length, see http://antigrain.com/research/adaptive_bezier/
    if (currentSegment.fVerb == SkPath::kQuad_Verb || currentSegment.fVerb == SkPath::kCubic_Verb) {
        SkPoint start;
        matrix.mapXY(startPoint.x(), startPoint.y(), &start);
        // take a quarter length of the curve as approximation
        SkPoint quarterPoint;
        if (measure.getPosTan(currentDistanceAlongSegment + (currentSegment.fDistance - currentDistanceAlongSegment) / 4.0f, &quarterPoint, 0)) {
            SkPoint quarter;
            matrix.mapXY(quarterPoint.x(), quarterPoint.y(), &quarter);
            // Compute distance as shown on screen, taking into account the global matrix
            // Note: this actually is (distanceBetweenPoints(start, quarter) * 4.0f) / 4.0f.
            const float numSteps = std::max(std::min(100.0f, SkPoint::Distance(start, quarter)), 1.0f);
            const float step = length / numSteps;
            // only use the result if it is large enough to increment the current distance (preventing an infinite loop)
            if ((currentDistanceAlongSegment + step) != currentDistanceAlongSegment)
                length = step;
        }
    }
    return length;
}

bool GentlPath::drawPath(GentlContext* context, size_t start, size_t end, const SkPaint& paint) const
{
    SkPoint alphaOne, alphaTwo, betaOne, betaTwo, prevAlphaOne, prevAlphaTwo, prevBetaOne, prevBetaTwo, firstJoint1, firstJoint2, prevNormVec;
    SkPoint startPoint = SkPath::getPoint(start);
    SkPoint endPoint = SkPath::getPoint(end);
    bool isClosed = startPoint == endPoint;
    bool handleBetaFlip = false;

    if (isClosed) {
        SkPath::Iter iter(*this, false);
        isClosed = iter.isClosedContour();
    }
    std::vector<SkPoint> vertices;
    std::vector<SkPoint> circularSegmentPoints;
    std::vector<size_t> roundedCaps;
    if (!isClosed && paint.getStrokeCap() == SkPaint::kRound_Cap) {
        roundedCaps.push_back(start);
        roundedCaps.push_back(end);
    }
    float halfStroke = paint.getStrokeWidth() / 2.0f;
    for (size_t i = start + 1; i <= end; ++i) {
        SkPoint fromFloat = SkPath::getPoint(i - 1);
        SkPoint toFloat = SkPath::getPoint(i);
        if (fromFloat == toFloat)
            continue;
        const SkPoint normVec = SkPoint::Make(fromFloat.y() - toFloat.y(), toFloat.x() - fromFloat.x());
        const float normLength = sqrtf(normVec.x() * normVec.x() + normVec.y() * normVec.y());
        const SkPoint normalStroke = SkPoint::Make(normVec.x() * halfStroke / normLength , normVec.y() * halfStroke / normLength);

        if (i > start + 1) {
            // keep track of the previous alpha and beta values.
            prevAlphaOne = alphaOne;
            prevAlphaTwo = alphaTwo;
            prevBetaOne = betaOne;
            prevBetaTwo = betaTwo;
        }
        alphaOne.set(fromFloat.x() + normalStroke.x(),
                     fromFloat.y() + normalStroke.y());
        alphaTwo.set(fromFloat.x() - normalStroke.x(),
                     fromFloat.y() - normalStroke.y());

        if ((i == start + 1) && !isClosed) {
            if (paint.getStrokeCap() == SkPaint::kSquare_Cap) {
                alphaOne.set(alphaOne.x() - normalStroke.y(),
                             alphaOne.y() + normalStroke.x());
                alphaTwo.set(alphaTwo.x() - normalStroke.y(),
                             alphaTwo.y() + normalStroke.x());
            }
        }

        betaOne.set(toFloat.x() + normalStroke.x(),
                    toFloat.y() + normalStroke.y());
        betaTwo.set(toFloat.x() - normalStroke.x(),
                    toFloat.y() - normalStroke.y());

        // We only want to draw normalized edge of the rectangle for a
        // single line segment (i.e. fPathBreaks consists of only 2 points).
        if (end - start == 1) {
            vertices.push_back(alphaOne);
            vertices.push_back(alphaTwo);
            if (paint.getStrokeCap() == SkPaint::kSquare_Cap) {
                betaOne.set(betaOne.x() + normalStroke.y(),
                            betaOne.y() - normalStroke.x());
                betaTwo.set(betaTwo.x() + normalStroke.y(),
                            betaTwo.y() - normalStroke.x());
            }
            vertices.push_back(betaOne);
            vertices.push_back(betaTwo);
            break;
        }
        if (i > start + 1) {
            // calculate the constant 'c' for the four lines with equation: ax + by + c = 0
            // where 'a' is normVec.x() and 'b' is normVec.y()
            const float constant1 = -1 * prevNormVec.dot(prevAlphaOne);
            const float constant2 = -1 * normVec.dot(betaOne);

            const float constant3 = -1 * prevNormVec.dot(prevAlphaTwo);
            const float constant4 = -1 * normVec.dot(betaTwo);

            // if the number is close enough to zero then the joints might as well be the end points.
            float xJoint1 = (constant2 * prevNormVec.y() - constant1 * normVec.y()) / prevNormVec.cross(normVec);
            float yJoint1 = (constant2 * prevNormVec.x() - constant1 * normVec.x()) / normVec.cross(prevNormVec);
            float xJoint2 = (constant4 * prevNormVec.y() - constant3 * normVec.y()) / prevNormVec.cross(normVec);
            float yJoint2 = (constant4 * prevNormVec.x() - constant3 * normVec.x()) / normVec.cross(prevNormVec);

            float sign = normVec.cross(prevNormVec);
            SkPoint joint1 = SkPoint::Make(xJoint1, yJoint1);
            SkPoint joint2 = SkPoint::Make(xJoint2, yJoint2);

            if (fabs(sign) < 0.001) {
                joint1 = prevBetaOne;
                joint2 = prevBetaTwo;
                // Swap betaOne and betaTwo so that there's no criss-crossing of triangles.
                if (prevNormVec.dot(normVec) < 0) {
                    std::swap(betaOne, betaTwo);

                    // Handling this special case requires adjustments on both ends of the triangle strip.
                    // The following flag is used on the consecutive iteration
                    if (FloatUtils::isEq(joint1, alphaTwo))
                        handleBetaFlip = true;

                    if (paint.getStrokeJoin() == SkPaint::kRound_Join)
                        roundedCaps.push_back(i - 1); // 180 degree turn with round join should look like a round cap
                }
            } else if (isJointFar(sign > 0 ? joint2 : joint1, SkPath::getPoint(i - 2), fromFloat, toFloat, halfStroke * 2))
                // We can't do this easily and quickly in hardware so we fall back to software.
                return false;

            bool shouldBevel = true;
            if (paint.getStrokeJoin() == SkPaint::kMiter_Join) {
                float deltaX1 = fromFloat.x() - xJoint1;
                float deltaY1 = fromFloat.y() - yJoint1;
                float deltaX2 = fromFloat.x() - xJoint2;
                float deltaY2 = fromFloat.y() - yJoint2;
                float miterHeight = sign < 0 ? sqrt(deltaX1 * deltaX1  + deltaY1 * deltaY1) : (sign > 0 ? sqrt(deltaX2 * deltaX2  + deltaY2 * deltaY2) : 0);
                shouldBevel = miterHeight > halfStroke * paint.getStrokeMiter();
            }
            if (isClosed && i == start + 2) {
                if (!shouldBevel) {
                    firstJoint1 = joint1;
                    firstJoint2 = joint2;
                } else if (sign > 0) {
                    firstJoint1 = prevBetaOne;
                    firstJoint2 = joint2;

                    const SkPoint v1 = alphaOne = prevBetaOne;
                    const SkPoint v2 = joint2 - prevBetaOne;

                    // Starting triangle of the triangle strip is expected to be CW, even if it has to be degenerate.
                    if (v1.cross(v2) < 0.0)
                        vertices.push_back(prevBetaOne);

                    vertices.push_back(prevBetaOne);
                    vertices.push_back(alphaOne);
                    vertices.push_back(joint2);
                    if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                        circularSegmentPoints.push_back(prevBetaOne);
                        circularSegmentPoints.push_back(alphaOne);
                        circularSegmentPoints.push_back(calculateArcControlPoint(prevBetaOne, alphaOne, joint1, prevNormVec, normVec));
                    }
                    joint1 = alphaOne;
                } else {
                    firstJoint1 = joint1;
                    firstJoint2 = prevBetaTwo;

                    const SkPoint v1 = joint1 - prevBetaTwo;
                    const SkPoint v2 = alphaTwo - prevBetaOne;

                    // Starting triangle of the triangle strip is expected to be CW, even if it has to be degenerate.
                    if (v1.cross(v2) < 0.0)
                        vertices.push_back(prevBetaOne);

                    if (handleBetaFlip) {
                        // Since this case handles both (sign == 0) && (sign < 0), adding a special case handling code here
                        // prevNormal.dot(normVec) = -1 that implies joint1 = alphaTwo, which needs to be handled seprately to avoid
                        // prevBetaTwojoint1-alphaTwo being a degenerate triangle.
                        vertices.push_back(prevBetaTwo);
                        vertices.push_back(alphaTwo);
                    } else {
                        // Generic case.
                        vertices.push_back(prevBetaTwo);
                        vertices.push_back(joint1);
                        vertices.push_back(alphaTwo);
                    }

                    if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                        circularSegmentPoints.push_back(prevBetaTwo);
                        circularSegmentPoints.push_back(alphaTwo);
                        circularSegmentPoints.push_back(calculateArcControlPoint(prevBetaTwo, alphaTwo, joint2, prevNormVec, normVec));
                    }
                    joint2 = alphaTwo;
                }
            }

            if (!isClosed || i > start + 2) {
                if (!vertices.size()) {
                    vertices.push_back(prevAlphaOne);
                    vertices.push_back(prevAlphaTwo);
                }
                if (!shouldBevel) {
                    vertices.push_back(joint1);
                    vertices.push_back(joint2);
                } else if (sign > 0) {
                    vertices.push_back(prevBetaOne);
                    vertices.push_back(joint2);
                    vertices.push_back(alphaOne);
                    vertices.push_back(joint2);
                    if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                        circularSegmentPoints.push_back(prevBetaOne);
                        circularSegmentPoints.push_back(alphaOne);
                        circularSegmentPoints.push_back(calculateArcControlPoint(prevBetaOne, alphaOne, joint1, prevNormVec, normVec));
                    }
                    joint1 = alphaOne;
                } else {
                    // Since this case handles both (sign == 0) && (sign < 0), adding a special case handling code here
                    // prevNormal.dot(normVec) = -1 that implies joint1 = alphaTwo, which needs to be handled seprately to avoid
                    // joint1-alphaTwo-joint1-prevBetaOne-betaOne being all degenerate triangles.
                    if (FloatUtils::isEq(joint1, alphaTwo)) {
                        // Corner-case
                        vertices.push_back(joint1);
                        vertices.push_back(prevBetaTwo);
                        vertices.push_back(prevBetaTwo);
                        vertices.push_back(joint1);
                    } else {
                        // Second stage of joint1 == alphaTwo special case.
                        if (handleBetaFlip) {
                            vertices.push_back(prevBetaTwo);
                        }

                        // Generic case
                        vertices.push_back(joint1);
                        vertices.push_back(prevBetaTwo);
                        vertices.push_back(alphaTwo);

                        if (handleBetaFlip) {
                            handleBetaFlip = false;
                            vertices.push_back(alphaTwo);
                        }

                        vertices.push_back(alphaTwo);
                        vertices.push_back(joint1);
                        vertices.push_back(alphaTwo);
                    }

                    if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                        circularSegmentPoints.push_back(prevBetaTwo);
                        circularSegmentPoints.push_back(alphaTwo);
                        circularSegmentPoints.push_back(calculateArcControlPoint(prevBetaTwo, alphaTwo, joint2, prevNormVec, normVec));
                    }
                    joint2 = alphaTwo;
                }
            }

            if (i == end) {
                if (!isClosed) {
                    if (paint.getStrokeCap() == SkPaint::kSquare_Cap) {
                        betaOne.set(betaOne.x() + normalStroke.y(),
                                    betaOne.y() - normalStroke.x());
                        betaTwo.set(betaTwo.x() + normalStroke.y(),
                                    betaTwo.y() - normalStroke.x());
                    }

                    if (handleBetaFlip) {
                        vertices.push_back(betaTwo);
                        vertices.push_back(betaOne);

                        // This flag will not be used after this point. This is just here as an illustration.
                        handleBetaFlip = false;
                    } else {
                        vertices.push_back(betaOne);
                        vertices.push_back(betaTwo);
                    }
                } else {
                    SkPoint secondPoint = SkPath::getPoint(start + 1);
                    const SkPoint normVec2 = SkPoint::Make(toFloat.y() - secondPoint.y(), secondPoint.x() - toFloat.x());
                    const float normLength2 = sqrtf(normVec2.x() * normVec2.x() + normVec2.y() * normVec2.y());
                    const SkPoint normalStroke2 = SkPoint::Make(normVec2.x() * halfStroke / normLength2 , normVec2.y() * halfStroke / normLength2);


                    SkPoint secondPointOne = SkPoint::Make(secondPoint.x() + normalStroke2.x(), secondPoint.y() + normalStroke2.y());
                    SkPoint secondPointTwo = SkPoint::Make(secondPoint.x() - normalStroke2.x(), secondPoint.y() - normalStroke2.y());
                    SkPoint firstPointOne = SkPoint::Make(toFloat.x() + normalStroke2.x(), toFloat.y() + normalStroke2.y());
                    SkPoint firstPointTwo = SkPoint::Make(toFloat.x() - normalStroke2.x(), toFloat.y() - normalStroke2.y());

                    float secondConstant1 = -1 * (normVec2.x() * secondPointOne.x() + normVec2.y() * secondPointOne.y());
                    float secondConstant3 = -1 * (normVec2.x() * secondPointTwo.x() + normVec2.y() * secondPointTwo.y());

                    float closeXJoint1 = (secondConstant1 * normVec.y() - constant2 * normVec2.y()) / normVec.cross(normVec2);
                    float closeYJoint1 = (secondConstant1 * normVec.x() - constant2 * normVec2.x()) / normVec2.cross(normVec);
                    float closeXJoint2 = (secondConstant3 * normVec.y() - constant4 * normVec2.y()) / normVec.cross(normVec2);
                    float closeYJoint2 = (secondConstant3 * normVec.x() - constant4 * normVec2.x()) / normVec2.cross(normVec);

                    SkPoint closeJoint1 = SkPoint::Make(closeXJoint1, closeYJoint1);
                    SkPoint closeJoint2 = SkPoint::Make(closeXJoint2, closeYJoint2);
                    sign = normVec.cross(normVec2);

                    bool handleClosingFlip = false;

                    if (fabs(sign) < 0.001) {
                        closeJoint1 = betaOne;
                        closeJoint2 = betaTwo;

                        // This is a corner-case similar to the one found in the generic body segment handling logic.
                        // In case of pi/2 turn in the closing segment, the generic logic would produce an erroneous triangle
                        // strip due to firstPointTwo == closeJoint1.
                        // if (normVec.dot(normVec2) > 0)
                        if (FloatUtils::isEq(closeJoint1, firstPointTwo))
                            handleClosingFlip = true;

                    } else if (isJointFar(sign > 0 ? closeJoint1 : closeJoint2, fromFloat, toFloat, secondPoint, halfStroke * 2))
                        // We can't do this easily and quickly in hardware so we fall back to software.
                        return false;

                    // update the cross product to the new angle.
                    shouldBevel = true;
                    if (paint.getStrokeJoin() == SkPaint::kMiter_Join) {
                        float deltaX1 = toFloat.x() - closeXJoint1;
                        float deltaY1 = toFloat.y() - closeYJoint1;
                        float deltaX2 = toFloat.x() - closeXJoint2;
                        float deltaY2 = toFloat.y() - closeYJoint2;
                        float miterHeight = sign < 0 ? sqrt(deltaX1 * deltaX1  + deltaY1 * deltaY1) : (sign > 0 ? sqrt(deltaX2 * deltaX2  + deltaY2 * deltaY2) : 0);
                        shouldBevel = miterHeight > halfStroke * paint.getStrokeMiter();
                    }

                    if (!shouldBevel) {
                        vertices.push_back(closeJoint1);
                        vertices.push_back(closeJoint2);
                        vertices.push_back(firstJoint1);
                        vertices.push_back(firstJoint2);
                    } else if (sign < 0) {
                        vertices.push_back(betaOne);
                        vertices.push_back(closeJoint2);
                        vertices.push_back(firstPointOne);
                        vertices.push_back(firstJoint2);
                        vertices.push_back(firstJoint1);
                        if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                            circularSegmentPoints.push_back(firstPointOne);
                            circularSegmentPoints.push_back(betaOne);
                            circularSegmentPoints.push_back(calculateArcControlPoint(firstPointOne, betaOne, closeJoint1, prevNormVec, normVec));
                        }
                    } else {
                        if (handleClosingFlip) {
                            vertices.push_back(betaTwo);
                            vertices.push_back(closeJoint1);
                            vertices.push_back(firstPointOne);
                            vertices.push_back(firstPointOne);
                            vertices.push_back(firstPointOne);
                            vertices.push_back(closeJoint1);
                            vertices.push_back(firstJoint1);
                            vertices.push_back(firstJoint2);

                            // This flag will not be used after this point. This is just here as an illustration.
                            handleClosingFlip = false;
                        } else {
                            // Due to a separate handling mechanics of the last body segment, there is a second handleBetaFlip case.
                            if (handleBetaFlip) {
                                vertices.push_back(betaTwo);
                            }

                            // Generic case
                            vertices.push_back(closeJoint1);
                            vertices.push_back(betaTwo);
                            vertices.push_back(firstPointTwo);

                            if (handleBetaFlip) {
                                vertices.push_back(firstPointTwo);

                                // This flag will not be used after this point. This is just here as an illustration.
                                handleBetaFlip = false;
                            }

                            vertices.push_back(firstPointTwo);
                            vertices.push_back(closeJoint1);
                            vertices.push_back(firstJoint2);
                            vertices.push_back(firstJoint1);
                        }

                        if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
                            circularSegmentPoints.push_back(firstPointTwo);
                            circularSegmentPoints.push_back(betaTwo);
                            circularSegmentPoints.push_back(calculateArcControlPoint(firstPointTwo, betaTwo, closeJoint2, prevNormVec, normVec));
                        }
                    }
                }
            }
            alphaOne = joint1;
            alphaTwo = joint2;
        }
        prevNormVec = normVec;
    }
    if (size_t nrVertices = vertices.size()) {
        SkRect rect = getStrokeBounds(context, paint);
        context->matrix().mapRect(&rect);
        if (context->currentClip().isRectInClip(rect, context->size())) {
            SolidColorGen stripGen(context, SkPaintToABGR32Premultiplied(paint), StripMode);
            context->matrix().mapPoints(&vertices[0], nrVertices);
            stripGen.appendPoints(vertices, rect);
        } else {
            context->addVertices(&vertices[0], nrVertices, SkCanvas::kTriangleStrip_VertexMode, paint);
        }

        if (size_t nrCaps = roundedCaps.size()) {
            SolidColorGen gen(context, SkPaintToABGR32Premultiplied(paint));
            float halfStroke = paint.getStrokeWidth() / 2.0f;
            for (size_t i = 0 ; i < nrCaps; ++i)
                drawRoundedCap(this, context, roundedCaps[i], roundedCaps[i] == start, gen, halfStroke);
        }
        if (paint.getStrokeJoin() == SkPaint::kRound_Join) {
            if (size_t circleVertices = circularSegmentPoints.size()) {
                SolidColorGen gen(context, SkPaintToABGR32Premultiplied(paint));
                for (size_t i = 0 ; i < circleVertices; i += 3)
                    context->addRound(gen, circularSegmentPoints[i], circularSegmentPoints[i + 1], circularSegmentPoints[i + 2]);
            }
        }
    }
    return true;
}

} // namespace Gentl
