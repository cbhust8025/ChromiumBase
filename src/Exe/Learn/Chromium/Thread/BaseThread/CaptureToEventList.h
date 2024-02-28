/************************************************************************/
// Copyright (c) 2022 Tencent Inc. All rights reserved.
/*@File Name         : CaputeEventThread.h
/*@Created Date      : 2024/1/5 13:01
/*@Author            : lealcheng
/*@Description       :
/************************************************************************/
#pragma once

#include "base/threading/thread.h"

enum ThreadEvent {
    // Thread::Init() was called.
    THREAD_EVENT_INIT = 0,

    // The MessageLoop for the thread was deleted.
    THREAD_EVENT_MESSAGE_LOOP_DESTROYED,

    // Thread::CleanUp() was called.
    THREAD_EVENT_CLEANUP,

    // Keep at end of list.
    THREAD_NUM_EVENTS
};

typedef std::vector<ThreadEvent> EventList;


class CaptureToEventList : public base::Thread {
public:
    // This Thread pushes events into the vector |event_list| to show
    // the order they occured in. |event_list| must remain valid for the
    // lifetime of this thread.
    explicit CaptureToEventList(EventList* event_list)
        : base::Thread("CaptureToEventList"),
        event_list_(event_list) {
    }

    ~CaptureToEventList() override { Stop(); }

    void Init() override { event_list_->push_back(THREAD_EVENT_INIT); }

    void CleanUp() override { event_list_->push_back(THREAD_EVENT_CLEANUP); }

private:
    EventList* event_list_;

    DISALLOW_COPY_AND_ASSIGN(CaptureToEventList);
};

// Observer that writes a value into |event_list| when a message loop has been
// destroyed.
class CapturingDestructionObserver
    : public base::MessageLoopCurrent::DestructionObserver {
public:
    // |event_list| must remain valid throughout the observer's lifetime.
    explicit CapturingDestructionObserver(EventList* event_list)
        : event_list_(event_list) {
    }

    // DestructionObserver implementation:
    void WillDestroyCurrentMessageLoop() override {
        event_list_->push_back(THREAD_EVENT_MESSAGE_LOOP_DESTROYED);
        event_list_ = nullptr;
    }

private:
    EventList* event_list_;

    DISALLOW_COPY_AND_ASSIGN(CapturingDestructionObserver);
};

// Task that adds a destruction observer to the current message loop.
void RegisterDestructionObserver(
    base::MessageLoopCurrent::DestructionObserver* observer) {
    base::MessageLoopCurrent::Get()->AddDestructionObserver(observer);
}