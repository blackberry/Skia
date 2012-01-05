{
  'includes': [
    'target_defaults.gypi',
  ],
  'targets': [
    # Due to an unfortunate intersection of lameness between gcc and gyp,
    # we have to build the *_SSE2.cpp files in a separate target.  The
    # gcc lameness is that, in order to compile SSE2 intrinsics code, it
    # must be passed the -msse2 flag.  However, with this flag, it may
    # emit SSE2 instructions even for scalar code, such as the CPUID
    # test used to test for the presence of SSE2.  So that, and all other
    # code must be compiled *without* -msse2.  The gyp lameness is that it
    # does not allow file-specific CFLAGS, so we must create this extra
    # target for those files to be compiled with -msse2.
    #
    # This is actually only a problem on 32-bit Linux (all Intel Macs have
    # SSE2, Linux x86_64 has SSE2 by definition, and MSC will happily emit
    # SSE2 from instrinsics, while generating plain ol' 386 for everything
    # else).  However, to keep the .gyp file simple and avoid platform-specific
    # build breakage, we do this on all platforms.

    # For about the same reason, we need to compile the ARM opts files
    # separately as well.
    {
      'target_name': 'opts',
      'type': 'static_library',
      'include_dirs': [
        '../include/config',
        '../include/core',
        '../src/core',
      ],
      'conditions': [
        [ '(OS == "linux" or OS == "freebsd" or OS == "openbsd")', {
          'cflags': [
            '-msse2',
          ],
        }],
      ],
      'sources': [
        '../src/opts/SkBitmapProcState_opts_SSE2.cpp',
        '../src/opts/SkBlitRow_opts_SSE2.cpp',
        '../src/opts/SkUtils_opts_SSE2.cpp',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
