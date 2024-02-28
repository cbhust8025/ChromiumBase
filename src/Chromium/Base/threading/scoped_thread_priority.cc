// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_thread_priority.h"

#include "base/location.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/trace_event.h"

namespace base {

ScopedThreadMayLoadLibraryOnBackgroundThread::
    ScopedThreadMayLoadLibraryOnBackgroundThread(const Location& from_here) {
  TRACE_EVENT_BEGIN2("base", "ScopedThreadMayLoadLibraryOnBackgroundThread",
                     "file_name", from_here.file_name(), "function_name",
                     from_here.function_name());

#if defined(OS_WIN)

  base::ThreadPriority priority = PlatformThread::GetCurrentThreadPriority();
  if (priority == base::ThreadPriority::BACKGROUND) {
    original_thread_priority_ = priority;
    PlatformThread::SetCurrentThreadPriority(base::ThreadPriority::NORMAL);
  }
#endif  // OS_WIN
}

ScopedThreadMayLoadLibraryOnBackgroundThread::
    ~ScopedThreadMayLoadLibraryOnBackgroundThread() {
  TRACE_EVENT_END0("base", "ScopedThreadMayLoadLibraryOnBackgroundThread");
#if defined(OS_WIN)
  if (original_thread_priority_)
    PlatformThread::SetCurrentThreadPriority(original_thread_priority_.value());
#endif  // OS_WIN
}

}  // namespace base
