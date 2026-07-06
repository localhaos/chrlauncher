#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#include "DataTypes.h"
#include <stddef.h>

class Allocator
{
public:
    Allocator(size_t size, UINT objects=0, CHAR* memory = NULL, const CHAR* name=NULL);
    ~Allocator();

    void* Allocate(size_t size);
    void Deallocate(void* pBlock);

    const CHAR* GetName() { return m_name; }
    size_t GetBlockSize() { return m_blockSize; }
    UINT GetBlockCount() { return m_blockCnt; }
    UINT GetBlocksInUse() { return m_blocksInUse; }
    UINT GetAllocations() { return m_allocations; }
    UINT GetDeallocations() { return m_deallocations; }

private:
    void Push(void* pMemory);
    void* Pop();

    struct Block
    {
        Block* pNext;
    };

	enum AllocatorMode { HEAP_BLOCKS, HEAP_POOL, STATIC_POOL };

    const size_t m_blockSize;
    const size_t m_objectSize;
    const UINT m_maxObjects;
	AllocatorMode m_allocatorMode;
    Block* m_pHead;
    CHAR* m_pPool;
    UINT m_poolIndex;
    UINT m_blockCnt;
    UINT m_blocksInUse;
    UINT m_allocations;
    UINT m_deallocations;
    const CHAR* m_name;
};

template <class T, UINT Objects>
class AllocatorPool : public Allocator
{
public:
	AllocatorPool() : Allocator(sizeof(T), Objects, m_memory)
	{
	}
private:
	CHAR m_memory[sizeof(T) * Objects];
};

#define DECLARE_ALLOCATOR \
    public: \
        void* operator new(size_t size) { \
            return _allocator.Allocate(size); \
        } \
        void operator delete(void* pObject) { \
            _allocator.Deallocate(pObject); \
        } \
    private: \
        static Allocator _allocator;

#define IMPLEMENT_ALLOCATOR(class, objects, memory) \
	Allocator class::_allocator(sizeof(class), objects, memory, #class);

#endif
