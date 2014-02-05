#include "SampleCode.h"

#include "SkCanvas.h"
#include "SkView.h"
#include "SkPicture.h"
#include "SkStream.h"

static SkPicture* loadSKP() {
    char path[4000];
    sprintf(path, "%s/layer_0.skp", getenv("HOME"));

    return SkPicture::CreateFromStream(new SkFILEStream(path));
}

class SKPictureView : public SampleView {
public:

    SKPictureView() {
        fPicture = loadSKP();
    }

protected:
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "SkPicture");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        canvas->save();
        if (fPicture)
            fPicture->draw(canvas);
        // Restore state
        canvas->restore();
    }

private:
    SkPicture* fPicture;

    typedef SkView INHERITED;
};
//////////////////////////////////////////////////////////////////////////////
static SkView* MyFactory() { return new SKPictureView; }
static SkViewRegister reg(MyFactory);

