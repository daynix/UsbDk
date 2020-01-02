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

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define USHORT_MAX ((USHORT)(-1))

class CWdmSpinLock
{
public:
    CWdmSpinLock()
    { KeInitializeSpinLock(&m_Lock); }
    void Lock()
    { KeAcquireSpinLock(&m_Lock, &m_OldIrql); }
    void Unlock()
    { KeReleaseSpinLock(&m_Lock, m_OldIrql); }
private:
    KSPIN_LOCK m_Lock;
    KIRQL m_OldIrql;

    CWdmSpinLock(const CWdmSpinLock&) = delete;
    CWdmSpinLock& operator= (const CWdmSpinLock&) = delete;
};

template <typename T, void (T::*LockFunc)(), void (T::*UnlockFunc)()>
class CBaseLockedContext
{
public:
    CBaseLockedContext(T &LockObject)
        : m_LockObject(LockObject)
    {
        (m_LockObject.*LockFunc)();
    }

    ~CBaseLockedContext()
    {
        (m_LockObject.*UnlockFunc)();
    }

protected:
    T &m_LockObject;
    CBaseLockedContext(const CBaseLockedContext&) = delete;
    CBaseLockedContext& operator= (const CBaseLockedContext&) = delete;
};

template <typename T>
using CLockedContext = CBaseLockedContext <T, &T::Lock, &T::Unlock>;

#if !TARGET_OS_WIN_XP

class CWdmExSpinLock
{
public:
    CWdmExSpinLock()
    {}

    void LockShared()
    {
        m_OldIrql = ExAcquireSpinLockShared(&m_Lock);
    }

    void UnlockShared()
    {
        ExReleaseSpinLockShared(&m_Lock, m_OldIrql);
    }

    void LockExclusive()
    {
        m_OldIrql = ExAcquireSpinLockExclusive(&m_Lock);
    }

    void UnlockExclusive()
    {
        ExReleaseSpinLockExclusive(&m_Lock, m_OldIrql);
    }

private:
    EX_SPIN_LOCK m_Lock = {};
    KIRQL m_OldIrql;

    CWdmExSpinLock(const CWdmExSpinLock&) = delete;
    CWdmExSpinLock& operator= (const CWdmExSpinLock&) = delete;
};

#else //!TARGET_OS_WIN_XP

//EX_SPIN_LOCK is not supported on Windows XP so we simulate
//it with usual spinlock.

class CWdmExSpinLock : public CWdmSpinLock
{
public:
    void LockShared()
    {
        Lock();
    }

    void UnlockShared()
    {
        Unlock();
    }

    void LockExclusive()
    {
        Lock();
    }

    void UnlockExclusive()
    {
        Unlock();
    }
};

#endif //TARGET_OS_WIN_XP

template <typename T = CWdmExSpinLock>
using CSharedLockedContext = CBaseLockedContext <T, &T::LockShared, &T::UnlockShared>;

template <typename T = CWdmExSpinLock>
using CExclusiveLockedContext = CBaseLockedContext <T, &T::LockExclusive, &T::UnlockExclusive>;

class CAtomicCounter
{
public:
    LONGLONG operator++()
    { return InterlockedIncrement64(&m_Counter); }

    LONGLONG operator++(int)
    { return operator++() - 1; }

    operator LONGLONG() { return m_Counter; }
private:
    volatile LONGLONG m_Counter = 0;
};

class CWdmRefCounter
{
public:
    void AddRef() { InterlockedIncrement(&m_Counter); }
    LONG Release() { return InterlockedDecrement(&m_Counter); }
    operator LONG () { return m_Counter; }
private:
    LONG m_Counter = 0;
};

class CWdmRefCountingObject
{
public:
    CWdmRefCountingObject()
    {
        AddRef();
    }

    void AddRef()
    {
        m_RefCounter.AddRef();
    }

    void Release()
    {
        if (m_RefCounter.Release() == 0)
        {
            OnLastReferenceGone();
        }
    }

protected:
    virtual void OnLastReferenceGone() = 0;

private:
    CWdmRefCounter m_RefCounter;

    CWdmRefCountingObject(const CWdmRefCountingObject&) = delete;
    CWdmRefCountingObject& operator= (const CWdmRefCountingObject&) = delete;
};

class CRefCountingDeleter
{
public:
    static void destroy(CWdmRefCountingObject *Obj){ if (Obj != nullptr) { Obj->Release(); } }
};

using CLockedAccess = CWdmSpinLock;
class CRawAccess
{
public:
    void Lock() { }
    void Unlock() { }
};

class CCountingObject
{
public:
    void CounterIncrement() { m_Counter++; }
    void CounterDecrement() { m_Counter--; }
    ULONG GetCount() { return m_Counter; }
private:
    ULONG m_Counter = 0;
};

class CNonCountingObject
{
public:
    void CounterIncrement() { }
    void CounterDecrement() { }
protected:
    ULONG GetCount() { return 0; }
};

#define DECLARE_CWDMLIST_ENTRY(type)                                                            \
    private:                                                                                    \
        PLIST_ENTRY GetListEntry()                                                              \
        { return &m_ListEntry; }                                                                \
                                                                                                \
        static type *GetByListEntry(PLIST_ENTRY entry)                                          \
        { return static_cast<type*>(CONTAINING_RECORD(entry, type, m_ListEntry)); }             \
                                                                                                \
        template<typename type, typename AnyAccess, typename AnyStrategy, typename AnyDeleter>  \
        friend class CWdmList;                                                                  \
                                                                                                \
        LIST_ENTRY m_ListEntry

template <typename TEntryType, typename TAccessStrategy, typename TCountingStrategy, typename TDeleter = CScalarDeleter<TEntryType>>
class CWdmList : private TAccessStrategy, public TCountingStrategy
{
public:
    CWdmList()
    { InitializeListHead(&m_List); }

    ~CWdmList()
    { Clear(); }

    void Clear()
    { ForEachDetached([](TEntryType* Entry) { TDeleter::destroy(Entry); return true; }); }

    bool IsEmpty()
    { return IsListEmpty(&m_List) ? true : false; }

    TEntryType *Pop()
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        return Pop_LockLess();
    }

    ULONG Push(TEntryType *Entry)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        InsertHeadList(&m_List, Entry->GetListEntry());
        CounterIncrement();
        return GetCount();
    }

    ULONG PushBack(TEntryType *Entry)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        InsertTailList(&m_List, Entry->GetListEntry());
        CounterIncrement();
        return GetCount();
    }

    void Remove(TEntryType *Entry)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        Remove_LockLess(Entry->GetListEntry());
    }

    template <typename TFunctor>
    bool ForEachDetached(TFunctor Functor)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        while (!IsListEmpty(&m_List))
        {
            if (!Functor(Pop_LockLess()))
            {
                return false;
            }
        }
        return true;
    }

    template <typename TPredicate, typename TFunctor>
    bool ForEachDetachedIf(TPredicate Predicate, TFunctor Functor)
    {
        return ForEachPrepareIf(Predicate, [this](PLIST_ENTRY Entry){ Remove_LockLess(Entry); }, Functor);
    }

    template <typename TFunctor>
    bool ForEach(TFunctor Functor)
    {
        return ForEachPrepareIf([](TEntryType*) { return true; }, [](PLIST_ENTRY){}, Functor);
    }

    template <typename TPredicate, typename TFunctor>
    bool ForEachIf(TPredicate Predicate, TFunctor Functor)
    {
        return ForEachPrepareIf(Predicate, [](PLIST_ENTRY){}, Functor);
    }

    CWdmList(const CWdmList&) = delete;
    CWdmList& operator= (const CWdmList&) = delete;

private:
    template <typename TPredicate, typename TPrepareFunctor, typename TFunctor>
    bool ForEachPrepareIf(TPredicate Predicate, TPrepareFunctor Prepare, TFunctor Functor)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);

        PLIST_ENTRY NextEntry = nullptr;

        for (auto CurrEntry = m_List.Flink; CurrEntry != &m_List; CurrEntry = NextEntry)
        {
            NextEntry = CurrEntry->Flink;
            auto Object = TEntryType::GetByListEntry(CurrEntry);

            if (Predicate(Object))
            {
                Prepare(CurrEntry);
                if (!Functor(Object))
                {
                    return false;
                }
            }
        }

        return true;
    }

    TEntryType *Pop_LockLess()
    {
        CounterDecrement();
        return TEntryType::GetByListEntry(RemoveHeadList(&m_List));
    }

    void Remove_LockLess(PLIST_ENTRY Entry)
    {
        RemoveEntryList(Entry);
        CounterDecrement();
    }

    LIST_ENTRY m_List;
};

static inline bool ConstTrue(...) { return true; }
static inline bool ConstFalse(...) { return false; }

template <typename TEntryType, typename TAccessStrategy, typename TCountingStrategy, typename TDeleter = CScalarDeleter<TEntryType>>
class CWdmSet : private TAccessStrategy, public TCountingStrategy
{
public:
    CWdmSet() {}
    ~CWdmSet() { Clear(); }

    bool Add(TEntryType *NewEntry)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        if (!Contains_LockLess(NewEntry))
        {
            m_Objects.PushBack(NewEntry);
            CounterIncrement();
            return true;
        }

        return false;
    }

    template <typename TEntryId>
    bool Delete(TEntryId *Id)
    {
        //This implementations avoids calling entry destruction
        //under object's lock for two reasons:
        // 1. minimize amount of time the object is locked
        // 2. do not run entry destruction code on DISPATCH_LEVEL
        TEntryType *ToBeRemoved = DetachById(Id);
        TDeleter::destroy(ToBeRemoved);

        return ToBeRemoved != nullptr;
    }

    void Dump()
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        m_Objects.ForEach([](TEntryType *Entry) { Entry->Dump(); return true; });
    }

    template <typename TEntryId>
    bool Contains(TEntryId *Id)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        return Contains_LockLess(Id);
    }

    template <typename TEntryId, typename TModifier>
    bool ModifyOne(TEntryId *Id, TModifier ModifierFunc, ULONG process = 0)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        return !m_Objects.ForEachIf([Id,process](TEntryType *ExistingEntry)
                                    {
                                        bool match = *ExistingEntry == *Id;
                                        if (process && match)
                                        {
                                            match = ExistingEntry->MatchProcess(process);
                                        }
                                        return match;
                                    },
                                    [&ModifierFunc](TEntryType *Entry) { ModifierFunc(Entry); return false; });
    }

    template <typename TFunctor>
    bool ForEach(TFunctor Functor)
    {
        CLockedContext<TAccessStrategy> LockedContext(*this);
        return m_Objects.ForEach(Functor);
    }

private:
    using TInternalList = CWdmList <TEntryType, CRawAccess, CNonCountingObject, TDeleter>;

public:
    void Clear()
    {
        //This implementations avoids calling entry destruction
        //under object's lock for two reasons:
        // 1. minimize amount of time the object is locked
        // 2. do not run entry destruction code on DISPATCH_LEVEL
        TInternalList TempList;
        SwapLists(TempList);
    }

    CWdmSet(const CWdmSet&) = delete;
    CWdmSet& operator= (const CWdmSet&) = delete;
private:
    void SwapLists(TInternalList &OtherList)
    {
        CLockedContext<TAccessStrategy> LockedContextThis(*this);

        m_Objects.ForEachDetached([this, &OtherList](TEntryType *ExistingEntry)
                                  {
                                      OtherList.PushBack(ExistingEntry);
                                      CounterDecrement();
                                      return true;
                                  });
    }

    template <typename TEntryId>
    bool Contains_LockLess(TEntryId *Id)
    {
        auto MatchFound = false;

        m_Objects.ForEachIf([Id](TEntryType *ExistingEntry) { return *ExistingEntry == *Id; },
                            [&MatchFound](TEntryType *) { MatchFound = true; return false; });

        return MatchFound;
    }

    template <typename TEntryId>
    TEntryType *DetachById(TEntryId *Id)
    {
        TEntryType *DetachedEntry = nullptr;
        CLockedContext<TAccessStrategy> LockedContext(*this);

        m_Objects.ForEachDetachedIf([Id](TEntryType *ExistingEntry) { return *ExistingEntry == *Id; },
                                    [this, &DetachedEntry](TEntryType *ExistingEntry)
                                    {
                                        DetachedEntry = ExistingEntry;
                                        CounterDecrement();
                                        return false;
                                    });

        return DetachedEntry;
    }

    TInternalList m_Objects;
};

class CWdmEvent : public CAllocatable<USBDK_NON_PAGED_POOL, 'VEHR'>
{
public:
    CWdmEvent(EVENT_TYPE Type = NotificationEvent, BOOLEAN InitialState = FALSE)
    { KeInitializeEvent(&m_Event, Type, InitialState); };

    NTSTATUS Wait(bool WithTimeout = false, LONGLONG Timeout = 0, bool Alertable = false);

    bool Set(KPRIORITY Increment = IO_NO_INCREMENT, bool Wait = false)
    { return KeSetEvent(&m_Event, Increment, Wait ? TRUE : FALSE) ? true : false; }
    void Clear()
    { KeClearEvent(&m_Event); }
    bool Reset()
    { return KeResetEvent(&m_Event) ? true : false; }

    operator PKEVENT () { return &m_Event; }

    CWdmEvent(const CWdmEvent&) = delete;
    CWdmEvent& operator= (const CWdmEvent&) = delete;
private:
    KEVENT m_Event;
};

class CStringBase
{
public:
    bool operator== (const CStringBase &Other) const
    { return *this == Other.m_String; }

    bool operator== (const UNICODE_STRING& Str) const;

    bool operator== (PCWSTR Other) const
    {
        UNICODE_STRING str;
        if (NT_SUCCESS(RtlUnicodeStringInit(&str, Other)))
        {
            return *this == str;
        }
        return false;
    }

    operator PCUNICODE_STRING() const { return &m_String; };

    NTSTATUS ToString(ULONG Val, ULONG Base)
    { return RtlIntegerToUnicodeString(Val, Base, &m_String); }

    size_t ToWSTR(PWCHAR Buffer, size_t SizeBytes) const;

protected:
    CStringBase(const CStringBase&) = delete;
    CStringBase& operator= (const CStringBase&) = delete;
    CStringBase() {};
    ~CStringBase() {};

    UNICODE_STRING m_String = {};
};
class CStringHolder : public CStringBase
{
public:
    NTSTATUS Attach(NTSTRSAFE_PCWSTR String)
    { return RtlUnicodeStringInit(&m_String, String); }

    NTSTATUS Attach(NTSTRSAFE_PCWSTR String, USHORT SizeInBytes)
    {
        m_String.Length = SizeInBytes;
        m_String.MaximumLength = SizeInBytes;
        m_String.Buffer = const_cast<PWCH>(String);
        return RtlUnicodeStringValidate(&m_String);
    }

    CStringHolder()
    { }

private:
    CStringHolder(const CStringHolder&) = delete;
    CStringHolder& operator= (const CStringHolder&) = delete;
};

class CString : public CStringBase
{
public:
    NTSTATUS Create(NTSTRSAFE_PCWSTR String);
    NTSTATUS Create(PCUNICODE_STRING String);

    template <typename TPrefix, typename TPostfix>
    NTSTATUS Create(const TPrefix &Prefix, const TPostfix &Postfix)
    {
        auto status = Create(Prefix);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        return Append(Postfix);
    }

    NTSTATUS Append(PCUNICODE_STRING String);
    NTSTATUS Append(ULONG Num, ULONG Base = 10);
    void Destroy();

    CString()
    { }

    ~CString()
    { Destroy(); }

private:

    NTSTATUS Resize(USHORT NewLenBytes);

    CString(const CString&) = delete;
    CString& operator= (const CString&) = delete;
};

PVOID DuplicateStaticBuffer(const void *Buffer, SIZE_T Length, POOL_TYPE PoolType = PagedPool);

template<typename T>
class CInstanceCounter
{
public:
    CInstanceCounter()
    {
        static LONG volatile Counter = 0;
        m_Number = InterlockedIncrement(&Counter);
    }

    operator ULONG() const { return static_cast<ULONG>(m_Number); };

private:
    LONG m_Number = 0;

    CInstanceCounter(const CInstanceCounter&) = delete;
    CInstanceCounter& operator= (const CInstanceCounter&) = delete;
};

static inline
LONGLONG SecondsTo100Nanoseconds(LONGLONG Seconds)
{
    return Seconds * 10 * 1000 * 1000;
}

static inline
LONGLONG MillisecondsTo100Nanoseconds(LONGLONG Milliseconds)
{
    return Milliseconds * 10 * 1000;
}

static inline
LONGLONG HundredNsToMilliseconds(LONGLONG HundredNs)
{
    return HundredNs / (10 * 1000);
}

class CStopWatch
{
public:
    CStopWatch() {}
    CStopWatch(const CStopWatch& Other) : CStopWatch() { *this = Other; }
    CStopWatch& operator= (const CStopWatch& Other);

    void Start() { KeQueryTickCount(&m_StartTime); }
    LONGLONG Time100Ns() const;
    LONGLONG TimeMs() const { return HundredNsToMilliseconds(Time100Ns()); }
private:
    const ULONG m_TimeIncrement = KeQueryTimeIncrement();
    LARGE_INTEGER m_StartTime = {};
};

NTSTATUS
UsbDkCreateCurrentProcessHandle(HANDLE &Handle);
