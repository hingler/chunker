#ifndef TYPED_CHUNK_THREAD_H_
#define TYPED_CHUNK_THREAD_H_

#include "chunker/ChunkIdentifier.hpp"
#include "chunker/traits/chunk_gen_type.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace chunker {
  template <typename ChunkGenerator, typename ChunkType>
  class TypedChunkThread {
    static_assert(chunker::traits::chunk_gen_type<ChunkGenerator, ChunkType>::value);

    public:
    TypedChunkThread(
      std::shared_ptr<ChunkGenerator> generator,
      util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>& cache
    ) : generator_(generator), chunk_cache_(cache), thread_active_(true) {
      thread_ = std::thread(&TypedChunkThread::ThreadFunc, this);
    }

    TypedChunkThread(const TypedChunkThread& other) = delete;
    TypedChunkThread(TypedChunkThread&& other) = delete;
    TypedChunkThread operator=(const TypedChunkThread& other) = delete;
    TypedChunkThread operator=(TypedChunkThread&& other) = delete;

    void Enqueue(const chunker::ChunkIdentifier& identifier) {
      // enqueues a chunk on this thread
      std::unique_lock<std::mutex> lock(queue_lock_);
      chunk_queue_.push(identifier);
      ++task_count_;
      cond_.notify_all();
    }

    size_t GetQueueSize() {
      // return size of underlying queue
      std::unique_lock lock(queue_lock_);
      return task_count_;
    }

    void Wait() {
      std::unique_lock<std::mutex> lock(queue_lock_);
        wait_cond_.wait(lock, [&]{ return chunk_queue_.size() == 0 || !thread_active_; });
    }

    ~TypedChunkThread() {
      thread_active_ = false;
      cond_.notify_all();
      thread_.join();
      wait_cond_.notify_all();
    }

    private:
    std::shared_ptr<ChunkGenerator> gen;

    // thinking: we need a little bit more work to make this completely generic - worth it?
    // (issue: two completely different libraries; might be worth specializing just for sampling behavior)
    // ((sampling is taking the most time rn))

    void ThreadFunc() {
      chunker::ChunkIdentifier next_chunk;
      while (true) {
        {
          std::unique_lock<std::mutex> lock(queue_lock_);
          if (chunk_queue_.size() <= 0) {
            // notify waiters that thread is done
            wait_cond_.notify_all();
          }

          cond_.wait(lock, [&] { return chunk_queue_.size() > 0 || !thread_active_; });
          if (!thread_active_) {
            return;
          }

          next_chunk = chunk_queue_.front();
        }

        std::shared_ptr<ChunkType> chunk;

        if (chunk_cache_.Has(next_chunk)) {
          chunk_cache_.Fetch(next_chunk, &chunk);
        } else {
          chunk = generator_->Generate(next_chunk);
          chunk_cache_.Put(
            next_chunk, chunk
          );
        }

        {
          std::unique_lock<std::mutex> lock(queue_lock_);
          chunk_queue_.pop();
          --task_count_;
        }
      }
    }

    util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>& chunk_cache_;
    std::shared_ptr<ChunkGenerator> generator_;
    std::queue<chunker::ChunkIdentifier> chunk_queue_;

    size_t task_count_;
    std::mutex queue_lock_;
    std::condition_variable cond_;
    std::condition_variable wait_cond_;

    bool thread_active_;
    std::thread thread_;
  };
}

#endif // TYPED_CHUNK_THREAD_H_