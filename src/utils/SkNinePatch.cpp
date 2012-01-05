/*
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "SkNinePatch.h"
#include "SkCanvas.h"
#include "SkShader.h"

static const uint16_t g3x3Indices[] = {
    0, 5, 1,    0, 4, 5,
    1, 6, 2,    1, 5, 6,
    2, 7, 3,    2, 6, 7,
    
    4, 9, 5,    4, 8, 9,
    5, 10, 6,   5, 9, 10,
    6, 11, 7,   6, 10, 11,
    
    8, 13, 9,   8, 12, 13,
    9, 14, 10,  9, 13, 14,
    10, 15, 11, 10, 14, 15
};

static int fillIndices(uint16_t indices[], int xCount, int yCount) {
    uint16_t* startIndices = indices;
    
    int n = 0;
    for (int y = 0; y < yCount; y++) {
        for (int x = 0; x < xCount; x++) {
            *indices++ = n;
            *indices++ = n + xCount + 2;
            *indices++ = n + 1;
            
            *indices++ = n;
            *indices++ = n + xCount + 1;
            *indices++ = n + xCount + 2;
            
            n += 1;
        }
        n += 1;
    }
    return indices - startIndices;
}

static void fillRow(SkPoint verts[], SkPoint texs[],
                    const SkScalar vy, const SkScalar ty,
                    const SkRect& bounds, const int32_t xDivs[], int numXDivs,
                    const SkScalar stretchX, int width) {
    SkScalar vx = bounds.fLeft;
    verts->set(vx, vy); verts++;
    texs->set(0, ty); texs++;
    for (int x = 0; x < numXDivs; x++) {
        SkScalar tx = SkIntToScalar(xDivs[x]);
        if (x & 1) {
            vx += stretchX;
        } else {
            vx += tx;
        }
        verts->set(vx, vy); verts++;
        texs->set(tx, ty); texs++;
    }
    verts->set(bounds.fRight, vy); verts++;
    texs->set(SkIntToScalar(width), ty); texs++;
}

struct Mesh {
    const SkPoint*  fVerts;
    const SkPoint*  fTexs;
    const SkColor*  fColors;
    const uint16_t* fIndices;
};

void SkNinePatch::DrawMesh(SkCanvas* canvas, const SkRect& bounds,
                           const SkBitmap& bitmap,
                           const int32_t xDivs[], int numXDivs,
                           const int32_t yDivs[], int numYDivs,
                           const SkPaint* paint) {
    if (bounds.isEmpty() || bitmap.width() == 0 || bitmap.height() == 0) {
        return;
    }
    
    // should try a quick-reject test before calling lockPixels
    SkAutoLockPixels alp(bitmap);
    // after the lock, it is valid to check
    if (!bitmap.readyToDraw()) {
        return;
    }
    
    // check for degenerate divs (just an optimization, not required)
    {
        int i;
        int zeros = 0;
        for (i = 0; i < numYDivs && yDivs[i] == 0; i++) {
            zeros += 1;
        }
        numYDivs -= zeros;
        yDivs += zeros;
        for (i = numYDivs - 1; i >= 0 && yDivs[i] == bitmap.height(); --i) {
            numYDivs -= 1;
        }
    }
    
    Mesh mesh;
    
    const int numXStretch = (numXDivs + 1) >> 1;
    const int numYStretch = (numYDivs + 1) >> 1;
    
    if (numXStretch < 1 && numYStretch < 1) {
    BITMAP_RECT:
//        SkDebugf("------ drawasamesh revert to bitmaprect\n");
        canvas->drawBitmapRect(bitmap, NULL, bounds, paint);
        return;
    }
    
    if (false) {
        int i;
        for (i = 0; i < numXDivs; i++) {
            SkDebugf("--- xdivs[%d] %d\n", i, xDivs[i]);
        }
        for (i = 0; i < numYDivs; i++) {
            SkDebugf("--- ydivs[%d] %d\n", i, yDivs[i]);
        }
    }
    
    SkScalar stretchX = 0, stretchY = 0;
    
    if (numXStretch > 0) {
        int stretchSize = 0;
        for (int i = 1; i < numXDivs; i += 2) {
            stretchSize += xDivs[i] - xDivs[i-1];
        }
        int fixed = bitmap.width() - stretchSize;
        stretchX = (bounds.width() - SkIntToScalar(fixed)) / numXStretch;
        if (stretchX < 0) {
            goto BITMAP_RECT;
        }
    }
    
    if (numYStretch > 0) {
        int stretchSize = 0;
        for (int i = 1; i < numYDivs; i += 2) {
            stretchSize += yDivs[i] - yDivs[i-1];
        }
        int fixed = bitmap.height() - stretchSize;
        stretchY = (bounds.height() - SkIntToScalar(fixed)) / numYStretch;
        if (stretchY < 0) {
            goto BITMAP_RECT;
        }
    }
    
#if 0
    SkDebugf("---- drawasamesh [%d %d] -> [%g %g] <%d %d> (%g %g)\n",
             bitmap.width(), bitmap.height(),
             SkScalarToFloat(bounds.width()), SkScalarToFloat(bounds.height()),
             numXDivs + 1, numYDivs + 1,
             SkScalarToFloat(stretchX), SkScalarToFloat(stretchY));
#endif

    const int vCount = (numXDivs + 2) * (numYDivs + 2);
    // number of celss * 2 (tris per cell) * 3 (verts per tri)
    const int indexCount = (numXDivs + 1) * (numYDivs + 1) * 2 * 3;
    // allocate 2 times, one for verts, one for texs, plus indices
    SkAutoMalloc storage(vCount * sizeof(SkPoint) * 2 +
                         indexCount * sizeof(uint16_t));
    SkPoint* verts = (SkPoint*)storage.get();
    SkPoint* texs = verts + vCount;
    uint16_t* indices = (uint16_t*)(texs + vCount);
    
    mesh.fVerts = verts;
    mesh.fTexs = texs;
    mesh.fColors = NULL;
    mesh.fIndices = NULL;
    
    // we use <= for YDivs, since the prebuild indices work for 3x2 and 3x1 too
    if (numXDivs == 2 && numYDivs <= 2) {
        mesh.fIndices = g3x3Indices;
    } else {
        SkDEBUGCODE(int n =) fillIndices(indices, numXDivs + 1, numYDivs + 1);
        SkASSERT(n == indexCount);
        mesh.fIndices = indices;
    }
    
    SkScalar vy = bounds.fTop;
    fillRow(verts, texs, vy, 0, bounds, xDivs, numXDivs,
            stretchX, bitmap.width());
    verts += numXDivs + 2;
    texs += numXDivs + 2;
    for (int y = 0; y < numYDivs; y++) {
        const SkScalar ty = SkIntToScalar(yDivs[y]);
        if (y & 1) {
            vy += stretchY;
        } else {
            vy += ty;
        }
        fillRow(verts, texs, vy, ty, bounds, xDivs, numXDivs,
                stretchX, bitmap.width());
        verts += numXDivs + 2;
        texs += numXDivs + 2;
    }
    fillRow(verts, texs, bounds.fBottom, SkIntToScalar(bitmap.height()),
            bounds, xDivs, numXDivs, stretchX, bitmap.width());
    
    SkShader* shader = SkShader::CreateBitmapShader(bitmap,
                                                    SkShader::kClamp_TileMode,
                                                    SkShader::kClamp_TileMode);
    SkPaint p;
    if (paint) {
        p = *paint;
    }
    p.setShader(shader)->unref();
    canvas->drawVertices(SkCanvas::kTriangles_VertexMode, vCount,
                         mesh.fVerts, mesh.fTexs, mesh.fColors, NULL,
                         mesh.fIndices, indexCount, p);
}

///////////////////////////////////////////////////////////////////////////////

static void drawNineViaRects(SkCanvas* canvas, const SkRect& dst,
                             const SkBitmap& bitmap, const SkIRect& margins,
                             const SkPaint* paint) {
    const int32_t srcX[4] = {
        0, margins.fLeft, bitmap.width() - margins.fRight, bitmap.width()
    };
    const int32_t srcY[4] = {
        0, margins.fTop, bitmap.height() - margins.fBottom, bitmap.height()
    };
    SkScalar dstX[4] = {
        dst.fLeft, dst.fLeft + SkIntToScalar(margins.fLeft),
        dst.fRight - SkIntToScalar(margins.fRight), dst.fRight
    };
    SkScalar dstY[4] = {
        dst.fTop, dst.fTop + SkIntToScalar(margins.fTop),
        dst.fBottom - SkIntToScalar(margins.fBottom), dst.fBottom
    };

    if (dstX[1] > dstX[2]) {
        dstX[1] = dstX[0] + (dstX[3] - dstX[0]) * SkIntToScalar(margins.fLeft) /
            (SkIntToScalar(margins.fLeft) + SkIntToScalar(margins.fRight));
        dstX[2] = dstX[1];
    }

    if (dstY[1] > dstY[2]) {
        dstY[1] = dstY[0] + (dstY[3] - dstY[0]) * SkIntToScalar(margins.fTop) /
            (SkIntToScalar(margins.fTop) + SkIntToScalar(margins.fBottom));
        dstY[2] = dstY[1];
    }

    SkIRect s;
    SkRect  d;
    for (int y = 0; y < 3; y++) {
        s.fTop = srcY[y];
        s.fBottom = srcY[y+1];
        d.fTop = dstY[y];
        d.fBottom = dstY[y+1];
        for (int x = 0; x < 3; x++) {
            s.fLeft = srcX[x];
            s.fRight = srcX[x+1];
            d.fLeft = dstX[x];
            d.fRight = dstX[x+1];
            canvas->drawBitmapRect(bitmap, &s, d, paint);
        }
    }
}

void SkNinePatch::DrawNine(SkCanvas* canvas, const SkRect& bounds,
                           const SkBitmap& bitmap, const SkIRect& margins,
                           const SkPaint* paint) {
    /** Our vertices code has numerical precision problems if the transformed
     coordinates land directly on a 1/2 pixel boundary. To work around that
     for now, we only take the vertices case if we are in opengl. Also,
     when not in GL, the vertices impl is slower (more math) than calling
     the viaRects code.
     */
    if (false /* is our canvas backed by a gpu?*/) {
        int32_t xDivs[2];
        int32_t yDivs[2];
        
        xDivs[0] = margins.fLeft;
        xDivs[1] = bitmap.width() - margins.fRight;
        yDivs[0] = margins.fTop;
        yDivs[1] = bitmap.height() - margins.fBottom;

        if (xDivs[0] > xDivs[1]) {
            xDivs[0] = bitmap.width() * margins.fLeft /
                (margins.fLeft + margins.fRight);
            xDivs[1] = xDivs[0];
        }
        if (yDivs[0] > yDivs[1]) {
            yDivs[0] = bitmap.height() * margins.fTop /
                (margins.fTop + margins.fBottom);
            yDivs[1] = yDivs[0];
        }
        
        SkNinePatch::DrawMesh(canvas, bounds, bitmap,
                              xDivs, 2, yDivs, 2, paint);
    } else {
        drawNineViaRects(canvas, bounds, bitmap, margins, paint);
    }
}
