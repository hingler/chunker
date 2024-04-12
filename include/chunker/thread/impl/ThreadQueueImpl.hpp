#ifndef CHUNKER_THREAD_QUEUE_IMPL_H_
#define CHUNKER_THREAD_QUEUE_IMPL_H_

#include "chunker/thread/ThreadHandle.hpp"
#include "chunker/thread/ThreadQueue.hpp"
#include "chunker/thread/impl/ThreadHandleImpl.hpp"
#include <mutex>
#include <queue>
namespace chunker {
  namespace impl {

    struct HandleComparator {
      typedef std::shared_ptr<ThreadHandleImpl> ptr_type;
      bool operator()(const ptr_type& left, const ptr_type& right) const {
        return (*left) < (*right);
      }
    };

    class ThreadQueueImpl : public ThreadQueue {
     public:
      ThreadQueueImpl(size_t count);
      std::shared_ptr<ThreadHandle> AcquireBlocking(size_t priority, int timeout_millis = -1) override;
      size_t MaxThreads() const override;
      void Refresh();

      void Release(ThreadHandle& handle) override;
      ~ThreadQueueImpl();
     private:
      std::recursive_mutex queue_mutex;
      std::priority_queue<
        std::shared_ptr<ThreadHandleImpl>,
        std::vector<std::shared_ptr<ThreadHandleImpl>>,
        HandleComparator
      > handle_queue;
      const size_t count_max;
      std::atomic_int available;

      // also: need a destructor for this hehe
    };
  }
}

#endif // CHUNKER_THREAD_QUEUE_IMPL_H_
