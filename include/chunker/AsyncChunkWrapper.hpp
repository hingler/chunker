#ifndef CHUNKER_ASYNC_CHUNK_WRAPPER_H_
#define CHUNKER_ASYNC_CHUNK_WRAPPER_H_

#include "chunker/AsyncChunkManager.hpp"
#include <chrono>
#include <mutex>
#include <optional>

#include <memory>
#include <vector>

namespace chunker {
  // wrap manager to provide future-handling behavior
  // (since we're rewriting it again and again hehe)
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
  class AsyncChunkWrapper {
   public:
    typedef GenFactory    gen_fac_type;
    typedef Generator     gen_type;
    typedef Chunker       chunker_type;
    typedef Chunk         chunk_type;
    typedef Job           job_type;
    typedef Result        result_type;
    AsyncChunkWrapper(
      Chunker chunker,
      const std::shared_ptr<GenFactory> factory,
      size_t max_threads
    ) : manager(chunker, factory, max_threads), result_fetched_(false) {}


    bool Update(const Job& job) {
      std::lock_guard<std::recursive_mutex> lock(job_lock_);

      UpdateFuture();
      if (!current_future_.valid() && job != stored_job) {
        current_job = job;
        current_future_ = manager.Enqueue(job);
        return true;
      }

      return false;
    }

    bool Wait(int duration_millis = 10) {
      if (current_future_.valid()) {
        return current_future_.wait_for(std::chrono::milliseconds(duration_millis)) == std::future_status::ready;
      }

      return false;
    }

    bool FetchLatest(Result& output) {
      std::lock_guard<std::recursive_mutex> lock(job_lock_);
      UpdateFuture();
      if (last_output_ && !result_fetched_.test_and_set()) {
        output = last_output_.value();
        return true;
      }

      return false;
    }

    bool GetCurrent(Result& output) {
      // don't try to fetch latest
      if (last_output_) {
        output = last_output_.value();
        return true;
      }

      return false;
    }
   private:
    bool UpdateFuture() {
      std::lock_guard<std::recursive_mutex> lock(job_lock_);
      if (current_future_.valid()
        && current_future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready
      ) {
        std::optional<Result> r = current_future_.get();
        if (r) {
          last_output_ = std::move(r);
          result_fetched_.clear();
          stored_job = current_job;
          current_future_ = std::future<std::optional<Result>>();
        }

        return !(!r);
      }

      return false;
    }
    AsyncChunkManager<GenFactory, Generator, Chunker, Chunk, Job, Result> manager;
    std::optional<Result> last_output_;
    std::atomic_flag result_fetched_;
    std::future<std::optional<Result>> current_future_;

    Job stored_job;
    Job current_job;

    std::recursive_mutex job_lock_;
  };
}

#endif // CHUNKER_ASYNC_CHUNK_WRAPPER_H_
