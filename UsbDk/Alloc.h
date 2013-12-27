#pragma once

#include <ntddk.h>

template <POOL_TYPE PoolType, ULONG Tag>
class CAllocatable
{
public:
    void* operator new(size_t /* size */, void *ptr) throw()
        { return ptr; }

    void* operator new[](size_t /* size */, void *ptr) throw()
        { return ptr; }

    void* operator new(size_t Size) throw()
        { return ExAllocatePoolWithTag(PoolType, Size, Tag); }

    void* operator new[](size_t Size) throw()
        { return ExAllocatePoolWithTag(PoolType, Size, Tag); }

    void operator delete(void *ptr)
        { if(ptr) { ExFreePoolWithTag(ptr, Tag); } }

    void operator delete[](void *ptr)
        { if(ptr) { ExFreePoolWithTag(ptr, Tag); } }

protected:
    CAllocatable() {};
    ~CAllocatable() {};
};

template<typename T>
class CObjHolder
{
public:
    CObjHolder(T *Obj)
        : m_Obj(Obj)
    {}

    ~CObjHolder()
    { delete m_Obj; }

    operator bool() { return m_Obj != NULL; }
    operator T *() { return m_Obj; }
    T *operator ->() { return m_Obj; }

    T *detach()
    {
        auto ptr = m_Obj;
        m_Obj = NULL;
        return ptr;
    }

    CObjHolder(const CObjHolder&) = delete;
    CObjHolder& operator= (const CObjHolder&) = delete;
private:
    T *m_Obj;
};
