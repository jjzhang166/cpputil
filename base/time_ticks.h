#ifndef __base_time_ticks_h__
#define __base_time_ticks_h__

#include <windows.h>

namespace base
{
    class TimeTicks
    {
    public:
        TimeTicks(unsigned long ticks)
            : ticks_(ticks) {}

        ~TimeTicks() {}

        static int Now()
        {
            return (int)::GetTickCount();
        }

    private:
        int ticks_;
    };
}

#endif