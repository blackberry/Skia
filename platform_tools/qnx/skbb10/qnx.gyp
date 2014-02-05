# Top-level gyp configuration for skbb10 test app.
#
{
    'targets': [
      {
        'target_name': 'skbb10app',
        'type': 'static_library',

        'dependencies': [
            '../gyp/skia_lib.gyp:skia_lib',
        ],

        'sources': [
            'ArcDemo.cpp',
            'BasicDrawing.cpp',
            'bbutil.cpp',
            'BigBlur.cpp',
            'BitmapShader.cpp',
            'BitmapSprite.cpp',
            'CircleDemo.cpp',
            'ClipDemo.cpp',
            'DemoBase.cpp',
            'GradientShaders.cpp',
            'LayerDemo.cpp',
            'PaintStrokeCap.cpp',
            'PaintStrokeJoin.cpp',
            'PaintStyles.cpp',
            'PathEffects.cpp',
            'PathFillType.cpp',
            'Picture.cpp',
            'RectDemo.cpp',
            'Rects.cpp',
            'TextDemo.cpp',
            'TextOnPathDemo.cpp',
            'XferDemo.cpp',
        ],

        'cflags': [
            '-Wl,--fix-cortex-a8',
            '-march=armv7-a',
            '-g',
        ],

        'include_dirs': [
            '<(skia_include_path)/config',
            '<(skia_include_path)/core',
            '<(skia_include_path)/lazy',
            '<(skia_include_path)/pathops',
            '<(skia_include_path)/pipe',
            '<(skia_include_path)/ports',
            '<(skia_include_path)/effects',
            '<(skia_include_path)/utils',
            '<(skia_include_path)/xml',
            '<(skia_include_path)/gpu',
            '<(skia_src_path)/core',
            '<(skia_src_path)/effects',
            '<(skia_src_path)/image',
            '<(skia_src_path)/gpu',
            '<(skia_src_path)/gpu/gl/qnx',
       ],

       'libraries': [
            '-lfreetype',
            '-lbps',
            '-lscreen',
            '-lcpp-ne',
            '-lm',
            '-lpng',
            '-lgif',
            '-lGLESv2',
            '-lEGL',
            '-lz',
            '-lprofilingS',
          ],
      },

      {
        'target_name': 'skbb10',
        'type': 'executable',

        'dependencies': [
            'skbb10app',
        ],

        'sources': [
            'main.cpp',
        ],

        'cflags': [
            '-Wl,--fix-cortex-a8',
            '-march=armv7-a',
        ],

        'include_dirs': [
            '<(skia_include_path)/core',
            '<(skia_include_path)/config',
            '<(skia_include_path)/gpu',
            '<(skia_include_path)/utils',
            '<(skia_src_path)/gpu',
            '<(skia_src_path)/gpu/gl/qnx',
            '<(skia_src_path)/gentl',
       ],

       'libraries': [
            '-lbps',
       ],

      },
      {
        'target_name': 'skbb10-grskia',
        'type': 'executable',

        'dependencies': [
            'skbb10app',
        ],

        'defines': [
          'USE_SKIA_OPENGL',
        ],

        'sources': [
            'main.cpp',
        ],

        'cflags': [
            '-Wl,--fix-cortex-a8',
            '-march=armv7-a',
        ],

        'include_dirs': [
            '<(skia_include_path)/config',
            '<(skia_include_path)/core',
            '<(skia_include_path)/lazy',
            '<(skia_include_path)/pathops',
            '<(skia_include_path)/pipe',
            '<(skia_include_path)/ports',
            '<(skia_include_path)/effects',
            '<(skia_include_path)/utils',
            '<(skia_include_path)/xml',
            '<(skia_include_path)/gpu',
            '<(skia_src_path)/core',
            '<(skia_src_path)/image',
            '<(skia_src_path)/gpu',
            '<(skia_src_path)/gpu/gl/qnx',
            '<(skia_src_path)/gentl',
       ],

       'libraries': [
            '-lbps',
       ],
      },
      {
        'target_name': 'skbb10-software',
        'type': 'executable',

        'dependencies': [
            'skbb10app',
        ],

        'defines': [
          'USE_SKIA_SOFTWARE',
        ],

        'sources': [
            'main.cpp',
        ],

        'cflags': [
            '-Wl,--fix-cortex-a8',
            '-march=armv7-a',
        ],

        'include_dirs': [
            '<(skia_include_path)/config',
            '<(skia_include_path)/core',
            '<(skia_src_path)/gpu/gl/qnx',
            '<(skia_src_path)/gentl',
       ],

       'libraries': [
            '-lbps',
       ],
      },
   ],
}
# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
