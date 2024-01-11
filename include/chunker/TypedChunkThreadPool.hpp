#ifndef TYPED_CHUNK_THREAD_POOL_H_
#define TYPED_CHUNK_THREAD_POOL_H_

#include "chunker/util/LRUCache.hpp"
#include "chunker/ChunkIdentifier.hpp"
#include "chunker/TypedChunkThread.hpp"

// reuse this???

namespace chunker {
  template <typename ChunkGenFactory, typename ChunkGenerator, typename ChunkType>
  class TypedChunkThreadPool {
    public:
    typedef util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>> CacheType;
    TypedChunkThreadPool(
      size_t thread_count,
      std::shared_ptr<ChunkGenFactory> factory
    ) : chunk_cache(1024) {
      this->threads = thread_count;
      this->thread_list = new TypedChunkThread<ChunkGenerator, ChunkType>*[threads];
      for (int i = 0; i < threads; i++) {
        // too much work to generify further, at least for me.
        // gonna make a separate thread pool for sampling work instead :3
        // (thinking: cache separation is typically 64b. break img buf along allocs of 16, have threads sample)

        // totally generic:
        // - job type (representing a unit of work)
        // - thread type (just needs to implement an enqueue function)
        // - output type (possibly void - just constituting a unit of work)

        // caching threads
        // - should be plug n play - job type needs to be hashable, and we just cache the results

        // chunk threads
        // - job type is always identifier
        // - return type is some chunk

        thread_list[i] = new TypedChunkThread<ChunkGenerator, ChunkType>(
          factory->Create(),
          chunk_cache
        );
      }
    }

    void Reserve(size_t cache_size) {
      chunk_cache.Reserve(cache_size);
    }

    void Enqueue(const chunker::ChunkIdentifier& identifier) {
      // try to refresh ID in cache
      if (!chunk_cache.Refresh(identifier)) {
        size_t min_index = 0;
        size_t min_items = thread_list[0]->GetQueueSize();

        for (size_t i = 1; i < threads; i++) {
          size_t item_count = thread_list[i]->GetQueueSize();
          if (item_count < min_items) {
            min_items = item_count;
            min_index = i;
          }
        }

        thread_list[min_index]->Enqueue(identifier);
      }
    }

    void Wait() {
      for (size_t i = 0; i < threads; i++) {
        thread_list[i]->Wait();
      }
    }

    size_t GetQueueSize() {
      size_t queue_size = 0;
      for (int i = 0; i < threads; i++) {
        queue_size += thread_list[i]->GetQueueSize();
      }

      return queue_size;
    }

    typename util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>::iterator BoundedIterator(size_t chunk_count) {
      return chunk_cache.begin_bounded(chunk_count);
    }

    typename util::LRUCache<chunker::ChunkIdentifier, std::shared_ptr<ChunkType>>::iterator End() {
      return chunk_cache.end();
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
    TypedChunkThread<ChunkGenerator, ChunkType>** thread_list;
    CacheType chunk_cache;
  };
}

#endif // TYPED_CHUNK_THREAD_POOL_H_