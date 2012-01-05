#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkGraphics.h"
#include "SkImageEncoder.h"
#include "SkNWayCanvas.h"
#include "SkPicture.h"
#include "SkString.h"
#include "GrContext.h"
#include "SkGpuDevice.h"
#include "SkEGLContext.h"

#include "SkBenchmark.h"
#include "BenchTimer.h"

#ifdef ANDROID
static void log_error(const char msg[]) { SkDebugf("%s", msg); }
static void log_progress(const char msg[]) { SkDebugf("%s", msg); }
#else
static void log_error(const char msg[]) { fprintf(stderr, "%s", msg); }
static void log_progress(const char msg[]) { printf("%s", msg); }
#endif

static void log_error(const SkString& str) { log_error(str.c_str()); }
static void log_progress(const SkString& str) { log_progress(str.c_str()); }

///////////////////////////////////////////////////////////////////////////////

static void erase(SkBitmap& bm) {
    if (bm.config() == SkBitmap::kA8_Config) {
        bm.eraseColor(0);
    } else {
        bm.eraseColor(SK_ColorWHITE);
    }
}

#if 0
static bool equal(const SkBitmap& bm1, const SkBitmap& bm2) {
    if (bm1.width() != bm2.width() ||
        bm1.height() != bm2.height() ||
        bm1.config() != bm2.config()) {
        return false;
    }
    
    size_t pixelBytes = bm1.width() * bm1.bytesPerPixel();
    for (int y = 0; y < bm1.height(); y++) {
        if (memcmp(bm1.getAddr(0, y), bm2.getAddr(0, y), pixelBytes)) {
            return false;
        }
    }
    return true;
}
#endif

class Iter {
public:
    Iter(void* param) {
        fBench = BenchRegistry::Head();
        fParam = param;
    }
    
    SkBenchmark* next() {
        if (fBench) {
            BenchRegistry::Factory f = fBench->factory();
            fBench = fBench->next();
            return f(fParam);
        }
        return NULL;
    }

private:
    const BenchRegistry* fBench;
    void* fParam;
};

static void make_filename(const char name[], SkString* path) {
    path->set(name);
    for (int i = 0; name[i]; i++) {
        switch (name[i]) {
            case '/':
            case '\\':
            case ' ':
            case ':':
                path->writable_str()[i] = '-';
                break;
            default:
                break;
        }
    }
}

static void saveFile(const char name[], const char config[], const char dir[],
                     const SkBitmap& bm) {
    SkBitmap copy;
    if (!bm.copyTo(&copy, SkBitmap::kARGB_8888_Config)) {
        return;
    }
    
    if (bm.config() == SkBitmap::kA8_Config) {
        // turn alpha into gray-scale
        size_t size = copy.getSize() >> 2;
        SkPMColor* p = copy.getAddr32(0, 0);
        for (size_t i = 0; i < size; i++) {
            int c = (*p >> SK_A32_SHIFT) & 0xFF;
            c = 255 - c;
            c |= (c << 24) | (c << 16) | (c << 8);
            *p++ = c | (SK_A32_MASK << SK_A32_SHIFT);
        }
    }
    
    SkString str;
    make_filename(name, &str);
    str.appendf("_%s.png", config);
    str.prepend(dir);
    ::remove(str.c_str());
    SkImageEncoder::EncodeFile(str.c_str(), copy, SkImageEncoder::kPNG_Type,
                               100);
}

static void performClip(SkCanvas* canvas, int w, int h) {
    SkRect r;
    
    r.set(SkIntToScalar(10), SkIntToScalar(10),
          SkIntToScalar(w*2/3), SkIntToScalar(h*2/3));
    canvas->clipRect(r, SkRegion::kIntersect_Op);
    
    r.set(SkIntToScalar(w/3), SkIntToScalar(h/3),
          SkIntToScalar(w-10), SkIntToScalar(h-10));
    canvas->clipRect(r, SkRegion::kXOR_Op);
}

static void performRotate(SkCanvas* canvas, int w, int h) {
    const SkScalar x = SkIntToScalar(w) / 2;
    const SkScalar y = SkIntToScalar(h) / 2;
    
    canvas->translate(x, y);
    canvas->rotate(SkIntToScalar(35));
    canvas->translate(-x, -y);
}

static void performScale(SkCanvas* canvas, int w, int h) {
    const SkScalar x = SkIntToScalar(w) / 2;
    const SkScalar y = SkIntToScalar(h) / 2;
    
    canvas->translate(x, y);
    // just enough so we can't take the sprite case
    canvas->scale(SK_Scalar1 * 99/100, SK_Scalar1 * 99/100);
    canvas->translate(-x, -y);
}

static bool parse_bool_arg(char * const* argv, char* const* stop, bool* var) {
    if (argv < stop) {
        *var = atoi(*argv) != 0;
        return true;
    }
    return false;
}

enum Backend {
    kRaster_Backend,
    kGPU_Backend,
    kPDF_Backend,
};

static SkDevice* make_device(SkBitmap::Config config, const SkIPoint& size,
                             Backend backend, GrContext* context) {
    SkDevice* device = NULL;
    SkBitmap bitmap;
    bitmap.setConfig(config, size.fX, size.fY);
    
    switch (backend) {
        case kRaster_Backend:
            bitmap.allocPixels();
            erase(bitmap);
            device = new SkDevice(bitmap);
            break;
        case kGPU_Backend:
            device = new SkGpuDevice(context, SkGpuDevice::Current3DApiRenderTarget());
//            device->clear(0xFFFFFFFF);
            break;
        case kPDF_Backend:
        default:
            SkASSERT(!"unsupported");
    }
    return device;
}

static const struct {
    SkBitmap::Config    fConfig;
    const char*         fName;
    Backend             fBackend;
} gConfigs[] = {
    { SkBitmap::kARGB_8888_Config,  "8888",     kRaster_Backend },
    { SkBitmap::kRGB_565_Config,    "565",      kRaster_Backend },
    { SkBitmap::kARGB_8888_Config,  "GPU",      kGPU_Backend },
};

static int findConfig(const char config[]) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gConfigs); i++) {
        if (!strcmp(config, gConfigs[i].fName)) {
            return i;
        }
    }
    return -1;
}

int main (int argc, char * const argv[]) {
    SkAutoGraphics ag;
    
    SkTDict<const char*> defineDict(1024);
    int repeatDraw = 1;
    int forceAlpha = 0xFF;
    bool forceAA = true;
    bool forceFilter = false;
    SkTriState::State forceDither = SkTriState::kDefault;
    bool timerWall = false;
    bool timerCpu = true;
    bool timerGpu = true;
    bool doScale = false;
    bool doRotate = false;
    bool doClip = false;
    const char* matchStr = NULL;
    bool hasStrokeWidth = false;
    float strokeWidth;
    
    SkString outDir;
    SkBitmap::Config outConfig = SkBitmap::kNo_Config;
    const char* configName = "";
    Backend backend = kRaster_Backend;  // for warning
    int configCount = SK_ARRAY_COUNT(gConfigs);
    
    char* const* stop = argv + argc;
    for (++argv; argv < stop; ++argv) {
        if (strcmp(*argv, "-o") == 0) {
            argv++;
            if (argv < stop && **argv) {
                outDir.set(*argv);
                if (outDir.c_str()[outDir.size() - 1] != '/') {
                    outDir.append("/");
                }
            }
        } else if (strcmp(*argv, "-repeat") == 0) {
            argv++;
            if (argv < stop) {
                repeatDraw = atoi(*argv);
                if (repeatDraw < 1) {
                    repeatDraw = 1;
                }
            } else {
                log_error("missing arg for -repeat\n");
                return -1;
            }
        } else if (strcmp(*argv, "-timers") == 0) {
            argv++;
            if (argv < stop) {
                timerWall = false;
                timerCpu = false;
                timerGpu = false;
                for (char* t = *argv; *t; ++t) {
                    switch (*t) {
                    case 'w': timerWall = true; break;
                    case 'c': timerCpu = true; break;
                    case 'g': timerGpu = true; break;
                    }
                }
            } else {
                log_error("missing arg for -timers\n");
                return -1;
            }
        } else if (!strcmp(*argv, "-rotate")) {
            doRotate = true;
        } else if (!strcmp(*argv, "-scale")) {
            doScale = true;
        } else if (!strcmp(*argv, "-clip")) {
            doClip = true;
        } else if (strcmp(*argv, "-forceAA") == 0) {
            if (!parse_bool_arg(++argv, stop, &forceAA)) {
                log_error("missing arg for -forceAA\n");
                return -1;
            }
        } else if (strcmp(*argv, "-forceFilter") == 0) {
            if (!parse_bool_arg(++argv, stop, &forceFilter)) {
                log_error("missing arg for -forceFilter\n");
                return -1;
            }
        } else if (strcmp(*argv, "-forceDither") == 0) {
            bool tmp;
            if (!parse_bool_arg(++argv, stop, &tmp)) {
                log_error("missing arg for -forceDither\n");
                return -1;
            }
            forceDither = tmp ? SkTriState::kTrue : SkTriState::kFalse;
        } else if (strcmp(*argv, "-forceBlend") == 0) {
            bool wantAlpha = false;
            if (!parse_bool_arg(++argv, stop, &wantAlpha)) {
                log_error("missing arg for -forceBlend\n");
                return -1;
            }
            forceAlpha = wantAlpha ? 0x80 : 0xFF;
        } else if (strcmp(*argv, "-strokeWidth") == 0) {
            argv++;
            if (argv < stop) {
                const char *strokeWidthStr = *argv;
                if (sscanf(strokeWidthStr, "%f", &strokeWidth) != 1) {
                  log_error("bad arg for -strokeWidth\n");
                  return -1;
                }
                hasStrokeWidth = true;
            } else {
                log_error("missing arg for -strokeWidth\n");
                return -1;
            }
        } else if (strcmp(*argv, "-match") == 0) {
            argv++;
            if (argv < stop) {
                matchStr = *argv;
            } else {
                log_error("missing arg for -match\n");
                return -1;
            }
        } else if (strcmp(*argv, "-config") == 0) {
            argv++;
            if (argv < stop) {
                int index = findConfig(*argv);
                if (index >= 0) {
                    outConfig = gConfigs[index].fConfig;
                    configName = gConfigs[index].fName;
                    backend = gConfigs[index].fBackend;
                    configCount = 1;
                } else {
                    SkString str;
                    str.printf("unrecognized config %s\n", *argv);
                    log_error(str);
                    return -1;
                }
            } else {
                log_error("missing arg for -config\n");
                return -1;
            }
        } else if (strlen(*argv) > 2 && strncmp(*argv, "-D", 2) == 0) {
            argv++;
            if (argv < stop) {
                defineDict.set(argv[-1] + 2, *argv);
            } else {
                log_error("incomplete '-Dfoo bar' definition\n");
                return -1;
            }
        } else {
            SkString str;
            str.printf("unrecognized arg %s\n", *argv);
            log_error(str);
            return -1;
        }
    }
    
    // report our current settings
    {
        SkString str;
        str.printf("skia bench: alpha=0x%02X antialias=%d filter=%d",
                   forceAlpha, forceAA, forceFilter);
        str.appendf(" rotate=%d scale=%d clip=%d",
                   doRotate, doScale, doClip);
                   
        const char * ditherName;
        switch (forceDither) {
            case SkTriState::kDefault: ditherName = "default"; break;
            case SkTriState::kTrue: ditherName = "true"; break;
            case SkTriState::kFalse: ditherName = "false"; break;
            default: ditherName = "<invalid>"; break;
        }
        str.appendf(" dither=%s", ditherName);
        
        if (hasStrokeWidth) {
            str.appendf(" strokeWidth=%f", strokeWidth);
        } else {
            str.append(" strokeWidth=none");
        }
        
#if defined(SK_SCALAR_IS_FLOAT)
        str.append(" scalar=float");
#elif defined(SK_SCALAR_IS_FIXED)
        str.append(" scalar=fixed");
#endif

#if defined(SK_BUILD_FOR_WIN32)
        str.append(" system=WIN32");
#elif defined(SK_BUILD_FOR_MAC)
        str.append(" system=MAC");
#elif defined(SK_BUILD_FOR_ANDROID)
        str.append(" system=ANDROID");
#elif defined(SK_BUILD_FOR_UNIX)
        str.append(" system=UNIX");
#else
        str.append(" system=other");
#endif

#if defined(SK_DEBUG)
        str.append(" DEBUG");
#endif
        str.append("\n");
        log_progress(str);
    }
    
    GrContext* context = NULL;
    //Don't do GL when fixed.
#if !defined(SK_SCALAR_IS_FIXED)
    SkEGLContext eglContext;
    if (eglContext.init(1024, 1024)) {
        context = GrContext::CreateGLShaderContext();
    }
#endif
    
    BenchTimer timer = BenchTimer();
    
    Iter iter(&defineDict);
    SkBenchmark* bench;
    while ((bench = iter.next()) != NULL) {
        SkIPoint dim = bench->getSize();
        if (dim.fX <= 0 || dim.fY <= 0) {
            continue;
        }
        
        bench->setForceAlpha(forceAlpha);
        bench->setForceAA(forceAA);
        bench->setForceFilter(forceFilter);
        bench->setDither(forceDither);
        if (hasStrokeWidth) {
            bench->setStrokeWidth(strokeWidth);
        }
        
        // only run benchmarks if their name contains matchStr
        if (matchStr && strstr(bench->getName(), matchStr) == NULL) {
            continue;
        }
        
        {
            SkString str;
            str.printf("running bench [%d %d] %28s", dim.fX, dim.fY,
                       bench->getName());
            log_progress(str);
        }
        
        for (int configIndex = 0; configIndex < configCount; configIndex++) {
            if (configCount > 1) {
                outConfig = gConfigs[configIndex].fConfig;
                configName = gConfigs[configIndex].fName;
                backend = gConfigs[configIndex].fBackend;
            }
            
            if (kGPU_Backend == backend && NULL == context) {
                continue;
            }
            
            SkDevice* device = make_device(outConfig, dim, backend, context);
            SkCanvas canvas(device);
            device->unref();
            
            if (doClip) {
                performClip(&canvas, dim.fX, dim.fY);
            }
            if (doScale) {
                performScale(&canvas, dim.fX, dim.fY);
            }
            if (doRotate) {
                performRotate(&canvas, dim.fX, dim.fY);
            }
            
            bool gpu = kGPU_Backend == backend && context;
            //warm up caches if needed
            if (repeatDraw > 1) {
                SkAutoCanvasRestore acr(&canvas, true);
                bench->draw(&canvas);
                if (gpu) {
                    context->flush();
                    glFinish();
                }
            }
            
            timer.start();
            for (int i = 0; i < repeatDraw; i++) {
                SkAutoCanvasRestore acr(&canvas, true);
                bench->draw(&canvas);
            }
            timer.end();
            
            if (repeatDraw > 1) {
                SkString str;
                str.printf("  %4s:", configName);
                if (timerWall) {
                    str.appendf(" msecs = %6.2f", timer.fWall / repeatDraw);
                }
                if (timerCpu) {
                    str.appendf(" cmsecs = %6.2f", timer.fCpu / repeatDraw);
                }
                if (timerGpu && gpu && timer.fGpu > 0) {
                    str.appendf(" gmsecs = %6.2f", timer.fGpu / repeatDraw);
                }
                log_progress(str);
            }
            if (outDir.size() > 0) {
                saveFile(bench->getName(), configName, outDir.c_str(),
                         device->accessBitmap(false));
            }
        }
        log_progress("\n");
    }
    
    return 0;
}
