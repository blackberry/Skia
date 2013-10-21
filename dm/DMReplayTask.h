#ifndef DMReplayTask_DEFINED
#define DMReplayTask_DEFINED

#include "DMTask.h"
#include "SkBitmap.h"
#include "SkString.h"
#include "SkTemplates.h"
#include "gm.h"

// Records a GM through an SkPicture, draws it, and compares against the reference bitmap.

namespace DM {

class ReplayTask : public Task {

public:
    ReplayTask(const Task& parent,
               skiagm::GM*,
               SkBitmap reference);

    virtual void draw() SK_OVERRIDE;
    virtual bool usesGpu() const SK_OVERRIDE { return false; }
    virtual bool shouldSkip() const SK_OVERRIDE;
    virtual SkString name() const SK_OVERRIDE { return fName; }

private:
    const SkString fName;
    SkAutoTDelete<skiagm::GM> fGM;
    const SkBitmap fReference;
};

}  // namespace DM

#endif  // DMReplayTask_DEFINED
