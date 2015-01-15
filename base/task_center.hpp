#ifndef __base_task_center_hpp__
#define __base_task_center_hpp__

#include "base/task_center.h"
#include "base/time_ticks.h"

#include <iostream>

namespace base
{
    template<template<typename Processor> class Pump>
    TaskCenter<Pump>::TaskCenter()
        : run_state_(STATE_DEFAULT) {}

    template<template<typename Processor> class Pump>
    TaskCenter<Pump>::~TaskCenter()
    {
        DiscardTasks();
        DiscardDelayTasks();
    }

    template<template<typename Processor> class Pump>
    int TaskCenter<Pump>::Run()
    {
        if (GetState() != STATE_DEFAULT)
        {
            return 0;
        }

        SetState(STATE_RUNNING);

        int code = pump_.Run(this);

        SetState(STATE_STOPED);

        return code;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::Quit(int code)
    {
        if (GetState() == STATE_RUNNING)
        {
            pump_.Quit(code);
        }

        return true;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::PostTask(Task* task)
    {
        if (GetState() == STATE_STOPED)
        {
            return false;
        }

        if (AddToTaskQueue(task))
        {
            pump_.ScheduleTask();
            return true;
        }

        return false;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::PostDelayTask(Task* task, int delay_time)
    {
        if (GetState() == STATE_STOPED)
        {
            return false;
        }

        if (AddToDelayTaskQueue(task, delay_time))
        {
            pump_.ScheduleDelayTask();
            return true;
        }

        return false;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::AddToTaskQueue(Task* task)
    {
        if (!task)
        {
            return false;
        }

        AutoLocker<CSLocker> guard(&locker_);
        task_queue_.push(PendingTask(task,0));
        return true;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::AddToDelayTaskQueue(Task* task, int delay_time)
    {
        if (!task)
        {
            return false;
        }

        AutoLocker<CSLocker> guard(&locker_);
        delay_task_queue_.push(PendingTask(task,delay_time));
        return true;
    }

    template<template<typename Processor> class Pump>
    Task* TaskCenter<Pump>::GetNextDelayTask()
    {
        AutoLocker<CSLocker> guard(&locker_);
        if (delay_task_queue_.empty())
            return 0;

        PendingTask pending_task = delay_task_queue_.top();
        if (pending_task.time_post_ + pending_task.time_delay_ < TimeTicks::Now())
        {
            delay_task_queue_.pop();
            return pending_task.task_;
        }

        return 0;
    }

    template<template<typename Processor> class Pump>
    int TaskCenter<Pump>::GetNextDelayTime()
    {
        AutoLocker<CSLocker> guard(&locker_);
        if (delay_task_queue_.empty())
            return 0;

        PendingTask pending_task = delay_task_queue_.top();
        return pending_task.time_delay_;
    }

    template<template<typename Processor> class Pump>
    LONG TaskCenter<Pump>::GetState()
    {
        return InterlockedExchangeAdd(&run_state_, 0L);
    }

    template<template<typename Processor> class Pump>
    void TaskCenter<Pump>::SetState(LONG state)
    {
        InterlockedExchange(&run_state_, state);
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::DiscardTasks()
    {
        if (GetState() == STATE_RUNNING)
        {
            return false;
        }

        AutoLocker<CSLocker> guard(&locker_);
        while (!task_queue_.empty())
        {
            PendingTask pending_task = task_queue_.front();
            task_queue_.pop();

            delete pending_task.task_;
        }

        return true;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::DiscardDelayTasks()
    {
        if (GetState() == STATE_RUNNING)
        {
            return false;
        }

        AutoLocker<CSLocker> guard(&locker_);
        while (!delay_task_queue_.empty())
        {
            PendingTask pending_task = delay_task_queue_.top();
            delay_task_queue_.pop();

            delete pending_task.task_;
        }

        return true;
    }

    template<template<typename Processor> class Pump>
    void TaskCenter<Pump>::RunTask(Task* task)
    {
        if (task)
        {
            task->Run();
        }

        delete task;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::DoTask()
    {
        bool has_more_task = false;
        {
            AutoLocker<CSLocker> guard(&locker_);
            has_more_task = !task_queue_.empty();
        }

        if (!has_more_task)
        {
            return false;
        }

        PendingTask pending_task;
        {
            AutoLocker<CSLocker> guard(&locker_);
            pending_task = task_queue_.front();
            task_queue_.pop();
        }
        RunTask(pending_task.task_);

        {
            AutoLocker<CSLocker> guard(&locker_);
            has_more_task = !task_queue_.empty();
        }
        return has_more_task;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::DoDelayTask(int *next_delay_time)
    {
        do
        {
            Task* task = GetNextDelayTask();
            if (!task)
                break;

            RunTask(task);

        } while (true);

        int delay = GetNextDelayTime();
        if (delay > 0)
        {
            *next_delay_time = delay;
            return true;
        }

        return false;
    }

    template<template<typename Processor> class Pump>
    bool TaskCenter<Pump>::DoIdleTask()
    {
        return false;
    }
}

#endif