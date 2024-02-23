#ifndef ASYNC_CHUNK_MANAGER_H_
#define ASYNC_CHUNK_MANAGER_H_

#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include "chunker/TypedChunkThreadPool.hpp"

#include "chunker/traits/chunker_type.hpp"

#include "gog43/Logger.hpp"

#include <type_traits>

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
    typedef std::promise<Result> PromiseType;
    static_assert(traits::chunker_type<Chunker, Chunk, Job, Result>::value);
    static_assert(traits::stitcher_type<Chunker, Chunk, Job, Result>::value);
   public:
    AsyncChunkManager(
      Chunker chunker,
      std::shared_ptr<GenFactory> factory,
      size_t max_threads
    ) : chunker_(chunker), factory_(factory), pool_(max_threads, factory_) {}
    // result type needs to be shared if this is the case
    // note: we still need to wrap this with some sort of "job queueing" or "job wrapping" system
    // whatever lol thats fine though
    std::future<Result> Enqueue(const Job& job) {
      PromiseType promise;
      std::future<Result> future = promise.get_future();
      auto pair = std::make_pair(job, std::move(promise));

      {
        std::lock_guard<std::mutex> lock(queue_lock_);
        bool queue_empty = job_queue_.empty();

        job_queue_.push(std::move(pair));
        if (queue_empty) {
          start_thread();
        }
      }

      return future;
    }

    void wait() {
      pool_.Wait();
    }

   private:
    typedef std::pair<Job, PromiseType> pair_type;
    typedef std::pair<ChunkIdentifier, Chunk> chunk_data_type;

    void start_thread() {
      std::thread { &AsyncChunkManager::async_func, this }.detach();
    }
    void async_func() {
      std::unique_lock<std::mutex> lock(queue_lock_);
      pair_type& item = job_queue_.front();
      lock.unlock();

      std::vector<ChunkIdentifier> ids = chunker_.Chunk(item.first);

      for (auto id : ids) {
        pool_.Enqueue(id);
      }

      pool_.Wake();
      pool_.Wait();

      std::vector<std::shared_ptr<Chunk>> chunks;
      for (auto id : ids) {
        // contiguous? should be
        chunks.push_back(pool_.GetChunk(id));
      }

      Result r = chunker_.Stitch(item.first, chunks);
      item.second.set_value(r);

      {
        std::lock_guard<std::mutex> lock(queue_lock_);
        job_queue_.pop();
        if (!job_queue_.empty()) {
          // while loop? iddk later lol
          this->start_thread();
        }
      }
    }

    mutable std::mutex queue_lock_;
    std::queue<std::pair<Job, PromiseType>> job_queue_;

    Chunker chunker_;
    std::shared_ptr<GenFactory> factory_;

    TypedChunkThreadPool<GenFactory, Generator, Chunk> pool_;
  };
}

#endif // ASYNC_CHUNK_MANAGER_H_