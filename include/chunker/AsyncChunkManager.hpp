#ifndef ASYNC_CHUNK_MANAGER_H_
#define ASYNC_CHUNK_MANAGER_H_

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <unordered_set>

#include "chunker/TypedChunkThreadPool.hpp"

#include "chunker/traits/chunker_type.hpp"

#include "gog43/Logger.hpp"

// any way to infer these?
namespace chunker {
  // this seems like a lot for no reason
  template <
    // splits jobs into chunks, and splits chunks into jobs
    typename GenFactory,
    typename Generator = typename GenFactory::gen_type,
    typename Chunker = typename GenFactory::chunker_type,
    // type which generates chunks
    typename Chunk = typename GenFactory::chunk_type,
    // denotes how tasks are added to the queue
    typename Job = typename GenFactory::job_type,
    // type of data returned by mgr
    typename Result = decltype(std::declval<Chunker&>().Stitch(std::declval<Job&>(), std::vector<std::shared_ptr<Chunk>> {}))
    // could virt this
  >
  class AsyncChunkManager {
    typedef std::promise<std::optional<Result>> PromiseType;
    static_assert(traits::chunker_type<Chunker, Chunk, Job, Result>::value);
    static_assert(traits::stitcher_type<Chunker, Chunk, Job, Result>::value);
   public:
    AsyncChunkManager(
      Chunker chunker,
      std::shared_ptr<GenFactory> factory,
      size_t max_threads
    ) : thread_running_(false), chunker_(chunker), factory_(factory), pool_(max_threads, factory_), last_job_(), priority(99999) {}

    AsyncChunkManager(
      Chunker chunker,
      std::shared_ptr<GenFactory> factory,
      std::shared_ptr<ThreadQueue> queue,
      size_t priority = 0
    ) : thread_running_(false), chunker_(chunker), factory_(factory), pool_(queue, factory, priority), last_job_(), priority(priority) {}
    // result type needs to be shared if this is the case
    // note: we still need to wrap this with some sort of "job queueing" or "job wrapping" system
    // whatever lol thats fine though
    std::future<std::optional<Result>> Enqueue(const Job& job, bool erase_queue = false) {
      PromiseType promise;
      std::future<std::optional<Result>> future = promise.get_future();
      auto pair = std::make_pair(job, std::move(promise));

      {
        // possible deadlock
        std::lock_guard<std::recursive_mutex> lock(queue_lock_);

        bool queue_empty = job_queue_.empty();

        if (erase_queue) {
          clear();

          // up next:
          // - work on tuning lod for performance

          // tba:
          // - tune grass appearance
          // - write shaders for it :)

          gog43::print("popped all prev jobs from queue!");
        }

        // something wrong with this behavior :/

        job_queue_.push(std::move(pair));

        {
          // grab order: queue lock, then async lock
          // (ie: only grab async lock if we already have queue lock)
          std::lock_guard<std::mutex> async_lock(async_lock_);
          if (!thread_running_.test_and_set()) {
            // thread wasn't running - start it
            start_thread();
          }

        }
      }

      return future;
    }

    void clear() {
      std::lock_guard<std::recursive_mutex> lock(queue_lock_);
      while (!job_queue_.empty()) {
        auto& removed_pair = job_queue_.front();
        PromiseType& promise = removed_pair.second;
        // resolve removed promises with empty optional
        promise.set_value(std::optional<Result>());

        job_queue_.pop();
      }
    }

    void wait() {
      pool_.Wait();
    }

    /**
      @returns number of tasks waiting to complete :):)
    */
    bool empty() {
      std::lock_guard<std::recursive_mutex> lock(queue_lock_);
      // async thread not running (last job completed) and queue is empty
      return (job_queue_.size() <= 0) && (!thread_running_.test());
    }

   private:
    typedef std::pair<Job, PromiseType> pair_type;
    typedef std::pair<ChunkIdentifier, Chunk> chunk_data_type;

    void start_thread() {
      std::thread { &AsyncChunkManager::async_func, this }.detach();
    }
    void async_func() {
      pair_type item;
      {
        std::unique_lock<std::recursive_mutex> lock(queue_lock_);
        // take ownership of this element, then pop
        if (job_queue_.empty()) {
          // could have been cleared - return
          std::lock_guard<std::mutex> async_lock(async_lock_);
          thread_running_.clear();
          return;
        }

        item = std::move(job_queue_.front());
        // pop here
        job_queue_.pop();
      }

      auto job_start = std::chrono::high_resolution_clock::now();
      bool distinct = true;
      std::vector<ChunkIdentifier> ids = chunker_.Chunk(item.first);
      if (ids.size() == last_job_.size()) {
        distinct = false;
        for (const auto& c : ids) {
          if (last_job_.find(c) == last_job_.end()) {
            distinct = true;
            break;
          }
        }
      }

      if (!distinct) {
        // last job matches current - bail
        // (this is it - jobs are the same so we get an empty)
        std::lock_guard<std::mutex> async_lock(async_lock_);
        item.second.set_value(std::optional<Result>());
        thread_running_.clear();

        return;
      }

      last_job_.clear();
      // this insertion is the issue - not sure why
      for (const auto& c : ids) {
        last_job_.insert(c);
      }

      // go w a relatively low number - just pad out to max ig
      // * 3 - plan for some extra room!
      pool_.Reserve(ids.size() * 3 + 1);

      for (auto id : ids) {
        pool_.Enqueue(id);
      }

      pool_.Wake();
      pool_.Wait();

      // detachjes once at a time hehe nvm

      std::vector<std::shared_ptr<Chunk>> chunks;
      for (auto id : ids) {
        // contiguous? should be
        chunks.push_back(pool_.GetChunk(id));
      }

      Result r = chunker_.Stitch(item.first, chunks);
      item.second.set_value(std::optional(r));
      auto job_end = std::chrono::high_resolution_clock::now();

      const double ms = std::chrono::duration_cast<std::chrono::nanoseconds>(job_end - job_start).count() / 1000000.0;


      {
        std::lock_guard<std::recursive_mutex> lock(queue_lock_);
        if (!job_queue_.empty()) {
          this->start_thread();
        } else {
          std::lock_guard<std::mutex> async_lock(async_lock_);
          thread_running_.clear();
        }
      }
    }

    mutable std::recursive_mutex queue_lock_;
    mutable std::mutex async_lock_;
    mutable std::atomic_flag thread_running_;
    std::queue<std::pair<Job, PromiseType>> job_queue_;

    mutable std::unordered_set<ChunkIdentifier> last_job_;

    Chunker chunker_;
    std::shared_ptr<GenFactory> factory_;

    const size_t priority;

    TypedChunkThreadPool<GenFactory, Generator, Chunk> pool_;
  };
}

#endif // ASYNC_CHUNK_MANAGER_H_
