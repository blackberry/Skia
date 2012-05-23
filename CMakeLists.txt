cmake_minimum_required(VERSION 2.6)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "-fstack-protector -fno-strict-aliasing -fno-rtti -fPIC -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-fstack-protector -fno-strict-aliasing -fPIC -fvisibility=hidden ${CMAKE_C_FLAGS}")
endif ()

if (TARGETING_PLAYBOOK)
    set(CMAKE_CXX_FLAGS "-mtune=cortex-a9 -mfpu=vfpv3 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-mtune=cortex-a9 -mfpu=vfpv3 ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-mthumb -mthumb-interwork ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-mthumb -mthumb-interwork ${CMAKE_C_FLAGS}")
endif ()

add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-D_LARGEFILE64_SOURCE)

add_subdirectory(third_party/harfbuzz)

set(LIBNAME grskia)

set(${LIBNAME}_publicheaders
    include/core/Sk64.h
    include/core/SkAdvancedTypefaceMetrics.h
    include/core/SkAutoKern.h
    include/core/SkBitmap.h
    include/core/SkBlitRow.h
    include/core/SkBlitter.h
    include/core/SkBounder.h
    include/core/SkBuffer.h
    include/core/SkCanvas.h
    include/core/SkChunkAlloc.h
    include/core/SkClampRange.h
    include/core/SkClipStack.h
    include/core/SkColor.h
    include/core/SkColorFilter.h
    include/core/SkColorPriv.h
    include/core/SkColorShader.h
    include/core/SkComposeShader.h
    include/core/SkData.h
    include/core/SkDeque.h
    include/core/SkDescriptor.h
    include/core/SkDevice.h
    include/core/SkDither.h
    include/core/SkDraw.h
    include/core/SkDrawFilter.h
    include/core/SkDrawLooper.h
    include/core/SkEndian.h
    include/core/SkFDot6.h
    include/core/SkFixed.h
    include/core/SkFlattenable.h
    include/core/SkFloatBits.h
    include/core/SkFloatingPoint.h
    include/core/SkFontHost.h
    include/core/SkGeometry.h
    include/core/SkGraphics.h
    include/core/SkImageFilter.h
    include/core/SkMallocPixelRef.h
    include/core/SkMask.h
    include/core/SkMaskFilter.h
    include/core/SkMath.h
    include/core/SkMatrix.h
    include/core/SkMetaData.h
    include/core/SkMMapStream.h
    include/core/SkOSFile.h
    include/core/SkPackBits.h
    include/core/SkPaint.h
    include/core/SkPath.h
    include/core/SkPathEffect.h
    include/core/SkPathMeasure.h
    include/core/SkPerspIter.h
    include/core/SkPicture.h
    include/core/SkPixelRef.h
    include/core/SkPoint.h
    include/core/SkPostConfig.h
    include/core/SkPreConfig.h
    include/core/SkPtrRecorder.h
    include/core/SkRandom.h
    include/core/SkRasterizer.h
    include/core/SkReader32.h
    include/core/SkRect.h
    include/core/SkRefCnt.h
    include/core/SkRefDict.h
    include/core/SkRegion.h
    include/core/SkScalar.h
    include/core/SkScalarCompare.h
    include/core/SkScalerContext.h
    include/core/SkScan.h
    include/core/SkShader.h
    include/core/SkSize.h
    include/core/SkStream.h
    include/core/SkString.h
    include/core/SkStroke.h
    include/core/SkTArray.h
    include/core/SkTDArray.h
    include/core/SkTDStack.h
    include/core/SkTDict.h
    include/core/SkTRegistry.h
    include/core/SkTScopedPtr.h
    include/core/SkTSearch.h
    include/core/SkTemplates.h
    include/core/SkThread.h
    include/core/SkThread_platform.h
    include/core/SkTime.h
    include/core/SkTLazy.h
    include/core/SkTrace.h
    include/core/SkTypeface.h
    include/core/SkTypes.h
    include/core/SkUnPreMultiply.h
    include/core/SkUnitMapper.h
    include/core/SkUtils.h
    include/core/SkWriter32.h
    include/core/SkXfermode.h

    include/config/SkUserConfig.h

    include/images/SkFlipPixelRef.h
    include/images/SkImageDecoder.h
    include/images/SkImageEncoder.h
    include/images/SkImageRef.h
    include/images/SkImageRef_GlobalPool.h
    include/images/SkJpegUtility.h
    include/images/SkMovie.h
    include/images/SkPageFlipper.h

    include/ports/SkHarfBuzzFont.h

    include/utils/SkBoundaryPatch.h
    include/utils/SkCamera.h
    include/utils/SkCubicInterval.h
    include/utils/SkCullPoints.h
    include/utils/SkDeferredCanvas.h
    include/utils/SkDumpCanvas.h
    include/utils/SkInterpolator.h
    include/utils/SkLayer.h
    include/utils/SkMatrix44.h
    include/utils/SkMeshUtils.h
    include/utils/SkNinePatch.h
    include/utils/SkNWayCanvas.h
    include/utils/SkParse.h
    include/utils/SkParsePaint.h
    include/utils/SkParsePath.h
    include/utils/SkProxyCanvas.h
    include/utils/SkSfntUtils.h
    include/utils/SkTextBox.h
    include/utils/SkUnitMappers.h
    include/utils/SkWGL.h

    include/effects/Sk1DPathEffect.h
    include/effects/Sk2DPathEffect.h
    include/effects/SkAvoidXfermode.h
    include/effects/SkArithmeticMode.h
    include/effects/SkBlurDrawLooper.h
    include/effects/SkBlurImageFilter.h
    include/effects/SkBlurMaskFilter.h
    include/effects/SkColorMatrix.h
    include/effects/SkColorMatrixFilter.h
    include/effects/SkCornerPathEffect.h
    include/effects/SkDashPathEffect.h
    include/effects/SkDiscretePathEffect.h
    include/effects/SkDrawExtraPathEffect.h
    include/effects/SkEffects.h
    include/effects/SkEmbossMaskFilter.h
    include/effects/SkGradientShader.h
    include/effects/SkGroupShape.h
    include/effects/SkKernel33MaskFilter.h
    include/effects/SkLayerDrawLooper.h
    include/effects/SkLayerRasterizer.h
    include/effects/SkPaintFlagsDrawFilter.h
    include/effects/SkPixelXorXfermode.h
    include/effects/SkPorterDuff.h
    include/effects/SkRectShape.h
    include/effects/SkTableColorFilter.h
    include/effects/SkTableMaskFilter.h
    include/effects/SkTransparentShader.h

    include/config/sk_stdint.h
    include/config/SkUserConfig.h

    include/gpu/SkGpuCanvas.h
    include/gpu/SkGpuDevice.h
    include/gpu/SkGr.h
    include/gpu/SkGrTexturePixelRef.h

    include/gpu/gl/SkGLContext.h
    include/gpu/gl/SkMesaGLContext.h
    include/gpu/gl/SkNativeGLContext.h
    include/gpu/gl/SkNullGLContext.h

    include/gpu/GrClip.h
    include/gpu/GrClipIterator.h
    include/gpu/GrColor.h
    include/gpu/GrConfig.h
    include/gpu/GrContext.h
    include/gpu/GrFontScaler.h
    include/gpu/GrGlyph.h
    include/gpu/GrInstanceCounter.h
    include/gpu/GrKey.h
    include/gpu/GrMatrix.h
    include/gpu/GrNoncopyable.h
    include/gpu/GrPaint.h
    include/gpu/GrPath.h
    include/gpu/GrPoint.h
    include/gpu/GrRect.h
    include/gpu/GrRefCnt.h
    include/gpu/GrRenderTarget.h
    include/gpu/GrResource.h
    include/gpu/GrSamplerState.h
    include/gpu/GrScalar.h
    include/gpu/GrTemplates.h
    include/gpu/GrTextContext.h
    include/gpu/GrTexture.h
    include/gpu/GrTypes.h
    include/gpu/GrUserConfig.h

    include/gpu/gl/GrGLConfig.h
    include/gpu/gl/GrGLConfig_chrome.h
    include/gpu/gl/GrGLDefines.h
    include/gpu/gl/GrGLInterface.h

    include/utils/SkBoundaryPatch.h
    include/utils/SkCamera.h
    include/utils/SkCubicInterval.h
    include/utils/SkCullPoints.h
    include/utils/SkDeferredCanvas.h
    include/utils/SkDumpCanvas.h
    include/utils/SkInterpolator.h
    include/utils/SkLayer.h
    include/utils/SkMatrix44.h
    include/utils/SkMeshUtils.h
    include/utils/SkNinePatch.h
    include/utils/SkNWayCanvas.h
    include/utils/SkParse.h
    include/utils/SkParsePaint.h
    include/utils/SkParsePath.h
    include/utils/SkProxyCanvas.h
    include/utils/SkSfntUtils.h
    include/utils/SkTextBox.h
    include/utils/SkUnitMappers.h
    include/utils/SkWGL.h
)


set(${LIBNAME}_src_core
    src/core/ARGB32_Clamp_Bilinear_BitmapShader.h
    src/core/Sk64.cpp
    src/core/SkAAClip.cpp
    src/core/SkAdvancedTypefaceMetrics.cpp
    src/core/SkAlphaRuns.cpp
    src/core/SkAntiRun.h
    src/core/SkBitmap.cpp
    src/core/SkBitmapProcShader.cpp
    src/core/SkBitmapProcShader.h
    src/core/SkBitmapProcState.cpp
    src/core/SkBitmapProcState.h
    src/core/SkBitmapProcState_matrix.h
    src/core/SkBitmapProcState_matrixProcs.cpp
    src/core/SkBitmapProcState_sample.h
    src/core/SkBitmapSampler.cpp
    src/core/SkBitmapSampler.h
    src/core/SkBitmapSamplerTemplate.h
    src/core/SkBitmapShader16BilerpTemplate.h
    src/core/SkBitmapShaderTemplate.h
    src/core/SkBitmap_scroll.cpp
    src/core/SkBlitBWMaskTemplate.h
    src/core/SkBlitMask_D32.cpp
    src/core/SkBlitRow_D16.cpp
    src/core/SkBlitRow_D32.cpp
    src/core/SkBlitRow_D4444.cpp
    src/core/SkBlitter.cpp
    src/core/SkBlitter_4444.cpp
    src/core/SkBlitter_A1.cpp
    src/core/SkBlitter_A8.cpp
    src/core/SkBlitter_ARGB32.cpp
    src/core/SkBlitter_RGB16.cpp
    src/core/SkBlitter_Sprite.cpp
    src/core/SkBuffer.cpp
    src/core/SkCanvas.cpp
    src/core/SkChunkAlloc.cpp
    src/core/SkClampRange.cpp
    src/core/SkClipStack.cpp
    src/core/SkColor.cpp
    src/core/SkColorFilter.cpp
    src/core/SkColorTable.cpp
    src/core/SkComposeShader.cpp
    src/core/SkConcaveToTriangles.cpp
    src/core/SkConcaveToTriangles.h
    src/core/SkConfig8888.cpp
    src/core/SkConfig8888.h
    src/core/SkCordic.cpp
    src/core/SkCordic.h
    src/core/SkCoreBlitters.h
    src/core/SkCubicClipper.cpp
    src/core/SkCubicClipper.h
    src/core/SkData.cpp
    src/core/SkDebug.cpp
    src/core/SkDeque.cpp
    src/core/SkDevice.cpp
    src/core/SkDither.cpp
    src/core/SkDraw.cpp
    src/core/SkDrawProcs.h
    src/core/SkEdgeBuilder.cpp
    src/core/SkEdgeClipper.cpp
    src/core/SkEdge.cpp
    src/core/SkEdge.h
    src/core/SkFP.h
    src/core/SkFilterProc.cpp
    src/core/SkFilterProc.h
    src/core/SkFlattenable.cpp
    src/core/SkFloat.cpp
    src/core/SkFloat.h
    src/core/SkFloatBits.cpp
    src/core/SkFontHost.cpp
    src/core/SkGeometry.cpp
    src/core/SkGlyphCache.cpp
    src/core/SkGlyphCache.h
    src/core/SkGraphics.cpp
    src/core/SkLineClipper.cpp
    src/core/SkMallocPixelRef.cpp
    src/core/SkMask.cpp
    src/core/SkMaskFilter.cpp
    src/core/SkMath.cpp
    src/core/SkMatrix.cpp
    src/core/SkMetaData.cpp
    src/core/SkMMapStream.cpp
    src/core/SkPackBits.cpp
    src/core/SkPaint.cpp
    src/core/SkPath.cpp
    src/core/SkPathEffect.cpp
    src/core/SkPathHeap.cpp
    src/core/SkPathHeap.h
    src/core/SkPathMeasure.cpp
    src/core/SkPicture.cpp
    src/core/SkPictureFlat.cpp
    src/core/SkPictureFlat.h
    src/core/SkPicturePlayback.cpp
    src/core/SkPicturePlayback.h
    src/core/SkPictureRecord.cpp
    src/core/SkPictureRecord.h
    src/core/SkPixelRef.cpp
    src/core/SkPoint.cpp
    src/core/SkProcSpriteBlitter.cpp
    src/core/SkPtrRecorder.cpp
    src/core/SkQuadClipper.cpp
    src/core/SkQuadClipper.h
    src/core/SkRasterClip.cpp
    src/core/SkRasterizer.cpp
    src/core/SkRect.cpp
    src/core/SkRefDict.cpp
    src/core/SkRegion.cpp
    src/core/SkRegionPriv.h
    src/core/SkRegion_path.cpp
    src/core/SkScalar.cpp
    src/core/SkScalerContext.cpp
    src/core/SkScan.cpp
    src/core/SkScanPriv.h
    src/core/SkScan_AntiPath.cpp
    src/core/SkScan_Antihair.cpp
    src/core/SkScan_Hairline.cpp
    src/core/SkScan_Path.cpp
    src/core/SkShader.cpp
    src/core/SkShape.cpp
    src/core/SkSpriteBlitter_ARGB32.cpp
    src/core/SkSpriteBlitter_RGB16.cpp
    src/core/SkSinTable.h
    src/core/SkSpriteBlitter.h
    src/core/SkSpriteBlitterTemplate.h
    src/core/SkStream.cpp
    src/core/SkString.cpp
    src/core/SkStroke.cpp
    src/core/SkStrokerPriv.cpp
    src/core/SkStrokerPriv.h
    src/core/SkTextFormatParams.h
    src/core/SkTSearch.cpp
    src/core/SkTSort.h
    src/core/SkTemplatesPriv.h
    src/core/SkTypeface.cpp
    src/core/SkTypefaceCache.cpp
    src/core/SkTypefaceCache.h
    src/core/SkUnPreMultiply.cpp
    src/core/SkUtils.cpp
    src/core/SkWriter32.cpp
    src/core/SkXfermode.cpp
)

if (TARGETING_PLAYBOOK)
    set(${LIBNAME}_src_opts
        src/opts/SkBlitRow_opts_arm.cpp
        src/opts/SkBitmapProcState_opts_arm.cpp
        src/opts/SkUtils_opts_none.cpp
    )
    set_property(SOURCE src/opts/SkBlitRow_opts_arm.cpp src/opts/SkBitmapProcState_opts_arm.cpp APPEND PROPERTY COMPILE_FLAGS -marm)
else ()
    set(${LIBNAME}_src_opts
        src/opts/SkBlitRow_opts_none.cpp
        src/opts/SkBitmapProcState_opts_none.cpp
        src/opts/SkUtils_opts_none.cpp
    )
endif ()

set(${LIBNAME}_src_ports
    src/ports/SkDebug_stdio.cpp
    src/ports/SkFontHost_sandbox_none.cpp
    src/ports/SkGlobalInitialization_default.cpp

    src/ports/SkFontHost_tables.cpp
    src/ports/SkMemory_malloc.cpp
    src/ports/SkOSFile_stdio.cpp
    src/ports/SkTime_Unix.cpp
    src/ports/sk_predefined_gamma.h

    src/ports/SkThread_pthread.cpp
    src/ports/SkFontHost_fontconfig.cpp
    src/ports/SkFontHost_gamma_none.cpp
    src/ports/SkFontHost_FreeType.cpp
    src/ports/SkFontHost_tables.cpp
)

set(${LIBNAME}_src_effects
    src/effects/Sk1DPathEffect.cpp
    src/effects/Sk2DPathEffect.cpp
    src/effects/SkAvoidXfermode.cpp
    src/effects/SkArithmeticMode.cpp
    src/effects/SkBitmapCache.cpp
    src/effects/SkBitmapCache.h
    src/effects/SkBlurDrawLooper.cpp
    src/effects/SkBlurMask.cpp
    src/effects/SkBlurMask.h
    src/effects/SkBlurImageFilter.cpp
    src/effects/SkBlurMaskFilter.cpp
    src/effects/SkColorFilters.cpp
    src/effects/SkColorMatrixFilter.cpp
    src/effects/SkCornerPathEffect.cpp
    src/effects/SkDashPathEffect.cpp
    src/effects/SkDiscretePathEffect.cpp
    src/effects/SkEffects.cpp
    src/effects/SkEmbossMask.cpp
    src/effects/SkEmbossMask.h
    src/effects/SkEmbossMask_Table.h
    src/effects/SkEmbossMaskFilter.cpp
    src/effects/SkGradientShader.cpp
    src/effects/SkGroupShape.cpp
    src/effects/SkKernel33MaskFilter.cpp
    src/effects/SkLayerDrawLooper.cpp
    src/effects/SkLayerRasterizer.cpp
    src/effects/SkPaintFlagsDrawFilter.cpp
    src/effects/SkPixelXorXfermode.cpp
    src/effects/SkPorterDuff.cpp
    src/effects/SkRadialGradient_Table.h
    src/effects/SkRectShape.cpp
    src/effects/SkTableColorFilter.cpp
    src/effects/SkTableMaskFilter.cpp
    src/effects/SkTestImageFilters.cpp
    src/effects/SkTransparentShader.cpp
)

set(${LIBNAME}_src_images
    src/images/SkImageDecoder.cpp
    src/images/SkImageEncoder.cpp
    src/images/SkImageDecoder_Factory.cpp
    src/images/SkImageEncoder_Factory.cpp
    src/images/SkImageDecoder_libpng.cpp
    src/images/SkScaledBitmapSampler.cpp
)

set(${LIBNAME}_src_utils
    src/utils/SkBase64.cpp
    src/utils/SkBase64.h
    src/utils/SkBoundaryPatch.cpp
    src/utils/SkCamera.cpp
    src/utils/SkColorMatrix.cpp
    src/utils/SkCubicInterval.cpp
    src/utils/SkCullPoints.cpp
    src/utils/SkDeferredCanvas.cpp
    src/utils/SkDumpCanvas.cpp
    src/utils/SkInterpolator.cpp
    src/utils/SkLayer.cpp
    src/utils/SkMatrix44.cpp
    src/utils/SkMeshUtils.cpp
    src/utils/SkNinePatch.cpp
    src/utils/SkNWayCanvas.cpp
    src/utils/SkOSFile.cpp
    src/utils/SkParse.cpp
    src/utils/SkParseColor.cpp
    src/utils/SkParsePath.cpp
    src/utils/SkProxyCanvas.cpp
    src/utils/SkSfntUtils.cpp
    src/utils/SkUnitMappers.cpp
)

set(${LIBNAME}_ganesh_src_core
    src/gpu/GrAAHairLinePathRenderer.cpp
    src/gpu/GrAAHairLinePathRenderer.h
    src/gpu/GrAAConvexPathRenderer.cpp
    src/gpu/GrAAConvexPathRenderer.h
    src/gpu/GrAddPathRenderers_default.cpp
    src/gpu/GrAllocator.h
    src/gpu/GrAllocPool.h
    src/gpu/GrAllocPool.cpp
    src/gpu/GrAtlas.cpp
    src/gpu/GrAtlas.h
    src/gpu/GrBinHashKey.h
    src/gpu/GrBufferAllocPool.cpp
    src/gpu/GrBufferAllocPool.h
    src/gpu/GrClip.cpp
    src/gpu/GrContext.cpp
    src/gpu/GrDefaultPathRenderer.cpp
    src/gpu/GrDefaultPathRenderer.h
    src/gpu/GrDrawState.h
    src/gpu/GrDrawTarget.cpp
    src/gpu/GrDrawTarget.h
    src/gpu/GrGeometryBuffer.h
    src/gpu/GrGpu.cpp
    src/gpu/GrGpu.h
    src/gpu/GrGpuFactory.cpp
    src/gpu/GrGpuVertex.h
    src/gpu/GrIndexBuffer.h
    src/gpu/GrInOrderDrawBuffer.cpp
    src/gpu/GrInOrderDrawBuffer.h
    src/gpu/GrMatrix.cpp
    src/gpu/GrMemory.cpp
    src/gpu/GrPathRendererChain.cpp
    src/gpu/GrPathRendererChain.h
    src/gpu/GrPathRenderer.cpp
    src/gpu/GrPathRenderer.h
    src/gpu/GrPathUtils.cpp
    src/gpu/GrPathUtils.h
    src/gpu/GrPlotMgr.h
    src/gpu/GrRandom.h
    src/gpu/GrRectanizer.cpp
    src/gpu/GrRectanizer.h
    src/gpu/GrRedBlackTree.h
    src/gpu/GrRenderTarget.cpp
    src/gpu/GrResource.cpp
    src/gpu/GrResourceCache.cpp
    src/gpu/GrResourceCache.h
    src/gpu/GrStencil.cpp
    src/gpu/GrStencil.h
    src/gpu/GrStencilBuffer.cpp
    src/gpu/GrStencilBuffer.h
    src/gpu/GrStringBuilder.h
    src/gpu/GrTBSearch.h
    src/gpu/GrTDArray.h
#    src/gpu/GrTesselatedPathRenderer.cpp
#    src/gpu/GrTesselatedPathRenderer.h
    src/gpu/GrTextContext.cpp
    src/gpu/GrTextStrike.cpp
    src/gpu/GrTextStrike.h
    src/gpu/GrTextStrike_impl.h
    src/gpu/GrTexture.cpp
    src/gpu/GrTHashCache.h
    src/gpu/GrTLList.h
    src/gpu/GrVertexBuffer.h
    src/gpu/gr_unittests.cpp

    src/gpu/gl/GrGLCaps.cpp
    src/gpu/gl/GrGLCaps.h
    src/gpu/gl/GrGLContextInfo.cpp
    src/gpu/gl/GrGLContextInfo.h
    src/gpu/gl/GrGLCreateNullInterface.cpp
    src/gpu/gl/GrGLDefaultInterface_native.cpp
    src/gpu/gl/GrGLIndexBuffer.cpp
    src/gpu/gl/GrGLIndexBuffer.h
    src/gpu/gl/GrGLInterface.cpp
    src/gpu/gl/GrGLIRect.h
    src/gpu/gl/GrGLProgram.cpp
    src/gpu/gl/GrGLProgram.h
    src/gpu/gl/GrGLRenderTarget.cpp
    src/gpu/gl/GrGLRenderTarget.h
    src/gpu/gl/GrGLShaderVar.h
    src/gpu/gl/GrGLSL.cpp
    src/gpu/gl/GrGLSL.h
    src/gpu/gl/GrGLStencilBuffer.cpp
    src/gpu/gl/GrGLStencilBuffer.h
    src/gpu/gl/GrGLTexture.cpp
    src/gpu/gl/GrGLTexture.h
    src/gpu/gl/GrGLUtil.cpp
    src/gpu/gl/GrGLVertexBuffer.cpp
    src/gpu/gl/GrGLVertexBuffer.h
    src/gpu/gl/GrGpuGL.cpp
    src/gpu/gl/GrGpuGL.h
    src/gpu/gl/GrGpuGLShaders.cpp
    src/gpu/gl/GrGpuGLShaders.h

    src/gpu/qnx/GrGLCreateNativeInterface_qnx.cpp
)

set(${LIBNAME}_ganesh_src_skia
    src/gpu/GrPrintf_skia.cpp
    src/gpu/SkGpuCanvas.cpp
    src/gpu/SkGpuDevice.cpp
    src/gpu/SkGr.cpp
    src/gpu/SkGrFontScaler.cpp
    src/gpu/SkGrTexturePixelRef.cpp

    src/gpu/gl/SkGLContext.cpp
    src/gpu/gl/SkNullGLContext.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gpu/include)
include_directories(${CMAKE_SOURCE_DIR}/include/config)
include_directories(${CMAKE_SOURCE_DIR}/include/core)
include_directories(${CMAKE_SOURCE_DIR}/include/effects)
include_directories(${CMAKE_SOURCE_DIR}/include/gpu)
include_directories(${CMAKE_SOURCE_DIR}/include/images)
include_directories(${CMAKE_SOURCE_DIR}/include/utils)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/gpu/gl)

# Include directories from staging (which is the CMAKE_INSTALL_PATH)
include_directories(${CMAKE_INSTALL_PREFIX}/usr/include)
include_directories(${CMAKE_INSTALL_PREFIX}/usr/include/skia)

add_definitions("-DSKIA_IMPLEMENTATION=1")
add_definitions("-DGR_IMPLEMENTATION=1")
add_definitions("-DGR_QNX_BUILD=1")
add_definitions("-DSK_SCALAR_IS_FLOAT")
add_definitions("-DSK_CAN_USE_FLOAT")
add_definitions("-DSK_USE_POSIX_THREADS")

if (ENABLE_GLES2)
    set(${LIBNAME}_srcs
        ${${LIBNAME}_src}
        ${${LIBNAME}_src_core}
        ${${LIBNAME}_src_opts}
        ${${LIBNAME}_src_ports}
        ${${LIBNAME}_src_effects}
        ${${LIBNAME}_src_images}
        ${${LIBNAME}_src_utils}
        ${${LIBNAME}_src_gl}
        ${${LIBNAME}_ganesh_src_core}
        ${${LIBNAME}_ganesh_src_skia}
    )
else ()
    set(${LIBNAME}_srcs
        ${${LIBNAME}_src}
        ${${LIBNAME}_src_core}
        ${${LIBNAME}_src_opts}
        ${${LIBNAME}_src_ports}
        ${${LIBNAME}_src_effects}
        ${${LIBNAME}_src_images}
        ${${LIBNAME}_src_utils}
    )
endif ()

add_library(${LIBNAME} SHARED
    ${${LIBNAME}_srcs}
)

#add_library(${LIBNAME}S STATIC
#	${${LIBNAME}_srcs}
#)

# Find a library only in the staging directory (which is the CMAKE_INSTALL_PATH)
macro (FIND_STAGING_LIBRARY _var _libname)
  FIND_LIBRARY(${_var} ${_libname}
    PATHS "${CMAKE_INSTALL_PREFIX}"
    ENV "QNX_TARGET"
    PATH_SUFFIXES "${CMAKE_SYSTEM_PROCESSOR}/lib" "${CMAKE_SYSTEM_PROCESSOR}/usr/lib"
    NO_DEFAULT_PATH)
endmacro ()

find_staging_library(LIB_FONTCONFIG fontconfig)
find_staging_library(LIB_PNG png)

if (ENABLE_GLES2)
    find_staging_library(LIB_GLES GLESv2)
    find_staging_library(LIB_EGL EGL)

    target_link_libraries(${LIBNAME}
        ${LIB_EGL}
        ${LIB_FONTCONFIG}
        ${LIB_GLES}
        ${LIB_PNG}
    )
else ()
    target_link_libraries(${LIBNAME}
        ${LIB_FONTCONFIG}
        ${LIB_PNG}
    )
endif ()

set_target_properties(${LIBNAME} PROPERTIES VERSION 1.0 SOVERSION 1)

get_target_property(CURRENT_LINK_FLAGS ${LIBNAME} LINK_FLAGS)

if (NOT CURRENT_LINK_FLAGS)
    set(CURRENT_LINK_FLAGS "")
endif (NOT CURRENT_LINK_FLAGS)

set_target_properties(${LIBNAME} PROPERTIES LINK_FLAGS "${CURRENT_LINK_FLAGS} -Wl,-zrelro")

install(TARGETS ${LIBNAME} DESTINATION ${CMAKE_SYSTEM_PROCESSOR}/usr/lib)
#install(TARGETS ${LIBNAME}S DESTINATION ${CMAKE_SYSTEM_PROCESSOR}/usr/lib)
install(FILES ${${LIBNAME}_publicheaders}
            DESTINATION usr/include/${LIBNAME}/skia)
