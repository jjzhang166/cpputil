#ifndef __base_message_center_h__
#define __base_message_center_h__

#include "base/locker.h"
#include "base/task.h"
#include "base/message_pump.hpp"
#include "base/singleton.h"

#include <queue>

namespace base
{
    template<template<typename Processor> class Pump>
    class TaskCenter
    {
    public:
        template<typename T> friend class MessagePump;

        TaskCenter();
        ~TaskCenter();

        int  Run();
        bool Quit(int code);

        bool PostTask(Task* task);
        bool PostDelayTask(Task* task, int delay_time);

    private:
        bool DoTask();
        bool DoDelayTask(int *next_delay_time);
        bool DoIdleTask();

    private:
        struct PendingTask
        {
            PendingTask() {}
            PendingTask(Task* task, int delay_time)
                : task_(task)
                , time_delay_(delay_time)
                , time_post_(TimeTicks::Now()) {}

            ~PendingTask() {}

            bool operator< (const PendingTask& task) const
            {
                return (time_post_ + time_delay_) < (task.time_delay_ + task.time_post_);
            }

            Task* task_;
            int time_post_;
            int time_delay_;
        };

        bool AddToTaskQueue(Task* task);
        bool AddToDelayTaskQueue(Task* task, int delay_time);
        Task* GetNextDelayTask();
        int   GetNextDelayTime();

        bool DiscardTasks();
        bool DiscardDelayTasks();

        void RunTask(Task* task);

        LONG GetState();
        void SetState(LONG state);

    private:
        MultiThreadGuard<CSLocker>       locker_;
        std::queue<PendingTask>          task_queue_;
        std::priority_queue<PendingTask> delay_task_queue_;

        enum State
        {
            STATE_DEFAULT = 0L,
            STATE_RUNNING = 1L,
            STATE_STOPED  = 2L
        };
        LONG                       run_state_;
        Pump<TaskCenter>           pump_;
    };
}

typedef base::TaskCenter<base::MessagePump> TaskCenterUI;

#endif