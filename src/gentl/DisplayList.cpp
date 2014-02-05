/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "DisplayList.h"

#include "FramebufferStack.h"
#include "GentlCommands.h"
#include "GrTypes.h"
#include "GrTexture.h"
#include "GlyphCache.h"
#include "Shaders.h"
#include "ShadersCache.h"
#include "SkMatrix44.h"
#include "SkShader.h"
#include "SkTypes.h"
#include "SkXfermode.h"
#include "VertexBufferCache.h"

#include "GrContext.h"
#include "gl/GrGLDefines.h"
#include "gl/GrGLFunctions.h"
#include "gl/GrGLInterface.h"
#include "gl/GrGLVertexBuffer.h"
#include "gl/GrGpuGL.h"

namespace Gentl {

DisplayList::DisplayList(GrContext* grContext, const SkISize& size, BufferBacking backing, Projection defaultProjection)
    : fBackingImage(0)
    , fXfermode(SkXfermode::kSrcOver_Mode)
    , fSpecifiedBacking(size.isEmpty() ? NoBacking : backing)
    , fCurrentBacking(fSpecifiedBacking == AlphaBacking ? AlphaBacking : NoBacking)
    , fVertexAttributeStorage(VertexBuffer)
    , fVbo(0)
    , fBackingVbo(0)
    , fLastVboSize(0)
    , fSize(size)
    , fShaders(ShadersCache::instance()->getShadersForGpu(grContext->getGpu()))
    , fGrContext(grContext)
    , fGpu(static_cast<GrGpuGL*>(grContext->getGpu()))
    , fGlInterface(fGpu->glInterface())
    , fDefaultProjection(defaultProjection)
{
}

DisplayList::~DisplayList()
{
    if (fBackingImage) {
        fBackingImage->unref();
        fBackingImage = 0;
    }

    VertexBufferCache::recycleThreadSafe(fVbo);
    fVbo = 0;

    VertexBufferCache::recycleThreadSafe(fBackingVbo);
    fBackingVbo = 0;
}

SkMatrix44 DisplayList::getDefaultProjection() {
    float scaleX = 2.0 / fSize.width();
    float scaleY = 2.0 / std::min(fSize.height(), fGrContext->getMaxRenderTargetSize());
    SkMatrix44 view(SkMatrix44::kIdentity_Constructor);
    view.preTranslate(-1, -1, 0);
    view.preScale(scaleX, scaleY, 1);
    return view;
}

bool DisplayList::isEmpty() const
{
    return fInstructions.isEmpty();
}

bool DisplayList::updateBackingSurface(const SkMatrix44& view)
{
    // FIXME: on demand backing is not implemented yet, so treat it as no backing.
    //        The basic plan is to use backing on the first draw of the DL, and then
    //        drop the backing if it turned out to not be needed (e.g. because drawing
    //        the DL was fast).
    switch (fCurrentBacking) {
    case NoBacking:
    case OnDemandBacking:
        if (fBackingImage) {
            fBackingImage->unref();
            fBackingImage = 0;
        }
        return false;
    case AlphaBacking:
        break;
    }

    // Shortcircuit if there are no commands queued up and we have already created the backing image
    // Note it's important to at least create the backing image, callers depend on us having a texture ID
    // after this call.
    if (fInstructions.isEmpty() && fBackingImage)
        return false;

    // Save current state and auto-restore after drawing.
    ViewportSaver viewportSaver(fGlInterface);
    GlobalFlagStateSaver flagSaver(fGlInterface,
        GlobalFlagState::RestoreScissorTest
        | GlobalFlagState::RestoreCullFace);
    fGlInterface->fFunctions.fDisable(GR_GL_SCISSOR_TEST);
    fGlInterface->fFunctions.fDisable(GR_GL_CULL_FACE);
    FramebufferSaver saver(fGlInterface);

    bool needClear = false;
    if (!fBackingImage) {
        GrTextureDesc desc;
        desc.fFlags = kRenderTarget_GrTextureFlagBit | kNoStencil_GrTextureFlagBit;
        desc.fConfig = kRGBA_8888_GrPixelConfig;
        desc.fWidth = fSize.width();
        desc.fHeight = fSize.height();
        fBackingImage = fGpu->createTexture(desc, 0, 0);

        if (!fBackingImage) {
           SkDebugf("Can't create backing image.");
           return false;
        }

        needClear = true;
    }

    fGlInterface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, fBackingImage->asRenderTarget()->getRenderTargetHandle());
    fGlInterface->fFunctions.fViewport(0, 0, fSize.width(), fSize.height());

    if (needClear) {
        fGlInterface->fFunctions.fClearColor(0, 0, 0, 0);
        fGlInterface->fFunctions.fClear(GR_GL_COLOR_BUFFER_BIT);
    }

    drawDisplayList(view, Identity, 255);
    return true;
}

void DisplayList::readPixels(const SkIRect& rect, unsigned char* buffer, bool unmultiply)
{
    if (!fBackingImage) {
        SkDebugf("Cannot read pixels without a backing image. Clearing buffer.\n");
        memset(buffer, 0, rect.width() * rect.height() * 4);
        return;
    }

    // Save current state and auto-restore after drawing.
    ViewportSaver viewportSaver(fGlInterface);
    GlobalFlagStateSaver flagSaver(fGlInterface, GlobalFlagState::RestoreScissorTest);
    fGlInterface->fFunctions.fDisable(GR_GL_SCISSOR_TEST);

    FramebufferSaver framebufferSaver(fGlInterface);
    fGlInterface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, fBackingImage->asRenderTarget()->getRenderTargetHandle());

    int otherFormat = 0, otherType = 0;
    GR_GL_GetIntegerv(fGlInterface, GR_GL_IMPLEMENTATION_COLOR_READ_FORMAT, &otherFormat);
    GR_GL_GetIntegerv(fGlInterface, GR_GL_IMPLEMENTATION_COLOR_READ_TYPE,   &otherType);

    bool swapRB = false;
    if (otherType == GR_GL_BGRA && otherType == GR_GL_UNSIGNED_BYTE) {
        fGlInterface->fFunctions.fReadPixels(rect.x(), rect.y(), rect.width(), rect.height(), GR_GL_BGRA, GR_GL_UNSIGNED_BYTE, buffer);
    } else {
        // BGRA isn't supported, need to swap some pixels
        fGlInterface->fFunctions.fReadPixels(rect.x(), rect.y(), rect.width(), rect.height(), GR_GL_RGBA, GR_GL_UNSIGNED_BYTE, buffer);
        swapRB = true;
    }

    if (unmultiply || swapRB) {
        for (int i = 0; i < rect.height(); ++i) {
            unsigned char* rgba = buffer + i * rect.width() * 4;
            for (int j = 0; j < rect.width(); ++j, rgba += 4) {
                if (rgba[3] && rgba[3] != 255) {
                    rgba[0] = std::min(rgba[0] * 255.0f / rgba[3], 255.0f);
                    rgba[1] = std::min(rgba[1] * 255.0f / rgba[3], 255.0f);
                    rgba[2] = std::min(rgba[2] * 255.0f / rgba[3], 255.0f);
                }
                if (swapRB) {
                    std::swap(rgba[0], rgba[2]);
                }
            }
        }
    }
}

void DisplayList::clear()
{
    if (fBackingImage && fBackingImage->getRefCnt() > 1) {
        fBackingImage->unref();
        fBackingImage = 0;
    }

    fInstructions.unrefAndClear();

    if (specifiedBacking() == OnDemandBacking)
        setCurrentBacking(NoBacking);
}

void DisplayList::draw(const SkIRect& dstRect, const SkIRect& srcRect, unsigned char globalAlpha)
{
    if (dstRect.isEmpty() || srcRect.isEmpty())
        return;

    // Save current state and auto-restore after drawing.
    ViewportSaver viewportSaver(fGlInterface);
    ScissorBoxSaver scissorBoxSaver(fGlInterface);
    GlobalFlagStateSaver flagSaver(fGlInterface,
        GlobalFlagState::RestoreScissorTest
        | GlobalFlagState::RestoreCullFace);

    fGlInterface->fFunctions.fViewport(dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height());
    fGlInterface->fFunctions.fScissor(dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height());
    fGlInterface->fFunctions.fEnable(GR_GL_SCISSOR_TEST);
    fGlInterface->fFunctions.fDisable(GR_GL_CULL_FACE);

    float scaleX = 2.0 / srcRect.width();
    float scaleY = 2.0 / srcRect.height();

    SkMatrix44 view(SkMatrix44::kIdentity_Constructor);
    view.preTranslate(-1, -1, 0);
    view.preScale(scaleX, scaleY, 1);
    view.preTranslate(-srcRect.x(), srcRect.y(), 0);

    draw(view, globalAlpha);
}

void DisplayList::draw(const SkMatrix44& view, unsigned char globalAlpha)
{
    updateBackingSurface(getDefaultProjection());

    if (fBackingImage) {
        if (!bindTexture(fBackingImage, SkPaint::kLow_FilterLevel)) {
            SkDebugf("Couldn't bind and blit backing surface to root framebuffer.\n");
            SkASSERT(false);
            return;
        }

        blendFunc(SkXfermode::kSrcOver_Mode);
        fShaders->reset(view, fDefaultProjection);
        const ImageProgram& p = fShaders->loadImage(DefaultVariant);
        bindBackingVertexBuffer();

        fGlInterface->fFunctions.fUniform1f(p.alpha, globalAlpha / 255.0);
        p.setVertexAttribPointers(0, fGlInterface);
        fGlInterface->fFunctions.fDrawArrays(GR_GL_TRIANGLE_FAN, 0, 4);
    } else {
        drawDisplayList(view, fDefaultProjection, globalAlpha);
    }
}

void DisplayList::bindBackingVertexBuffer()
{
    if (!fBackingVbo) {
        // Even though we could represent these values with shorts instead of
        // floats the ImageProgram internals demand 32 floating point data.
        const float w = fSize.width();
        const float h = fSize.height();
        const float data[] =  { 0, 0, 0, 0,
                                w, 0, 1, 0,
                                w, h, 1, 1,
                                0, h, 0, 1 };

        const unsigned bytes = sizeof(data);
        SkASSERT(bytes == 64);

        fBackingVbo = VertexBufferCache::instance()->createFromCache(bytes, fGpu);
        fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, fBackingVbo->bufferID());
        fGlInterface->fFunctions.fBufferSubData(GR_GL_ARRAY_BUFFER, 0, bytes, data);
    } else {
        fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, fBackingVbo->bufferID());
    }
}

void DisplayList::blendFunc(SkXfermode::Mode mode)
{
    switch (mode) {
    case SkXfermode::kClear_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ZERO, GR_GL_ZERO);
        break;
    case SkXfermode::kSrc_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE, GR_GL_ZERO);
        break;
    case SkXfermode::kDst_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ZERO, GR_GL_ONE);
        break;
    case SkXfermode::kSrcOver_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE, GR_GL_ONE_MINUS_SRC_ALPHA);
        break;
    case SkXfermode::kDstOver_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE_MINUS_DST_ALPHA, GR_GL_ONE);
        break;
    case SkXfermode::kSrcIn_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_DST_ALPHA, GR_GL_ZERO);
        break;
    case SkXfermode::kDstIn_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ZERO, GR_GL_SRC_ALPHA);
        break;
    case SkXfermode::kSrcOut_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE_MINUS_DST_ALPHA, GR_GL_ZERO);
        break;
    case SkXfermode::kDstOut_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ZERO, GR_GL_ONE_MINUS_SRC_ALPHA);
        break;
    case SkXfermode::kSrcATop_Mode:
        fGlInterface->fFunctions.fBlendFuncSeparate(GR_GL_DST_ALPHA, GR_GL_ONE_MINUS_SRC_ALPHA, GR_GL_ZERO, GR_GL_ONE);
        break;
    case SkXfermode::kDstATop_Mode:
        fGlInterface->fFunctions.fBlendFuncSeparate(GR_GL_ONE_MINUS_DST_ALPHA, GR_GL_SRC_ALPHA, GR_GL_ONE, GR_GL_ZERO);
        break;
    case SkXfermode::kXor_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE_MINUS_DST_ALPHA, GR_GL_ONE_MINUS_SRC_ALPHA);
        break;
    case SkXfermode::kPlus_Mode:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE, GR_GL_ONE);
        break;
    default:
        fGlInterface->fFunctions.fBlendFunc(GR_GL_ONE, GR_GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

inline void DisplayList::updateGLBlend(float currentAlpha) const
{
    if (fXfermode == SkXfermode::kSrc_Mode && currentAlpha == 1.0)
        fGlInterface->fFunctions.fDisable(GR_GL_BLEND);
    else
        fGlInterface->fFunctions.fEnable(GR_GL_BLEND);
}

inline bool DisplayList::bindTexture(GrTexture* texture, SkPaint::FilterLevel filterLevel)
{
    if (!texture)
        return false;

    static const GrTextureParams bilinear(SkShader::kClamp_TileMode, GrTextureParams::kBilerp_FilterMode);
    static const GrTextureParams nearest(SkShader::kClamp_TileMode, GrTextureParams::kNone_FilterMode);
    fGpu->bindTexture(0, filterLevel == SkPaint::kNone_FilterLevel ? nearest : bilinear, static_cast<GrGLTexture*>(texture));
    return true;
}

inline const void* DisplayList::getVertexAttributePointer(unsigned index) const
{
    // When using VBOs, the vertex attribute 'pointer' is actually a byte offset into the VBO.
    // However fGlInterface->fFunctions.fVertexAttribPointer still requires a void* so we follow that convention here.
    if (fVertexAttributeStorage == VertexBuffer)
        return reinterpret_cast<void*>(fInstructions[index].vertexAttributeIndex * sizeof(InstructionSetData));

    SkASSERT(fVertexAttributeStorage == ApplicationMemory);
    return fInstructions.vertexAttributeData() + fInstructions[index].vertexAttributeIndex;
}

inline void DisplayList::drawTriangles(GrGLint triangleMode, unsigned numElements) {
    switch (triangleMode) {
    case GR_GL_TRIANGLES:
        fGlInterface->fFunctions.fDrawElements(GR_GL_TRIANGLES, numElements, GR_GL_UNSIGNED_SHORT, 0);
        break;
    case GR_GL_TRIANGLE_STRIP:
        fGlInterface->fFunctions.fDrawArrays(GR_GL_TRIANGLE_STRIP, 0, numElements);
        break;
    }
}

class VertexDataAccessor {
public:
    VertexDataAccessor(const GrGLInterface* glInterface)
        : fGlInterface(glInterface)
        , pos(0)
    { }

    void onAccessChunk(InstructionSetData* data, unsigned count)
    {
        const unsigned bytes = count * sizeof(InstructionSetData);
        fGlInterface->fFunctions.fBufferSubData(GR_GL_ARRAY_BUFFER, pos, bytes, data);
        pos += bytes;
    }

private:
    const GrGLInterface* fGlInterface;
    unsigned pos;
};

void DisplayList::drawDisplayList(const SkMatrix44& view, Projection projection, unsigned char inGlobalAlpha)
{
    GlyphCache::getGlyphCacheForGpu(fGrContext->getGpu())->update();

    // If we have no data, and we never did, we have nothing to draw.
    if (!fLastVboSize && !fInstructions.dataBytes())
        return;

    // Using VBOs in this method requires caching the state. A bound array buffer
    // affects every fGlInterface->fFunctions.fDrawArrays command so we need to restore before returning.
    BufferBindingSaver bufferBindingSaver(fGlInterface);

    if (fVertexAttributeStorage == VertexBuffer) {
        // If our InstructionSet contains any vertex attribute data that means it
        // has been changed because we clear it out after we upload it to the vbo.
        if (fInstructions.dataBytes()) {
            // Recycle the old vbo even if it's big enough to hold the new data
            // because the cache may return us a more appropriately sized one.
            VertexBufferCache::recycleThreadSafe(fVbo);
            fLastVboSize = fInstructions.dataBytes();
            fVbo = VertexBufferCache::instance()->createFromCache(fLastVboSize, fGpu);
            fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, fVbo->bufferID());
            VertexDataAccessor accessor(fGlInterface);
            fInstructions.accessVertexAttributes(accessor);
            fInstructions.destroyVertexAttributes();
        } else {
            fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, fVbo->bufferID());
        }
    } else {
        fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, 0);
    }

    VertexBufferCache::instance()->bindQuadEBO(fGpu);
    blendFunc(fXfermode);

    const float globalAlpha = inGlobalAlpha / 255.0f;

    // Approximate the size of one screen pixel in glyph cache texture local coordinates.
    // The gl_FragCoord.w can be used in the shader to account for the scaling performed by
    // the GPU when doing the perspective divide (but it was found to be faster to pass w as
    // a varying, v_w, from the vertex shader to the fragment shader).
    // Beyond w, we need to find any scaling inherent in the view matrix. Scaling from the
    // GentlContext's transform at the time the glyphs were added is already baked into the
    // cutoff uniform.
    int tempViewport[4] = {0,0,0,0};
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_VIEWPORT, tempViewport);
    SkIRect currentViewport = SkIRect::MakeXYWH(tempViewport[0], tempViewport[1], tempViewport[2], tempViewport[3]);
    double sw = currentViewport.width() * 0.5;
    double sh = -currentViewport.height() * 0.5;
    SkMatrix44 m(SkMatrix44::kIdentity_Constructor);
    m.preScale(sw, sh, 1);
    m.preConcat(view);

    // The idea here is to unmap a vector of size (1, 1) from screen space back to model space.
    // The reason we use a vector of size sqrt(2) rather than a normalized one, is that the
    // GLSL expression this is intended to replace, "length(fwidth(v_texcoord))", yields sqrt(2)
    // when the derivatives are all equal to 1.
    // FIXME: This is a simplistic calculation that doesn't fully account for the perspective.
    //        When we move a distance dx, dy from a particular point x, y in model space, we will
    //        actually move a distance dx', dy', dz', dw' in screen space (before perspective
    //        divide) that depends on the starting point. The end result is that we smooth too
    //        little for objects that are near perpendicular to the image plane. However, no matter
    //        what we do there, text in such a plane isn't legible anyway, so it's no biggie.
    float fragmentScale;
    if (!m.isScaleTranslate()) {
        SkMatrix44 inv(SkMatrix44::kUninitialized_Constructor);
        m.invert(&inv);
        float sample[] = { 1, 1, 0, 1 };
        inv.mapScalars(sample, sample);
        fragmentScale = sqrtf(sample[0] * sample[0] + sample[1] * sample[1] + sample[2] * sample[2]);
    } else {
        // If m doesn't mess with z, we can get by with inverting the upper left 2x2 matrix.
        float det = (m.getFloat(1, 1) * m.getFloat(2, 2) - m.getFloat(1, 2) * m.getFloat(2, 1));
        if (FloatUtils::isNearZero(det)) {
            fragmentScale = M_SQRT2;
        } else {
            float invdet = 1 / det;
            float dx = invdet * (m.getFloat(2, 2) - m.getFloat(1, 2));
            float dy = invdet * (m.getFloat(1, 1) - m.getFloat(2, 1));
            fragmentScale = SkVector::Make(dx, dy).length();
        }
    }

    instructionLoop(view, Identity, globalAlpha, fragmentScale, OffscreenRun);
    instructionLoop(view, projection, globalAlpha, fragmentScale, FinalRun);
}

void DisplayList::instructionLoop(const SkMatrix44& view, Projection projection, unsigned char globalAlpha, const float fragmentScale, InstructionRun run)
{
    float currentAlpha = globalAlpha;

    fShaders->reset(view, projection);

    updateGLBlend(globalAlpha); // update blending flags

    // normalizingAlpha maps a byte from 0-255 to a float from 0-1, premultiplied by currentAlpha.
    float normalizingAlpha = currentAlpha / 255.0f;
    FramebufferStack fboStack(fGrContext->getGpu(), view);
    const void* vertexAttributes = 0;
    float colorVector[4];

    unsigned transparencyCount = 0;
    const unsigned commandSize = fInstructions.commandSize();
    for (unsigned i = 0; i < commandSize; ++i) {

        const DisplayListCommand command = fInstructions[i].command;
        const CommandVariant variant = command.variant();

        // Bump the uniform pointer to skip the uniform count so that uniforms[0] is now the vertex count.
        const unsigned vertexCount = fInstructions[i].vertexCount;
        vertexAttributes = getVertexAttributePointer(i);

        const bool layersOnly = run == OffscreenRun ? !transparencyCount : transparencyCount;
        switch (command.id() | (layersOnly << SkipToLayerShift)) {
        case Clear: {
            const ClearUniforms& uniforms = fInstructions.uniformsAt<ClearUniforms>(i);
            fGlInterface->fFunctions.fClearColor(uniforms.red, uniforms.green, uniforms.blue, uniforms.alpha);
            fGlInterface->fFunctions.fClear(GR_GL_COLOR_BUFFER_BIT);
            break;
        }
        case SetAlpha:
            currentAlpha = globalAlpha * fInstructions.uniformsAt<SetAlphaUniforms>(i).alpha;
            normalizingAlpha = currentAlpha / 255.0f;
            updateGLBlend(currentAlpha);
            break;
        case SetXferMode: {
            const SetXferModeUniforms& uniforms = fInstructions.uniformsAt<SetXferModeUniforms>(i);
            if (fXfermode != uniforms.mode) {
                fXfermode = uniforms.mode;
                blendFunc(fXfermode);
                updateGLBlend(currentAlpha);
            }
            break;
        }
        case DrawOpaqueColor:
            // We can skip blending when Opaque, DefaultVariant, 1.0 Alpha, and SkXfermode::kSrc_Mode.
            // This is unsurprisingly common; thus we disable blend and fall through to DrawColor.
            if (fXfermode == SkXfermode::kSrc_Mode && variant == DefaultVariant && currentAlpha == 1.0f)
                fGlInterface->fFunctions.fDisable(GR_GL_BLEND);

        case DrawColor: {
            const ColorProgram& p = fShaders->loadColor(variant);

            fGlInterface->fFunctions.fUniform1f(p.alpha, currentAlpha);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            updateGLBlend(currentAlpha); // DrawOpaqueColor path - may be flags should be reset for the next command
            break;
        }
        case DrawBlurredGlyph: {
            const BlurredEdgeTextProgram& p = fShaders->loadBlurredEdgeText(variant);
            const DrawBlurredGlyphUniforms& uniforms = fInstructions.uniformsAt<DrawBlurredGlyphUniforms>(i);
            if (!bindTexture(uniforms.texture, SkPaint::kLow_FilterLevel))
                break;

            fGlInterface->fFunctions.fUniform4fv(p.color, 1, ABGR32MultiplyToGlfv(uniforms.color, normalizingAlpha, colorVector));
            fGlInterface->fFunctions.fUniform2f(p.cutoff, fragmentScale * uniforms.insideCutoff, fragmentScale * uniforms.outsideCutoff);
            fGlInterface->fFunctions.fUniform1f(p.amount, uniforms.blurRadius);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawEdgeGlyph: {
            const EdgeTextProgram& p = fShaders->loadEdgeText(variant);
            const DrawEdgeGlyphUniforms& uniforms = fInstructions.uniformsAt<DrawEdgeGlyphUniforms>(i);
            if (!bindTexture(uniforms.texture, SkPaint::kLow_FilterLevel))
                break;

            fGlInterface->fFunctions.fUniform4fv(p.color, 1, ABGR32MultiplyToGlfv(uniforms.color, normalizingAlpha, colorVector));
            fGlInterface->fFunctions.fUniform2f(p.cutoff, fragmentScale * uniforms.insideCutoff, fragmentScale * uniforms.outsideCutoff);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawStrokedEdgeGlyph: {
            const StrokedEdgeTextProgram& p = fShaders->loadStrokedEdgeText(variant);
            const DrawStrokedEdgeGlyphUniforms& uniforms = fInstructions.uniformsAt<DrawStrokedEdgeGlyphUniforms>(i);
            if (!bindTexture(uniforms.texture, SkPaint::kLow_FilterLevel))
                break;

            fGlInterface->fFunctions.fUniform4fv(p.fillColor, 1, ABGR32MultiplyToGlfv(uniforms.color, normalizingAlpha, colorVector));
            fGlInterface->fFunctions.fUniform4fv(p.strokeColor, 1, ABGR32MultiplyToGlfv(uniforms.strokeColor, normalizingAlpha, colorVector));
            fGlInterface->fFunctions.fUniform1f(p.halfStrokeWidth, uniforms.halfStrokeWidth);
            fGlInterface->fFunctions.fUniform2f(p.cutoff, fragmentScale * uniforms.insideCutoff, fragmentScale * uniforms.outsideCutoff);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawBitmapGlyph: {
            const TextProgram& p = fShaders->loadText(variant);
            const DrawBitmapGlyphUniforms& uniforms = fInstructions.uniformsAt<DrawBitmapGlyphUniforms>(i);
            if (!bindTexture(uniforms.texture, SkPaint::kLow_FilterLevel))
                break;

            fGlInterface->fFunctions.fUniform4fv(p.color, 1, ABGR32MultiplyToGlfv(uniforms.color, normalizingAlpha, colorVector));
            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawSkShader: {
            const DrawSkShaderUniforms& uniforms = fInstructions.uniformsAt<DrawSkShaderUniforms>(i);
            const SkShaderProgram& p = fShaders->loadSkShader(variant, uniforms.gradientType);

            if (!uniforms.texture)
                break;

            const GrTextureParams textureParameters(uniforms.tileMode, uniforms.filterLevel == SkPaint::kNone_FilterLevel ? GrTextureParams::kNone_FilterMode : GrTextureParams::kBilerp_FilterMode);
            fGpu->bindTexture(0, textureParameters, static_cast<GrGLTexture*>(uniforms.texture));
            fGlInterface->fFunctions.fUniform1f(p.alpha, currentAlpha);

            if (uniforms.gradientType != SkShader::kNone_GradientType) {
                fGlInterface->fFunctions.fUniformMatrix3fv(p.transform, 1, false, uniforms.localMatrix);
            }

            if (uniforms.gradientType == SkShader::kRadial2_GradientType) {
                const SkShaderTwoPointRadialGradientProgram& ref = reinterpret_cast<const SkShaderTwoPointRadialGradientProgram&>(p);
                fGlInterface->fFunctions.fUniform1f(ref.xOffset, uniforms.xOffset);
                fGlInterface->fFunctions.fUniform1f(ref.a, uniforms.a);
                fGlInterface->fFunctions.fUniform1f(ref.r1Squared, uniforms.r1Squared);
                fGlInterface->fFunctions.fUniform1f(ref.r1TimesDiff, uniforms.r1TimesDiff);
            }

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawArc: {
            const ArcProgram& p = fShaders->loadArc(variant);
            const DrawArcUniforms& uniforms = fInstructions.uniformsAt<DrawArcUniforms>(i);

            fGlInterface->fFunctions.fUniform1f(p.alpha, currentAlpha);
            fGlInterface->fFunctions.fUniform4fv(p.color, 1, ABGR32MultiplyToGlfv(uniforms.color, normalizingAlpha, colorVector));
            fGlInterface->fFunctions.fUniform1f(p.inside, uniforms.inside);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case DrawTexture: {
            const DrawTextureUniforms& uniforms = fInstructions.uniformsAt<DrawTextureUniforms>(i);
            const ImageProgram& p = GrPixelConfigIsAlphaOnly(uniforms.texture->config()) ? fShaders->loadImageAlpha(variant) : fShaders->loadImage(variant);
            if (!bindTexture(uniforms.texture, uniforms.filterLevel))
                break;

            fGlInterface->fFunctions.fUniform1f(p.alpha, currentAlpha);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case BeginTransparencyLayer | SkipToLayer:
        case BeginTransparencyLayer: {
            ++transparencyCount;
            if (run == FinalRun)
                break;

            const BeginTransparencyLayerUniforms& uniforms = fInstructions.uniformsAt<BeginTransparencyLayerUniforms>(i);
            SkMatrix44 offsetMatrix(view);
            offsetMatrix.preTranslate(-uniforms.offset.x(), -uniforms.offset.y(), 0);
            fboStack.push(uniforms.size, offsetMatrix, fGrContext);
            fShaders->reset(offsetMatrix, Identity);
            break;
        }
        case EndTransparencyLayer | SkipToLayer:
        case EndTransparencyLayer: {
            --transparencyCount;
            if (run == FinalRun)
                break;

            const FramebufferStack::RestoreInfo restoreInfo = fboStack.pop();
            fShaders->reset(restoreInfo.view, transparencyCount ? Identity : fDefaultProjection);
            if (i + 1 < commandSize){
                const DrawCurrentTextureUniforms& uniforms = fInstructions.uniformsAt<DrawCurrentTextureUniforms>(i + 1);
                const_cast<DrawCurrentTextureUniforms&>(uniforms).texture = restoreInfo.texture;
            }
            break;
        }
        case DrawCurrentTexture: {
            const DrawCurrentTextureUniforms& uniforms = fInstructions.uniformsAt<DrawCurrentTextureUniforms>(i);
            if (!bindTexture(uniforms.texture, view.isTranslate() ? SkPaint::kNone_FilterLevel : SkPaint::kLow_FilterLevel))
                break;

            if (fXfermode != uniforms.mode) {
                fXfermode = uniforms.mode;
                blendFunc(fXfermode);
                updateGLBlend(currentAlpha);
            }
            // if we have no blur then we just draw the image with the specified opacity, otherwise we blur with currentAlpha
            if (!uniforms.blurAmount) {
                const ImageProgram& p = fShaders->loadImage(variant);
                fGlInterface->fFunctions.fUniform1f(p.alpha, uniforms.opacity * globalAlpha);
                p.setVertexAttribPointers(vertexAttributes, fGlInterface);
                drawTriangles(command.getTriangleMode(), vertexCount);
                break;
            }

            const ImageBlurProgram& p = fShaders->loadImageBlur(variant);
            const float blurSizeTotalInv = 1.0 / (uniforms.blurSizeWidth * uniforms.blurVectorX + uniforms.blurSizeHeight * uniforms.blurVectorY);
            // The following incrx and incry are constant inputs to the fast guassian blur algorithm.
            const float incrx = 1.0 / (std::sqrt(2.0 * M_PI) * uniforms.blurAmount);
            const float incry = std::exp(-0.5 / (uniforms.blurAmount * uniforms.blurAmount));

            fGlInterface->fFunctions.fUniform1f(p.amount, uniforms.blurAmount);
            fGlInterface->fFunctions.fUniform2f(p.blurvector, uniforms.blurVectorX * blurSizeTotalInv, uniforms.blurVectorY * blurSizeTotalInv);
            fGlInterface->fFunctions.fUniform3f(p.incr, incrx, incry, incry * incry);
            fGlInterface->fFunctions.fUniform1f(p.alpha, uniforms.opacity * globalAlpha);

            p.setVertexAttribPointers(vertexAttributes, fGlInterface);
            drawTriangles(command.getTriangleMode(), vertexCount);
            break;
        }
        case NoOp:
        default:
            break;
        }
    }
}


DisplayList::GlobalFlagState::GlobalFlagState(const GrGLInterface* interface, int restoreFlags)
    : fRestoreFlags(restoreFlags)
    , fGlInterface(interface)
{
}

DisplayList::GlobalFlagState::~GlobalFlagState()
{
}

void DisplayList::GlobalFlagState::save()
{
    GrGLenum* flags = GlobalFlagState::flags();
    for (int i = 0; i < NumFlags; ++i) {
        if (fRestoreFlags & (1 << i))
            flagState[i] = fGlInterface->fFunctions.fIsEnabled(flags[i]);
    }

    if (fRestoreFlags & RestoreFrontFace) {
        frontFace = 0;
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_FRONT_FACE, &frontFace);
    }

    if (fRestoreFlags & RestoreBlendFunc) {
        blendSrcRGB = blendDstRGB = blendSrcAlpha = blendDstAlpha = 0;
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_BLEND_SRC_RGB, &blendSrcRGB);
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_BLEND_DST_RGB, &blendDstRGB);
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_BLEND_DST_ALPHA, &blendDstAlpha);
    }

    if (fRestoreFlags & RestoreColorMask) {
        colorMask[0] = colorMask[1] = colorMask[2] = colorMask[3] = 0;
        fGlInterface->fFunctions.fGetIntegerv(GR_GL_COLOR_WRITEMASK, &colorMask[0]);
    }
}

void DisplayList::GlobalFlagState::restore()
{
    GrGLenum* flags = GlobalFlagState::flags();

    for (int i = 0; i < NumFlags; ++i) {
        if (!(fRestoreFlags & (1 << i)))
            continue;

        if (flagState[i])
            fGlInterface->fFunctions.fEnable(flags[i]);
        else
            fGlInterface->fFunctions.fDisable(flags[i]);
    }

    if (fRestoreFlags & RestoreFrontFace)
        fGlInterface->fFunctions.fFrontFace(frontFace);
    if (fRestoreFlags & RestoreBlendFunc)
        fGlInterface->fFunctions.fBlendFuncSeparate(blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha);
    if (fRestoreFlags & RestoreColorMask)
        fGlInterface->fFunctions.fColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
}

void DisplayList::ViewportState::save()
{
    currentViewport[0] = currentViewport[1] = currentViewport[2] = currentViewport[3] = 0;
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_VIEWPORT, &currentViewport[0]);
}

void DisplayList::ViewportState::restore()
{
    fGlInterface->fFunctions.fViewport(currentViewport[0], currentViewport[1], currentViewport[2], currentViewport[3]);
}

void DisplayList::FramebufferState::save()
{
    currentFbo = 0;
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &currentFbo);
}

void DisplayList::FramebufferState::restore()
{
    fGlInterface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, currentFbo);
}

void DisplayList::ScissorBoxState::save()
{
    currentScissorBox[0] = currentScissorBox[1] = currentScissorBox[2] = currentScissorBox[3] = 0;
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_SCISSOR_BOX, &currentScissorBox[0]);
}

void DisplayList::ScissorBoxState::restore()
{
    fGlInterface->fFunctions.fScissor(currentScissorBox[0], currentScissorBox[1], currentScissorBox[2], currentScissorBox[3]);
}

void DisplayList::BufferBindingState::save()
{
    currentArrayBufferBinding = currentElementArrayBufferBinding = 0;
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GrGLint*>(&currentArrayBufferBinding));
    fGlInterface->fFunctions.fGetIntegerv(GR_GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GrGLint*>(&currentElementArrayBufferBinding));
}

void DisplayList::BufferBindingState::restore()
{
    fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, currentArrayBufferBinding);
    fGlInterface->fFunctions.fBindBuffer(GR_GL_ELEMENT_ARRAY_BUFFER, currentElementArrayBufferBinding);
}

void DisplayList::BufferBindingState::resetBufferBinding()
{
    fGlInterface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, 0);
    fGlInterface->fFunctions.fBindBuffer(GR_GL_ELEMENT_ARRAY_BUFFER, 0);
}

} // namespace Gentl
