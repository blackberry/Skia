/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Shaders.h"

#include "InstructionSet.h"
#include "SkMatrix44.h"
#include "SkTypes.h"
#include "SkShader.h"
#include "gl/GrGpuGL.h"

namespace Gentl {

static const unsigned s_colorStride = 1 * sizeof(unsigned);
static const unsigned s_positionStride = 2 * sizeof(float);
static const unsigned s_roundOneStride = 2 * sizeof(float);
static const unsigned s_roundTwoStride = 4 * sizeof(float);
static const unsigned s_texcoordStride = 2 * sizeof(float);
static const unsigned s_packedTexcoordStride = 2 * sizeof(unsigned short);

static const SkMatrix44& yInverter() {
    static SkMatrix44* inverter = 0;
    if (!inverter) {
        inverter = new SkMatrix44(SkMatrix44::kIdentity_Constructor);
        inverter->preScale(1, -1, 1);
    }
    return *inverter;
}

PrecompiledProgram::PrecompiledProgram()
    : status(Uninitialized)
    , size(0)
    , binary(0)
    , format(0)
{
}

PrecompiledProgram::~PrecompiledProgram()
{
    delete[] binary;
}

PrecompiledProgram& PrecompiledProgram::operator=(const PrecompiledProgram& other)
{
    if (this == &other)
        return *this;

    status = other.status;
    size = other.size;
    format = other.format;
    delete[] binary;
    binary = 0;

    if (size) {
        binary = new unsigned char[size];
        memcpy(binary, other.binary, size);
    }

    return *this;
}

static void slogMultilineBuffer(const char* buffer)
{
    if (buffer && *buffer)
        SkDebugf(buffer);
}

GrGLuint ProgramBase::compile(const GrGLInterface* glInterface, const char* vsSource, const char* fsSource, CommandVariant variant)
{
    GrGLenum glErr = glInterface->fFunctions.fGetError(); // clean any possible errors.
    if (glErr != GR_GL_NO_ERROR)
        SkDebugf("Errors before compilation glErr=0x%04x", glErr);

    const size_t bufferSize = 1000;
    char buffer[bufferSize];
    GrGLuint program = glInterface->fFunctions.fCreateProgram();
    if (!program) {
        SkDebugf("glCreateProgram failed.");
        return 0;
    }

    const char* vsLines[] = {
        (glInterface->fStandard == kDesktop_GrGLBinding) ?
        "#version 120\n"
        "#define lowp\n"
        "#define mediump\n"
        "#define highp\n" : "",

        vsSource,
        "}\n"
    };

    const char* vsLinesRounded[] = {
        (glInterface->fStandard == kDesktop_GrGLBinding) ?
        "#version 120\n"
        "#define lowp\n"
        "#define mediump\n"
        "#define highp\n" : "",

        // We need a vec4 if we have inner and outer rounding.
        // However a vec2 is sufficient if there is just one or the other.
        (variant == BothRoundedVariant) ?  "attribute mediump vec4 a_round; \n"
                                           "varying mediump vec4 v_round;   \n" :
                                           "attribute mediump vec2 a_round; \n"
                                           "varying mediump vec2 v_round;   \n",

        // Then add the source and pass through the varying value.
        vsSource,
        "  v_round = a_round; \n"
        "}                    \n"
    };
    int count = 3;

    GrGLuint vs = glInterface->fFunctions.fCreateShader(GR_GL_VERTEX_SHADER);
    glInterface->fFunctions.fShaderSource(vs, variant == DefaultVariant ? count : count + 1, variant == DefaultVariant ? vsLines : vsLinesRounded, 0);
    glInterface->fFunctions.fCompileShader(vs);
    if (glInterface->fFunctions.fGetShaderSource) {
        glInterface->fFunctions.fGetShaderSource(vs, bufferSize, 0, buffer);
    }
//    slogMultilineBuffer(buffer);
    glInterface->fFunctions.fGetShaderInfoLog(vs, bufferSize, 0, buffer);
    slogMultilineBuffer(buffer);

    glInterface->fFunctions.fAttachShader(program, vs);

    const char* fsLines[] = {
        (glInterface->fStandard == kDesktop_GrGLBinding) ?
        "#version 120\n"
        "#define lowp\n"
        "#define mediump\n"
        "#define highp\n" : "",

        fsSource,
        "}\n"
    };

    const char* fsLinesRounded[] = {
        (glInterface->fStandard == kDesktop_GrGLBinding)?
        "#version 120\n"
        "#define lowp\n"
        "#define mediump\n"
        "#define highp\n" :

        // All rounded fragment shaders require standard derivatives.
        "#extension GL_OES_standard_derivatives : enable                                                   \n",


        // As above, the varying v_round here matches the output of vsLinesRounded.
        (variant == BothRoundedVariant) ?  "varying mediump vec4 v_round;                                     \n" :
                                           "varying mediump vec2 v_round;                                     \n",

        // Add the specific source for this shader.
        fsSource,

        // The derivatives of the texture coords are the same dimension as the coords.
        (variant == BothRoundedVariant) ?  "highp vec4 dx = dFdx(v_round);                                    \n"
                                           "highp vec4 dy = dFdy(v_round);                                    \n" :
                                           "highp vec2 dx = dFdx(v_round);                                    \n"
                                           "highp vec2 dy = dFdy(v_round);                                    \n",

        // Now we choose one of three implementations of rounding depending on whether
        // we are rounding off the inner part, the outer part, or both.
        // Source for handling just outer variant as a vec2.
        (variant == OuterRoundedVariant) ?
        "  highp vec2 grad_out = 2.0 * vec2(dot(v_round.xy, dx.xy), dot(v_round.xy, dy.xy));                  \n"
        "  highp float dist_out = (1.0 - dot(v_round.xy, v_round.xy)) * inversesqrt(dot(grad_out, grad_out)); \n"
        "  gl_FragColor *= smoothstep(-0.5, 0.5, dist_out);                                                   \n" :

        // Source for handling just inner variant as a vec2.
        (variant == InnerRoundedVariant) ?
        "  highp vec2 grad_in = 2.0 * vec2(dot(v_round.xy, dx.xy), dot(v_round.xy, dy.xy));                   \n"
        "  highp float dist_in = (-1.0 + dot(v_round.xy, v_round.xy)) * inversesqrt(dot(grad_in, grad_in));   \n"
        "  gl_FragColor *= smoothstep(-0.5, 0.5, dist_in);                                                    \n" :

        // Both rounded as a vec4, notice outer uses xy and inner uses zw.
        "  highp vec2 grad_out = 2.0 * vec2(dot(v_round.xy, dx.xy), dot(v_round.xy, dy.xy));                  \n"
        "  highp float dist_out = (1.0 - dot(v_round.xy, v_round.xy)) * inversesqrt(dot(grad_out, grad_out)); \n"
        "  gl_FragColor *= smoothstep(-0.5, 0.5, dist_out);                                                   \n"
        "  highp vec2 grad_in = 2.0 * vec2(dot(v_round.zw, dx.zw), dot(v_round.zw, dy.zw));                   \n"
        "  highp float dist_in = (-1.0 + dot(v_round.zw, v_round.zw)) * inversesqrt(dot(grad_in, grad_in));   \n"
        "  gl_FragColor *= smoothstep(-0.5, 0.5, dist_in);                                                    \n",

        "}                                                                                                    \n"
    };
    int fsCount = (variant == DefaultVariant? 3 : 6);
    GrGLuint fs = glInterface->fFunctions.fCreateShader(GR_GL_FRAGMENT_SHADER);
    glInterface->fFunctions.fShaderSource(fs, fsCount, variant == DefaultVariant ? fsLines : fsLinesRounded, 0);
    glInterface->fFunctions.fCompileShader(fs);
    if (glInterface->fFunctions.fGetShaderSource) {
        glInterface->fFunctions.fGetShaderSource(fs, bufferSize, 0, buffer);
    }
//    slogMultilineBuffer(buffer);
    glInterface->fFunctions.fGetShaderInfoLog(fs, bufferSize, 0, buffer);
    slogMultilineBuffer(buffer);

    glInterface->fFunctions.fAttachShader(program, fs);

    glInterface->fFunctions.fLinkProgram(program);
    glInterface->fFunctions.fGetProgramInfoLog(program, bufferSize, 0, buffer);

    // Uncomment to debug shaders.
    // slogMultilineBuffer(buffer);

    glInterface->fFunctions.fDetachShader(program, vs); // Not needed any more.
    glInterface->fFunctions.fDetachShader(program, fs);
    glInterface->fFunctions.fDeleteShader(vs);
    glInterface->fFunctions.fDeleteShader(fs);

    glErr = glInterface->fFunctions.fGetError(); // clean any possible errors.
    if (glErr != GR_GL_NO_ERROR) {
        SkDebugf("Compilation error glErr=0x%04x", glErr);
        glInterface->fFunctions.fDeleteProgram(program);
        return 0;
    }

    return program;
}

// The destination PrecompiledProgram is a structure we want to actually store
// The application will then use the precompiled binary when next using this
// shader program.
void ProgramBase::getBinary(const GrGLInterface* glInterface, GrGLuint program, PrecompiledProgram& dst)
{
    SkASSERT(program);
    dst = PrecompiledProgram(); // Destroy the precompiled program and free its resources.
    dst.status = PrecompiledProgram::Failed; // set to succeeded later if it works.

    GrGLenum glErr = glInterface->fFunctions.fGetError(); // clean any possible errors.
    if (glErr != GR_GL_NO_ERROR)
        SkDebugf("Errors before binary load glErr=0x%04x", glErr);

    glInterface->fFunctions.fGetProgramiv(program, GR_GL_PROGRAM_BINARY_LENGTH, &dst.size);
    glErr = glInterface->fFunctions.fGetError();
    if (glErr != GR_GL_NO_ERROR || !dst.size) {
        SkDebugf("Cannot get shader binary length glErr=0x%04x size=%d", glErr, dst.size);
        dst.size = 0;
        return;
    }

    dst.binary = new unsigned char[dst.size];
    if (!dst.binary) {
        SkDebugf("Cannot allocate memory for shader binary.");
        dst.size = 0;
        return;
    }

    glInterface->fFunctions.fGetProgramBinary(program, dst.size, &dst.size, &dst.format, dst.binary);
    glErr = glInterface->fFunctions.fGetError();
    if (glErr != GR_GL_NO_ERROR) {
        SkDebugf("Cannot copy shader binary glErr=0x%04x", glErr);
        delete[] dst.binary;
        dst.binary = 0;
        dst.size = 0;
        return;
    }

    // The dst.program might be set outside of the method only.
    dst.status = PrecompiledProgram::Succeeded;
}

void ProgramVariantCommon::enableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fEnableVertexAttribArray(position);
    if (AttribIdDisabled != round)
        glInterface->fFunctions.fEnableVertexAttribArray(round);
}

void ProgramVariantCommon::disableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fDisableVertexAttribArray(position);
    if (AttribIdDisabled != round)
        glInterface->fFunctions.fDisableVertexAttribArray(round);
}

void ProgramVariantCommon::setVertexAttribPointers(const void* data, const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fVertexAttribPointer(position, 2, GR_GL_FLOAT, GR_GL_FALSE, vertexStride, data);
    if (AttribIdDisabled != round)
        glInterface->fFunctions.fVertexAttribPointer(round, 4, GR_GL_FLOAT, GR_GL_FALSE, vertexStride, dataOffset(data, s_positionStride));
}

const void* ProgramVariantCommon::dataOffset(const void* data, int arg1, int arg2)
{
    /**
     * This method assists with the pointer arithmetic required to give OpenGL
     * the correct address of a specific vertex attribute within an array.
     *
     * Consider the following stream, where each letter represents a byte.
     *
     *             texcoord stride (8)
     *             |
     * XXXXYYYYCCCCUUUUVVVVXXXXYYYYCCCCUUUUVVVVXXXXYYYYCCCCUUUUVVVV....
     * |                   |
     * data                vertex stride (20)
     *
     * The caller desires a pointer to texcoord. They pass in data*, stride (20),
     * and negative texcoord stride (-8). By adding the three values together
     * a pointer to the first word of the tex coordinates will be returned, which
     * can then be given to OpenGL as the VertexAttribPointer.
     */

    return reinterpret_cast<const void*>(reinterpret_cast<size_t>(data) + arg1 + arg2);
}

template <typename Program>
const Program& ProgramSet<Program>::variant(const GrGLInterface* glInterface, CommandVariant variant)
{
    Program& p = fVariant[variant];
    if (p.program)
        return p;

    create(glInterface, variant); // Compile the program variant and query program specific attributes
    if (!p.program)
        return p;

    // Query all the common attributes
    p.view = glInterface->fFunctions.fGetUniformLocation(p.program, "g_view");
    p.position = glInterface->fFunctions.fGetAttribLocation(p.program, "a_position");
    p.vertexStride += s_positionStride;
    p.alpha = glInterface->fFunctions.fGetUniformLocation(p.program, "g_alpha");

    if (variant != DefaultVariant) {
        p.round = glInterface->fFunctions.fGetAttribLocation(p.program, "a_round");
        p.vertexStride += variant == BothRoundedVariant ? s_roundTwoStride : s_roundOneStride;
    }

    return p;
}

template <typename Program>
bool ProgramSet<Program>::loadFromPrecompiled(const GrGLInterface* glInterface, CommandVariant variant)
{
    GrGLint success = false;
    if (fPrecompiled[variant].status == PrecompiledProgram::Succeeded) {
        fVariant[variant].program = glInterface->fFunctions.fCreateProgram();
        glInterface->fFunctions.fProgramBinary(fVariant[variant].program, fPrecompiled[variant].format, fPrecompiled[variant].binary, fPrecompiled[variant].size);
        glInterface->fFunctions.fGetProgramiv(fVariant[variant].program, GR_GL_LINK_STATUS, &success);
        // Destroy the precompiled program and free its resources.
        fPrecompiled[variant] = PrecompiledProgram();
    }

    return success;
}

template <typename Program>
void ProgramSet<Program>::precompile(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (fPrecompiled[variant].status != PrecompiledProgram::Uninitialized)
        return;

    create(glInterface, variant);
    getBinary(glInterface, fVariant[variant].program, fPrecompiled[variant]);
    glInterface->fFunctions.fDeleteProgram(fVariant[variant].program);
    fVariant[variant].program = 0;
    fVariant[variant].vertexStride = 0;
}

static const char s_colorVertexShader[] =
"attribute vec4 a_position;           \n"
"attribute vec4 a_color;              \n"
"uniform highp mat4 g_view;           \n"
"varying lowp vec4 v_color;           \n"
"void main() {                        \n"
"  gl_Position = g_view * a_position; \n"
"  gl_Position.z = 0.0;               \n"
"  v_color = a_color;                 \n";

static const char s_colorFragmentShader[] =
"uniform lowp float g_alpha;         \n"
"varying lowp vec4 v_color;          \n"
"void main() {                       \n"
"  gl_FragColor = g_alpha * v_color; \n";

template<>
void ProgramSet<ColorProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_colorVertexShader, s_colorFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].color = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_color");
    fVariant[variant].vertexStride += s_colorStride;
}

template <typename Program>
void ProgramSet<Program>::destroy(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (fVariant[variant].program == 0)
        return;
    glInterface->fFunctions.fDeleteProgram(fVariant[variant].program);
    fVariant[variant].program = 0;
    fVariant[variant].vertexStride = 0;
}

void ColorProgram::enableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fEnableVertexAttribArray(color);
    ProgramVariantCommon::enableVertexAttribArrays(glInterface);
}

void ColorProgram::disableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fDisableVertexAttribArray(color);
    ProgramVariantCommon::disableVertexAttribArrays(glInterface);
}

void ColorProgram::setVertexAttribPointers(const void* data, const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fVertexAttribPointer(color, 4, GR_GL_UNSIGNED_BYTE, GR_GL_TRUE, vertexStride, dataOffset(data, vertexStride, -s_colorStride));
    ProgramVariantCommon::setVertexAttribPointers(data, glInterface);
}

// The vertex shader passes w as a varying to the fragment shader, because
// that turned out to be faster than using gl_FragCoord.w (which is actually 1/w).
// We need to pass w^2 since the perspective correct interpolation divides by w.
// The formula for interpolation of quantity q at a point p, known to be q0, q1 and q2
// at vertices p0, p1 and p2 is
// q = b0*q0/w0 + b1*q1/w1 + b2*q2/w2, where b0, b1 and b2 are barycentric coordinates of p.
// Setting q = w would do no good, the formula would degenerate to b0 + b1 + b2 (which is 1).
// So we use q = w^2:
// w = b0*w0^2/w0 + b1*w1^2/w1 + b2*w2^2/w2 = b0*w0 + b1*w1 + b2*w2
// This does not give quite accurate interpolation but good enough to approximate the contribution
// to fwidth from the perspective divide.
static const char s_highTexcoord2VertexShader[] =
"attribute vec4 a_position;           \n"
"attribute vec2 a_texcoord;           \n"
"uniform highp mat4 g_view;           \n"
"varying highp vec2 v_texcoord;       \n"
"varying highp float v_w;\n"
"void main() {                        \n"
"  gl_Position = g_view * a_position; \n"
"  gl_Position.z = 0.0;               \n"
"  v_texcoord = a_texcoord;           \n"
"  v_w = gl_Position.w * gl_Position.w;\n";

static const char s_mediumTexcoord2VertexShader[] =
"attribute vec4 a_position;           \n"
"attribute vec2 a_texcoord;           \n"
"uniform highp mat4 g_view;           \n"
"varying mediump vec2 v_texcoord;     \n"
"void main() {                        \n"
"  gl_Position = g_view * a_position; \n"
"  gl_Position.z = 0.0;               \n"
"  v_texcoord = a_texcoord;           \n";

static const char s_textureFragmentShader[] =
"uniform lowp float g_alpha;                                  \n"
"uniform lowp sampler2D g_texture;                            \n"
"varying mediump vec2 v_texcoord;                             \n"
"void main() {                                                \n"
"  gl_FragColor = texture2D(g_texture, v_texcoord) * g_alpha; \n";

template<>
void ProgramSet<ImageProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_textureFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
}

void ImageProgram::enableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fEnableVertexAttribArray(texcoord);
    ProgramVariantCommon::enableVertexAttribArrays(glInterface);
}

void ImageProgram::disableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fDisableVertexAttribArray(texcoord);
    ProgramVariantCommon::disableVertexAttribArrays(glInterface);
}

void ImageProgram::setVertexAttribPointers(const void* data, const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fVertexAttribPointer(texcoord, 2, GR_GL_FLOAT, GR_GL_FALSE, vertexStride, dataOffset(data, vertexStride, -s_texcoordStride));
    ProgramVariantCommon::setVertexAttribPointers(data, glInterface);
}

static const char s_textureAlphaFragmentShader[] =
"uniform lowp float g_alpha;                                  \n"
"uniform lowp sampler2D g_texture;                            \n"
"varying mediump vec2 v_texcoord;                             \n"
"void main() {                                                \n"
"  gl_FragColor = vec4(0.0);                                  \n"
"  gl_FragColor.a = texture2D(g_texture, v_texcoord).a * g_alpha; \n";

template<>
void ProgramSet<ImageAlphaProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_textureAlphaFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
}

void PackedCoordImageProgram::setVertexAttribPointers(const void* data, const GrGLInterface* glInterface) const
{
    // PackedCoordImagePrograms use denormalized shorts instead of floats to save a word.
    // Then skip the parent class implementation. It would overwrite the texcoord pointer.
    glInterface->fFunctions.fVertexAttribPointer(texcoord, 2, GR_GL_UNSIGNED_SHORT, GR_GL_TRUE, vertexStride, dataOffset(data, vertexStride, -s_packedTexcoordStride));
    ProgramVariantCommon::setVertexAttribPointers(data, glInterface);
}

static const char s_textureBlurFragmentShader[] =
"uniform lowp float g_alpha;                               \n"
"uniform lowp sampler2D s_texture;                         \n"
"varying mediump vec2 v_texcoord;                          \n"
"uniform highp vec2 u_blurvector;                          \n"
"uniform highp float u_amount;                             \n"
"uniform mediump vec3 u_incr;                              \n"
"void main(void) {                                         \n"
"mediump vec3 incr = u_incr;                               \n"
"                                                          \n"
"mediump vec4 avg = vec4(0.0, 0.0, 0.0, 0.0);              \n"
"mediump float coefficientSum = 0.0;                       \n"
"                                                          \n"
"avg += texture2D(s_texture, v_texcoord.xy) * incr.x;      \n"
"coefficientSum += incr.x;                                 \n"
"incr.xy *= incr.yz;                                       \n"
"                                                          \n"
"for (highp float i = 1.0; i <= u_amount; i++) {                 \n"
"    avg += texture2D(s_texture, v_texcoord.xy - i * u_blurvector) * incr.x;\n"
"    avg += texture2D(s_texture, v_texcoord.xy + i * u_blurvector) * incr.x;\n"
"    coefficientSum += 2.0 * incr.x;                       \n"
"    incr.xy *= incr.yz;                                   \n"
"}                                                         \n"
"gl_FragColor = (avg / coefficientSum) * g_alpha;          \n";

template<>
void ProgramSet<ImageBlurProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_textureBlurFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
    fVariant[variant].blurvector = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "u_blurvector");
    fVariant[variant].amount = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "u_amount");
    fVariant[variant].incr = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "u_incr");
}

static const char s_blurredTextVertexShader[] =
"attribute vec4 a_position;           \n"
"attribute vec2 a_texcoord;           \n"
"uniform highp mat4 g_view;           \n"
"varying highp vec2 v_texcoord;       \n"
"void main() {                        \n"
"  gl_Position = g_view * a_position; \n"
"  gl_Position.z = 0.0;               \n"
"  v_texcoord = a_texcoord;           \n";

static const char s_blurredTextFragmentShader[] =
"uniform lowp vec4 g_color;                                                        \n"
"uniform highp vec2 g_cutoff;                                                      \n"
"uniform mediump float u_amount;                                                   \n"
// The term "amount" and not "blurRadius" is used because this filter is not truly
// a blur and depending on the font, "amount" is not necessarily exactly the radius
// of the blur.
"uniform lowp sampler2D g_texture;                                                 \n"
"varying highp vec2 v_texcoord;                                                    \n"
"void main() {                                                                     \n"
"  highp float normDistance =                                                      \n"
"      (texture2D(g_texture, v_texcoord).a - 0.5) / u_amount + 0.5;                \n"
"                                                                                  \n"
"  highp float rounding = cos((normDistance - 0.5) * 3.14);                        \n"
"                                                                                  \n"
"  gl_FragColor =                                                                  \n"
"      g_color * 2.0 *                                                             \n"
"      smoothstep(0.3, 0.7, normDistance) *                                        \n"
"      smoothstep(0.2, 0.8, normDistance) *                                        \n"
"      rounding * rounding;                                                        \n";

template<>
void ProgramSet<BlurredEdgeTextProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_blurredTextVertexShader, s_blurredTextFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].color = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_color");
    fVariant[variant].cutoff = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_cutoff");
    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_packedTexcoordStride;
    fVariant[variant].amount = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "u_amount");
}

template<>
void ProgramSet<SkShaderProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_textureFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
}

static const char s_skShaderLinearGradientFragmentShader[] =
"uniform lowp float g_alpha;                                  \n"
"uniform lowp sampler2D g_texture;                            \n"
"uniform mediump mat3 g_transform;                            \n"
"varying mediump vec2 v_texcoord;                             \n"
"void main() {                                                \n"
"  mediump vec3 tex = g_transform * vec3(v_texcoord, 1.0);    \n"
"  gl_FragColor = texture2D(g_texture, tex.xy) * g_alpha;     \n";

template<>
void ProgramSet<SkShaderLinearGradientProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_skShaderLinearGradientFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
    fVariant[variant].transform = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_transform");
}

static const char s_skShaderSweepGradientFragmentShader[] =
"uniform lowp float g_alpha;                                  \n"
"uniform lowp sampler2D g_texture;                            \n"
"uniform mediump mat3 g_transform;                            \n"
"varying mediump vec2 v_texcoord;                             \n"
"const mediump float inv2pi = 0.159155;                       \n"
"void main() {                                                \n"
"  mediump vec3 pt = g_transform * vec3(v_texcoord, 1.0);     \n"
"  mediump vec2 tex = vec2(0.5 - atan(pt.y, -pt.x) * inv2pi, 0.0);    \n"
"  gl_FragColor = texture2D(g_texture, tex) * g_alpha;        \n";

template<>
void ProgramSet<SkShaderSweepGradientProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_skShaderSweepGradientFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
    fVariant[variant].transform = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_transform");
}

static const char s_skShaderRadialGradientFragmentShader[] =
"uniform lowp float g_alpha;                                  \n"
"uniform lowp sampler2D g_texture;                            \n"
"uniform mediump mat3 g_transform;                            \n"
"varying mediump vec2 v_texcoord;                             \n"
"void main() {                                                \n"
"  mediump vec3 pt = g_transform * vec3(v_texcoord, 1.0);     \n"
"  mediump vec2 tex = vec2(length(pt.xy), 0.0);               \n"
"  gl_FragColor = texture2D(g_texture, tex) * g_alpha;        \n";

template<>
void ProgramSet<SkShaderRadialGradientProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_skShaderRadialGradientFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
    fVariant[variant].transform = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_transform");
}

// This is a correct implementation of 2 point conic gradients, and an ok approximation of 2 point radial
static const char s_skShaderTwoPointRadialGradientFragmentShader[] =
"uniform lowp float g_alpha;                                      \n"
"uniform lowp sampler2D g_texture;                                \n"
"uniform mediump mat3 g_transform;                                \n"
"uniform mediump float f_xOffset;                                 \n"
"uniform mediump float f_a;                                       \n"
"uniform mediump float f_r1Squared;                               \n"
"uniform mediump float f_r1TimesDiff;                             \n"
"varying mediump vec2 v_texcoord;                                 \n"
"void main() {                                                    \n"
"  mediump vec3 pt = g_transform * vec3(v_texcoord, 1.0);         \n"
"  mediump float b = f_xOffset * pt.x + f_r1TimesDiff;            \n"
"  mediump float c = pt.x * pt.x + pt.y * pt.y - f_r1Squared;     \n"
"  highp vec2 tex = vec2((b - sqrt(b * b - f_a * c)) / f_a, 0.0); \n"
"  gl_FragColor = texture2D(g_texture, tex) * g_alpha;            \n";

template<>
void ProgramSet<SkShaderTwoPointRadialGradientProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_skShaderTwoPointRadialGradientFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].transform = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_transform");
    fVariant[variant].xOffset = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "f_xOffset");
    fVariant[variant].a = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "f_a");
    fVariant[variant].r1Squared = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "f_r1Squared");
    fVariant[variant].r1TimesDiff = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "f_r1TimesDiff");
    fVariant[variant].vertexStride += s_texcoordStride;
}

static const char s_edgeTextVertexShader[] =
"attribute vec4 a_position;                         \n"
"attribute vec2 a_texcoord;                         \n"
"uniform highp mat4 g_view;                         \n"
"uniform highp vec2 g_cutoff;                       \n"
"varying mediump vec4 v_texcoord;                   \n"
"void main() {                                      \n"
"  gl_Position = g_view * a_position;               \n"
"  gl_Position.z = 0.0;                             \n"
"                                                   \n"
"  mediump float w = gl_Position.w * gl_Position.w; \n"
"                                                   \n"
"  v_texcoord.xy = a_texcoord;                      \n"
"  v_texcoord.zw = g_cutoff * w + 0.5;              \n"
"  v_texcoord.z = v_texcoord.z - v_texcoord.w;      \n";

static const char s_edgeTextFragmentShader[] =
"uniform lowp vec4 g_color;                                                            \n"
"uniform lowp sampler2D g_texture;                                                     \n"
"varying mediump vec4 v_texcoord;                                                      \n"
"void main() {                                                                         \n"
"  lowp float distance = texture2D(g_texture, v_texcoord.xy).a;                        \n"
"  gl_FragColor = g_color * clamp((distance - v_texcoord.w) / v_texcoord.z, 0.0, 1.0); \n";

template<>
void ProgramSet<EdgeTextProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_edgeTextVertexShader, s_edgeTextFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].color = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_color");
    fVariant[variant].cutoff = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_cutoff");
    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_packedTexcoordStride;
}

static const char s_strokedEdgeTextFragmentShader[] =
"uniform lowp vec4 g_fillColor;                                                                                                        \n"
"uniform lowp vec4 g_strokeColor;                                                                                                      \n"
"uniform lowp float g_halfStrokeWidth;                                                                                                 \n"
"uniform lowp sampler2D g_texture;                                                                                                     \n"
"varying mediump vec4 v_texcoord;                                                                                                      \n"
"void main() {                                                                                                                         \n"
"  lowp float distance = texture2D(g_texture, v_texcoord.xy).a;                                                                        \n"
"  lowp float strokeAlpha = smoothstep(v_texcoord.w - g_halfStrokeWidth, v_texcoord.w + v_texcoord.z - g_halfStrokeWidth, distance)    \n"
"                  * (1.0 - smoothstep(v_texcoord.w + g_halfStrokeWidth, v_texcoord.w + v_texcoord.z + g_halfStrokeWidth, distance));  \n"
"  lowp vec4 fillColor = g_fillColor * clamp((distance - v_texcoord.w) / v_texcoord.z, 0.0, 1.0);                                      \n"
"  gl_FragColor = mix(fillColor, g_strokeColor, strokeAlpha);                                                                          \n";

template<>
void ProgramSet<StrokedEdgeTextProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_edgeTextVertexShader, s_strokedEdgeTextFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].fillColor = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_fillColor");
    fVariant[variant].strokeColor = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_strokeColor");
    fVariant[variant].halfStrokeWidth = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_halfStrokeWidth");
    fVariant[variant].cutoff = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_cutoff");
    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_packedTexcoordStride;
}

static const char s_textFragmentShader[] =
"uniform lowp vec4 g_color;                                        \n"
"uniform lowp sampler2D g_texture;                                 \n"
"varying mediump vec2 v_texcoord;                                  \n"
"void main() {                                                     \n"
"  lowp float alpha = texture2D(g_texture, v_texcoord).a;          \n"
"  gl_FragColor = g_color * alpha;                                 \n";

template<>
void ProgramSet<TextProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_textFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].color = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_color");
    fVariant[variant].texture = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_texture");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_packedTexcoordStride;
}

static const char s_arcFragmentShader[] =
"uniform lowp float g_alpha;                                       \n"
"uniform lowp vec4 g_color;                                        \n"
"uniform lowp float g_inside;                                      \n"
"varying mediump vec2 v_texcoord;                                  \n"
"void main() {                                                     \n"
"lowp float isInside = step(0.5, g_inside);                        \n"
"lowp float texLength = length(v_texcoord);                        \n"
"lowp float mystep = smoothstep(0.0, 1.0, (1.0 - isInside) * (1.0 - texLength) + isInside * texLength);\n"
"gl_FragColor = g_color * g_alpha * mystep;                        \n";

template<>
void ProgramSet<ArcProgram>::create(const GrGLInterface* glInterface, CommandVariant variant)
{
    if (!loadFromPrecompiled(glInterface, variant))
        fVariant[variant].program = compile(glInterface, s_mediumTexcoord2VertexShader, s_arcFragmentShader, variant);

    if (!fVariant[variant].program)
        return;

    fVariant[variant].color = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_color");
    fVariant[variant].inside = glInterface->fFunctions.fGetUniformLocation(fVariant[variant].program, "g_inside");
    fVariant[variant].texcoord = glInterface->fFunctions.fGetAttribLocation(fVariant[variant].program, "a_texcoord");
    fVariant[variant].vertexStride += s_texcoordStride;
}

void ArcProgram::enableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fEnableVertexAttribArray(texcoord);
    ProgramVariantCommon::enableVertexAttribArrays(glInterface);
}

void ArcProgram::disableVertexAttribArrays(const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fDisableVertexAttribArray(texcoord);
    ProgramVariantCommon::disableVertexAttribArrays(glInterface);
}

void ArcProgram::setVertexAttribPointers(const void* data, const GrGLInterface* glInterface) const
{
    glInterface->fFunctions.fVertexAttribPointer(texcoord, 2, GR_GL_FLOAT, GR_GL_FALSE, vertexStride, dataOffset(data, vertexStride, -s_texcoordStride));
    ProgramVariantCommon::setVertexAttribPointers(data, glInterface);
}

Shaders::Shaders(GrGpu* gpu)
    : GrResource(gpu, false)
    , fGpu(static_cast<GrGpuGL*>(gpu))
    , fGlInterface(fGpu->glInterface())
    , fP(0)
{ }

Shaders::~Shaders() {
}

size_t Shaders::sizeInBytes() const {
    return 0;
}
void Shaders::internal_dispose() const {
    this->internal_dispose_restore_refcnt_to_1();
    SkDELETE(this);
}

void Shaders::onRelease() {
    for (unsigned i = 0; i < NumberOfVariants; ++i) {
        CommandVariant variant = (CommandVariant)i;
        fColor.destroy(fGlInterface, variant);
        fImage.destroy(fGlInterface, variant);
        fSkShader.destroy(fGlInterface, variant);
        fSkShaderLinearGradient.destroy(fGlInterface, variant);
        fSkShaderRadialGradient.destroy(fGlInterface, variant);
        fSkShaderSweepGradient.destroy(fGlInterface, variant);
        fSkShaderTwoPointRadialGradient.destroy(fGlInterface, variant);
        fImageBlur.destroy(fGlInterface, variant);
        fBlurredEdgeText.destroy(fGlInterface, variant);
        fEdgeText.destroy(fGlInterface, variant);
        fStrokedEdgeText.destroy(fGlInterface, variant);
        fText.destroy(fGlInterface, variant);
        fArc.destroy(fGlInterface, variant);
    }
    GrResource::onRelease();
}

void Shaders::onAbandon()
{
    // TODO: This is certainly unrecoverable, maybe memset everything to 0?
    GrResource::onAbandon();
}

void Shaders::reset(const SkMatrix44& view, Projection projection)
{
    SkMatrix44 concatenated = projection == Identity ? view : yInverter() * view;
    concatenated.asColMajorf(fView);

    // First disable everything, although it should not matter the sequence
    // of steps it's better this way. Also the spec is not clear if the
    // attributes should be disabled if program = 0 is being loaded. However
    // vertex attributes has to be disabled-enabled if one valid program is
    // simply exchanged to another. Hence let's disable evrything to be on a
    // safe side.
    static const GrGLint maxVertexAttribs = fGpu->glCaps().maxVertexAttributes();
    for (int i = 0; i < maxVertexAttribs; ++i) // reset means reset All!
        fGlInterface->fFunctions.fDisableVertexAttribArray(i);

    // If program is zero, then the current rendering state refers to an
    // invalid program object and the results of shader execution are
    // undefined. However, this is not an error.
    fGlInterface->fFunctions.fUseProgram(0);
    fP = 0;
}

void Shaders::compileAndStore()
{
    // Precompiling default shaders and some commonly used shaders.
    fColor.precompile(fGlInterface, DefaultVariant);
    fColor.precompile(fGlInterface, InnerRoundedVariant);
    fColor.precompile(fGlInterface, OuterRoundedVariant);
    fColor.precompile(fGlInterface, BothRoundedVariant);
    fImage.precompile(fGlInterface, DefaultVariant);
    fImage.precompile(fGlInterface, OuterRoundedVariant);
    fImageBlur.precompile(fGlInterface, DefaultVariant);
    fText.precompile(fGlInterface, DefaultVariant);
    fBlurredEdgeText.precompile(fGlInterface, DefaultVariant);
    fEdgeText.precompile(fGlInterface, DefaultVariant);
    fStrokedEdgeText.precompile(fGlInterface, DefaultVariant);
    fArc.precompile(fGlInterface, DefaultVariant);
    fSkShaderRadialGradient.precompile(fGlInterface, DefaultVariant);
    fSkShaderSweepGradient.precompile(fGlInterface, DefaultVariant);
    fSkShaderTwoPointRadialGradient.precompile(fGlInterface, DefaultVariant);
    fSkShaderLinearGradient.precompile(fGlInterface, DefaultVariant);
    fSkShader.precompile(fGlInterface, DefaultVariant);
}

void Shaders::loadCommon(const ProgramVariantCommon& p)
{
    if (fP == &p)
        return;

    if (fP)
        fP->disableVertexAttribArrays(fGlInterface);

    fGlInterface->fFunctions.fUseProgram(p.program);
    fGlInterface->fFunctions.fUniformMatrix4fv(p.view, 1, false, fView);
    p.enableVertexAttribArrays(fGlInterface);
    fP = &p;
}

void Shaders::loadImage(const ImageProgram& p)
{
    if (fP == &p)
        return;

    if (fP)
        fP->disableVertexAttribArrays(fGlInterface);

    fGlInterface->fFunctions.fUseProgram(p.program);
    fGlInterface->fFunctions.fUniformMatrix4fv(p.view, 1, false, fView);
    fGlInterface->fFunctions.fUniform1i(p.texture, 0);
    p.enableVertexAttribArrays(fGlInterface);
    fP = &p;
}

// These methods can't be placed into *.h file because they call for template
// "variant" and actualy "create" it right here.
const ColorProgram& Shaders::loadColor(CommandVariant variant)
{
    const ColorProgram& p = fColor.variant(fGlInterface, variant);
    loadCommon(p);
    return p;
}

const ImageProgram& Shaders::loadImage(CommandVariant variant)
{
    const ImageProgram& p = fImage.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const ImageAlphaProgram& Shaders::loadImageAlpha(CommandVariant variant)
{
    const ImageAlphaProgram& p = fImageAlpha.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const ImageBlurProgram& Shaders::loadImageBlur(CommandVariant variant)
{
    const ImageBlurProgram& p = fImageBlur.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const BlurredEdgeTextProgram& Shaders::loadBlurredEdgeText(CommandVariant variant)
{
    const BlurredEdgeTextProgram& p = fBlurredEdgeText.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const SkShaderProgram& Shaders::loadSkShader(CommandVariant variant, SkShader::GradientType type)
{
    const SkShaderProgram *p = 0;
    switch (type) {
    case SkShader::kRadial_GradientType:
        p = &fSkShaderRadialGradient.variant(fGlInterface, variant);
        break;
    case SkShader::kSweep_GradientType:
        p = &fSkShaderSweepGradient.variant(fGlInterface, variant);
        break;
    case SkShader::kRadial2_GradientType:
        p = &fSkShaderTwoPointRadialGradient.variant(fGlInterface, variant);
        break;
    case SkShader::kLinear_GradientType:
        p = &fSkShaderLinearGradient.variant(fGlInterface, variant);
        break;
    default:
        p = &fSkShader.variant(fGlInterface, variant);
        break;
    }
    loadImage(*p);
    return *p;
}

const EdgeTextProgram& Shaders::loadEdgeText(CommandVariant variant)
{
    const EdgeTextProgram& p = fEdgeText.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const StrokedEdgeTextProgram& Shaders::loadStrokedEdgeText(CommandVariant variant)
{
    const StrokedEdgeTextProgram& p = fStrokedEdgeText.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const TextProgram& Shaders::loadText(CommandVariant variant)
{
    const TextProgram& p = fText.variant(fGlInterface, variant);
    loadImage(p);
    return p;
}

const ArcProgram& Shaders::loadArc(CommandVariant variant)
{
    const ArcProgram& p = fArc.variant(fGlInterface, variant);
    loadCommon(p);
    return p;
}

} // namespace Gentl
