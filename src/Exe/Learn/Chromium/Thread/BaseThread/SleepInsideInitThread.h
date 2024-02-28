/************************************************************************/
// Copyright (c) 2022 Tencent Inc. All rights reserved.
/*@File Name         : CusThread.h
/*@Created Date      : 2024/1/5 12:34
/*@Author            : lealcheng
/*@Description       :
/************************************************************************/
#pragma once
#include "base/threading/thread.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"

class SleepInsideInitThread : public base::Thread {
public:
    SleepInsideInitThread() : base::Thread("none") {
        init_called_ = false;
    }
    ~SleepInsideInitThread() override { Stop(); }

    void Init() override {
        base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(500));
        init_called_ = true;
    }
    bool InitCalled() { return init_called_; }

private:
    bool init_called_;

    DISALLOW_COPY_AND_ASSIGN(SleepInsideInitThread);
};