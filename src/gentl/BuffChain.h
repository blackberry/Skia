/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BuffChain_h
#define BuffChain_h

#include "SkTypes.h"

namespace Gentl {

struct BuffChunk {
    ~BuffChunk() { free(data); }

    template <typename T>
    static BuffChunk* makeForType(unsigned capacity) {
        return new BuffChunk(capacity * sizeof(T));
    }

    template <typename T>
    inline T* dataAsType() { return reinterpret_cast<T*>(data); }

    template <typename T>
    unsigned capacityAsType() { return size / sizeof(T); }

    enum { DefaultChunkSize = 1024 * 16 }; // in bytes
    BuffChunk* next;
    unsigned pos;

private:
    BuffChunk(unsigned size) : next(0), pos(0), size(size), data(malloc(size)) { }
    unsigned size; // in bytes
    void* data;
};

class BuffChunkCache {
public:
    static BuffChunkCache* instance();

    BuffChunk* fromCache()
    {
        if (fHead) {
            fSize -= BuffChunk::DefaultChunkSize;
            BuffChunk* buff = fHead;
            fHead = fHead->next;
            buff->next = 0;
            buff->pos = 0;
            return buff;
        } else {
            return BuffChunk::makeForType<char>(BuffChunk::DefaultChunkSize);
        }
    }

    void toCache(BuffChunk* chunk)
    {
        if (chunk->capacityAsType<char>() != BuffChunk::DefaultChunkSize || fSize >= MaxCacheSize) {
            delete chunk;
            return;
        }
        fSize += BuffChunk::DefaultChunkSize;
        chunk->next = fHead;
        fHead = chunk;
    }

private:
    BuffChunkCache()
        : fHead(0)
        , fSize(0)
    { }

    enum { MaxCacheSize = 1024 * 1024 * 2 }; // two MB
    BuffChunk* fHead;
    unsigned fSize;
};

template <typename T>
class BuffChain {
public:
    // Starts with a BuffChunk of default size
    BuffChain()
        : fBuffChunkCache(BuffChunkCache::instance())
        , fTail(0)
        , fChainedSize(0)
    {
    }

    ~BuffChain() {
        clear();
        fBuffChunkCache = 0;
    }

    // returns the total number of assigned InstructionSetDatas in the chain
    unsigned size() const
    {
        return fTail ? fChainedSize + fTail->pos : 0;
    }

    // releases all of the BuffChunks in the chain
    void clear()
    {
        clearList(breakCycle());
    }

    // appends the BuffChunk chain of other and takes ownership of them, buffChain is now empty. Size increases by buffChain->size()
    void adopt(BuffChain<T>* other) {
        if (!other->fTail) {
            return;
        }
        fChainedSize = size() + other->fChainedSize;
        if (fTail) {
            BuffChunk* currentHead = head();
            fTail->next = other->head();
            other->fTail->next = currentHead;
        }
        fTail = other->fTail;
        other->fTail = 0;
        other->fChainedSize = 0;
    }

    // appends data to the tail BuffChunk, size increases by one
    inline void append(T data)
    {
        fTail->dataAsType<T>()[fTail->pos++] = data;
    }

    // allocates a new BuffChunk if amount exceeds the free space available in the tail BuffChunk
    void makeRoom(unsigned amount)
    {
        SkASSERT(amount < BuffChunk::DefaultChunkSize / sizeof(T));

        if (!fTail) {
            fTail = fBuffChunkCache->fromCache();
            fTail->next = fTail;
            return;
        }
        if (fTail->pos + amount >= fTail->capacityAsType<T>()) {
            fChainedSize = size();
            BuffChunk* chunk = fBuffChunkCache->fromCache();
            BuffChunk* headNode = head();
            chunk->next = headNode;
            fTail->next = chunk;
            fTail = chunk;
        }
    }

    // memcpys all of the individual BuffChunks into a single contiguous BuffChunk
    T* squash()
    {
        if (head() == fTail) {
            return fTail->dataAsType<T>();
        }
        BuffChunk* bigChunk = BuffChunk::makeForType<T>(size());
        bigChunk->pos = size();
        BuffChunk* head = breakCycle();
        unsigned pos = 0;
        for (BuffChunk* node = head; node; node = node->next) {
            memcpy(bigChunk->dataAsType<T>() + pos, node->dataAsType<T>(), node->pos * sizeof(T));
            pos += node->pos;
        }
        clearList(head);
        fTail = bigChunk;
        fTail->next = fTail;
        return fTail->dataAsType<T>();
    }

    template<class Accessor>
    void accessChunks(Accessor& accessor) {
        BuffChunk* head = breakCycle();
        for (BuffChunk* node = head; node; node = node->next) {
            accessor.onAccessChunk(node->dataAsType<T>(), node->pos);
        }

        if (fTail) {
            fTail->next = head;
        }
    }

private:
    BuffChunk* head() const
    {
        return fTail->next;
    }
    // detaches the head from tail, returns head
    BuffChunk* breakCycle() const
    {
        if (!fTail) {
            return 0;
        }
        BuffChunk* node = head();
        fTail->next = 0;
        return node;
    }
    // releases BuffChunk list starting at head
    void clearList(BuffChunk* head)
    {
        BuffChunk* prev = head;
        while (prev) {
            head = head ? head->next : 0;
            fBuffChunkCache->toCache(prev);
            prev = head;
        }
        fTail = 0;
        fChainedSize = 0;
    }

private:
    BuffChunkCache* fBuffChunkCache;
    BuffChunk* fTail;
    unsigned fChainedSize;
};

} // namespace Gentl

#endif //BuffChain_h
