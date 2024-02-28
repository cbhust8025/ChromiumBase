/************************************************************************/
/*@File Name         : RunLoop.cpp
/*@Created Date      : 2024/1/7 18:18
/*@Author            : lealc
/*@Description       :
/************************************************************************/
#include <iostream>
#include "base/run_loop.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

// 定义RunLoop必须的delegate
class MyRunLoopDelegate : public base::RunLoop::Delegate {
public:
    explicit MyRunLoopDelegate() {}
    ~MyRunLoopDelegate() override = default;

    void Run(bool application_tasks_allowed, base::TimeDelta timeout) override {
        //L_TRACE(L"%s", __FUNCTIONW__);
    };
    void Quit() override {
        //L_TRACE(L"%s", __FUNCTIONW__);
    };

    void EnsureWorkScheduled() override {
        //L_TRACE(L"%s", __FUNCTIONW__);
    };
};

void RegisterRunLoop() {
    // 在线程中创建RunLoop
    // 这里base::Thread已经有了delegate，不需要额外创建自定义的
    // MyRunLoopDelegate runloop_delegate;
    // base::RunLoop::RegisterDelegateForCurrentThread(&runloop_delegate);

    // 等注册了delegate函数后，可以创建run_loop了，如果是base::Thread，则不需要注册
    base::RunLoop run_loop(base::RunLoop::Type::kDefault);

    // 3秒后退出事件循环
    // base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE, base::BindOnce(&base::RunLoop::Quit, base::Unretained(&run_loop)), base::TimeDelta::FromSeconds(3));
    // 这里的延时任务会因为下面的run_loop执行hung住当前线程导致无法执行，所以换成RunWithTimeout，或者另起线程来做退出的操作

    run_loop.RunWithTimeout(base::TimeDelta::FromSeconds(3)); // 这里会hung住当前线程，3秒自动退出事件循环
    //L_TRACE(L"%s begin run loop", __FUNCTIONW__);
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(5));
    //L_TRACE(L"%s after sleep", __FUNCTIONW__);
    run_loop.Quit();
    //L_TRACE(L"%s after quit = [%d]", __FUNCTIONW__, run_loop.running());
}

int main() {
    // 创建SimpleThread
    base::Thread thread("run loop test");
    thread.Start();
    // 线程创建RunLoop
    thread.task_runner()->PostTask(FROM_HERE, base::BindOnce(&RegisterRunLoop));

    // 等待所有任务完成
    thread.FlushForTesting();

    // 停止线程
    thread.Stop();
}
