#ifndef MUTEX_QUEUE_H_
#define MUTEX_QUEUE_H_

#include <mutex>
#include <queue>
namespace chunker {
  template <typename OutputType>
  class MutexQueue {
   public:
    MutexQueue() {}
    void emplace(const OutputType& item) {
      std::lock_guard<std::mutex> lock(mutex);
      queue.push(item);
    }

    bool try_pop(OutputType& output) {
      std::lock_guard<std::mutex> lock(mutex);
      if (!queue.empty()) {
        output = queue.front();
        queue.pop();
        return true;
      }

      return false;
    }

    bool empty() {
      std::lock_guard<std::mutex> lock(mutex);
      return (queue.empty());
    }
   private:
    std::queue<OutputType> queue;
    std::mutex mutex;
  };
}

#endif // MUTEX_QUEUE_H_
