# Include this gypi to include all 'gentl' files
# Gentl depends on skia_gr.
#
# The skia build defines these in common_variables.gypi
#
{
  'variables': {
    'gentl_sources': [
      '<(skia_src_path)/gentl/BuffChain.cpp',
      '<(skia_src_path)/gentl/Clip.cpp',
      '<(skia_src_path)/gentl/DisplayList.cpp',
      '<(skia_src_path)/gentl/FramebufferStack.cpp',
      '<(skia_src_path)/gentl/Generators.cpp',
      '<(skia_src_path)/gentl/GentlContext.cpp',
      '<(skia_src_path)/gentl/GentlDevice.cpp',
      '<(skia_src_path)/gentl/GentlPath.cpp',
      '<(skia_src_path)/gentl/GentlTrace_none.cpp',
      '<(skia_src_path)/gentl/GlyphCache.cpp',
      '<(skia_src_path)/gentl/InstructionSet.cpp',
      '<(skia_src_path)/gentl/Layers.cpp',
      '<(skia_src_path)/gentl/Precompile.cpp',
      '<(skia_src_path)/gentl/Shaders.cpp',
      '<(skia_src_path)/gentl/ShadersCache.cpp',
      '<(skia_src_path)/gentl/VertexBufferCache.cpp',

      '<(skia_src_path)/gentl/tess/bucketalloc.c',
      '<(skia_src_path)/gentl/tess/dict.c',
      '<(skia_src_path)/gentl/tess/geom.c',
      '<(skia_src_path)/gentl/tess/mesh.c',
      '<(skia_src_path)/gentl/tess/priorityq.c',
      '<(skia_src_path)/gentl/tess/sweep.c',
      '<(skia_src_path)/gentl/tess/tess.c',
    ],
  }
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
