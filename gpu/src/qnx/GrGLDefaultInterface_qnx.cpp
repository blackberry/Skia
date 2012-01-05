/*
   Copyright 2011 Google Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include "GrGLInterface.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

void GrGLSetDefaultGLInterface() {
    static GrGLInterface gDefaultInterface;
    static bool gDefaultInterfaceInit;
    if (!gDefaultInterfaceInit) {
        gDefaultInterface.fNPOTRenderTargetSupport = 1;
        gDefaultInterface.fMinRenderTargetHeight = 16;
        gDefaultInterface.fMinRenderTargetWidth = 16;

        gDefaultInterface.fActiveTexture = glActiveTexture;
        gDefaultInterface.fAttachShader = glAttachShader;
        gDefaultInterface.fBindAttribLocation = glBindAttribLocation;
        gDefaultInterface.fBindBuffer = glBindBuffer;
        gDefaultInterface.fBindFramebuffer = glBindFramebuffer;
        gDefaultInterface.fBindRenderbuffer = glBindRenderbuffer;
        gDefaultInterface.fBindTexture = glBindTexture;
        gDefaultInterface.fBlendColor = glBlendColor;
        gDefaultInterface.fBlendEquation = glBlendEquation;
        gDefaultInterface.fBlendEquationSeparate = glBlendEquationSeparate;
        gDefaultInterface.fBlendFunc = glBlendFunc;
        gDefaultInterface.fBlendFuncSeparate = glBlendFuncSeparate;
        gDefaultInterface.fBufferData = glBufferData;
        gDefaultInterface.fBufferSubData = glBufferSubData;
        gDefaultInterface.fCheckFramebufferStatus = glCheckFramebufferStatus;
        gDefaultInterface.fClear = glClear;
        gDefaultInterface.fClearColor = glClearColor;
        gDefaultInterface.fClearDepthf = glClearDepthf;
        gDefaultInterface.fClearStencil = glClearStencil;
        gDefaultInterface.fColorMask = glColorMask;
        gDefaultInterface.fCompileShader = glCompileShader;
        gDefaultInterface.fCompressedTexImage2D = glCompressedTexImage2D;
        gDefaultInterface.fCompressedTexSubImage2D = glCompressedTexSubImage2D;
        gDefaultInterface.fCopyTexImage2D = glCopyTexImage2D;
        gDefaultInterface.fCopyTexSubImage2D = glCopyTexSubImage2D;
        gDefaultInterface.fCreateProgram = glCreateProgram;
        gDefaultInterface.fCreateShader = glCreateShader;
        gDefaultInterface.fCullFace = glCullFace;
        gDefaultInterface.fDeleteBuffers = glDeleteBuffers;
        gDefaultInterface.fDeleteFramebuffers = glDeleteFramebuffers;
        gDefaultInterface.fDeleteProgram = glDeleteProgram;
        gDefaultInterface.fDeleteRenderbuffers = glDeleteRenderbuffers;
        gDefaultInterface.fDeleteShader = glDeleteShader;
        gDefaultInterface.fDeleteTextures = glDeleteTextures;
        gDefaultInterface.fDepthFunc = glDepthFunc;
        gDefaultInterface.fDepthMask = glDepthMask;
        gDefaultInterface.fDepthRangef = glDepthRangef;
        gDefaultInterface.fDetachShader = glDetachShader;
        gDefaultInterface.fDisable = glDisable;
        gDefaultInterface.fDisableVertexAttribArray = glDisableVertexAttribArray;
        gDefaultInterface.fDrawArrays = glDrawArrays;
        gDefaultInterface.fDrawElements = glDrawElements;
        gDefaultInterface.fEnable = glEnable;
        gDefaultInterface.fEnableVertexAttribArray = glEnableVertexAttribArray;
        gDefaultInterface.fFinish = glFinish;
        gDefaultInterface.fFlush = glFlush;
        gDefaultInterface.fFramebufferRenderbuffer = glFramebufferRenderbuffer;
        gDefaultInterface.fFramebufferTexture2D = glFramebufferTexture2D;
        gDefaultInterface.fFrontFace = glFrontFace;
        gDefaultInterface.fGenBuffers = glGenBuffers;
        gDefaultInterface.fGenerateMipmap = glGenerateMipmap;
        gDefaultInterface.fGenFramebuffers = glGenFramebuffers;
        gDefaultInterface.fGenRenderbuffers = glGenRenderbuffers;
        gDefaultInterface.fGenTextures = glGenTextures;
        gDefaultInterface.fGetActiveAttrib = glGetActiveAttrib;
        gDefaultInterface.fGetActiveUniform = glGetActiveUniform;
        gDefaultInterface.fGetAttachedShaders = glGetAttachedShaders;
        gDefaultInterface.fGetAttribLocation = glGetAttribLocation;
        gDefaultInterface.fGetBooleanv = glGetBooleanv;
        gDefaultInterface.fGetBufferParameteriv = glGetBufferParameteriv;
        gDefaultInterface.fGetError = glGetError;
        gDefaultInterface.fGetFloatv = glGetFloatv;
        gDefaultInterface.fGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameteriv;
        gDefaultInterface.fGetIntegerv = glGetIntegerv;
        gDefaultInterface.fGetProgramiv = glGetProgramiv;
        gDefaultInterface.fGetProgramInfoLog = glGetProgramInfoLog;
        gDefaultInterface.fGetRenderbufferParameteriv = glGetRenderbufferParameteriv;
        gDefaultInterface.fGetShaderiv = glGetShaderiv;
        gDefaultInterface.fGetShaderInfoLog = glGetShaderInfoLog;
        gDefaultInterface.fGetShaderPrecisionFormat = glGetShaderPrecisionFormat;
        gDefaultInterface.fGetShaderSource = glGetShaderSource;
        gDefaultInterface.fGetString = glGetString;
        gDefaultInterface.fGetTexParameterfv = glGetTexParameterfv;
        gDefaultInterface.fGetTexParameteriv = glGetTexParameteriv;
        gDefaultInterface.fGetUniformfv = glGetUniformfv;
        gDefaultInterface.fGetUniformiv = glGetUniformiv;
        gDefaultInterface.fGetUniformLocation = glGetUniformLocation;
        gDefaultInterface.fGetVertexAttribfv = glGetVertexAttribfv;
        gDefaultInterface.fGetVertexAttribiv = glGetVertexAttribiv;
        gDefaultInterface.fGetVertexAttribPointerv = glGetVertexAttribPointerv;
        gDefaultInterface.fHint = glHint;
        gDefaultInterface.fIsBuffer = glIsBuffer;
        gDefaultInterface.fIsEnabled = glIsEnabled;
        gDefaultInterface.fIsFramebuffer = glIsFramebuffer;
        gDefaultInterface.fIsProgram = glIsProgram;
        gDefaultInterface.fIsRenderbuffer = glIsRenderbuffer;
        gDefaultInterface.fIsShader = glIsShader;
        gDefaultInterface.fIsTexture = glIsTexture;
        gDefaultInterface.fLineWidth = glLineWidth;
        gDefaultInterface.fLinkProgram = glLinkProgram;
        gDefaultInterface.fPixelStorei = glPixelStorei;
        gDefaultInterface.fPolygonOffset = glPolygonOffset;
        gDefaultInterface.fReadPixels = glReadPixels;
        gDefaultInterface.fReleaseShaderCompiler = glReleaseShaderCompiler;
        gDefaultInterface.fRenderbufferStorage = glRenderbufferStorage;
        gDefaultInterface.fSampleCoverage = glSampleCoverage;
        gDefaultInterface.fScissor = glScissor;
        gDefaultInterface.fShaderBinary = glShaderBinary;
        gDefaultInterface.fShaderSource = glShaderSource;
        gDefaultInterface.fStencilFunc = glStencilFunc;
        gDefaultInterface.fStencilFuncSeparate = glStencilFuncSeparate;
        gDefaultInterface.fStencilMask = glStencilMask;
        gDefaultInterface.fStencilMaskSeparate = glStencilMaskSeparate;
        gDefaultInterface.fStencilOp = glStencilOp;
        gDefaultInterface.fStencilOpSeparate = glStencilOpSeparate;
        // QNX uses GLenum for internalFormat param (non-standard)
        // amounts to int vs. uint.
        gDefaultInterface.fTexImage2D = (GrGLTexImage2DProc)glTexImage2D;
        gDefaultInterface.fTexParameterf = glTexParameterf;
        gDefaultInterface.fTexParameterfv = glTexParameterfv;
        gDefaultInterface.fTexParameteri = glTexParameteri;
        gDefaultInterface.fTexParameteriv = glTexParameteriv;
        gDefaultInterface.fTexSubImage2D = glTexSubImage2D;
        gDefaultInterface.fUniform1f = glUniform1f;
        gDefaultInterface.fUniform1fv = glUniform1fv;
        gDefaultInterface.fUniform1i = glUniform1i;
        gDefaultInterface.fUniform1iv = glUniform1iv;
        gDefaultInterface.fUniform2f = glUniform2f;
        gDefaultInterface.fUniform2fv = glUniform2fv;
        gDefaultInterface.fUniform2i = glUniform2i;
        gDefaultInterface.fUniform2iv = glUniform2iv;
        gDefaultInterface.fUniform3f = glUniform3f;
        gDefaultInterface.fUniform3fv = glUniform3fv;
        gDefaultInterface.fUniform3i = glUniform3i;
        gDefaultInterface.fUniform3iv = glUniform3iv;
        gDefaultInterface.fUniform4f = glUniform4f;
        gDefaultInterface.fUniform4fv = glUniform4fv;
        gDefaultInterface.fUniform4i = glUniform4i;
        gDefaultInterface.fUniform4iv = glUniform4iv;
        gDefaultInterface.fUniformMatrix2fv = glUniformMatrix2fv;
        gDefaultInterface.fUniformMatrix3fv = glUniformMatrix3fv;
        gDefaultInterface.fUniformMatrix4fv = glUniformMatrix4fv;
        gDefaultInterface.fUseProgram = glUseProgram;
        gDefaultInterface.fValidateProgram = glValidateProgram;
        gDefaultInterface.fVertexAttrib1f = glVertexAttrib1f;
        gDefaultInterface.fVertexAttrib1fv = glVertexAttrib1fv;
        gDefaultInterface.fVertexAttrib2f = glVertexAttrib2f;
        gDefaultInterface.fVertexAttrib2fv = glVertexAttrib2fv;
        gDefaultInterface.fVertexAttrib3f = glVertexAttrib3f;
        gDefaultInterface.fVertexAttrib3fv = glVertexAttrib3fv;
        gDefaultInterface.fVertexAttrib4f = glVertexAttrib4f;
        gDefaultInterface.fVertexAttrib4fv = glVertexAttrib4fv;
        gDefaultInterface.fVertexAttribPointer = glVertexAttribPointer;
        gDefaultInterface.fViewport = glViewport;

        // MapBuffer Extension
        gDefaultInterface.fMapBuffer = (PFNGLMAPBUFFEROESPROC)eglGetProcAddress("glMapBufferOES");
        gDefaultInterface.fUnmapBuffer = (PFNGLUNMAPBUFFEROESPROC)eglGetProcAddress("glUnmapBufferOES");

        // IMG_multisample
        gDefaultInterface.fRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC)eglGetProcAddress("glRenderbufferStorageMultisampleIMG");
        gDefaultInterface.fFramebufferTexture2DMultisample = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC)eglGetProcAddress("glFramebufferTexture2DMultisampleIMG");

        gDefaultInterface.fEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

        gDefaultInterface.fBindingsExported = kES2_GrGLBinding;
        gDefaultInterfaceInit = true;
    }
    GrGLSetGLInterface(&gDefaultInterface);
}
