/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GraphicsPrimitives_h
#define GraphicsPrimitives_h

#include "SkColorFilter.h"
#include "SkPaint.h"

#define ARGB32GetA(color)      (((color) >> 24) & 0xFF)
#define ARGB32GetR(color)      (((color) >> 16) & 0xFF)
#define ARGB32GetG(color)      (((color) >>  8) & 0xFF)
#define ARGB32GetB(color)      (((color) >>  0) & 0xFF)

#define ABGR32GetA(color)      (((color) >> 24) & 0xFF)
#define ABGR32GetB(color)      (((color) >> 16) & 0xFF)
#define ABGR32GetG(color)      (((color) >>  8) & 0xFF)
#define ABGR32GetR(color)      (((color) >>  0) & 0xFF)

namespace Gentl {

// This is the preferred color format. It's the easiest for human manipulation.
typedef unsigned ARGB32;

// This is only used when communicating with the graphics driver. We do not
// perform any logical color calculations with ABGR32Premultipled data.
typedef unsigned ABGR32Premultiplied;

static inline ARGB32 ARGB32Set(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

static inline ABGR32Premultiplied ABGR32PremultipliedSet(unsigned char a, unsigned char b, unsigned char g, unsigned char r)
{
    // Input bytes are already assumed to be premultiplied.
    return (a << 24) | (b << 16) | (g << 8) | (r << 0);
}

// This is the method that actually converts unpremultiplied a r g b bytes into a 32 bit premultipled ABGR32 word.
static inline ABGR32Premultiplied ARGB32Premultiply(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
    if (a != 255) {
        b = roundf(b * static_cast<float>(a) / 255.0f);
        g = roundf(g * static_cast<float>(a) / 255.0f);
        r = roundf(r * static_cast<float>(a) / 255.0f);
    }

    return ABGR32PremultipliedSet(a, b, g, r);
}

// This method takes unpremultiplied ARGB32, swizzles and premultiplies it into ABGR32Premultiplied.
static inline ABGR32Premultiplied SkPaintToABGR32Premultiplied(const SkPaint& paint)
{
    const ARGB32 argb = paint.getColorFilter() ? paint.getColorFilter()->filterColor(paint.getColor()) : paint.getColor();
    return ARGB32Premultiply(ARGB32GetA(argb), ARGB32GetR(argb), ARGB32GetG(argb), ARGB32GetB(argb));
}

static inline unsigned ARGB32ToABGR32(const ARGB32 argb)
{
    return ARGB32Set(ARGB32GetA(argb), ARGB32GetB(argb), ARGB32GetG(argb), ARGB32GetR(argb));
}

static inline float* ABGR32MultiplyToGlfv(const ABGR32Premultiplied abgr, const float multiplier, float result[4])
{
    // OpenGL actually uses RGBA vectors in GLSL.
    result[0] = ABGR32GetR(abgr) * multiplier;
    result[1] = ABGR32GetG(abgr) * multiplier;
    result[2] = ABGR32GetB(abgr) * multiplier;
    result[3] = ABGR32GetA(abgr) * multiplier;
    return result;
}

static inline ABGR32Premultiplied ABGR32PremultipliedMix(ABGR32Premultiplied c0, ABGR32Premultiplied c1, ABGR32Premultiplied c2, float u, float v)
{
    unsigned char a = lroundf((1 - u - v) * ABGR32GetA(c0) + u * ABGR32GetA(c1) + v * ABGR32GetA(c2)) & 0xff;
    unsigned char b = lroundf((1 - u - v) * ABGR32GetB(c0) + u * ABGR32GetB(c1) + v * ABGR32GetB(c2)) & 0xff;
    unsigned char g = lroundf((1 - u - v) * ABGR32GetG(c0) + u * ABGR32GetG(c1) + v * ABGR32GetG(c2)) & 0xff;
    unsigned char r = lroundf((1 - u - v) * ABGR32GetR(c0) + u * ABGR32GetR(c1) + v * ABGR32GetR(c2)) & 0xff;
    return ABGR32PremultipliedSet(a, b, g, r);
}

} // namespace Gentl

#endif
