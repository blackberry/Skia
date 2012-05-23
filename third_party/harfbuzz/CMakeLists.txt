set(LIBNAME harfbuzz)

set(${LIBNAME}_publicheaders
    contrib/harfbuzz-freetype.h
    contrib/tables/grapheme-break-properties.h
    contrib/tables/script-properties.h
    contrib/tables/mirroring-properties.h
    contrib/tables/category-properties.h
    contrib/tables/combining-properties.h
    contrib/harfbuzz-unicode.h
    src/harfbuzz-shaper.h
    src/harfbuzz-gsub-private.h
    src/harfbuzz-stream-private.h
    src/harfbuzz-gdef-private.h
    src/harfbuzz-gsub.h
    src/harfbuzz.h
    src/harfbuzz-gpos-private.h
    src/harfbuzz-shaper-private.h
    src/harfbuzz-gpos.h
    src/harfbuzz-impl.h
    src/harfbuzz-buffer-private.h
    src/harfbuzz-shape.h
    src/harfbuzz-stream.h
    src/harfbuzz-open-private.h
    src/harfbuzz-global.h
    src/harfbuzz-external.h
    src/harfbuzz-gdef.h
    src/harfbuzz-dump.h
    src/harfbuzz-buffer.h
    src/harfbuzz-open.h)

set(${LIBNAME}_srcs
    src/harfbuzz-buffer.c
    src/harfbuzz-stream.c
    src/harfbuzz-dump.c
    src/harfbuzz-gdef.c
    src/harfbuzz-gpos.c
    src/harfbuzz-greek.c
    src/harfbuzz-gsub.c
    src/harfbuzz-impl.c
    src/harfbuzz-open.c
    src/harfbuzz-shaper.cpp
    src/harfbuzz-tibetan.c
    src/harfbuzz-khmer.c
    src/harfbuzz-indic.cpp
    src/harfbuzz-hebrew.c
    src/harfbuzz-arabic.c
    src/harfbuzz-hangul.c
    src/harfbuzz-myanmar.c
    src/harfbuzz-thai.c
    contrib/harfbuzz-freetype.c
    contrib/harfbuzz-unicode.c
    contrib/harfbuzz-unicode-tables.c)

include_directories(${CMAKE_SOURCE_DIR}/third_party/harfbuzz/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party/harfbuzz/contrib)

# Include directories from staging (which is the CMAKE_INSTALL_PATH)
include_directories(${CMAKE_INSTALL_PREFIX}/usr/include)
include_directories(${CMAKE_INSTALL_PREFIX}/usr/include/skia)

add_library(${LIBNAME} SHARED ${${LIBNAME}_srcs})

add_definitions("-DHAVE_PTHREADS=1")
add_definitions("-DHAVE_QNX_ATOMIC=1")
add_definitions("-DHB_BUILD=1")

set_target_properties(${LIBNAME} PROPERTIES VERSION 0.3 SOVERSION 0)

get_target_property(CURRENT_LINK_FLAGS ${LIBNAME} LINK_FLAGS)

if (NOT CURRENT_LINK_FLAGS)
    set(CURRENT_LINK_FLAGS "")
endif (NOT CURRENT_LINK_FLAGS)

set_target_properties(${LIBNAME} PROPERTIES LINK_FLAGS "${CURRENT_LINK_FLAGS} -Wl,-zrelro")

# CMAKE_INSTALL_PREFIX should be set to the parent directory of all the
# CPU-specific stages.
install(TARGETS ${LIBNAME} DESTINATION ${CMAKE_SYSTEM_PROCESSOR}/usr/lib)
install(FILES ${${LIBNAME}_publicheaders}
    DESTINATION usr/include/harfbuzz)
