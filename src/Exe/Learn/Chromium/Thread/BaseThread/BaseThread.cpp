/************************************************************************/
/*@File Name         : BaseThread.cpp
/*@Created Date      : 2024/1/5 10:50
/*@Author            : lealc
/*@Description       :
/************************************************************************/
#include <iostream>
#include "base/at_exit.h"
#include "base/threading/thread.h"
#include "base/synchronization/waitable_event.h"
#include "SleepInsideInitThread.h"
#include "CaptureToEventList.h"
#include "base/bind.h"
#include "base/callback.h"
#include "PlatformThread.h"
#include "base/synchronization/lock.h"
#include <mutex>
#include "base/bind.h"
#include "base/logging.h"

void ToggleValue(bool* value) {
    *value = !*value;
}

base::Lock lock;
void increment(int* data) {
    //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, *data);
    lock.Acquire(); // 在进入临界区之前获取锁，并在退出临界区时自动释放锁
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(500));
    // 临界区代码
    (*data) = *data + 1;
    //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, *data);
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(500));
    lock.Release(); // 解锁
}

void increment2(int* data) {
    //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, *data);
    {
        // 一般AutoLock配合大括号使用更好
        base::AutoLock auto_lock(lock);
        base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(500));
        // 临界区代码
        (*data) = *data + 1;
        //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, *data);
        base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(500));
    }
}

void AtExitTask(void* param) {
    //L_TRACE(L"%s %d", __FUNCTIONW__, *static_cast<int*>(param));
}

int main() {
    {
        // 延迟初始化的线程
        SleepInsideInitThread t;
        //L_TRACE(L"%s InitCalled = [%d]", __FUNCTIONW__, t.InitCalled());
        t.StartAndWaitForTesting();
        //L_TRACE(L"%s InitCalled = [%d]", __FUNCTIONW__, t.InitCalled());
    }
    {
        // 观测线程的各个事件
        EventList captured_events;
        CapturingDestructionObserver loop_destruction_observer(&captured_events);

        {
            // Start a thread which writes its event into |captured_events|.
            CaptureToEventList t(&captured_events);
            t.Start();
            //L_TRACE(L"%s running=[%d]", __FUNCTIONW__, t.IsRunning());

            // Register an observer that writes into |captured_events| once the
            // thread's message loop is destroyed.
            t.task_runner()->PostTask(
                FROM_HERE,
                base::BindOnce(&RegisterDestructionObserver,
                    base::Unretained(&loop_destruction_observer)));

            // Upon leaving this scope, the thread is deleted.
        }

        // Check the order of events during shutdown.
        //L_TRACE(L"%s size=[%d]", __FUNCTIONW__, captured_events.size());
        //L_TRACE(L"%s captured_events[0]=[%d]", __FUNCTIONW__, captured_events[0]);
        //L_TRACE(L"%s captured_events[1]=[%d]", __FUNCTIONW__, captured_events[1]);
        //L_TRACE(L"%s captured_events[2]=[%d]", __FUNCTIONW__, captured_events[2]);
    }

    {
        base::Thread a("StartWithStackSize");
        // Ensure that the thread can work with only 12 kb and still process a
        // message. At the same time, we should scale with the bitness of the system
        // where 12 kb is definitely not enough.
        // 12 kb = 3072 Slots on a 32-bit system, so we'll scale based off of that.
        base::Thread::Options options;
        options.stack_size = 3072 * sizeof(uintptr_t);
        a.StartWithOptions(options);

        base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED);
        a.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&base::WaitableEvent::Signal, base::Unretained(&event)));
        //L_TRACE(L"%s event begin wait", __FUNCTIONW__);
        event.Wait();
        //L_TRACE(L"%s event trigger", __FUNCTIONW__);
    }

    {
        base::Thread* a = new base::Thread("StartNonJoinable");
        // 非可连接线程目前必须泄漏（参见 Thread::Options::joinable 的详细信息）。

        base::Thread::Options options;
        options.joinable = false;
        a->StartWithOptions(options);
        //L_TRACE(L"%s IsRunning=[%d]", __FUNCTIONW__, a->IsRunning());

        // 如果没有这个调用，这个测试就会有竞态条件。上面的 IsRunning() 成功是因为在 Start() 和 StopSoon() 之间有一个提前返回的条件，
        // 在调用 StopSoon() 之后，这个提前返回的条件不再满足，必须检查真正的 |is_running_| 位。
        // 如果消息循环实际上还没有真正启动，它仍然可能为 false。
        // 这只是这个测试的要求，因为非可连接属性强制它使用 StopSoon() 而不是等待完全的 Stop()。
        a->WaitUntilThreadStarted();

        // Make the thread block until |block_event| is signaled.
        base::WaitableEvent block_event(
            base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED);

        // 卡住这个线程
        a->task_runner()->PostTask(FROM_HERE,
            base::BindOnce(&base::WaitableEvent::Wait,
                base::Unretained(&block_event)));

        // 在调用 StopSoon() 之后，这个提前返回的条件不再满足，必须检查真正的 | is_running_ | 位。
        a->StopSoon();
        //L_TRACE(L"%s IsRunning=[%d]", __FUNCTIONW__, a->IsRunning());

        // 解除任务的阻塞，并给予一些额外的时间来解开 QuitWhenIdle()。
        block_event.Signal();
        base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(20));

        // 线程现在应该已经自行停止。
        //L_TRACE(L"%s IsRunning=[%d]", __FUNCTIONW__, a->IsRunning());
    }

    {
        bool was_invoked = false;
        {

            base::Thread a("TwoTasksOnJoinableThread");
            a.Start();

            // Test that all events are dispatched before the Thread object is
            // destroyed.  We do this by dispatching a sleep event before the
            // event that will toggle our sentinel value.
            a.task_runner()->PostTask(
                FROM_HERE, base::BindOnce(static_cast<void (*)(base::TimeDelta)>(
                    &base::PlatformThread::Sleep),
                    base::TimeDelta::FromMilliseconds(20)));
            a.task_runner()->PostTask(FROM_HERE,
                base::BindOnce(&ToggleValue, &was_invoked));
        }
        //L_TRACE(L"%s was_invoked=[%d]", __FUNCTIONW__, was_invoked);
    }
    {
        std::unique_ptr<base::Thread> a =
            std::make_unique<base::Thread>("TransferOwnershipAndStop");
        a->StartAndWaitForTesting();
        //L_TRACE(L"%s a->IsRunning()=[%d]", __FUNCTIONW__, a->IsRunning());

        base::Thread b("TakingOwnershipThread");
        b.Start();

        base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
            base::WaitableEvent::InitialState::NOT_SIGNALED);

        // a->DetachFromSequence() should allow |b| to use |a|'s Thread API.
        a->DetachFromSequence();
        b.task_runner()->PostTask(
            FROM_HERE, base::BindOnce(
                [](std::unique_ptr<base::Thread> thread_to_stop,
                    base::WaitableEvent* event_to_signal) -> void {
                        thread_to_stop->Stop();
                        event_to_signal->Signal();
                },
                std::move(a), base::Unretained(&event)));
        //L_TRACE(L"%s event begin wait", __FUNCTIONW__);
        event.Wait();
        //L_TRACE(L"%s event trigger", __FUNCTIONW__);
    }
    {
        base::Thread a("ThreadIdWithRestart");
        base::PlatformThreadId previous_id = base::kInvalidThreadId;

        for (size_t i = 0; i < 5; ++i) {
            a.Start();
            base::PlatformThreadId current_id = a.GetThreadId();
            //L_TRACE(L"%s previous_id=[%d] current_id=[%d]", __FUNCTIONW__, previous_id, current_id);
            previous_id = current_id;
            a.Stop();
        }
    }
    {
        std::string str = "fff123123123";
        base::Thread a(str);

        // 线程未启动，无影响，不执行任何操作
        a.FlushForTesting();

        a.Start();

        // 线程无Task，无影响，不执行任何操作
        a.FlushForTesting();

        constexpr base::TimeDelta kSleepPerTestTask =
            base::TimeDelta::FromMilliseconds(50);
        constexpr size_t kNumSleepTasks = 5;

        const base::TimeTicks ticks_before_post = base::TimeTicks::Now();

        for (size_t i = 0; i < kNumSleepTasks; ++i) {
            a.task_runner()->PostTask(
                FROM_HERE,
                base::BindOnce(&base::PlatformThread::Sleep, kSleepPerTestTask));
        }

        // 会阻塞，等待线程中所有Task完成
        a.FlushForTesting();

        a.Stop();

        // 线程已停止，无影响，不执行任何操作
        a.FlushForTesting();
    }

    {
        TrivialThread thread;
        base::PlatformThreadHandle handle;
        base::PlatformThread::Create(0, &thread, &handle);
        base::PlatformThread::Detach(handle);
        thread.run_event().Wait();
    }
    {
        base::Thread t("TestPlatformThread");
        t.Start();
        // 让线程执行500ms
        t.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&base::PlatformThread::Sleep, base::TimeDelta::FromMilliseconds(500)));

        t.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce([]() {
                //L_TRACE("GetCurrentThreadPriority = [%d]", base::PlatformThread::GetCurrentThreadPriority());
                base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
                //L_TRACE("GetName = [%s]", base::PlatformThread::GetName());
                }));

        //L_TRACE(L"%s thread wait", __FUNCTIONW__);
        t.FlushForTesting();
        //L_TRACE(L"%s thread end", __FUNCTIONW__);
        t.Stop();
        //L_TRACE(L"%s thread running= [%d]", __FUNCTIONW__, t.IsRunning());
    }

    {
        int sharedData = 0; // 共享资源

        base::Thread t1("测试线程1");
        base::Thread t2("Thread 2");
        t1.Start();
        t2.Start();

        t1.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&increment, &sharedData));
        t2.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&increment, &sharedData));

        t1.Stop();
        t2.Stop();
        //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, sharedData);
    }

    {
        int sharedData = 0; // 共享资源

        base::Thread t1("AutoLock 1");
        base::Thread t2("AutoLock 2");
        t1.Start();
        t2.Start();

        t1.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&increment2, &sharedData));
        t2.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(&increment2, &sharedData));

        t1.Stop();
        t2.Stop();
        //L_TRACE(L"%s sharedData = [%d]", __FUNCTIONW__, sharedData);
    }

    {
        base::AtExitManager at_exit;
        int i = 1;
        at_exit.RegisterCallback(&AtExitTask, &i);
        at_exit.RegisterCallback(&AtExitTask, &i);
        at_exit.ProcessCallbacksNow();
    }
}
