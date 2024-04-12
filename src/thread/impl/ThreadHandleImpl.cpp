#include "chunker/thread/impl/ThreadHandleImpl.hpp"

namespace chunker {
  namespace impl {
    ThreadHandleImpl::ThreadHandleImpl(
      size_t priority,
      std::promise<void>&& promise,
      ThreadQueue& queue
    ) : queue(queue), priority(priority), promise(std::move(promise)), released(false), resolved(false) {}

    void ThreadHandleImpl::Release() {
      // normal functionality: resolve should always return before release
      // when complete: just release
      if (!released.test_and_set() && resolved.test()) {
        queue.Release(*this);
      }
    }

    void ThreadHandleImpl::_Resolve() {
      if (!resolved.test_and_set() && !released.test()) {
        promise.set_value();
      }
    }

    bool ThreadHandleImpl::_Invalidated() {
      // indicates that the handle is no longer valid
      return (released.test());
    }

    bool ThreadHandleImpl::operator<(const ThreadHandleImpl& other) {
      return (priority < other.priority);
    }

    bool ThreadHandleImpl::operator==(const ThreadHandleImpl& other) {
      return (priority == other.priority);
    }

    ThreadHandleImpl::~ThreadHandleImpl() {
      Release();
    }
  }
}
