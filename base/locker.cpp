#include "locker.h"

namespace base
{
    CSLocker::CSLocker()
    {
        ::InitializeCriticalSection(&critical_section_);
    }

    CSLocker::~CSLocker()
    {
        ::DeleteCriticalSection(&critical_section_);
    }

    void CSLocker::Lock()
    {
        ::EnterCriticalSection(&critical_section_);
    }

    void CSLocker::Unlock()
    {
        ::LeaveCriticalSection(&critical_section_);
    }


    CSpinLock::CSpinLock()
        : locked_(0L) {}

    CSpinLock::~CSpinLock() {}

    void CSpinLock::Lock()
    {
        while (InterlockedExchange(&locked_, 1L) == 1L)
            Sleep(0);
    }

    void CSpinLock::Unlock()
    {
        InterlockedExchange(&locked_, 0L);
    }
}