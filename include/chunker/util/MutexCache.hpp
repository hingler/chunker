#ifndef CHUNKER_MUTEX_CACHE_H_
#define CHUNKER_MUTEX_CACHE_H_

#include <mutex>
#include <unordered_map>

namespace chunker {


  template <typename KeyType, typename ValueType>
  class MutexCache {
   public:

    class LockHandle {
      // womp womp
      friend class MutexCache;
     public:
      LockHandle() : key(), lock(nullptr), value() {}

      bool Available() const {
        return value.has_value();
      }

      ValueType Get() const {
        return value.value();
      }

      void Release() {
        if (lock != nullptr) {
          lock->unlock();
          // ensure we can't continue working with it
          lock = nullptr;
        }
      }

      ~LockHandle() {
        Release();
      }

     private:
      LockHandle(
        const KeyType& key,
        const std::shared_ptr<std::recursive_mutex>& mutex,
        std::optional<ValueType>&& value
      ) : key(key), lock(mutex), value(std::move(value)) {}

      KeyType key;
      std::shared_ptr<std::recursive_mutex> lock;
      std::optional<ValueType> value;
    };

    MutexCache() {}

    LockHandle Acquire(const KeyType& key) {
      LockHandle res;
      Acquire(res, key, true);
      return res;
    }

    // only returns false if "block" is set to false
    bool Acquire(
      LockHandle& output,
      const KeyType& key,
      bool block = true
    ) {
      std::unique_lock cache_lock(cache_mutex);

      auto itr_value = value_cache.find(key);
      if (itr_value != value_cache.end()) {
        output = LockHandle(
          key,
          nullptr,
          std::optional<ValueType>(itr_value->second)
        );

        return true;
      }

      mutex_ptr key_mutex;

      // not currently cached
      auto itr_lock = mutex_cache.find(key);
      if (itr_lock != mutex_cache.end()) {
        key_mutex = itr_lock->second;
        // key mutex is locked on this flow

        cache_lock.unlock();

        // to improve perf: timeout here

        if (block) {
          key_mutex->lock();
        } else if (!key_mutex->try_lock()) {
          // could not lock it right away, return false
          return false;
        }

        cache_lock.lock();

        itr_value = value_cache.find(key);
        if (itr_value != value_cache.end()) {
          // same thing - value's not available
          // key lock released on return
          key_mutex->unlock();
          output = LockHandle(
            key,
            nullptr,
            std::optional<ValueType>(itr_value->second)
          );

          return true;
        }

        // if here: cache lock is owned
      } else {
        // no mutex lock - cache lock is still owned
        key_mutex = std::make_shared<std::recursive_mutex>();
        key_mutex->lock();
        mutex_cache.insert(std::make_pair(key, key_mutex));
      }

      // cache lock is held
      // key mutex is held
      cache_lock.unlock();

      // cache lock not held
      // instance lock held
      // value not present atm
      // key mutex populated and held

      // when lockhandle goes out of scope, we release
      output = LockHandle(
        key,
        key_mutex,
        std::optional<ValueType>()
      );

      return true;
    }

    void Release(
      LockHandle& handle,
      const ValueType& value
    ) {
      if (handle.lock != nullptr && !handle.value.has_value()) {
        std::lock_guard cache_lock(cache_mutex);
        auto itr_lock = mutex_cache.find(handle.key);

        if (itr_lock->second == handle.lock) {
          // lock handle corresponds with it
          value_cache.insert(std::make_pair(
            handle.key,
            value
          ));
        }

        handle.Release();
      }
    }

   private:
    typedef std::shared_ptr<std::recursive_mutex> mutex_ptr;

    std::recursive_mutex cache_mutex;
    std::unordered_map<KeyType, mutex_ptr> mutex_cache;
    std::unordered_map<KeyType, ValueType> value_cache;
  };
}


#endif // CHUNKER_MUTEX_CACHE_H_
