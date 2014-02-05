/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef Generators_h
#define Generators_h

#include "DisplayList.h"
#include "GentlContext.h"
#include "GentlCommands.h"
#include "InstructionSet.h"

#include "SkMatrix.h"
#include "SkShader.h"
#include "GrTexture.h"
#include <limits>

namespace Gentl {

class Inserter {
public:
    void appendTriangle(const Clip::Triangle&);
    void appendPoints(const std::vector<SkPoint>&, const SkRect& rect);
    void appendQuad(const Clip::Vertex& v0, const Clip::Vertex& v1, const Clip::Vertex& v2, const Clip::Vertex& v3, const CommandVariant variant);

    enum InsertionTime { Immediate, Deferred };
    void setInsertionTime(InsertionTime);

protected:
    enum AdditionalVertexAttributes { Texcoords, PackedTexcoords, OneInstructionSetData };
    Inserter(GentlContext*, DisplayListCommand, InsertionTime roundedClipInsertion, const void* uniforms, unsigned size, AdditionalVertexAttributes);
    ~Inserter();

protected:
    InstructionSet* fInstructions;
    TransparencyLayer* fTransparency;
    DisplayListCommand fCommand;

    // FIXME: Investigate a better hack for this. If either of these are non-null they need
    // to be reffed at the moment the uniforms are pushed into the InstructionSet.
    GrTexture* fGrTexture;

    AdditionalVertexAttributes fAttributes;
    InstructionSetData fData;

private:
    inline void appendVertex(const Clip::Vertex&, const CommandVariant);
    inline void pushCommandAndUniforms(DisplayListCommand);

private:
    InsertionTime fInsertion;
    BuffChain<Clip::Vertex> fDeferred[NumberOfVariants];

    const void* fUniformData;
    size_t fUniformSize;
};

class SolidColorGen : public Inserter {
public:
    SolidColorGen(GentlContext* context, ABGR32Premultiplied color, CommandTriangleMode triangleMode = SoupMode)
        : Inserter(context, DrawColor, Deferred, 0, 0, OneInstructionSetData)
    {
        // Don't bother switching blend modes to opaque if the previous instruction was DrawColor
        if (SkColorGetA(color) == SK_AlphaOPAQUE && !(fInstructions->commandSize() && fInstructions->lastCommand().id() == DrawColor))
            fCommand = DrawOpaqueColor | triangleMode;
        fCommand = (fCommand & ~(SoupMode | FanMode | StripMode)) | triangleMode;

        fData.u = color;
    }
};

class TextureGen : public Inserter {
public:
    TextureGen(GentlContext* context, GrTexture* tex, SkPaint::FilterLevel filterLevel)
        : Inserter(context, DrawTexture, Deferred, &fUniforms, sizeof(DrawTextureUniforms), Texcoords)
        , fUniforms(tex, filterLevel)
    {
        SkASSERT(tex); // checked by GentlContext::addImage()
        SkASSERT(tex->width());
        SkASSERT(tex->height());
        fGrTexture = tex;
    }

    DrawTextureUniforms fUniforms;
};

class SkShaderGen : public Inserter {
public:
    SkShaderGen(GentlContext* context, GrTexture* tex, SkPaint::FilterLevel filterLevel, const SkMatrix& localMatrix, const SkShader::TileMode tileMode[2], const SkShader::GradientType& type, SkShader::GradientInfo& info)
        : Inserter(context, DrawSkShader, Deferred, &fUniforms, sizeof(DrawSkShaderUniforms), Texcoords)
        , fUniforms(tex, filterLevel)
    {
        SkASSERT(tex); // checked by GentlContext::addPattern()
        SkASSERT(tex->width()); // ditto
        SkASSERT(tex->height()); // ditto
        fGrTexture = tex;

        fUniforms.gradientType = type;

        SkMatrix tempMatrix(localMatrix);
        // if it's a radial gradient we need to adjust the scale by the texture width
        if (fUniforms.gradientType == SkShader::kRadial_GradientType) {
            const float invTexWidth = 1.0f / fUniforms.texture->width();
            for (unsigned i = SkMatrix::kMScaleX; i < SkMatrix::kMPersp0; ++i)
                tempMatrix[i] *= invTexWidth;
        }

        fUniforms.localMatrix[0] = tempMatrix.getScaleX();
        fUniforms.localMatrix[1] = tempMatrix.getSkewY();
        fUniforms.localMatrix[2] = tempMatrix.getPerspX();
        fUniforms.localMatrix[3] = tempMatrix.getSkewX();
        fUniforms.localMatrix[4] = tempMatrix.getScaleY();
        fUniforms.localMatrix[5] = tempMatrix.getPerspY();
        fUniforms.localMatrix[6] = tempMatrix.getTranslateX();
        fUniforms.localMatrix[7] = tempMatrix.getTranslateY();
        fUniforms.localMatrix[8] = 1.0f;

        fUniforms.tileMode[0] = tileMode[0];
        fUniforms.tileMode[1] = tileMode[1];

        const float scaleX = tempMatrix.getScaleX();
        fUniforms.distance = SkPoint::Distance(info.fPoint[0], info.fPoint[1]) * scaleX;
        fUniforms.radius1 = info.fRadius[0] * scaleX;
        fUniforms.radiusDiff = info.fRadius[1] * scaleX - fUniforms.radius1;
        if (fUniforms.gradientType == SkShader::kRadial2_GradientType) {
              fUniforms.xOffset = fUniforms.distance;
              fUniforms.a = fUniforms.distance * fUniforms.distance - fUniforms.radiusDiff * fUniforms.radiusDiff; // this is 'a' in a quadratic
              fUniforms.r1Squared = fUniforms.radius1 * fUniforms.radius1;
              fUniforms.r1TimesDiff = fUniforms.radius1 * fUniforms.radiusDiff;
        }
    }

    DrawSkShaderUniforms fUniforms;
};

class ArcGen : public Inserter {
public:
    ArcGen(GentlContext* context, ABGR32Premultiplied color, float inside)
        : Inserter(context, DrawArc, Deferred, &fUniforms, sizeof(DrawArcUniforms), Texcoords)
        , fUniforms(color, inside)
    { }

    DrawArcUniforms fUniforms;
};

class EdgeGlyphGen : public Inserter {
public:
    EdgeGlyphGen(GentlContext* context, GrTexture* tex, ABGR32Premultiplied color,
                 float insideCutoff, float outsideCutoff)
        : Inserter(context, DrawEdgeGlyph, Immediate, &fUniforms, sizeof(DrawEdgeGlyphUniforms), PackedTexcoords)
        , fUniforms(tex, color, insideCutoff, outsideCutoff)
    {
        fGrTexture = tex;
    }

    DrawEdgeGlyphUniforms fUniforms;
};

class BlurredGlyphGen : public Inserter {
public:
    BlurredGlyphGen(GentlContext* context, GrTexture* tex, ABGR32Premultiplied color,
                    float insideCutoff, float outsideCutoff, float blurRadius)
        : Inserter(context, DrawBlurredGlyph, Immediate, &fUniforms, sizeof(DrawBlurredGlyphUniforms), PackedTexcoords)
        , fUniforms(tex, color, insideCutoff, outsideCutoff, blurRadius)
    {
        fGrTexture = tex;
    }

    DrawBlurredGlyphUniforms fUniforms;
};

class StrokedEdgeGlyphGen : public Inserter {
public:
    StrokedEdgeGlyphGen(GentlContext* context, GrTexture* tex, ABGR32Premultiplied color,
                        ABGR32Premultiplied strokeColor, float halfStrokeWidth, float insideCutoff, float outsideCutoff)
        : Inserter(context, DrawStrokedEdgeGlyph, Immediate, &fUniforms, sizeof(DrawStrokedEdgeGlyphUniforms), PackedTexcoords)
        , fUniforms(tex, color, insideCutoff, outsideCutoff, strokeColor, halfStrokeWidth)
    {
        fGrTexture = tex;
    }

    DrawStrokedEdgeGlyphUniforms fUniforms;
};

class BitmapGlyphGen : public Inserter {
public:
    BitmapGlyphGen(GentlContext* context, GrTexture* tex, unsigned color)
        : Inserter(context, DrawBitmapGlyph, Deferred, &fUniforms, sizeof(DrawBitmapGlyphUniforms), PackedTexcoords)
        , fUniforms(tex, color)
    {
        fGrTexture = tex;
    }

    DrawBitmapGlyphUniforms fUniforms;
};

class DrawCurrentTextureGen : public Inserter {
public:
    DrawCurrentTextureGen(GentlContext* context, float opacity, const SkSize& size, float blurAmount, const SkSize& blurVector, SkXfermode::Mode mode)
        : Inserter(context, DrawCurrentTexture, Deferred, &fUniforms, sizeof(DrawCurrentTextureUniforms), Texcoords)
        , fUniforms(0, opacity, blurAmount, size.width(), size.height(), blurVector.width(), blurVector.height(), mode)
    {
        SkASSERT(FloatUtils::isGreaterZero(blurVector.width()) || FloatUtils::isGreaterZero(blurVector.height()));
        SkASSERT(FloatUtils::isGreaterZero(blurAmount));
    }

    DrawCurrentTextureGen(GentlContext* context, float opacity, SkXfermode::Mode mode)
        : Inserter(context, DrawCurrentTexture, Deferred, &fUniforms, sizeof(DrawCurrentTextureUniforms), Texcoords)
        , fUniforms(0, opacity, 0, 0, 0, 0, 0, mode)
    {
        SkASSERT(FloatUtils::isGreaterZero(opacity));
    }

    DrawCurrentTextureUniforms fUniforms;
};

} // namespace Gentl

#endif // Generators_h
