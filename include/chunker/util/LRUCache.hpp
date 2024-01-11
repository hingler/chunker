#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <cassert>
#include <list>
#include <mutex>
#include <unordered_map>

#include "chunker/util/HashList.hpp"
#include "chunker/util/impl/LRUCacheIterator.hpp"

namespace chunker {
  namespace util {

    enum CachePutResult {
      // last element in cache was removed
      REMOVE_LAST,

      // wrote over inserted element
      OVERWRITE,

      // succeeded!
      SUCCESS
    };

    /**
     * @brief LRU cache impl - thread safe on calls not returning iterator impl.
     * 
     * @tparam KeyType - type for key
     * @tparam ValueType - type for value
     */
    template <typename KeyType, typename ValueType>
    class LRUCache {
    public:
      typedef impl::LRUCacheIterator<KeyType, ValueType> iterator;
      
      LRUCache(int capacity) : capacity_(capacity) {}
      bool Fetch(const KeyType& key, ValueType* output) {
        std::lock_guard lock(cache_mutex);
        if (key_cache.Contains(key)) {
          key_cache.PushFront(key);
          if (output != nullptr) {
            *output = value_cache.at(key);
          }
          return true;
        }

        return false;
      }

      bool Has(const KeyType& key) {
        std::lock_guard lock(cache_mutex);
        return (key_cache.Contains(key));
      }

      bool Refresh(const KeyType& key) {
        std::lock_guard lock(cache_mutex);
        if (Has(key)) {
          Fetch(key, nullptr);
          return true;
        }

        return false;
      }

      /**
       * @brief ensure cache has capacity for specified items
       * 
       * @param new_capacity 
       */
      void Reserve(int new_capacity) {
        std::lock_guard lock(cache_mutex);
        if (capacity_ < new_capacity) {
          capacity_ = new_capacity;
        }
      }

      int Capacity() {
        std::lock_guard lock(cache_mutex);
        return capacity_;
      }

      // put, ignore result
      void Put(const KeyType& key, const ValueType& value) {
        std::lock_guard lock(cache_mutex);
        CachePutResult __ = Put(key, value, nullptr);
      }

      // output receives booted out value
      CachePutResult Put(const KeyType& key, const ValueType& value, ValueType* output) {
        std::lock_guard lock(cache_mutex);
        CachePutResult res = SUCCESS;
        key_cache.PushFront(key);
        auto itr = value_cache.find(key);
        if (itr != value_cache.end()) {
          if (output != nullptr) {
            *output = itr->second;
          }
          res = OVERWRITE;
        } else if (key_cache.Size() > capacity_) {
          KeyType key_last; 

          // might want to clean this up later
          // whatever :3
          bool key_available = key_cache.PopBack(&key_last);
          assert(key_available);
          assert(value_cache.find(key_last) != value_cache.end());
          if (output != nullptr) {
            *output = value_cache.at(key_last);
          }
          res = REMOVE_LAST;
        }

        value_cache.insert_or_assign(key, value);
        return res;
      }

      /**
       * @brief Creates an iterator at start of cache. NOT THREAD SAFE.
       * 
       * @return impl::LRUCacheIterator<KeyType, ValueType> 
       */
      impl::LRUCacheIterator<KeyType, ValueType> begin() {
        return impl::LRUCacheIterator<KeyType, ValueType>(key_cache.begin(), &value_cache);
      }

      /**
       * @brief Creates an iterator at emd of cache. NOT THREAD SAFE.
       * 
       * @return impl::LRUCacheIterator<KeyType, ValueType> 
       */
      impl::LRUCacheIterator<KeyType, ValueType> end() {
        return impl::LRUCacheIterator<KeyType, ValueType>();
      }


      /**
       * @brief Creates an iterator at begin of cache, which is bounded by a pre-specified length. NOT THREAD SAFE.
       * 
       * @return impl::LRUCacheIterator<KeyType, ValueType> 
       */
      impl::LRUCacheIterator<KeyType, ValueType> begin_bounded(int max_length) {
        return impl::LRUCacheIterator<KeyType, ValueType>(key_cache.begin(), &value_cache, max_length);
      }

    private:
      HashList<KeyType> key_cache;
      std::unordered_map<KeyType, ValueType> value_cache;
      int capacity_;
      std::recursive_mutex cache_mutex;
    };     
  }
}

#endif // LRU_CACHE_H_