/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlCommands_h
#define GentlCommands_h

#include "Clip.h"
#include "GraphicsPrimitives.h"
#include "SkShader.h"

class GrTexture;

namespace Gentl {

struct ClearUniforms {
    static const CommandId Id = Clear;
    float red, green, blue, alpha;

    ClearUniforms(float inRed, float inGreen, float inBlue, float inAlpha)
        : red(inRed), green(inGreen), blue(inBlue), alpha(inAlpha) { }
};

struct SetAlphaUniforms {
    static const CommandId Id = SetAlpha;
    float alpha;

    SetAlphaUniforms(float inAlpha) : alpha(inAlpha) { }
};

struct SetXferModeUniforms {
    static const CommandId Id = SetXferMode;
    SkXfermode::Mode mode;

    SetXferModeUniforms(SkXfermode::Mode inMode) : mode(inMode) { }
};

struct GrTextured {
    GrTexture* texture;

    GrTextured(GrTexture* inTexture) : texture(inTexture) {}
};

struct DrawTextureUniforms : GrTextured {
    static const CommandId Id = DrawTexture;
    SkPaint::FilterLevel filterLevel;

    DrawTextureUniforms(GrTexture* inTexture, SkPaint::FilterLevel inFilterLevel)
        : GrTextured(inTexture)
        , filterLevel(inFilterLevel) { }
};

struct DrawBitmapGlyphUniforms : GrTextured {
    static const CommandId Id = DrawBitmapGlyph;
    ABGR32Premultiplied color;

    DrawBitmapGlyphUniforms(GrTexture* inTexture, ABGR32Premultiplied inColor)
        : GrTextured(inTexture)
        , color(inColor) { }
};

struct DrawEdgeGlyphUniforms : GrTextured {
    static const CommandId Id = DrawEdgeGlyph;
    ABGR32Premultiplied color;
    float insideCutoff;
    float outsideCutoff;

    DrawEdgeGlyphUniforms(GrTexture* inTexture, ABGR32Premultiplied inColor, float inInsideCutoff, float inOutsideCutoff)
        : GrTextured (inTexture) , color(inColor), insideCutoff(inInsideCutoff), outsideCutoff(inOutsideCutoff) { }
};

struct DrawBlurredGlyphUniforms : DrawEdgeGlyphUniforms {
    static const CommandId Id = DrawBlurredGlyph;
    float blurRadius;

    DrawBlurredGlyphUniforms(GrTexture* inTexture, int inColor, float inInsideCutoff, float inOutsideCutoff, float inBlurRadius)
        : DrawEdgeGlyphUniforms(inTexture, inColor, inInsideCutoff, inOutsideCutoff)
        , blurRadius(inBlurRadius) { }
};

struct DrawStrokedEdgeGlyphUniforms : DrawEdgeGlyphUniforms {
    static const CommandId Id = DrawStrokedEdgeGlyph;
    ABGR32Premultiplied strokeColor;
    float halfStrokeWidth;

    DrawStrokedEdgeGlyphUniforms(GrTexture* inTexture, int inColor, float inInsideCutoff, float inOutsideCutoff, float inStrokeColor, float inHalfStrokeWidth)
        : DrawEdgeGlyphUniforms(inTexture, inColor, inInsideCutoff, inOutsideCutoff)
        , strokeColor(inStrokeColor)
        , halfStrokeWidth(inHalfStrokeWidth) { }
};

struct DrawSkShaderUniforms : GrTextured {
    static const CommandId Id = DrawSkShader;
    SkPaint::FilterLevel filterLevel;
    float localMatrix[9];
    SkShader::TileMode tileMode[2];
    SkShader::GradientType gradientType;
    float distance;
    float radius1;
    float radiusDiff;
    float xOffset;
    float a;
    float r1Squared;
    float r1TimesDiff;

    DrawSkShaderUniforms(GrTexture* inTexture, SkPaint::FilterLevel inFilterLevel)
        : GrTextured(inTexture)
        , filterLevel(inFilterLevel) { }
};

struct DrawArcUniforms {
    static const CommandId Id = DrawArc;
    ABGR32Premultiplied color;
    float inside;

    DrawArcUniforms(ABGR32Premultiplied inColor, float inInside)
        : color(inColor), inside(inInside) { }
};

struct DrawCurrentTextureUniforms : GrTextured {
    static const CommandId Id = DrawCurrentTexture;
    float opacity;
    float blurAmount;
    float blurSizeWidth;
    float blurSizeHeight;
    float blurVectorX;
    float blurVectorY;
    SkXfermode::Mode mode;

    DrawCurrentTextureUniforms(GrTexture* inTexture, float opacity, float inBlurAmount, float inBlurSizeWidth, float inBlurSizeHeight, float inBlurVectorX, float inBlurVectorY, SkXfermode::Mode inMode)
        : GrTextured(inTexture)
        , opacity(opacity)
        , blurAmount(inBlurAmount)
        , blurSizeWidth(inBlurSizeWidth)
        , blurSizeHeight(inBlurSizeHeight)
        , blurVectorX(inBlurVectorX)
        , blurVectorY(inBlurVectorY)
        , mode(inMode)
        { }
};

struct BeginTransparencyLayerUniforms {
    static const CommandId Id = BeginTransparencyLayer;

    SkPoint offset;
    SkISize size;
};

} // namespace Gentl

#endif
