/*
 * Copyright 2011 Google Inc.
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "BitmapShader.h"
#include "SkBitmapProcShader.h"
#include "SkTypeface.h"

const char *BitmapShader::tileModes[] = {"Clamp", "Repeat", "Mirror", "Raw DrawBitmap", "DrawSprite Loop"};

static void drawLogo(SkCanvas *c, SkColor bbColor)
{
    SkPaint clearColor;
    clearColor.setARGB(32, 0, 0, 0);

    c->save();

    SkRect fullScreen;
    fullScreen.set(0,0,128,128);

    c->scale(c->getDeviceSize().width() / fullScreen.width(), c->getDeviceSize().height() / fullScreen.height());

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
    paint.setColor(bbColor);

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
    c->restore();
}

BitmapShader::BitmapShader(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Red BB Bitmap Demo";

    // Set up outline and text paints
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(4);
    outlinePaint.setColor(SK_ColorBLACK);

    SkTypeface *pTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal);
    textPaint.setAntiAlias(true);
    textPaint.setTypeface(pTypeface);
    textPaint.setColor(SK_ColorBLACK);
    textPaint.setTextSize(20);
    textPaint.setTextAlign(SkPaint::kCenter_Align);
    pTypeface->unref();

    // Set up bitmap for the shaders
    bitmap = new SkBitmap();
    bitmap->setConfig(SkBitmap::kARGB_8888_Config, 128, 128);
    bitmap->allocPixels();
    {
        // Draw to the bitmap
        SkCanvas temp(*bitmap);
        drawLogo(&temp, SK_ColorRED);
    }

    // Set up shaders
    clampShader = SkShader::CreateBitmapShader(*bitmap, SkShader::kClamp_TileMode, SkShader::kClamp_TileMode);
    tileShader = SkShader::CreateBitmapShader(*bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    mirrorShader = SkShader::CreateBitmapShader(*bitmap, SkShader::kMirror_TileMode, SkShader::kMirror_TileMode);

    // Set up rectangle
    rect.set(0, 0, 256, 256);
}

BitmapShader::~BitmapShader()
{
    // Clean up shaders
    clampShader->unref();
    tileShader->unref();
    mirrorShader->unref();
    delete bitmap;
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

    canvas->translate(-319, 319);
    drawWithShader(mirrorShader, tileModes[2]);

    canvas->translate(319, 0);
    SkRect srcRect = SkRect::MakeXYWH(0, 0, bitmap->width(), bitmap->height());
    canvas->drawBitmapRectToRect(*bitmap, &srcRect, rect, &shapePaint);
    canvas->drawRect(rect, outlinePaint);
    canvas->drawText(tileModes[3], strlen(tileModes[3]), 128, 285, textPaint);

    // This rotate and the existing translation should be ignored for drawSprite
    // such that the resulting bitmaps fit in the box drawn afterward.
    canvas->save();
    canvas->rotate(10);
    for (int i = 0; i < 256; i += 128)
        for (int j = 0; j < 256; j += 128)
            canvas->drawSprite(*bitmap, 70 + i, 660 + j, 0);
    canvas->restore();

    canvas->translate(-319, 319);
    canvas->drawRect(rect, outlinePaint);
    canvas->drawText(tileModes[4], strlen(tileModes[4]), 128, 285, textPaint);

    // Restore state
    canvas->restore();
}

ImageRotoBlitter::ImageRotoBlitter(SkCanvas *c, int w, int h)
    : DemoBase(c, w, h)
{
    name = "Image Roto-Blitter";

    imagePaint.setAlpha(127);

    SkColor colors[TileCount] = { SK_ColorRED, 0XFFCC8800, SK_ColorYELLOW, 0xFF580010 };

    for (int i = 0; i < TileCount; ++i) {
        tiles[i].setConfig(SkBitmap::kARGB_8888_Config, 999, 999); // Intentionally funky.
        tiles[i].allocPixels();
        SkCanvas temp(tiles[i]);
        drawLogo(&temp, colors[i]);
    }

    src = SkRect::MakeWH(tiles[0].width(), tiles[0].height());
    dst = SkRect::MakeWH(223, 223); // Intentionally odd.
}

ImageRotoBlitter::~ImageRotoBlitter()
{
}

void ImageRotoBlitter::draw()
{
    canvas->save();
    canvas->translate(dst.width() * 1.75, dst.height() * 1.75);

    const int count = 32;
    for (int i = 0; i < count; ++i) {
        canvas->rotate(360 / count);
        canvas->drawBitmapRectToRect(tiles[i % TileCount], &src, dst, &imagePaint);
    }

    canvas->restore();
}
