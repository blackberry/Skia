{
  'target_defaults': {
    'conditions': [
      ['skia_os != "win"', {
        'sources/': [ ['exclude', '_win.(h|cpp)$'],
        ],
      }],
      ['skia_os != "mac"', {
        'sources/': [ ['exclude', '_mac.(h|cpp|m|mm)$'],
        ],
      }],
      ['skia_os != "linux"', {
        'sources/': [ ['exclude', '_unix.(h|cpp)$'],
        ],
      }],
      ['skia_os != "ios"', {
        'sources/': [ ['exclude', '_iOS.(h|cpp|m|mm)$'],
        ],
      }],
      ['skia_os != "android"', {
        'sources/': [ ['exclude', '_android.(h|cpp)$'],
        ],
      }],
      ['skia_os != "qnx"', {
        'sources/': [ ['exclude', '_qnx.(h|cpp)$'],
        ],
      }, {
        'sources/': [ ['exclude', 'GentlTrace_none.cpp$'] ],
        'sources': [ '<(skia_src_path)/gentl/GentlTrace_qnx.cpp' ],
      }],
      ['skia_os != "nacl"', {
        'sources/': [ ['exclude', '_nacl.(h|cpp)$'],
        ],
      }],
      [ 'skia_os == "android"', {
        'defines': [
          'GR_ANDROID_BUILD=1',
        ],
      }],
      [ 'skia_os == "mac"', {
        'defines': [
          'GR_MAC_BUILD=1',
        ],
      }],
      [ 'skia_os == "linux"', {
        'defines': [
          'GR_LINUX_BUILD=1',
        ],
      }],
      [ 'skia_os == "ios"', {
        'defines': [
          'GR_IOS_BUILD=1',
        ],
      }],
      [ 'skia_os == "win"', {
        'defines': [
          'GR_WIN32_BUILD=1',
        ],
      }],
      [ 'skia_os == "qnx"', {
        'defines': [
          'GR_QNX_BUILD=1',
        ],
        'link_settings': {
          'libraries': [
            '-lslog2',
          ],
        },
      }],
      # nullify the targets in this gyp file if skia_gentl is 0
      [ 'skia_gentl == 0', {
        'sources/': [
          ['exclude', '.*'],
        ],
        'defines/': [
          ['exclude', '.*'],
        ],
        'include_dirs/': [
           ['exclude', '.*'],
        ],
        'link_settings': {
          'libraries/': [
            ['exclude', '.*'],
          ],
        },
        'direct_dependent_settings': {
          'defines/': [
            ['exclude', '.*'],
          ],
          'include_dirs/': [
            ['exclude', '.*'],
          ],
        },
      }],
      [ 'skia_resource_cache_mb_limit != 0', {
        'defines': [
          'GR_DEFAULT_TEXTURE_CACHE_MB_LIMIT=<(skia_resource_cache_mb_limit)',
        ],
      }],
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        '../include/gpu',
      ],
    },
  },
  'targets': [
    {
      'target_name': 'gentl',
      'product_name': 'skia_gentl',
      'type': 'static_library',
      'standalone_static_library': 1,
      'defines': [
        'GENTL_DEBUG_EGL=0',
        'GENTL_TRACE=0',
      ],
      'includes': [
        'gentl.gypi',
      ],
      'include_dirs': [
        '../include/config',
        '../include/core',
        '../include/utils',
        '../src/core',
        '../include/gpu',
        '../src/gpu',
        '../include/effects',
      ],
      'dependencies': [
        'gpu.gyp:*',
      ],
      'export_dependent_settings': [
        'gpu.gyp:*',
      ],
      'sources': [
        '<@(gentl_sources)',
        'gpu.gypi', # Makes the gypi appear in IDEs (but does not modify the build).
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
