/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlDisplayList_h
#define GentlDisplayList_h

#include "InstructionSet.h"
#include "Shaders.h"
#include "SkTypes.h"

struct GrGLInterface;
class GrContext;
class GrGLVertexBuffer;
class GrGpuGL;

namespace Gentl {

enum BufferBacking {
    NoBacking,        // Never pixel backed. Always rendered as a series of commands.
    OnDemandBacking,  // Render without pixel backing if possible, but convertable to AlphaBacking if required.
    AlphaBacking,     // Always rendered to a versatile pixel buffer which can in turn be blended.
};

class DisplayList : public SkNoncopyable {
public:
    DisplayList(GrContext*, const SkISize&, BufferBacking, Projection);
    ~DisplayList();

    SkMatrix44 getDefaultProjection();

    // true if all commands have been turned into pixels
    // (disregarding graphics contexts that have not yet been flushed)
    bool isEmpty() const;

    BufferBacking specifiedBacking() const { return fSpecifiedBacking; }
    BufferBacking currentBacking() const { return fCurrentBacking; }
    void setCurrentBacking(BufferBacking backing) { fCurrentBacking = backing; }

    void clear();
    void draw(const SkIRect& dstRect, const SkIRect& srcRect, unsigned char globalAlpha = 255);
    void draw(const SkMatrix44& view, unsigned char globalAlpha);
    bool updateBackingSurface(const SkMatrix44& view);
    void readPixels(const SkIRect& rect, unsigned char* buffer, bool unmultiply);
    const SkISize& size() const { return fSize; }
    InstructionSet& instructions() { return fInstructions; }

private:
    struct GlobalFlagState : public SkNoncopyable {
        static GrGLenum* flags()
        {
            static GrGLenum flags[] = {
                GR_GL_DEPTH_TEST,
                GR_GL_STENCIL_TEST,
                GR_GL_SCISSOR_TEST,
                GR_GL_CULL_FACE,
                GR_GL_BLEND
            };
            return flags;
        }
        enum {
            NumFlags = 5,
        };


        enum RestoreFlags {
            RestoreDepthTest = 1 << 0,
            RestoreStencilTest = 1 << 1,
            RestoreScissorTest = 1 << 2,
            RestoreCullFace = 1 << 3,
            RestoreBlend = 1 << 4,

            RestoreFrontFace = 1 << 5,
            RestoreBlendFunc = 1 << 6,
            RestoreColorMask = 1 << 7,

            RestoreAll = 0xFFFFFFFF,

            DontRestoreBlendFunc = RestoreAll & ~RestoreBlendFunc,
        };
        GlobalFlagState(const GrGLInterface* glInterface, int restoreFlags = RestoreAll);
        ~GlobalFlagState();

        void save();
        void restore();

        GrGLboolean flagState[NumFlags];
        GrGLint frontFace;
        GrGLint blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;
        GrGLint colorMask[4];

    private:
        int fRestoreFlags;
        const GrGLInterface* fGlInterface;
    };

    class GlobalFlagStateSaver {
    public:
        GlobalFlagStateSaver(const GrGLInterface* interface, int restoreFlags)
            : fGlobalFlagState(interface, restoreFlags)
        {
            fGlobalFlagState.save();
        }

        ~GlobalFlagStateSaver() { fGlobalFlagState.restore(); }

    private:
        GlobalFlagState fGlobalFlagState;
    };

    struct FramebufferState {
        FramebufferState(const GrGLInterface* interface) : fGlInterface(interface) { };
        void save();
        void restore();

        const GrGLInterface* fGlInterface;
        GrGLint currentFbo;
    };

    class FramebufferSaver {
    public:
        FramebufferSaver(const GrGLInterface* interface) : fState(interface) { fState.save(); }
        ~FramebufferSaver() { fState.restore(); }

    private:
        FramebufferState fState;
    };

    struct ViewportState {
        ViewportState(const GrGLInterface* interface) : fGlInterface(interface){};
        void save();
        void restore();

        GrGLint currentViewport[4];
        const GrGLInterface* fGlInterface;
    };
    class ViewportSaver {
    public:
        ViewportSaver(const GrGLInterface* interface) : fViewportState(interface) { fViewportState.save(); }
        ~ViewportSaver() { fViewportState.restore(); }

    private:
        ViewportState fViewportState;
    };

    struct ScissorBoxState {
        ScissorBoxState(const GrGLInterface* interface) : fGlInterface(interface) {};
        void save();
        void restore();

        GrGLint currentScissorBox[4];
        const GrGLInterface* fGlInterface;
    };
    class ScissorBoxSaver {
    public:
        ScissorBoxSaver(const GrGLInterface* interface) : fScissorState(interface) { fScissorState.save(); }
        ~ScissorBoxSaver() { fScissorState.restore(); }

    private:
        ScissorBoxState fScissorState;
    };

    struct BufferBindingState {
    public:
        BufferBindingState(const GrGLInterface* interface) : fGlInterface(interface) {};
        void save();
        void restore();

        void resetBufferBinding();

    private:
        GrGLuint currentArrayBufferBinding;
        GrGLuint currentElementArrayBufferBinding;
        const GrGLInterface* fGlInterface;
    };

    class BufferBindingSaver {
    public:
        BufferBindingSaver(const GrGLInterface* interface) : fBufferBindingState(interface) { fBufferBindingState.save(); }
        ~BufferBindingSaver() { fBufferBindingState.restore(); }

        void resetBufferBinding() { fBufferBindingState.resetBufferBinding(); }

    private:
        BufferBindingState fBufferBindingState;
    };

private:
    void blendFunc(SkXfermode::Mode mode);
    void drawTriangles(GrGLint triangleMode, unsigned numElements);

    enum InstructionRun {
        OffscreenRun,
        FinalRun,
    };

    void drawDisplayList(const SkMatrix44& view, Projection projection, unsigned char globalAlpha);
    void instructionLoop(const SkMatrix44& view, Projection projection, unsigned char inGlobalAlpha, const float fragmentScale, InstructionRun runType);
    void updateGLBlend(float currentAlpha) const;
    const void* getVertexAttributePointer(unsigned index) const;
    inline bool bindTexture(GrTexture*, SkPaint::FilterLevel);
    void bindBackingVertexBuffer();

private:
    InstructionSet fInstructions;
    GrTexture* fBackingImage;

    SkXfermode::Mode fXfermode;

    const BufferBacking fSpecifiedBacking;
    BufferBacking fCurrentBacking;

    enum VertexAttributeStorage { ApplicationMemory, VertexBuffer };
    const VertexAttributeStorage fVertexAttributeStorage;

    GrGLVertexBuffer* fVbo;
    GrGLVertexBuffer* fBackingVbo;
    unsigned fLastVboSize;
    SkISize fSize;
    Shaders* fShaders;
    GrContext* fGrContext;
    GrGpuGL* fGpu;
    const GrGLInterface* fGlInterface;
    Projection fDefaultProjection;
};

} // namespace Gentl

#endif // GentlDisplayList_h
