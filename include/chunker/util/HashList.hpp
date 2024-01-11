#ifndef HASH_LIST_H_
#define HASH_LIST_H_

#include <cassert>
#include <unordered_map>

#include "chunker/util/impl/ListNode.hpp"
#include "chunker/util/impl/HashListIterator.hpp"

namespace chunker {
  namespace util {

    template <typename KeyType>
    class HashList {
    public:
      HashList();

      /**
       * @brief Pushes to the front of this list
       * 
       * @param key - key to store
       */
      void PushFront(const KeyType& key);

      /**
       * @brief Pushes to the back of this list
       * 
       * @param key - key to store
       */
      void PushBack(const KeyType& key);

      /**
       * @brief Removes first element from list.
       * 
       * @param output output param for list
       * @return true if list has items
       * @return false otherwise
       */
      bool PopFront(KeyType* output);

      /**
       * @brief Removes last element from this list.
       * 
       * @param output output param for list
       * @returns true if list has items, false otherwise.
       */
      bool PopBack(KeyType* output);

      /**
       * @brief Checks if key is in list.
       * 
       * @param key - key to check
       * @return true if in the list
       * @return false otherwise
       */
      bool Contains(const KeyType& key);

      /**
       * @brief Removes specified key from list if present
       * 
       * @param key - key to remove
       * @return true if key in list and removed
       * @return false otherwise
       */
      bool RemoveKey(const KeyType& key);

      /**
       * @return size_t size of this list
       */
      size_t Size();

      impl::HashListIterator<KeyType> begin();

      impl::HashListIterator<KeyType> end();

      ~HashList();
    private:
      // store in disorganized heap space?
      // eh, for now
      // we can come up with something better, late
      
      impl::ListNode<KeyType>* CreateListNode(const KeyType& key);
      void DeleteNode(impl::ListNode<KeyType>* node);
      
      impl::ListNode<KeyType>* front;
      impl::ListNode<KeyType>* back;
      std::unordered_map<KeyType, impl::ListNode<KeyType>*> node_cache;
      size_t mem_watch;
    };

    template <typename KeyType>
    HashList<KeyType>::HashList() : front(nullptr), back(nullptr), node_cache(), mem_watch(0) {}

    template <typename KeyType>
    size_t HashList<KeyType>::Size() { return node_cache.size(); }

    template <typename KeyType>
    impl::ListNode<KeyType>* HashList<KeyType>::CreateListNode(const KeyType& key) {
      impl::ListNode<KeyType>* node = new impl::ListNode<KeyType>();
      mem_watch++;
      node->value = key;
      node_cache.insert(std::make_pair(key, node));
      return node;
    }
    
    template <typename KeyType>
    void HashList<KeyType>::PushFront(const KeyType& key) {
      impl::ListNode<KeyType>* node;
      if (this->Contains(key)) {
        node = node_cache.at(key);
        if (node == front) {
          return;
        }

        node->prev->next = node->next;

        if (node != back) {
          node->next->prev = node->prev;
        } else {
          back = node->prev;
        }
      } else {
        node = CreateListNode(key);
        if (Size() == 1) {
          front = node;
          back = node;
          return;
        }
      }

      node->next = front;
      node->prev = nullptr;
      front->prev = node;
      front = node;
    }

    template <typename KeyType>
    void HashList<KeyType>::PushBack(const KeyType& key) {
      impl::ListNode<KeyType>* node;
      if (this->Contains(key)) {
        node = node_cache.at(key);
        if (node == back) {
          return;
        }

        node->next->prev = node->prev;
        if (node != front) {
          node->prev->next = node->next;
        } else {
          front = node->next;
        }
      } else {
        node = CreateListNode(key);
        if (Size() == 1) {
          front = node;
          back = node;
          return;
        }
      }

      node->prev = back;
      back->next = node;
      node->next = nullptr;
      back = node;
    }

    template <typename KeyType>
    bool HashList<KeyType>::PopFront(KeyType* output) {
       if (Size() == 0) {
        return false;
      }

      impl::ListNode<KeyType>* node = front;
      front = front->next;
      if (node == back) {
        back = nullptr;
      }

      auto itr = node_cache.find(node->value);
      node_cache.erase(itr);

      if (output != nullptr) {
        *output = node->value;
      }

      DeleteNode(node);
      return true;
    }

    template <typename KeyType>
    bool HashList<KeyType>::PopBack(KeyType* output) {
      if (Size() == 0) {
        return false;
      }

      impl::ListNode<KeyType>* node = back;
      back = node->prev;
      if (back != nullptr) {
        back->next = nullptr;
      }

      if (node == front) {
        front = nullptr;
      }

      auto itr = node_cache.find(node->value);
      node_cache.erase(itr);

      if (output != nullptr) {
        *output = node->value;
      }

      DeleteNode(node);
      // note: need copy/move defns :(
      return true;
    }

    template <typename KeyType>
    bool HashList<KeyType>::RemoveKey(const KeyType& key) {
      if (Size() == 0) {
        return false;
      }

      auto itr = node_cache.find(key);
      if (itr == node_cache.end()) {
        return false;
      }

      impl::ListNode<KeyType>* node = node_cache.at(key);

      if (node == back) {
        return PopBack(nullptr);
      } else if (node == front) {
        return PopFront(nullptr);
      } else {
        // somewhere between the two
        node->next->prev = node->prev;
        node->prev->next = node->next;
        node_cache.erase(itr);
        DeleteNode(node);
        return true;
      }
    }

    template <typename KeyType>
    bool HashList<KeyType>::Contains(const KeyType& key) {
      return (node_cache.find(key) != node_cache.end());
    }

    template <typename KeyType>
    void HashList<KeyType>::DeleteNode(impl::ListNode<KeyType>* node) {
      delete node;
      mem_watch--;
    }

    template <typename KeyType>
    HashList<KeyType>::~HashList() {
      for (auto& pair : node_cache) {
        delete pair.second;
        mem_watch--;
      }

      // ensure everything is freed!
      assert(mem_watch == 0);
    }

    template <typename KeyType>
    impl::HashListIterator<KeyType> HashList<KeyType>::begin() {
      return impl::HashListIterator<KeyType>(front);
    }

    template <typename KeyType>
    impl::HashListIterator<KeyType> HashList<KeyType>::end() {
      return impl::HashListIterator<KeyType>(nullptr);
    }
  }
}

#endif