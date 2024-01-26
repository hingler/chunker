#ifndef TYPED_CHUNK_THREAD_H_
#define TYPED_CHUNK_THREAD_H_

#include "chunker/ChunkIdentifier.hpp"
#include "chunker/traits/chunk_gen_type.hpp"

#include <tbb/concurrent_queue.h>


#include "debug/Logger.hpp"

#include <condition_variable>
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
      util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>& cache,
      tbb::concurrent_queue<chunker::ChunkIdentifier>& queue,
      size_t thread_id
    ) : generator_(generator), chunk_cache_(cache), chunk_queue_(queue), thread_active_(true), thread_id_(thread_id) {
      thread_ = std::thread(&TypedChunkThread::ThreadFunc, this);
      running_job_ = false;
    }

    TypedChunkThread(const TypedChunkThread& other) = delete;
    TypedChunkThread(TypedChunkThread&& other) = delete;
    TypedChunkThread operator=(const TypedChunkThread& other) = delete;
    TypedChunkThread operator=(TypedChunkThread&& other) = delete;


    void Wait() {
      std::unique_lock<std::mutex> lock(queue_lock_);
      // notify cond - remove predicate so that it will wake when notified
      // *always* try to pull once, then sleep again
      cond_.notify_all();
      wait_cond_.wait(lock, [&]{ return PredicateCondition(); });
    }

    bool PredicateCondition() {
      return (chunk_queue_.empty() && !running_job_) || !thread_active_;
    }

    void RefreshThread() {
      cond_.notify_all();
      wait_cond_.notify_all();
    }

    ~TypedChunkThread() {
      // threads arent cleaning up very nicely
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
          // we acquire this lock - the queue isn't empty. we run a loop.
          if (chunk_queue_.empty()) {
            // notify waiters that thread is done
            wait_cond_.notify_all();
            cond_.wait(lock);
          }

          if (!thread_active_) {
            return;
          }
        }

        // if false: re-runs
        running_job_ = true;
        if (chunk_queue_.try_pop(next_chunk)) {
          std::shared_ptr<ChunkType> chunk;

          if (chunk_cache_.Has(next_chunk)) {
            chunk_cache_.Fetch(next_chunk, &chunk);
          } else {
            chunk = generator_->Generate(next_chunk);
            chunk_cache_.Put(
              next_chunk, chunk
            );
          } 

        }

        // true while we're crunching a chunk.
        running_job_ = false;
      }
    }

    util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>& chunk_cache_;

    std::mutex queue_lock_;
    tbb::concurrent_queue<chunker::ChunkIdentifier>& chunk_queue_;
    
    std::shared_ptr<ChunkGenerator> generator_;

    // condition variable indicating work to be done
    std::condition_variable cond_;

    // condition variable notified when task complete
    std::condition_variable wait_cond_;

    bool thread_active_;

    bool running_job_;
    std::thread thread_;

    size_t thread_id_;
  };
}

#endif // TYPED_CHUNK_THREAD_H_