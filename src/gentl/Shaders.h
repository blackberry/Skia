/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef Shaders_h
#define Shaders_h

#include "Clip.h"
#include "SkShader.h"
#include "GrResource.h"

class GrGpuGL;
class SkMatrix44;
class GrGLInterface;

namespace Gentl {

class PrecompiledProgram {
public:
    enum PrecompilationStatus {
        Uninitialized,
        Succeeded,
        Failed,
    };

    PrecompiledProgram();
    PrecompiledProgram& operator=(const PrecompiledProgram&);
    ~PrecompiledProgram();

    PrecompilationStatus status;
    int size;
    unsigned char* binary;
    unsigned format;


    PrecompiledProgram(const PrecompiledProgram&);
};

struct ProgramBase {
    static GrGLuint compile(const GrGLInterface* glInterface, const char* vsSource, const char* fsSource, CommandVariant);
    static void getBinary(const GrGLInterface* glInterface, GrGLuint program, PrecompiledProgram& dst);
};

struct ProgramVariantCommon {
    ProgramVariantCommon()
        : program(0)
        , view(AttribIdDisabled)
        , position(AttribIdDisabled)
        , round(AttribIdDisabled)
        , alpha(AttribIdDisabled)
        , vertexStride(0)
    { }

    virtual ~ProgramVariantCommon() { };

    virtual void enableVertexAttribArrays(const GrGLInterface*) const; // the attribute ids itself do not change
    virtual void disableVertexAttribArrays(const GrGLInterface*) const; // only if virtual it might be nested
    void setVertexAttribPointers(const void*, const GrGLInterface*) const;

    GrGLuint program;
    GrGLint view, position, round, alpha;
    unsigned vertexStride;

protected:
    enum { AttribIdDisabled = -1 };

    static const void* dataOffset(const void* data, int arg1 = 0, int arg2 = 0);
};

template <typename Program>
class ProgramSet : private ProgramBase {
public:
    ProgramSet() { }
    const Program& variant(const GrGLInterface*, CommandVariant);
    void precompile(const GrGLInterface*, CommandVariant);
    void destroy(const GrGLInterface*, CommandVariant);
private:
    void create(const GrGLInterface*, CommandVariant);
    bool loadFromPrecompiled(const GrGLInterface*, CommandVariant);
    Program fVariant[NumberOfVariants];
    PrecompiledProgram fPrecompiled[NumberOfVariants];
};

struct ColorProgram : public ProgramVariantCommon {
    ColorProgram()
        : color(AttribIdDisabled)
    { }

    void enableVertexAttribArrays(const GrGLInterface*) const;
    void disableVertexAttribArrays(const GrGLInterface*) const;
    void setVertexAttribPointers(const void*, const GrGLInterface*) const;

    GrGLint color;
};

struct ImageProgram : public ProgramVariantCommon {
    ImageProgram()
        : texcoord(AttribIdDisabled)
        , texture(AttribIdDisabled)
    { }

    virtual ~ImageProgram() { };

    virtual void enableVertexAttribArrays(const GrGLInterface*) const;
    virtual void disableVertexAttribArrays(const GrGLInterface*) const; // only if virtual it might be nested
    void setVertexAttribPointers(const void*, const GrGLInterface*) const;

    GrGLint texcoord, texture;
};

struct ImageAlphaProgram : public ImageProgram {
    ImageAlphaProgram()
    { }
};

struct ImageBlurProgram : public ImageProgram {
    ImageBlurProgram()
        : amount(AttribIdDisabled)
        , blurvector(AttribIdDisabled)
        , incr(AttribIdDisabled)
    { }

    GrGLint amount, blurvector, incr;
};

struct PackedCoordImageProgram : public ImageProgram {
    void setVertexAttribPointers(const void*, const GrGLInterface*) const;
};

struct BlurredEdgeTextProgram : public PackedCoordImageProgram {
    BlurredEdgeTextProgram()
        : color(AttribIdDisabled)
        , cutoff(AttribIdDisabled)
        , amount(AttribIdDisabled)
    { }

    GrGLint color, cutoff, amount;
};

struct SkShaderProgram : public ImageProgram {
    SkShaderProgram()
        : transform(AttribIdDisabled)
    { }

    GrGLint transform;
};

struct EdgeTextProgram : public PackedCoordImageProgram {
    EdgeTextProgram()
        : color(AttribIdDisabled)
        , cutoff(AttribIdDisabled)
    { }

    GrGLint color, cutoff;
};

struct StrokedEdgeTextProgram : public PackedCoordImageProgram {
    StrokedEdgeTextProgram()
        : fillColor(AttribIdDisabled)
        , strokeColor(AttribIdDisabled)
        , halfStrokeWidth(AttribIdDisabled)
        , cutoff(AttribIdDisabled)
    { }

    GrGLint fillColor, strokeColor, halfStrokeWidth, cutoff;
};

struct TextProgram : public PackedCoordImageProgram {
    TextProgram()
        : color(AttribIdDisabled)
    { }

    GrGLint color;
};

struct ArcProgram : public ProgramVariantCommon {
    ArcProgram()
        : color(AttribIdDisabled)
        , texcoord(AttribIdDisabled)
        , inside(AttribIdDisabled)
    { }

    void enableVertexAttribArrays(const GrGLInterface*) const;
    void disableVertexAttribArrays(const GrGLInterface*) const;
    void setVertexAttribPointers(const void*, const GrGLInterface*) const;

    GrGLint color, texcoord, inside;
};

struct  SkShaderLinearGradientProgram : public SkShaderProgram {
    SkShaderLinearGradientProgram()
    { }
};

struct  SkShaderSweepGradientProgram : public SkShaderProgram {
    SkShaderSweepGradientProgram()
    { }
};

struct  SkShaderRadialGradientProgramCommon : public SkShaderProgram {
    SkShaderRadialGradientProgramCommon()
    { }
};

struct  SkShaderRadialGradientProgram : public SkShaderRadialGradientProgramCommon {
    SkShaderRadialGradientProgram()
    { }
};

struct  SkShaderTwoPointRadialGradientProgram : public SkShaderRadialGradientProgramCommon {
    SkShaderTwoPointRadialGradientProgram()
    : xOffset(AttribIdDisabled)
    , a(AttribIdDisabled)
    , r1Squared(AttribIdDisabled)
    , r1TimesDiff(AttribIdDisabled)
    { }

    GrGLint xOffset, a, r1Squared, r1TimesDiff;
};

enum Projection { Identity, YAxisInverted };

class Shaders : public GrResource {
public:
    Shaders(GrGpu* gpu);
    ~Shaders();

    virtual void internal_dispose() const;

    virtual size_t sizeInBytes() const;
    virtual void onRelease();
    virtual void onAbandon();

    void compileAndStore();
    void reset(const SkMatrix44&, Projection);

    const ColorProgram& loadColor(CommandVariant);
    const ImageProgram& loadImage(CommandVariant);
    const ImageAlphaProgram& loadImageAlpha(CommandVariant);
    const ImageBlurProgram& loadImageBlur(CommandVariant);
    const BlurredEdgeTextProgram& loadBlurredEdgeText(CommandVariant);
    const SkShaderProgram& loadSkShader(CommandVariant, SkShader::GradientType);
    const EdgeTextProgram& loadEdgeText(CommandVariant);
    const StrokedEdgeTextProgram& loadStrokedEdgeText(CommandVariant);
    const TextProgram& loadText(CommandVariant);
    const ArcProgram& loadArc(CommandVariant);

private:
    void loadCommon(const ProgramVariantCommon& p);
    void loadImage(const ImageProgram& p);

    const GrGpuGL* fGpu;
    const GrGLInterface* fGlInterface;
    const ProgramVariantCommon* fP;
    float fView[16];

    ProgramSet<ColorProgram> fColor;
    ProgramSet<ImageProgram> fImage;
    ProgramSet<ImageAlphaProgram> fImageAlpha;
    ProgramSet<SkShaderProgram> fSkShader;
    ProgramSet<SkShaderLinearGradientProgram> fSkShaderLinearGradient;
    ProgramSet<SkShaderRadialGradientProgram> fSkShaderRadialGradient;
    ProgramSet<SkShaderSweepGradientProgram> fSkShaderSweepGradient;
    ProgramSet<SkShaderTwoPointRadialGradientProgram> fSkShaderTwoPointRadialGradient;
    ProgramSet<ImageBlurProgram> fImageBlur;
    ProgramSet<BlurredEdgeTextProgram> fBlurredEdgeText;
    ProgramSet<EdgeTextProgram> fEdgeText;
    ProgramSet<StrokedEdgeTextProgram> fStrokedEdgeText;
    ProgramSet<TextProgram> fText;
    ProgramSet<ArcProgram> fArc;
};

} // namespace Gentl Graphics

#endif // Shaders_h
