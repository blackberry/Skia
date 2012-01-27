/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BitmapShader.h"
#include "SkBitmapProcShader.h"
#include "SkTypeface.h"

const char *BitmapShader::tileModes[] = {"Clamp", "Repeat", "Mirror"};

BitmapShader::BitmapShader(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Bitmap Shader Demo";

    // Set up outline and text paints
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(4);
    outlinePaint.setColor(SK_ColorBLUE);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up bitmap for the shaders
    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, 128, 128);
    bitmap.allocPixels();
    {
        // Draw to the bitmap
        SkCanvas temp(bitmap);
        drawLogo(&temp);
        temp.setDevice(0);
    }

    // Set up shaders
    clampShader = SkShader::CreateBitmapShader(bitmap, SkShader::kClamp_TileMode, SkShader::kClamp_TileMode);
    tileShader = SkShader::CreateBitmapShader(bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    mirrorShader = SkShader::CreateBitmapShader(bitmap, SkShader::kMirror_TileMode, SkShader::kMirror_TileMode);

    // Set up rectangle
    rect.set(0, 0, 256, 256);
}

BitmapShader::~BitmapShader()
{
    // Clean up shaders
    clampShader->unref();
    tileShader->unref();
    mirrorShader->unref();
}

void BitmapShader::drawLogo(SkCanvas *c)
{
    SkPaint clearColor;
    outlinePaint.setColor(SK_ColorBLACK);
    SkRect fullScreen;
    fullScreen.set(0,0,128,128);
    c->drawRect(fullScreen, clearColor);

    SkPath path;
    path.moveTo(3,0);
    path.lineTo(10, 0);
    path.quadTo(30, 0, 20, 13);
    path.quadTo(0, 14, 0, 13);
    path.lineTo(3, 0);
    path.close();

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setARGB(0xff, 0x80, 0x80, 0x80);

    c->save();

    c->translate(20, 20);

    c->translate(0.15*30, 0);
    c->drawPath(path, paint);

    c->translate(-0.15*30, 0.7*30);
    c->drawPath(path, paint);

    c->translate(1.15*30, -0.7*30);
    c->drawPath(path, paint);

    c->translate(-0.15*30, 0.7*30);
    c->drawPath(path, paint);

    c->translate(-0.15*30, 0.7*30);
    c->drawPath(path, paint);

    c->translate(1.15*30, -0.9*30);
    c->drawPath(path, paint);

    c->translate(-0.15*30, 0.7*30);
    c->drawPath(path, paint);

    c->restore();
}

void BitmapShader::drawWithShader(SkShader *shader, const char *label)
{
    if (!shader)
        return;

    // Set up paint with shader
    shapePaint.setStyle(SkPaint::kFill_Style);
    shapePaint.setAntiAlias(true);
    shapePaint.setShader(shader);

    // Draw rectangle and its outline
    canvas->drawRect(rect, shapePaint);
    canvas->drawRect(rect, outlinePaint);

    // Draw label
    canvas->drawText(label, strlen(label), 128, 285, textPaint);
}

void BitmapShader::draw()
{
    // Save state
    canvas->save();

    canvas->translate(68, 20);
    drawWithShader(clampShader, tileModes[0]);

    canvas->translate(319, 0);
    drawWithShader(tileShader, tileModes[1]);

    canvas->translate(319, 0);
    drawWithShader(mirrorShader, tileModes[2]);

    // Restore state
    canvas->restore();
}
