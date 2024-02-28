/************************************************************************/
/*@File Name         : PlatformThread.h
/*@Created Date      : 2024/1/5 23:54
/*@Author            : Leal
/*@Description       :
/************************************************************************/
#pragma once
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"

class TrivialThread : public base::PlatformThread::Delegate {
public:
    TrivialThread() : run_event_(base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED) {
    }

    void ThreadMain() override {
        //L_TRACE(L"%s", __FUNCTIONW__);
        run_event_.Signal();
    }

    base::WaitableEvent& run_event() { return run_event_; }

private:
    base::WaitableEvent run_event_;

    DISALLOW_COPY_AND_ASSIGN(TrivialThread);
};
