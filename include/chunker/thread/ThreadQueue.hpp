#ifndef CHUNKER_THREAD_QUEUE_H_
#define CHUNKER_THREAD_QUEUE_H_

#include "chunker/thread/ThreadHandle.hpp"
#include <memory>

namespace chunker {
  class ThreadQueue {
   public:
    // idea: queue for tasks
    // - raii request -> free guard
    // - hold something "lock-like" while task runs
    // - assc w atomic counter which we inc/dec
    //   - thinking: holder makes an acquire call and waits inside func to run
    // - thread handle auto-decrements internal counter + lets someone else acquire
    // on queue:
    // - check counter

    // acquires a thread with some given priority.
    // blocks thread if not currently available.
    virtual std::shared_ptr<ThreadHandle> AcquireBlocking(size_t priority, int timeout_millis = -1) = 0;
    // releases the passed handle and refreshes if possible
    virtual void Release(ThreadHandle& handle) = 0;

    // max number of threads which can run on this queue
    virtual size_t MaxThreads() const  = 0;
  };
}

#endif // CHUNKER_THREAD_QUEUE_H_
