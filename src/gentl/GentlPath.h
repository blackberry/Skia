/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlPath_h
#define GentlPath_h

#include "SkPaint.h"
#include "SkPath.h"

#include <vector>

class SkPathMeasure;
class SkPathSegment;

namespace Gentl {

class GentlContext;
class SolidColorGen;

class GentlPath : public SkPath {
public:
    GentlPath();
    GentlPath(const SkPath&);
    ~GentlPath();

    void moveTo(const SkPoint& point);
    void closeSubpath();
    void reset();

    void fill(GentlContext* context, const SkPaint& paint) const;
    void stroke(GentlContext* context, const SkPaint& paint) const;

    static void collectForTesselator(const SkMatrix& matrix, const SkPath& path, std::vector<std::vector<float> >& contours, const SkPaint& skPaint);
private:
    bool drawPath(GentlContext* context, size_t start, size_t end, const SkPaint& paint) const;
    static void drawRoundedCap(const SkPath* path, GentlContext* platformContext, size_t pos, bool startCap, SolidColorGen&, float halfStroke);
    static float calculateStepForSegment(SkPathMeasure& measure, const SkPathSegment& currentSegment, const SkPoint& startPoint, float currentDistanceAlongSegment, const SkMatrix& matrix);

    bool drawTrianglesForPath(GentlContext* context, const SkPaint &paint) const;

    void paintStrokeGeometry(GentlContext*, std::vector<SkPoint>& triangleStrip, const std::vector<SkPoint>& roundSegments, const SkPaint& argb, bool usingTransparencyLayer) const;
    void extractStrokeGeometry(const GentlContext*, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPaint& paint) const;
    void appendCap(const SkPaint::Cap, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPoint& point, const SkPoint& tangent, bool isStartCap) const;
    void appendJoin(const GentlContext*, std::vector<SkPoint>& triangleStrip, std::vector<SkPoint>& roundSegments, const SkPoint& intersection, const SkPoint& n1, const SkPoint& n2, const SkPaint& paint) const;

    SkRect getStrokeBounds(const GentlContext*, const SkPaint& paint) const;

private:
    std::vector<unsigned> fPathBreaks;
};

} // namespace Gentl

#endif /* GentlPath_h */
