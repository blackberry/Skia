/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlInstructionSet_h
#define GentlInstructionSet_h

#include "Clip.h"

#include "BuffChain.h"

#include <vector>

class GrTexture;

namespace Gentl {

struct InstructionSetCommand {
    DisplayListCommand command;
    unsigned uniformIndex;
    unsigned vertexAttributeIndex;
    unsigned vertexCount;

    InstructionSetCommand(DisplayListCommand, unsigned uIndex, unsigned vaIndex);
};

union InstructionSetData {
    float f;
    unsigned u;

    InstructionSetData() : u(0) { }
    InstructionSetData(float inFloat) : f(inFloat) { }
    InstructionSetData(unsigned inUnsigned) : u(inUnsigned) { }
    bool operator==(const InstructionSetData& rhs) const { return this->u == rhs.u; }
};

class InstructionSet {
public:
    InstructionSet();
    ~InstructionSet();

    inline void growIfNeeded(int more) { fBuffChain->makeRoom(more); }

    // InstructionSets can receive new commands either through calling
    // pushCommand directly, or by absorbing them from other InstructionSets.
    void push(DisplayListCommand);
    void makeNoOp(unsigned index) { fCommands[index].command = NoOp; }
    void transferFrom(InstructionSet&);
    void transferFrom(InstructionSet&, unsigned size);
    void swapWith(InstructionSet&);

    // Push data onto the end of the data buffer to go with the last pushed command.
    template <class T>
    inline void pushUniforms(const T& uniforms) { pushUniformData(&uniforms, sizeof(T)); }
    void pushUniformData(const void*, size_t bytes);

    inline void pushVertexAttribute(InstructionSetData attributeValue) { fBuffChain->append(attributeValue); }
    void pushAndPackCoords(float, float);

    // Increment the vertex counter in the uniform buffer for the most recent
    // command. Each time another vertex is added for a command this must be called.
    void incrementVertexCount();
    size_t currentVertexCount() const { return fCommands.back().vertexCount; }

    // Deref image textures contained in the data and resize or clear buffers.
    void unrefAndResize(unsigned firstCommand);
    void unrefAndClear();

    bool isEmpty() const;
    unsigned commandSize() const;
    unsigned dataBytes() const;

    const InstructionSetCommand& operator[](unsigned index) const { return fCommands[index]; }

    template <class T>
    inline const T& uniformsAt(unsigned index) const
    {
        SkASSERT(fCommands[index].command.id() == T::Id);
        return reinterpret_cast<const T&>(fUniforms[fCommands[index].uniformIndex]);
    }

    template <class T>
    void setUniforms(const T& uniforms, unsigned index)
    {
        SkASSERT(fCommands[index].command.id() == T::Id);
        memcpy(&fUniforms[fCommands[index].uniformIndex], &uniforms, sizeof(T));
    }

    const InstructionSetData* vertexAttributeData() const { return fBuffChain->squash(); }

    DisplayListCommand lastCommand() const;
    const uint32_t* lastUniforms() const;

    template<class T>
    void accessVertexAttributes(T& accessor) const { fBuffChain->accessChunks(accessor); }
    void destroyVertexAttributes() { fBuffChain->clear(); }
    void dump(unsigned cursorFirst = 0, unsigned cursorLast = static_cast<unsigned>(-1), unsigned dataLimit = 0) const;

private:
    InstructionSet(const InstructionSet&);
    InstructionSet& operator=(const InstructionSet&);

    void unref(unsigned beginIndex);
    void unref(unsigned beginIndex, unsigned endIndex);
    void clear();

    // Debugging utilities.
    enum ValidationRequirement { MustBeComplete, CanBeIncomplete, };
    void validate(ValidationRequirement) const;
    void dumpVertexAttributes(unsigned cursor, unsigned dataLimit) const;

private:
    std::vector<InstructionSetCommand> fCommands;
    std::vector<uint32_t> fUniforms;

    BuffChain<InstructionSetData>* fBuffChain;
};

} // namespace Gentl

#endif // GentlSections_h
