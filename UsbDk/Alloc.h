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
    typedef void(*TDeleteFunc)(T *Obj);

    CObjHolder(T *Obj = nullptr, TDeleteFunc DeleteFunc = [](T *Obj){ delete Obj; })
        : m_Obj(Obj)
        , m_DeleteFunc(DeleteFunc)
    {}

    ~CObjHolder()
    { if (*this)
      {
        m_DeleteFunc(m_Obj);
      }
    }

    operator bool() const { return m_Obj != nullptr; }
    operator T *() const { return m_Obj; }
    T *operator ->() const { return m_Obj; }

    T *detach()
    {
        auto ptr = m_Obj;
        m_Obj = nullptr;
        return ptr;
    }

    void operator= (T *ptr)
    { m_Obj = ptr; }

    CObjHolder(const CObjHolder&) = delete;
    CObjHolder& operator= (const CObjHolder&) = delete;

private:
    T *m_Obj = nullptr;
    TDeleteFunc m_DeleteFunc;
};

template<typename T>
class CRefCountingHolder : public CAllocatable < NonPagedPool, 'CRHR'>
{
public:
    CRefCountingHolder() {};

    bool InitialAddRef()
    {
        return InterlockedIncrement(&m_RefCount) == 1;
    }

    void AddRef()
    {
        InterlockedIncrement(&m_RefCount);
    }

    void Release()
    {
        if (InterlockedDecrement(&m_RefCount) == 0)
        {
            delete m_Obj;
        }
    }

    operator T *() { return m_Obj; }
    T *Get() { return m_Obj; }
    T *operator ->() { return m_Obj; }

    void operator= (T *ptr)
    {
        m_Obj = ptr;
    }

    CRefCountingHolder(const CRefCountingHolder&) = delete;
    CRefCountingHolder& operator= (const CRefCountingHolder&) = delete;

private:
    T *m_Obj = nullptr;
    LONG m_RefCount = 0;
};
