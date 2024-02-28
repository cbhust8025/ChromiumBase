/************************************************************************/
/*@File Name         : TestCusBaseLib.cpp
/*@Created Date      : 2024/1/1 22:10
/*@Author            : Lealc
/*@Description       :
/************************************************************************/
#include "base/at_exit.h"
#include "base/containers/span.h"
#include "base/task/post_task.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/optional.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool.h"

#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>


static std::atomic_size_t num_tasks_pending_{ 0 };
// static int num_posted_tasks_ = 0;

void ContinuouslyBindAndPostNoOpTasks(size_t num_tasks) {
    scoped_refptr<base::TaskRunner> task_runner = base::CreateTaskRunner({ base::ThreadPool() });
    for (size_t i = 0; i < num_tasks; ++i) {
        ++num_tasks_pending_;
        task_runner->PostTask(FROM_HERE,
            base::BindOnce(
                [](std::atomic_size_t* num_task_pending) {
                    std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟耗时操作
                    (*num_task_pending)--;
                },
                &num_tasks_pending_));
    }
}

void Hello(void* data) {
    std::vector<int> vec = { 1,2,3,4,5 };
    base::span<const int> const_span(vec);

    for (size_t i = 0; i < const_span.size(); ++i)
        std::cout << const_span[i] << std::endl;
    std::cout << "Hello World!\n";
}

int main() {
    //base::AtExitManager manager;
    base::ThreadPoolInstance::Create("TestCusBaseLib");
    base::ThreadPoolInstance::Get()->Start({ 10 });
    ContinuouslyBindAndPostNoOpTasks(100);
    base::ThreadPoolInstance::Get()->FlushForTesting();
    // base:: ThreadPoolInstance::Set(nullptr);
    //base::AtExitManager::RegisterCallback(&Hello, nullptr);
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
