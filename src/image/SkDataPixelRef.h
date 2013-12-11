/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkDataPixelRef_DEFINED
#define SkDataPixelRef_DEFINED

#include "SkPixelRef.h"

class SkData;

class SkDataPixelRef : public SkPixelRef {
public:
            SkDataPixelRef(const SkImageInfo&, SkData* data, size_t rowBytes);
    virtual ~SkDataPixelRef();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDataPixelRef)

protected:
    virtual bool onNewLockPixels(LockRec*) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual size_t getAllocatedSizeInBytes() const SK_OVERRIDE;

    SkDataPixelRef(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkData*     fData;
    size_t      fRB;

    typedef SkPixelRef INHERITED;
};

#endif
