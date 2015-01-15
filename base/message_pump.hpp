#ifndef __base_message_pump_hpp__
#define __base_message_pump_hpp__

#include "message_pump.h"

namespace base
{
    static const wchar_t kWndClass[] = L"MessagePumpWindow";

    static const int kMsgHaveTask = (WM_USER + 1);

    template<typename Processor>
    MessagePump<Processor>::MessagePump()
        : have_task_(0L)
        , have_delay_task_(0L)
    {
        InitMessageWnd();
    }

    template<typename Processor>
    MessagePump<Processor>::~MessagePump()
    {
        UninitMessageWnd();
    }

    template<typename Processor>
    int MessagePump<Processor>::Run(Processor* processor)
    {
        state_.processor = processor;
        state_.should_quit = false;
        state_.code = 0;

        RunLoop();

        return state_.code;
    }

    template<typename Processor>
    void MessagePump<Processor>::Quit(int code)
    {
        PostQuitMessage(code);
    }

    template<typename Processor>
    bool MessagePump<Processor>::ScheduleTask()
    {
        if (InterlockedExchange(&have_task_, 1L))
        {
            return false;
        }

        PostMessage(message_hwnd_, kMsgHaveTask, reinterpret_cast<WPARAM>(this), 0);
        return true;
    }

    template<typename Processor>
    bool MessagePump<Processor>::ScheduleDelayTask(int time)
    {
        if (InterlockedExchange(&have_delay_task_, 1L))
        {
            return false;
        }

        SetTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this), time, NULL);
        return true;
    }

    template<typename Processor>
    void MessagePump<Processor>::InitMessageWnd()
    {
        HINSTANCE hinst = GetModuleHandle(NULL);

        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProcThunk;
        wc.hInstance = hinst;
        wc.lpszClassName = kWndClass;
        RegisterClassEx(&wc);

        message_hwnd_ = CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hinst, 0);
    }

    template<typename Processor>
    void MessagePump<Processor>::UninitMessageWnd()
    {
        DestroyWindow(message_hwnd_);
        UnregisterClass(kWndClass, GetModuleHandle(NULL));
    }

    template<typename Processor>
    LRESULT CALLBACK MessagePump<Processor>::WndProcThunk(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch(message)
        {
        case kMsgHaveTask:
            reinterpret_cast<MessagePump*>(wparam)->HandleTaskMessage();
            break;

        case WM_TIMER:
            reinterpret_cast<MessagePump*>(wparam)->HandleTimerMessage();
            break;
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    template<typename Processor>
    void MessagePump<Processor>::RunLoop()
    {
        while (true)
        {
            bool more_work = ProcessNextWindowMessage();
            if (state_.should_quit)
            {
                break;
            }

            if (more_work)
            {
                continue;
            }

            more_work |= state_.processor->DoIdleTask();
            if (state_.should_quit)
            {
                break;
            }

            if (more_work)
            {
                continue;
            }

            MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE, QS_ALLINPUT);
        }
    }

    template<typename Processor>
    bool MessagePump<Processor>::ProcessNextWindowMessage()
    {
        bool sent_messages_in_queue = false;
        DWORD queue_status = GetQueueStatus(QS_SENDMESSAGE);
        if (HIWORD(queue_status) & QS_SENDMESSAGE)
        {
            sent_messages_in_queue = true;
        }

        MSG msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            return ProcessWindowMessage(msg);
        }

        return sent_messages_in_queue;
    }

    template<typename Processor>
    bool MessagePump<Processor>::ProcessWindowMessage(const MSG& msg)
    {
        if (msg.message == WM_QUIT)
        {
            state_.should_quit = true;
            state_.code = msg.wParam;
            return false;
        }

        if (msg.message == kMsgHaveTask && msg.hwnd == message_hwnd_)
        {
            InterlockedExchange(&have_task_, 0L);
            return ProcessReplaceMessage(msg);
        }

        DispatchWindowMessage(msg);

        return true;
    }

    template<typename Processor>
    bool MessagePump<Processor>::ProcessReplaceMessage(const MSG& msg)
    {
        MSG next_msg;
        if (!PeekMessage(&next_msg, NULL, 0, 0, PM_REMOVE))
        {
            DispatchWindowMessage(msg);
            return false;
        }

        ScheduleTask();
        return ProcessWindowMessage(next_msg);
    }

    template<typename Processor>
    void MessagePump<Processor>::DispatchWindowMessage(const MSG& msg)
    {
        WillProcessMessage(msg);

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        DidProcessMessage(msg);
    }

    template<typename Processor>
    void MessagePump<Processor>::WillProcessMessage(const MSG& msg)
    {
    }

    template<typename Processor>
    void MessagePump<Processor>::DidProcessMessage(const MSG& msg)
    {
    }

    template<typename Processor>
    void MessagePump<Processor>::HandleTaskMessage()
    {
        bool more_work = state_.processor->DoTask();
        if (more_work)
        {
            ScheduleTask();
        }
    }

    template<typename Processor>
    void MessagePump<Processor>::HandleTimerMessage()
    {
        KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));
        InterlockedExchange(&have_delay_task_, 0L);

        int delay_time = 0;
        bool more_delay_work = state_.processor->DoDelayTask(&delay_time);
        if (more_delay_work)
        {
            ScheduleDelayTask(delay_time);
        }
    }
}

#endif