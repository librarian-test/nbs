#pragma once

#include "public.h"

#include "sglist.h"

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

// The TGuardedSgList class allows you to safely terminate access to a memory
// object in a multithreaded environment.
// The owner of the memory, who wants to give access to it to one or more
// clients, must create a new TGuardedSgList, and pass to it a list of memory
// blocks (TSgList). The owner keeps this instance for himself, and gives other
// instances of TGuardedSgList created via Create() or CreateDepender() to
// clients. All these TGuardedSgList instances are linked to each other through
// a common GuardedObject or their hierarchy. The client that needs to access
// memory must access it through the Acquire() call. TGuard is returned to him,
// which guarantees that as long as TGuard exists, the memory will exist. The
// Acquire() call occurs without locks. After receiving the TGuard, it must be
// checked that the owner has not yet stopped accessing it and access has been
// obtained. This is done via operator bool(). Next, you can get a list of
// blocks from TGuard, it is not formally guaranteed that the list of
// blocks is not empty. When the owner of the memory wants to stop accessing it,
// he calls the Destroy() method from his TGuardedSgList instance. The Destroy()
// call blocks the thread in SpinLock until all clients finish access (delete
// their TGuards).

// From TGuardedSgList objects, you can build hierarchies by combining access to
// multiple TGuardedSgList or build chains via CreateDepender(). Under the hood,
// this is achieved through TUnionGuardedObject and TDependentGuardedObject.
// TUnionGuardedObject stores the TGuardedSgList vector, and
// TDependentGuardedObject stores only one. The Acquire() call on such objects
// will be successful if it is possible to set its own lock and the lock of the
// child IGuardedObject. For such objects, after Destroy(), access is
// terminated for objects located at the current level and lower in the
// hierarchy. This is the difference between the behavior of Create() and
// CreateDepender(). In the case of objects constructed via Create(), all
// instances are equivalent and calling Destroy() on any instance blocks access
// for everyone (there is no hierarchy). In the case of CreateDepender(), a
// hierarchy is created and access is blocked in the chain below.

// Usage example:
// auto sharedData = TString(8192, 'a');
// TSgList sglist = {{ sharedData.data(), 4096 }, { &sharedData[4096], 4096 }};
// TGuardedSgList guardedSgList(sglist);
//
// auto copy1 =  guardedSgList; // Simple copy
// auto copy2 =  guardedSgList.Create({sglist[1]}); // Access only to second 4K
// Pass copy1 & copy2 where you need
// {
//   if (auto guard = copy1.Acquire()) {
//     const TSgList& sgList = guard.Get();
//     Here you can safely access the sharedData
//   }
//   if (auto guard = copy2.Acquire()) {
//     const TSgList& sgList = guard.Get();
//     Here you can safely access the sharedData
//   }
// }
// guardedSgList.Destroy(); // Finish access to sharedData
// Here you can safely destroy sharedData, it is guaranteed that no one accesses
// it.

class TGuardedSgList final
{
private:
    struct IGuardedObject
        : public TAtomicRefCount<IGuardedObject>
        , private TMoveOnly
    {
        virtual ~IGuardedObject() = default;

        virtual bool Acquire() = 0;
        virtual void Release() = 0;

        virtual void Close() = 0;
    };

    class TGuardedObject;
    class TDependentGuardedObject;
    class TUnionGuardedObject;

    TIntrusivePtr<IGuardedObject> GuardedObject;
    TSgList Sglist;

public:
    // Creates a union of all sgLists.
    static TGuardedSgList CreateUnion(TVector<TGuardedSgList> sgLists);

    TGuardedSgList();

    explicit TGuardedSgList(TSgList sglist);

    // Creates a new TGuardedSgList that depends on the current one. The
    // connection is one-way, Destroy() called from a new object does not
    // terminate access to the original one.
    TGuardedSgList CreateDepender() const;

    // Creates a new TGuardedSgList that is equal to the current one. A two-way
    // connection is created, Destroy() called on the new object also terminates
    // access to the original one.
    TGuardedSgList Create(TSgList sglist) const;

    // Checks if the sglist is empty.
    bool Empty() const;

    // Sets a new sglist.
    void SetSgList(TSgList sglist);

    class TGuard
    {
    private:
        TIntrusivePtr<IGuardedObject> GuardedObject;
        const TSgList& Sglist;

    public:
        TGuard(TIntrusivePtr<IGuardedObject> guarded, const TSgList& sglist);
        TGuard(TGuard&&) = default;
        TGuard& operator=(TGuard&&) = delete;
        ~TGuard();

        operator bool() const;
        const TSgList& Get() const;
    };

    // Grant acess to memory described by the TSgList.
    // It is necessary to get access for a minimum time.
    // Always check whether access has been obtained.
    // This is non-blocking call.
    TGuard Acquire() const;

    // Terminates memory access of all associated TGuardedSgList on other
    // threads.
    // This call can be blocked until everyone who has previously gained access
    // destroys their TGuard.
    // It is safe to call Destroy() multiple times.
    // Attention! A sequential call to Acquire() and Destroy() on the same
    // thread will result in a deadlock.
    void Destroy();

private:
    TGuardedSgList(TIntrusivePtr<IGuardedObject> guarded, TSgList sglist);
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class TGuardedBuffer
{
private:
    class TImpl;
    TIntrusivePtr<TImpl> Impl;

public:
    TGuardedBuffer() = default;
    explicit TGuardedBuffer(T buffer);

    const T& Get() const;
    T Extract();

    TGuardedSgList GetGuardedSgList() const;
    TGuardedSgList CreateGuardedSgList(TSgList sglist) const;
};

}   // namespace NCloud::NBlockStore

#define BLOCKSTORE_INCLUDE_GUARDED_SGLIST_INL
#include "guarded_sglist_inl.h"
#undef BLOCKSTORE_INCLUDE_GUARDED_SGLIST_INL
