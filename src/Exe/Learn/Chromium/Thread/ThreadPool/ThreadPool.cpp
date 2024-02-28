/************************************************************************/
/*@File Name         : ThreadPool.cpp
/*@Created Date      : 2024/1/9 17:29
/*@Author            : lealc
/*@Description       :
/************************************************************************/
#include <iostream>
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/worker_thread_observer.h"
#include "base/task/task_traits.h"
#include "base/bind.h"
#include "base/time/time.h"

#define kMaxNumForegroundThreads 4;

class MyObserver : public base::WorkerThreadObserver {
public:
    // Invoked at the beginning of the main function of a ThreadPool worker,
    // before any task runs.
    void OnWorkerThreadMainEntry() override {
        //L_TRACE(L"%s", __FUNCTIONW__);
    }

    // Invoked at the end of the main function of a ThreadPool worker, when it
    // can no longer run tasks.
    void OnWorkerThreadMainExit() override {
        //L_TRACE(L"%s", __FUNCTIONW__);
    };
};

int main() {
    {
        // 测试ThreadPoolImpl
        int max_num_foreground_threads = kMaxNumForegroundThreads;
        base::ThreadPoolInstance::InitParams init_params(max_num_foreground_threads);

        std::unique_ptr<base::internal::ThreadPoolImpl> thread_pool_ = std::make_unique<base::internal::ThreadPoolImpl>("Test Thread Pool");

        // Register a worker thread observer.
        MyObserver observer;

        thread_pool_->Start(init_params, &observer);
        base::TaskTraits traits;
        for (int i = 0; i < 10; ++i) {
            thread_pool_->PostDelayedTask(FROM_HERE, traits, base::BindOnce(static_cast<void (*)(base::TimeDelta)>(
                &base::PlatformThread::Sleep),
                base::TimeDelta::FromMilliseconds(20)), base::TimeDelta::FromMilliseconds(20));
        }
        //L_TRACE(L"%s", __FUNCTIONW__);

        thread_pool_->FlushForTesting();
        thread_pool_->JoinForTesting();

        //L_TRACE(L"%s", __FUNCTIONW__);
    }
}