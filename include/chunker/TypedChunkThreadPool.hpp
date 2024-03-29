#ifndef TYPED_CHUNK_THREAD_POOL_H_
#define TYPED_CHUNK_THREAD_POOL_H_

#include "chunker/util/LRUCache.hpp"
#include "chunker/ChunkIdentifier.hpp"
#include "chunker/TypedChunkThread.hpp"

#include "gog43/Logger.hpp"

// tba: chunker needs its own gog copy jej

#include <tbb/concurrent_queue.h>

// reuse this???

namespace chunker {
  template <typename ChunkGenFactory, typename ChunkGenerator, typename ChunkType>
  class TypedChunkThreadPool {
    public:
    typedef util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>> CacheType;
    TypedChunkThreadPool(
      size_t max_threads,
      std::shared_ptr<ChunkGenFactory> factory
    ) : chunk_cache(1024), chunk_queue() {
      this->threads = max_threads;
      this->thread_list = new TypedChunkThread<ChunkGenerator, ChunkType>*[threads];
      for (int i = 0; i < threads; i++) {

        thread_list[i] = new TypedChunkThread<ChunkGenerator, ChunkType>(
          factory->Create(),
          chunk_cache,
          chunk_queue,
          i
        );
      }
    }

    void Reserve(size_t cache_size) {
      chunk_cache.Reserve(cache_size);
    }

    void Enqueue(const chunker::ChunkIdentifier& identifier) {
      // try to refresh ID in cache
      chunk_queue.emplace(identifier);
    }

    void Wake() {
      for (size_t i = 0; i < threads; i++) {
        thread_list[i]->RefreshThread();
      }
    }

    void Wait() {
      // won't work anymore? neh it'll wait until all threads are done processing chunks
      for (size_t i = 0; i < threads; i++) {
        thread_list[i]->Wait();
      }
    }

    bool Empty() {
      return chunk_queue.empty();
    }

    typename util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>::iterator BoundedIterator(size_t chunk_count) {

      // not thread safe - any scenario in which we'd read this while the chunk is active??
      return chunk_cache.begin_bounded(chunk_count);
    }

    typename util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>::iterator End() {
      return chunk_cache.end();
    }

    std::shared_ptr<ChunkType> GetChunk(const chunker::ChunkIdentifier& chunk) {
      std::shared_ptr<ChunkType> out;
      bool found = chunk_cache.Fetch(chunk, &out);
      return out;
    }

    TypedChunkThreadPool(const TypedChunkThreadPool& other) = delete;
    TypedChunkThreadPool(TypedChunkThreadPool&& other) = delete;
    TypedChunkThreadPool operator=(const TypedChunkThreadPool& other) = delete;
    TypedChunkThreadPool operator=(TypedChunkThreadPool&& other) = delete;

    ~TypedChunkThreadPool() {
      for (int i = 0; i < threads; i++) {
        delete thread_list[i];
      }

      delete thread_list;
    }

    private:
    size_t threads;
    // with this approach: threads need to pull work from a common queue st work isn't over-shared
    // how can we re-implement inactive threads?
    // - call some func on a wait cond telling them to sleep (FOREVER)
    // - once re-activated: re-awaken :3

    size_t active_threads;
    TypedChunkThread<ChunkGenerator, ChunkType>** thread_list;
    CacheType chunk_cache;

    tbb::concurrent_queue<ChunkIdentifier> chunk_queue;
  };
}

#endif // TYPED_CHUNK_THREAD_POOL_H_