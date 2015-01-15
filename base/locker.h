#ifndef __base_locker_h__
#define __base_locker_h__

#include <windows.h>

namespace base
{
    /*
     * locks
     */
    class CSLocker
    {
    public:
        CSLocker();
        ~CSLocker();

        void Lock();
        void Unlock();

    private:
        CRITICAL_SECTION critical_section_;
    };

    class CSpinLock
    {
    public:
        CSpinLock();
        ~CSpinLock();

        void Lock();
        void Unlock();

    private:
        LONG locked_;
    };


    /*
     * thread guard
     */
    template<typename Locker = CSLocker>
    class MultiThreadGuard
    {
    public:
        void Acquire()
        {
            locker_.Lock();
        }

        void Release()
        {
            locker_.Unlock();
        }

    private:
        Locker locker_;
    };

    template<typename Locker = CSLocker>
    class SingleThreadGuard
    {
    public:
        void Acquire() {}
        void Release() {}
    };


    /*
     * auto guard
     */
    template<typename Locker,
             template <typename> class Guard = MultiThreadGuard>
    class AutoLocker
    {
    public:
        AutoLocker(Guard<Locker>* guard)
            : guard_(guard)
        {
            guard_->Acquire();
        }

        ~AutoLocker()
        {
            guard_->Release();
        }

    private:
        Guard<Locker>* guard_;
    };

    template<typename Locker,
             template <typename> class Guard = MultiThreadGuard>
    class AutoUnlocker
    {
    public:
        AutoUnlocker(Guard<Locker>* guard)
            : guard_(guard)
        {
            guard_->Release();
        }

        ~AutoUnlocker()
        {
            guard_->Acquire();
        }

    private:
        Guard<Locker>* guard_;
    };

}

#endif