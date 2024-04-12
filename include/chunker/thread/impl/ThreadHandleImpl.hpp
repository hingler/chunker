#ifndef CHUNKER_THREAD_HANDLE_IMPL_H_
#define CHUNKER_THREAD_HANDLE_IMPL_H_

#include <cstdlib>
#include <future>

#include "chunker/thread/ThreadHandle.hpp"
#include "chunker/thread/ThreadQueue.hpp"

namespace chunker {
  namespace impl {
    class ThreadHandleImpl : public ThreadHandle {
     public:
      ThreadHandleImpl(
        size_t priority,
        std::promise<void>&& promise,
        ThreadQueue& queue
      );

      void Release() override;

      // resolves underlying promise
      void _Resolve();

      // true if state was invalidated
      bool _Invalidated();

      ~ThreadHandleImpl();

      bool operator<(const ThreadHandleImpl& other);
      bool operator==(const ThreadHandleImpl& other);
     private:
      ThreadQueue& queue;
      const size_t priority;
      std::promise<void> promise;

      // states
      // - created  (create handle)
      // - resolved (handle can run)
      // - released (handle is freed) [internal only, rn - return status?]
      // - invalid  (ie discarded)    [release, return nullptr?]

      //

      std::atomic_flag released;
      std::atomic_flag resolved;

      std::mutex state_lock;
    };
  }
}

#endif // CHUNKER_THREAD_HANDLE_IMPL_H_
