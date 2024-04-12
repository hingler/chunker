#include "chunker/thread/impl/ThreadQueueImpl.hpp"
#include "chunker/thread/ThreadHandle.hpp"

#include <cassert>
#include <chrono>

// tba: need some dumb test code!!!

namespace chunker {
  namespace impl {
    ThreadQueueImpl::ThreadQueueImpl(size_t count) : count_max(count), available(count) {}

    size_t ThreadQueueImpl::MaxThreads() const {
      return count_max;
    }

    std::shared_ptr<ThreadHandle> ThreadQueueImpl::AcquireBlocking(size_t priority, int timeout_millis) {
      std::promise<void> p;
      auto f = p.get_future();
      auto handle = std::make_shared<ThreadHandleImpl>(
        priority,
        std::move(p),
        *this
      );

      {
        std::lock_guard<std::recursive_mutex> lock(queue_mutex);
        int capacity = available.load();

        // tba: timeout field?
        if (
          capacity > 0
          && handle_queue.empty()
          && available.compare_exchange_strong(capacity, capacity - 1)
        ) {
          // has capacity - resolve and return immediately
          handle->_Resolve();
          return handle;
        }

        handle_queue.push(handle);
      }

      // wait until we return...
      Refresh();
      if (timeout_millis < 0) {
        f.wait();
      } else if (
        f.wait_for(std::chrono::milliseconds(timeout_millis)) == std::future_status::timeout
      ) {
        handle->Release();
        return nullptr;
      }

      return handle;
    }

    void ThreadQueueImpl::Refresh() {
      // acquire lock
      // while queue is non-empty and atomic > 0:
      // - grab top ptr
      // - pop off queue
      // - decrement capacity
      // - resolve
      {
        std::lock_guard<std::recursive_mutex> lock(queue_mutex);
        int capacity = available.load();
        while (
          capacity > 0
          && !handle_queue.empty()
        ) {
          auto handle = handle_queue.top();
          // if valid, but compare fails: what do we do?
          handle_queue.pop();

          if (
            !handle->_Invalidated()
            && available.compare_exchange_strong(capacity, capacity - 1)
          ) {
            // dec capacity var
            // fetch handle on top
            // resolve it!
            handle->_Resolve();
          } else if (!handle->_Invalidated()) {
            // not expecting compare to fail!
            assert(handle->_Invalidated());
          }
          // else: leave popped and skip
          capacity = available.load();
        }
      }

    }

    void ThreadQueueImpl::Release(ThreadHandle& handle) {
      // resolve handle - increment thread count
      {
        std::lock_guard<std::recursive_mutex> lock(queue_mutex);
        int capacity = available.fetch_add(1);
        assert(capacity >= 0);
        assert(capacity < count_max);
        Refresh();
      }

    }

    // dtor won't work - need to do something like "cleanup"
    // (tba it  :3)
    ThreadQueueImpl::~ThreadQueueImpl() {
      std::lock_guard<std::recursive_mutex> lock(queue_mutex);
      while (!handle_queue.empty()) {
        // resolve everything until flushed out - let threads complete all at once if they have to!
        auto top = handle_queue.top();
        handle_queue.pop();
        top->_Resolve();
        top->Release();
      }
    }
  }
}
