/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "InstructionSet.h"

#include "GentlCommands.h"
#include "GrTexture.h"
#include "SkTypes.h"

#include <limits>
#include <stdio.h>

namespace Gentl {

InstructionSetCommand::InstructionSetCommand(DisplayListCommand inCommand, unsigned uIndex, unsigned vaIndex)
    : command(inCommand)
    , uniformIndex(uIndex)
    , vertexAttributeIndex(vaIndex)
    , vertexCount(0)
{
}

InstructionSet::InstructionSet()
    : fBuffChain(new BuffChain<InstructionSetData>)
{
}

InstructionSet::~InstructionSet()
{
    unrefAndClear();
    delete fBuffChain;
}

void InstructionSet::push(DisplayListCommand command)
{
    fCommands.push_back(InstructionSetCommand(command, fUniforms.size(), fBuffChain->size()));
}

void InstructionSet::incrementVertexCount()
{
    ++fCommands.back().vertexCount;
}

void InstructionSet::pushUniformData(const void* uniforms, size_t bytes)
{
    SkASSERT(uniforms && bytes);
    size_t oldSize = fUniforms.size();
    fUniforms.resize(fUniforms.size() + (bytes / sizeof(InstructionSetData)));
    memcpy(&fUniforms[oldSize], uniforms, bytes);
}

void InstructionSet::pushAndPackCoords(float u, float v)
{
    // This optimization is only allowed when the texture coordinates are already
    // normalized within the 0 to 1 range. It is designed to be used for text in
    // the glyph cache where this assumption always holds.
    SkASSERT(FloatUtils::isInBetween(u, 0, 1));
    SkASSERT(FloatUtils::isInBetween(v, 0, 1));

    // Pack the two floating point coordinates as denormalized unsigned short values
    // ranging from 0 to max unsigned short. This reduces them from two words to one,
    // and leverages the gl driver to normalize them back into floats for us later.
    static const unsigned DenormalizationConstant = std::numeric_limits<unsigned short>::max();
    fBuffChain->append(InstructionSetData(static_cast<unsigned>(u * DenormalizationConstant) |
                                           static_cast<unsigned>(v * DenormalizationConstant) << 16));
}

void InstructionSet::transferFrom(InstructionSet& instructions)
{
    transferFrom(instructions, instructions.fCommands.size());
}

void InstructionSet::transferFrom(InstructionSet& instructions, unsigned size)
{
    unsigned oldUniformSize = fUniforms.size();
    unsigned oldVertexAttributeSize = fBuffChain->size();
    unsigned oldCommandSize = fCommands.size();

    std::vector<InstructionSetCommand>::iterator begin = instructions.fCommands.begin();
    std::vector<InstructionSetCommand>::iterator end = begin + size;

    // Copy over all the data, and the specified number of command elements.
    fCommands.insert(fCommands.end(), begin, end);
    fUniforms.insert(fUniforms.end(), instructions.fUniforms.begin(), instructions.fUniforms.end());
    fBuffChain->adopt(instructions.fBuffChain);

    // After inserting all the commands and data we need to augment all of the
    // data indeces by the old data size as they are at the back of that array.
    if (oldUniformSize || oldVertexAttributeSize) {
        for (unsigned i = oldCommandSize; i < fCommands.size(); ++i) {
            fCommands[i].uniformIndex += oldUniformSize;
            fCommands[i].vertexAttributeIndex += oldVertexAttributeSize;
        }
    }

    // Then we need to remove the copied commands from the other buffer. If
    // we have copied over the entire command buffer we can clear its data.
    instructions.fCommands.erase(begin, end);
    if (!instructions.fCommands.size()) {
        instructions.fUniforms.clear();
    }
}

void InstructionSet::swapWith(InstructionSet& rhs)
{
    std::swap(fCommands, rhs.fCommands);
    std::swap(fUniforms, rhs.fUniforms);
    std::swap(fBuffChain, rhs.fBuffChain);
}

bool InstructionSet::isEmpty() const
{
    return fCommands.empty();
}

void InstructionSet::clear()
{
    fCommands.clear();
    fUniforms.clear();
    fBuffChain->clear();
}

unsigned InstructionSet::commandSize() const
{
    return fCommands.size();
}

unsigned InstructionSet::dataBytes() const
{
    return fBuffChain->size() * sizeof(InstructionSetData);
}

DisplayListCommand InstructionSet::lastCommand() const
{
    return fCommands.back().command;
}

const uint32_t* InstructionSet::lastUniforms() const
{
    return &fUniforms[fCommands.back().uniformIndex];
}

void InstructionSet::unrefAndClear()
{
    unref(0, fCommands.size());
    clear();
}

void InstructionSet::unrefAndResize(unsigned newSize)
{
    if (newSize >= fCommands.size())
        return;

    unref(newSize, fCommands.size());
    fCommands.erase(fCommands.begin() + newSize, fCommands.end());
}

#define UNREFFER(Command) case Command: {                                                           \
                              const Command##Uniforms& uniforms = uniformsAt<Command##Uniforms>(i); \
                              if (uniforms.texture)                                                 \
                                  uniforms.texture->unref();                                        \
                              break;                                                                \
                          }

void InstructionSet::unref(unsigned beginIndex, unsigned endIndex)
{
    for (unsigned i = beginIndex; i < endIndex; ++i) {
        switch (fCommands[i].command.id()) {
        UNREFFER(DrawBitmapGlyph);
        UNREFFER(DrawStrokedEdgeGlyph);
        UNREFFER(DrawEdgeGlyph);
        UNREFFER(DrawBlurredGlyph);
        UNREFFER(DrawSkShader);
        UNREFFER(DrawTexture);
        UNREFFER(DrawCurrentTexture);

        case Clear:
        case SetAlpha:
        case SetXferMode:
        case NoOp:
        case DrawColor:
        case DrawOpaqueColor:
        case DrawArc:
        case BeginTransparencyLayer:
        case EndTransparencyLayer:
            // Nothing to do for all the other cases.
            break;
        }
    }
}

void InstructionSet::validate(ValidationRequirement requirement) const
{
    int nestingIndex = 0;
    bool hasCurrentTexture = false;
    for (unsigned i = 0; i < fCommands.size(); ++i) {
        const DisplayListCommand c = fCommands[i].command;
        switch (c.id()) {

        // All the image drawing commands make the current texture invalid.
        case DrawBitmapGlyph:
        case DrawBlurredGlyph:
        case DrawEdgeGlyph:
        case DrawStrokedEdgeGlyph:
        case DrawSkShader:
        case DrawTexture:
            hasCurrentTexture = false;
            // Fall through.

        case Clear:
        case SetAlpha:
        case SetXferMode:
        case NoOp:
        case DrawColor:
        case DrawOpaqueColor:
        case DrawArc:
            // For most commands make sure the data pointer falls inside the data vector.
            if (fCommands[i].uniformIndex < fUniforms.size() && fCommands[i].vertexAttributeIndex < fBuffChain->size())
                break;

        // All of the transparency layer related commands have specific conditions.
        case BeginTransparencyLayer:
            hasCurrentTexture = false;
            nestingIndex++;
            break;

        case EndTransparencyLayer:
            if (!nestingIndex--) {
                dump();
                SkDebugf("Invalid InstructionSet: EndTransparencyLayer but nestingIndex == 0!\n");
                SK_CRASH();
            }

            hasCurrentTexture = true;
            break;

        case DrawCurrentTexture:
            if (!hasCurrentTexture) {
                dump();
                SkDebugf("Invalid InstructionSet: DrawCurrentTexture without texture!\n");
                SK_CRASH();
            }
            break;
        }
    }

    if (requirement == MustBeComplete && nestingIndex) {
        dump();
        SkDebugf("Invalid InstructionSet: Incomplete TransparencyLayers not allowed!\n");
        SK_CRASH();
    }
}

void InstructionSet::dump(unsigned cursorFirst, unsigned cursorLast, unsigned dataLimit) const
{
    cursorLast = std::min(cursorLast, commandSize());

    const unsigned size = fBuffChain->size();
    SkDebugf("InstructionSet Dump Commands %d - %d. CommandSize: %u, DataSize: %u%s\n",
        cursorFirst, cursorLast, fCommands.size(), size, size ? "" : " No data to dump.");

    const unsigned len = 128;
    char context[len];
    for (unsigned i = cursorFirst; i < cursorLast; ++i) {
        const DisplayListCommand c = fCommands[i].command;

        switch (c.id()) {
        case Clear: {
            const ClearUniforms& uniforms = uniformsAt<ClearUniforms>(i);
            snprintf(context, len, "ClearColor=(%g, %g, %g %g)", uniforms.red, uniforms.green, uniforms.blue, uniforms.alpha);
            break;
        }
        case SetAlpha: {
            const SetAlphaUniforms& uniforms = uniformsAt<SetAlphaUniforms>(i);
            snprintf(context, len, "Alpha=%g", uniforms.alpha);
            break;
        }
        case SetXferMode: {
            const SetXferModeUniforms& uniforms = uniformsAt<SetXferModeUniforms>(i);
            snprintf(context, len, "Op=%s", SkXfermode::ModeName(uniforms.mode));
            break;
        }
        case NoOp: {
            snprintf(context, len, "NoOp");
            break;
        }
        case DrawBitmapGlyph: {
            const DrawBitmapGlyphUniforms& uniforms = uniformsAt<DrawBitmapGlyphUniforms>(i);
            snprintf(context, len, "Tex=0x%p Color=0x%08x", uniforms.texture, uniforms.color);
            break;
        }
        case DrawBlurredGlyph: {
            const DrawBlurredGlyphUniforms& uniforms = uniformsAt<DrawBlurredGlyphUniforms>(i);
            snprintf(context, len, "Tex=0x%p Color=0x%08x Cutoff=[%g %g] Amount=%g",
                                   uniforms.texture, uniforms.color, uniforms.insideCutoff, uniforms.outsideCutoff, uniforms.blurRadius);
            break;
        }
        case DrawEdgeGlyph: {
            const DrawEdgeGlyphUniforms& uniforms = uniformsAt<DrawEdgeGlyphUniforms>(i);
            snprintf(context, len, "Tex=0x%p Color=0x%08x Cutoff=[%g %g]",
                                   uniforms.texture, uniforms.color, uniforms.insideCutoff, uniforms.outsideCutoff);
            break;
        }
        case DrawStrokedEdgeGlyph: {
            const DrawStrokedEdgeGlyphUniforms& uniforms = uniformsAt<DrawStrokedEdgeGlyphUniforms>(i);
            snprintf(context, len, "Tex=0x%p Stroke=0x%08x Fill=0x%08x Cutoff=[%g %g %g]",
                                   uniforms.texture, uniforms.strokeColor, uniforms.color,
                                   uniforms.insideCutoff, uniforms.outsideCutoff, uniforms.halfStrokeWidth);
            break;
        }
        case DrawCurrentTexture: {
            const DrawCurrentTextureUniforms& uniforms = uniformsAt<DrawCurrentTextureUniforms>(i);
            snprintf(context, len, "Blur Tex=0x%p opacity=%g amount=%g width=%g height=%g vectorx=%g vectory=%g",
                                   uniforms.texture, uniforms.opacity, uniforms.blurAmount, uniforms.blurSizeWidth,
                                   uniforms.blurSizeHeight, uniforms.blurVectorX, uniforms.blurVectorY);
            break;
        }
        case DrawSkShader: {
            const DrawSkShaderUniforms& uniforms = uniformsAt<DrawSkShaderUniforms>(i);
            snprintf(context, len, "Tiling=[%d %d]", uniforms.tileMode[0], uniforms.tileMode[1]);
            break;
        }
        case DrawTexture: {
            const DrawTextureUniforms& uniforms = uniformsAt<DrawTextureUniforms>(i);
            snprintf(context, len, "GrTexture=0x%p glTexId=%zd", uniforms.texture, uniforms.texture ? uniforms.texture->getTextureHandle() : 0);
            break;
        }
        case DrawColor:
        case DrawOpaqueColor:
        case DrawArc:
        case BeginTransparencyLayer:
        case EndTransparencyLayer:
            snprintf(context, len, " ");
            break;
        }

        SkDebugf("%5d %24s %c%c %5uv %s\n", i, dlcCommand(c), dlcVariant(c), dlcTriangles(c), fCommands[i].vertexCount, context);
        if (dataLimit && size)
            dumpVertexAttributes(i, dataLimit);
    }
}

void InstructionSet::dumpVertexAttributes(unsigned cursor, unsigned dataLimit) const
{
    const unsigned currentDataIndex = fCommands[cursor].vertexAttributeIndex;
    SkASSERT(fBuffChain->size() > currentDataIndex);

    const InstructionSetData* vertexAttributes = &vertexAttributeData()[currentDataIndex];
    const unsigned nextDataIndex = (cursor + 1 < fCommands.size()) ? fCommands[cursor + 1].vertexAttributeIndex : fBuffChain->size();
    SkASSERT(currentDataIndex <= nextDataIndex);

    const int size = std::min(nextDataIndex - currentDataIndex, dataLimit);
    for (int i = 0; i < size; ++i)
        SkDebugf("data[%d : %d]:   0x%08x  %g\n", currentDataIndex + i, i, vertexAttributes[i].u, vertexAttributes[i].f);
}

} // namespace Gentl
