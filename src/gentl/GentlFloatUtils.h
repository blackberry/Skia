/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlFloatUtils_h
#define GentlFloatUtils_h

#include "SkPoint.h"
#include "SkRect.h"

#include <float.h>
#include <cmath>

namespace Gentl {

class FloatUtils {
public:
    static bool isNaN(const float x)
    {
        using namespace std;
        return isnan(x);
    }
    static bool isInfinity(const float x)
    {
        using namespace std;
        return isinf(x);
    }
    static bool isNearLow(const float value, const float near)
    {
        return ((value <= near) && (near - FLT_EPSILON <= value));
    }

    static bool isNearHigh(const float value, const float near)
    {
        return ((near <= value) && (value <= near + FLT_EPSILON));
    }

    static bool isInRange(const float value, const float near)
    {
        return ((near - FLT_EPSILON <= value) && (value <= near + FLT_EPSILON));
    }

    static bool isInBetween(const float value, const float low, const float high) {
        return ((low - FLT_EPSILON <= value) && (value <= high + FLT_EPSILON));
    }

    static bool isInBetweenUnsorted(const float value, const float x0, const float x1)
    {
        return (x0 < x1) ? isInBetween(value, x0, x1) : isInBetween(value, x1, x0);
    }

    // if a ~= b + eps then check b - a < eps && a - b < 0 < eps => true, OK
    // if b ~= a + eps then check a - b < eps && b - a < 0 < eps => true, OK
    // if a < b then b - a > eps && a - b < 0 < eps => false, OK
    // if a > b then a - b > eps && b - a < 0 < eps => false, OK
    // if a == b then a - b = b - a = 0 < eps = > true, OK
    static bool isEqual(const float a, const float b)
    {
        return ((a - b) <= FLT_EPSILON && (b - a) <= FLT_EPSILON);
    }

    static bool isPositiveZero(const float value) { return isNearHigh(value, 0.0f); }
    static bool isNegativeZero(const float value) { return isNearLow(value, 0.0f); }
    static bool isNearZero(const float value) { return isInRange(value, 0.0f); }
    static bool isGreaterZero(const float value) { return FLT_EPSILON < value; }
    static bool isLessZero(const float value) { return value < -FLT_EPSILON; }

    static bool isLowCloseOne(const float value) { return isNearHigh(value, 1.0f); }
    static bool isHighCloseOne(const float value) { return isNearLow(value, 1.0f); }
    static bool isNearOne(const float value) { return isInRange(value, 1.0f); }
    static bool isLowCloseMinusOne(const float value) { return isNearHigh(value, -1.0f); }
    static bool isHighCloseMinusOne(const float value) { return isNearLow(value, -1.0f); }
    static bool isNearMinusOne(const float value) { return isInRange(value, -1.0f); }

    static float interpolate(const float x0, const float x1, const float fraction)
    {
        return x0 + (x1 - x0) * fraction;
    }

    static bool isEq(const SkPoint& lhs, const SkPoint& rhs)
    {
        return isEqual(lhs.x(), rhs.x()) && (isEqual(lhs.y(), rhs.y()));
    }

    static bool isNEq(const SkPoint& lhs, const SkPoint& rhs)
    {
        return !isEq(lhs, rhs);
    }

    static bool isEqual(const SkRect& lhs, const SkRect& rhs)
    {
        return lhs == rhs;
    }
};

} // namespace Gentl

#endif // GentlUtils_h
