/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Generators.h"

#include "Clip.h"
#include "InstructionSet.h"

namespace Gentl {

Inserter::Inserter(GentlContext* context, DisplayListCommand type, InsertionTime roundedClipInsertion, const void* uniforms, unsigned size, AdditionalVertexAttributes attributes)
    : fInstructions(context->instructions())
    , fTransparency(context->topLayer())
    , fCommand(type | SoupMode)
    , fGrTexture(0)
    , fAttributes(attributes)
    , fData()
    , fInsertion(Immediate)
    , fUniformData(uniforms)
    , fUniformSize(size)
{
    // Gens that are created once and used multiple times will benefit from Deferred.
    // This will prevent the gen from inserting fInstructions of alternating variant type.
    if (roundedClipInsertion == Deferred && context->currentClip().canCreateRoundedVariants())
        setInsertionTime(Deferred);
}

Inserter::~Inserter()
{
    // Setting to immediate here will now insert any deferred triangles.
    setInsertionTime(Immediate);
}

void Inserter::appendQuad(const Clip::Vertex& v0, const Clip::Vertex& v1, const Clip::Vertex& v2, const Clip::Vertex& v3, const CommandVariant variant)
{
    if (fInsertion == Deferred) {
        fDeferred[variant].makeRoom(16);
        fDeferred[variant].append(v0);
        fDeferred[variant].append(v1);
        fDeferred[variant].append(v2);
        fDeferred[variant].append(v3);
        return;
    }

    fInstructions->growIfNeeded(100);
    // Can only append up to MAX_SHORT vertices into a single command, that's how big the element buffer is.
    if (!fInstructions->commandSize()
        || fInstructions->lastCommand() != (fCommand | variant)
        || (fUniformSize && memcmp(fInstructions->lastUniforms(), fUniformData, fUniformSize))
        || (fInstructions->currentVertexCount() + 4 >= std::numeric_limits<unsigned short>::max()))
    {
        pushCommandAndUniforms(fCommand | variant);
    }

    appendVertex(v0, variant);
    appendVertex(v1, variant);
    appendVertex(v2, variant);
    appendVertex(v3, variant);

    // There is only the vertex data for the 4 vertices above but there are a total of 4 + 2 logical vertices.
    fInstructions->incrementVertexCount();
    fInstructions->incrementVertexCount();

    if (fTransparency) {
        SkPoint quad[4] = { v0.position, v1.position, v2.position, v3.position };
        SkRect rect;
        rect.setBounds(quad, 4);
        fTransparency->dirty(rect);
    }
}

void Inserter::appendTriangle(const Clip::Triangle& t)
{
    appendQuad(t.v[0], t.v[1], t.v[2], t.v[2], t.variant);
}

void Inserter::appendPoints(const std::vector<SkPoint>& points, const SkRect& rect)
{
    pushCommandAndUniforms(fCommand);
    size_t size = points.size();
    for (size_t j = 0; j < size; ++j) {
        fInstructions->growIfNeeded(10);
        appendVertex(Clip::Vertex(points[j].x(), points[j].y()), DefaultVariant);
    }
    if (fTransparency)
        fTransparency->dirty(rect);
}

void Inserter::appendVertex(const Clip::Vertex& v, const CommandVariant variant)
{
    fInstructions->pushVertexAttribute(v.x());
    fInstructions->pushVertexAttribute(v.y());
    if (variant & OuterRoundedVariant) {
        fInstructions->pushVertexAttribute(v.ou());
        fInstructions->pushVertexAttribute(v.ov());
    }
    if (variant & InnerRoundedVariant) {
        fInstructions->pushVertexAttribute(v.iu());
        fInstructions->pushVertexAttribute(v.iv());
    }

    switch (fAttributes) {
    case Texcoords:
        fInstructions->pushVertexAttribute(v.u());
        fInstructions->pushVertexAttribute(v.v());
        break;
    case PackedTexcoords:
        fInstructions->pushAndPackCoords(v.u(), v.v());
        break;
    case OneInstructionSetData:
        fInstructions->pushVertexAttribute(fData);
        break;
    }

    fInstructions->incrementVertexCount();
}

void Inserter::pushCommandAndUniforms(DisplayListCommand command)
{
    fInstructions->push(command);
    if (fUniformSize)
        fInstructions->pushUniformData(fUniformData, fUniformSize);

    if (fGrTexture)
        fGrTexture->ref();
}

class DeferredAccessor {
public:
    DeferredAccessor(Inserter& inserter)
        : fInserter(inserter)
        , fVariant(DefaultVariant)
    { }

    void onAccessChunk(Clip::Vertex* vertices, unsigned count)
    {
        for (unsigned i = 0; i < count; i += 4)
            fInserter.appendQuad(vertices[i], vertices[i + 1], vertices[i + 2], vertices[i + 3], fVariant);
    }

    Inserter& fInserter;
    CommandVariant fVariant;
};

void Inserter::setInsertionTime(InsertionTime insertion)
{
    if (fInsertion == insertion)
        return;

    // Deferred Inserters create a few vectors to categorize and store all of the
    // incoming triangles. When the InsertionTime is returned to Immediate,
    // (either in the destructor or before), the deferred vertices are then processed
    // out of their original order. This has the effect of coalescing the variants
    // which results in a maximum of four shader switches per Inserter instance.
    fInsertion = insertion;
    if (fInsertion == Immediate) {
        DeferredAccessor accessor(*this);
        for (size_t i = 0; i < NumberOfVariants; ++i) {
            accessor.fVariant = static_cast<CommandVariant>(i);
            fDeferred[i].accessChunks(accessor);
            fDeferred[i].clear();
        }
    }
}

}
