#ifndef LRU_CACHE_ITERATOR_H_
#define LRU_CACHE_ITERATOR_H_

#include <unordered_map>

namespace chunker {
  namespace util {
    namespace impl {
      template <typename KeyType, typename ValueType>
      class LRUCacheIterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;

        using value_type = ValueType;
        using reference = ValueType&;
        using pointer = ValueType*;

        // create hash list iterator
        LRUCacheIterator(HashListIterator<KeyType> itr, std::unordered_map<KeyType, ValueType>* cache) : LRUCacheIterator(itr, cache, SIZE_MAX) {}

        LRUCacheIterator(HashListIterator<KeyType> itr, std::unordered_map<KeyType, ValueType>* cache, size_t length) : key_itr(itr), value_cache(cache), max_(length), ind(0) {}

        LRUCacheIterator(const LRUCacheIterator<KeyType, ValueType>& other) : key_itr(other.key_itr), value_cache(other.value_cache), max_(other.max_), ind(other.ind) {}
        LRUCacheIterator(LRUCacheIterator<KeyType, ValueType>&& other) : key_itr(other.key_itr), value_cache(other.value_cache), max_(other.max_), ind(other.ind) {}

        LRUCacheIterator<KeyType, ValueType>& operator=(const LRUCacheIterator<KeyType, ValueType>& other) {
          key_itr = other.key_itr;
          value_cache = other.value_cache;
          max_ = other.max_;
          ind = other.ind;

          return *this;
        }

        LRUCacheIterator<KeyType, ValueType>& operator=(LRUCacheIterator<KeyType, ValueType>&& other) {
          key_itr = other.key_itr;
          value_cache = other.value_cache;
          max_ = other.max_;
          ind = other.ind;
        }

        LRUCacheIterator() : key_itr(), value_cache(nullptr), max_(0), ind(SIZE_MAX) {}

        LRUCacheIterator<KeyType, ValueType>& operator++() {
          key_itr++;
          ind++; 
          return *this;
        }

        LRUCacheIterator<KeyType, ValueType> operator++(int) {
          auto stop = LRUCacheIterator<KeyType, ValueType>(*this);
          key_itr++;
          ind++;
          return stop;
        }

        LRUCacheIterator<KeyType, ValueType>& operator--() {
          key_itr--;
          ind--;
          return *this;
        }

        LRUCacheIterator<KeyType, ValueType> operator--(int) {
          auto stop = LRUCacheIterator<KeyType, ValueType>(*this);
          key_itr--;
          ind--;
          return stop;
        }

        reference operator*() {
          return value_cache->at(*key_itr);
        }

        pointer operator->() {
          return &value_cache->at(*key_itr);
        }

        bool operator==(const LRUCacheIterator<KeyType, ValueType>& other) const {
          if (ind >= max_ && other.ind >= other.max_) {
            return true;
          }

          return (key_itr == other.key_itr);
        }

        bool operator!=(const LRUCacheIterator<KeyType, ValueType >& other) const {
          return !(*this == other);
        }

      private:
        HashListIterator<KeyType> key_itr;
        std::unordered_map<KeyType, ValueType>* value_cache;
        size_t max_;
        size_t ind;
      };
    }
  }
}

#endif // LRU_CACHE_ITERATOR_H_