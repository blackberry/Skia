// Modified from chromium/src/webkit/glue/gl_bindings_skia_cmd_buffer.cc

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gl/GrGLExtensions.h"
#include "gl/GrGLInterface.h"
#include "gl/GrGLUtil.h"

#include <GLES2/gl2.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>

const GrGLInterface* GrGLCreateNativeInterface() {
    static SkAutoTUnref<GrGLInterface> glInterface;
    if (!glInterface.get()) {
        const char* verStr = reinterpret_cast<const char*>(glGetString(GR_GL_VERSION));
        GrGLVersion version = GrGLGetVersionFromString(verStr);
        GrGLExtensions extensions;
        if (!extensions.init(kES2_GrGLBinding, glGetString, NULL, glGetIntegerv)) {
            return NULL;
        }
        GrGLInterface* interface = new GrGLInterface;
        glInterface.reset(interface);
        interface->fStandard = kES2_GrGLBinding;
        interface->fFunctions.fActiveTexture = glActiveTexture;
        interface->fFunctions.fAttachShader = glAttachShader;
        interface->fFunctions.fDetachShader = glDetachShader;
        interface->fFunctions.fBindAttribLocation = glBindAttribLocation;
        interface->fFunctions.fBindBuffer = glBindBuffer;
        interface->fFunctions.fBindTexture = glBindTexture;
        interface->fFunctions.fBindVertexArray = glBindVertexArrayOES;
        interface->fFunctions.fBlendColor = glBlendColor;
        interface->fFunctions.fBlendFunc = glBlendFunc;
        interface->fFunctions.fBlendFuncSeparate = glBlendFuncSeparate;
        interface->fFunctions.fBufferData = glBufferData;
        interface->fFunctions.fBufferSubData = glBufferSubData;
        interface->fFunctions.fClear = glClear;
        interface->fFunctions.fClearColor = glClearColor;
        interface->fFunctions.fClearStencil = glClearStencil;
        interface->fFunctions.fColorMask = glColorMask;
        interface->fFunctions.fCompileShader = glCompileShader;
        interface->fFunctions.fCompressedTexImage2D = glCompressedTexImage2D;
        interface->fFunctions.fCopyTexSubImage2D = glCopyTexSubImage2D;
        interface->fFunctions.fCreateProgram = glCreateProgram;
        interface->fFunctions.fCreateShader = glCreateShader;
        interface->fFunctions.fCullFace = glCullFace;
        interface->fFunctions.fDeleteBuffers = glDeleteBuffers;
        interface->fFunctions.fDeleteProgram = glDeleteProgram;
        interface->fFunctions.fDeleteShader = glDeleteShader;
        interface->fFunctions.fDeleteTextures = glDeleteTextures;
        interface->fFunctions.fDeleteVertexArrays = glDeleteVertexArraysOES;
        interface->fFunctions.fDepthMask = glDepthMask;
        interface->fFunctions.fDisable = glDisable;
        interface->fFunctions.fDisableVertexAttribArray = glDisableVertexAttribArray;
        interface->fFunctions.fDrawArrays = glDrawArrays;
        interface->fFunctions.fDrawElements = glDrawElements;
        interface->fFunctions.fIsEnabled = glIsEnabled;
        interface->fFunctions.fEnable = glEnable;
        interface->fFunctions.fEnableVertexAttribArray = glEnableVertexAttribArray;
        interface->fFunctions.fFinish = glFinish;
        interface->fFunctions.fFlush = glFlush;
        interface->fFunctions.fFrontFace = glFrontFace;
        interface->fFunctions.fGenBuffers = glGenBuffers;
        interface->fFunctions.fGenerateMipmap = glGenerateMipmap;
        interface->fFunctions.fGenTextures = glGenTextures;
        interface->fFunctions.fGenVertexArrays = glGenVertexArraysOES;
        interface->fFunctions.fGetBufferParameteriv = glGetBufferParameteriv;
        interface->fFunctions.fGetError = glGetError;
        interface->fFunctions.fGetIntegerv = glGetIntegerv;
        interface->fFunctions.fGetProgramInfoLog = glGetProgramInfoLog;
        interface->fFunctions.fGetProgramiv = glGetProgramiv;
        interface->fFunctions.fGetShaderInfoLog = glGetShaderInfoLog;
        interface->fFunctions.fGetShaderiv = glGetShaderiv;
        interface->fFunctions.fGetString = glGetString;
#if GL_ES_VERSION_30
    interface->fFunctions.fGetStringi = glGetStringi;
#else
    interface->fFunctions.fGetStringi = (GrGLGetStringiProc) eglGetProcAddress("glGetStringi");
#endif
        interface->fFunctions.fGetUniformLocation = glGetUniformLocation;
        interface->fFunctions.fGetAttribLocation = glGetAttribLocation;
        interface->fFunctions.fLineWidth = glLineWidth;
        interface->fFunctions.fLinkProgram = glLinkProgram;
        interface->fFunctions.fPixelStorei = glPixelStorei;
        interface->fFunctions.fReadPixels = glReadPixels;
        interface->fFunctions.fScissor = glScissor;
#if GR_GL_USE_NEW_SHADER_SOURCE_SIGNATURE
        interface->fFunctions.fShaderSource = (GrGLShaderSourceProc) glShaderSource;
#else
        interface->fFunctions.fShaderSource = glShaderSource;
#endif
        interface->fFunctions.fGetShaderSource = glGetShaderSource;
        interface->fFunctions.fStencilFunc = glStencilFunc;
        interface->fFunctions.fStencilFuncSeparate = glStencilFuncSeparate;
        interface->fFunctions.fStencilMask = glStencilMask;
        interface->fFunctions.fStencilMaskSeparate = glStencilMaskSeparate;
        interface->fFunctions.fStencilOp = glStencilOp;
        interface->fFunctions.fStencilOpSeparate = glStencilOpSeparate;
        interface->fFunctions.fTexImage2D = glTexImage2D;
        interface->fFunctions.fTexParameteri = glTexParameteri;
        interface->fFunctions.fTexParameteriv = glTexParameteriv;
        interface->fFunctions.fTexSubImage2D = glTexSubImage2D;

        if (version >= GR_GL_VER(3,0)) {
#if GL_ES_VERSION_3_0
        interface->fFunctions.fTexStorage2D = glTexStorage2D;
#else
        interface->fFunctions.fTexStorage2D = (GrGLTexStorage2DProc) eglGetProcAddress("glTexStorage2D");
#endif
    } else {
#if 0//GL_EXT_texture_storage
        interface->fFunctions.fTexStorage2D = glTexStorage2DEXT;
#else
        interface->fFunctions.fTexStorage2D = (GrGLTexStorage2DProc) eglGetProcAddress("glTexStorage2DEXT");
#endif
        }
#if GL_EXT_discard_framebuffer
        interface->fFunctions.fDiscardFramebuffer = glDiscardFramebufferEXT;
#endif
        interface->fFunctions.fUniform1f = glUniform1f;
        interface->fFunctions.fUniform1i = glUniform1i;
        interface->fFunctions.fUniform1fv = glUniform1fv;
        interface->fFunctions.fUniform1iv = glUniform1iv;
        interface->fFunctions.fUniform2f = glUniform2f;
        interface->fFunctions.fUniform2i = glUniform2i;
        interface->fFunctions.fUniform2fv = glUniform2fv;
        interface->fFunctions.fUniform2iv = glUniform2iv;
        interface->fFunctions.fUniform3f = glUniform3f;
        interface->fFunctions.fUniform3i = glUniform3i;
        interface->fFunctions.fUniform3fv = glUniform3fv;
        interface->fFunctions.fUniform3iv = glUniform3iv;
        interface->fFunctions.fUniform4f = glUniform4f;
        interface->fFunctions.fUniform4i = glUniform4i;
        interface->fFunctions.fUniform4fv = glUniform4fv;
        interface->fFunctions.fUniform4iv = glUniform4iv;
        interface->fFunctions.fUniformMatrix2fv = glUniformMatrix2fv;
        interface->fFunctions.fUniformMatrix3fv = glUniformMatrix3fv;
        interface->fFunctions.fUniformMatrix4fv = glUniformMatrix4fv;
        interface->fFunctions.fUseProgram = glUseProgram;
        interface->fFunctions.fVertexAttrib4fv = glVertexAttrib4fv;
        interface->fFunctions.fVertexAttribPointer = glVertexAttribPointer;
        interface->fFunctions.fViewport = glViewport;
        interface->fFunctions.fBindFramebuffer = glBindFramebuffer;
        interface->fFunctions.fBindRenderbuffer = glBindRenderbuffer;
        interface->fFunctions.fCheckFramebufferStatus = glCheckFramebufferStatus;
        interface->fFunctions.fDeleteFramebuffers = glDeleteFramebuffers;
        interface->fFunctions.fDeleteRenderbuffers = glDeleteRenderbuffers;
        interface->fFunctions.fFramebufferRenderbuffer = glFramebufferRenderbuffer;
        interface->fFunctions.fFramebufferTexture2D = glFramebufferTexture2D;
        if (extensions.has("GL_EXT_multisampled_render_to_texture")) {
#if 0//GL_EXT_multisampled_render_to_texture
            interface->fFunctions.fFramebufferTexture2DMultisample = glFramebufferTexture2DMultisampleEXT;
            interface->fFunctions.fRenderbufferStorageMultisample = glRenderbufferStorageMultisampleEXT;
#else
            interface->fFunctions.fFramebufferTexture2DMultisample = (GrGLFramebufferTexture2DMultisampleProc) eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");
            interface->fFunctions.fRenderbufferStorageMultisample = (GrGLRenderbufferStorageMultisampleProc) eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
#endif
        } else if (extensions.has("GL_IMG_multisampled_render_to_texture")) {
#if 0//GL_IMG_multisampled_render_to_texture
            interface->fFunctions.fFramebufferTexture2DMultisample = glFramebufferTexture2DMultisampleIMG;
            interface->fFunctions.fRenderbufferStorageMultisample = glRenderbufferStorageMultisampleIMG;
#else
            interface->fFunctions.fRenderbufferStorageMultisampleES2EXT = (GrGLRenderbufferStorageMultisampleProc) eglGetProcAddress("glRenderbufferStorageMultisampleIMG");
            interface->fFunctions.fFramebufferTexture2DMultisample = (GrGLFramebufferTexture2DMultisampleProc) eglGetProcAddress("glFramebufferTexture2DMultisampleIMG");
            interface->fFunctions.fRenderbufferStorageMultisample = (GrGLRenderbufferStorageMultisampleProc) eglGetProcAddress("glRenderbufferStorageMultisampleIMG");
#endif
        } else if (extensions.has("GL_IMG_multisampled_render_to_texture")) {
#if 0//GL_IMG_multisampled_render_to_texture
            interface->fFunctions.fFramebufferTexture2DMultisample = glFramebufferTexture2DMultisampleIMG;
            interface->fFunctions.fRenderbufferStorageMultisampleES2EXT = glRenderbufferStorageMultisampleIMG;
#else
            interface->fFunctions.fFramebufferTexture2DMultisample = (GrGLFramebufferTexture2DMultisampleProc) eglGetProcAddress("glFramebufferTexture2DMultisampleIMG");
            interface->fFunctions.fRenderbufferStorageMultisampleES2EXT = (GrGLRenderbufferStorageMultisampleProc) eglGetProcAddress("glRenderbufferStorageMultisampleIMG");
#endif
        }
        interface->fFunctions.fGenFramebuffers = glGenFramebuffers;
        interface->fFunctions.fGenRenderbuffers = glGenRenderbuffers;
        interface->fFunctions.fGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameteriv;
        interface->fFunctions.fGetRenderbufferParameteriv = glGetRenderbufferParameteriv;
        interface->fFunctions.fRenderbufferStorage = glRenderbufferStorage;
#if GL_OES_mapbuffer
        interface->fFunctions.fMapBuffer = glMapBufferOES;
        interface->fFunctions.fUnmapBuffer = glUnmapBufferOES;
#else
        interface->fFunctions.fMapBuffer = (GrGLMapBufferProc) eglGetProcAddress("glMapBufferOES");
        interface->fFunctions.fUnmapBuffer = (GrGLUnmapBufferProc) eglGetProcAddress("glUnmapBufferOES");
#endif
        interface->fFunctions.fGetProgramBinary = glGetProgramBinaryOES;
        interface->fFunctions.fProgramBinary = glProgramBinaryOES;
    }
    glInterface.get()->ref();
    return glInterface.get();
}
