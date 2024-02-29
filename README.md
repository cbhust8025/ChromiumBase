# 介绍
[Chromium](https://github.com/chromium/chromium)中的[Base库](https://github.com/chromium/chromium/tree/main/base)是Chromium中的公共库，精炼了许多好用的软件开发必不可少的基础操作：线程、文件、时间、内存、字符串、进程等等。

基于使用和学习的目的，想要将Chromium中的Base库集成到自己的项目中，并且支持MSVC编译，经过研究发现Chromium的Base库已经从78大版本开始不再支持MSVC编译（具体可以查看tag=78.0.3905.58中compiler_specific.h文件line=12）

本次提取基于Tag=[77.0.3865.129](https://github.com/chromium/chromium/tree/77.0.3865.129)，也就是77大版本的最后一个子版本，发布时间为2019年10月18日。

# 支持

平台：Windows（其他平台理论上可行）

软件：Microsoft Visual C++ 2022（64位）版本 17.8.4

Windows SDK版本：10.0.22621.0

平台工具集：v143

C++语言标准：ISO C++17标准（/std:c++17）

编译工具：MSVC 1938版本（[版本说明](https://sourceforge.net/p/predef/wiki/Compilers/#microsoft-visual-c)）

# 使用

1、首先打开Project中的Base.sln进行编译Base的dll或者lib

2、打开Project中Project.sln来测试是否可以正常使用Base.dll和Base_static.lib

示例代码片段
## 基本线程测试
```C++
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
```

## 测试异步任务

```C++
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
```

## 测试线程池
 
```C++
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
```

# 优点

1、对于使用VS为主进行开发C++十分友好

2、去除了Chromium中代码冗余的问题，可以单独使用、学习

3、支持Debug和Release

4、支持动态库和静态链接

5、不需要使用ninja来编译

# 注意

1、使用最新的VS和MSVC来编译即可，理论上是向前兼容的

2、demo中主要是针对Base库的线程、线程池、异步任务做了简单的代码示例，还有其他很多模块可自行探索

感兴趣或者有用到的麻烦Star一下，谢谢🙏