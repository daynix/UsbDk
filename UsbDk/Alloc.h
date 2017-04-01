/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
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
*
**********************************************************************/

#pragma once

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
class CScalarDeleter
{
public:
    static void destroy(T *Obj){ delete Obj; }
};

template<typename T>
class CVectorDeleter
{
public:
    static void destroy(T *Obj){ delete[] Obj; }
};

template<POOL_TYPE Pooltype, typename TObject, ULONG Tag>
class CPrimitiveAllocator
{
public:
    static TObject* allocate(size_t NumObjects) { return static_cast<TObject*>(ExAllocatePoolWithTag(Pooltype, sizeof(TObject) * NumObjects, Tag)); }
    static void destroy(TObject *Obj){ if (Obj) ExFreePoolWithTag(Obj, Tag); }
};

template<typename T, typename Deleter = CScalarDeleter<T> >
class CObjHolder
{
public:
    CObjHolder(T *Obj = nullptr)
        : m_Obj(Obj)
    {}

    ~CObjHolder()
    { destroy(); }

    operator bool() const { return m_Obj != nullptr; }
    operator T *() const { return m_Obj; }
    T *operator ->() const { return m_Obj; }

    T *detach()
    {
        auto ptr = m_Obj;
        m_Obj = nullptr;
        return ptr;
    }

    void destroy()
    {
        Deleter::destroy(m_Obj);
    }

    void reset(T *Ptr = nullptr)
    {
        destroy();
        m_Obj = Ptr;
    }

    T* operator= (T *ptr)
    {
      m_Obj = ptr;
      return ptr;
    }

    CObjHolder(const CObjHolder&) = delete;
    CObjHolder& operator= (const CObjHolder&) = delete;

private:
    T *m_Obj = nullptr;
};

template <POOL_TYPE PoolType, ULONG Tag, typename TObject>
class CBufferSet final
{
public:
    explicit CBufferSet(size_t NumEntries)
        : m_NumEntries(NumEntries)
    {}

    bool Create()
    {
        m_Entries = new CBufferHolder[m_NumEntries];
        return m_Entries;
    }

    size_t Size()
    { return m_NumEntries; }

    template <typename TConstructor>
    bool EmplaceEntry(size_t Index, size_t NumObjects, TConstructor EntryConstructor)
    {
        ASSERT(Index < m_NumEntries);

        m_Entries[Index] = TAllocator::allocate(NumObjects);
        m_Entries[Index].m_NumObjects = NumObjects;
        return (m_Entries[Index] != nullptr) ? EntryConstructor(m_Entries[Index]) : false;
    }

    void CopyEntry(size_t Index, PVOID Buffer, size_t NumObjects)
    {
        ASSERT(Index < m_NumEntries);
        RtlCopyBytes(Buffer, m_Entries[Index], min(NumObjects, m_Entries[Index].m_NumObjects) * sizeof(TObject));
    }

    CBufferSet(CBufferSet<PoolType, Tag, TObject> &Other)
    {
        *this = Other;
    }

    CBufferSet& operator= (CBufferSet<PoolType, Tag, TObject> &Other)
    {
        m_NumEntries = Other.m_NumEntries;
        m_Entries = Other.m_Entries.detach();
        return *this;
    }

private:
    size_t m_NumEntries;

    using TAllocator = CPrimitiveAllocator < PoolType, TObject, Tag > ;
    struct CBufferHolder : public CObjHolder<TObject, TAllocator>, public CAllocatable<PoolType, Tag>
    {
        size_t m_NumObjects = 0;

        TObject* operator= (TObject *Ptr)
        {
            return *static_cast< CObjHolder<TObject, TAllocator>* > (this) = Ptr;
        }
    };

    CObjHolder<CBufferHolder, CVectorDeleter<CBufferHolder> > m_Entries;
};

template<typename T>
class CRefCountingHolder : public CAllocatable < USBDK_NON_PAGED_POOL, 'CRHR'>
{
public:
    typedef void(*TDeleteFunc)(T *Obj);

    CRefCountingHolder(TDeleteFunc DeleteFunc = [](T *Obj){ delete Obj; })
        : m_DeleteFunc(DeleteFunc)
    {};

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
        if ((InterlockedDecrement(&m_RefCount) == 0) && (m_Obj != nullptr))
        {
            m_DeleteFunc(m_Obj);
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
    TDeleteFunc m_DeleteFunc;
};
