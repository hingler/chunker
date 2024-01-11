#ifndef HASH_LIST_ITERATOR_H_
#define HASH_LIST_ITERATOR_H_

#include <cstddef>
#include <iterator>

#include "chunker/util/impl/ListNode.hpp"

namespace chunker {
  namespace util {
    namespace impl {
      template <typename ValueType>
      class HashListIterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        // superfluous in c++17 i think
        using value_type = ValueType;
        using reference = ValueType&;
        using pointer = ValueType*;

        HashListIterator(ListNode<ValueType>* front) {
          node = front;
        }

        // copy ctor
        HashListIterator(const HashListIterator<ValueType>& other) { node = other.node; }
        // copy assign
        HashListIterator<ValueType>& operator=(const HashListIterator<ValueType>& other) { node = other.node; return *this; }
        // dtor
        ~HashListIterator() {}
        // lvalue swappable? (move ctor / move assign)
        HashListIterator(HashListIterator<ValueType>&& other) { node = other.node; }
        HashListIterator<ValueType>& operator=(HashListIterator<ValueType>&& other) { node = other.node; return *this; }

        // default ctor
        HashListIterator() : node(nullptr) {}

        // operator++
        // pre
        HashListIterator<ValueType>& operator++() {
          if (node != nullptr) {
            node = node->next;
          }

          return *this;
        }

        // post
        HashListIterator<ValueType> operator++(int) {
          auto stop = HashListIterator<ValueType>(*this);
          if (node != nullptr) {
            node = node->next;
          }

          return stop;
        }
        // operator--
        HashListIterator<ValueType>& operator--() {
          if (node != nullptr) {
            node = node->prev;
          }

          return *this;
        }

        HashListIterator<ValueType> operator--(int) {
          auto stop = HashListIterator<ValueType>(*this);
          if (node != nullptr) {
            node = node->prev;
          }

          return stop;
        }
        // operator* (dereference)
        reference operator*() {
          // if nullptr: just let it fail
          return node->value;
        }

        pointer operator->() {
          return &node->value;
        }

        bool operator==(const HashListIterator<ValueType>& other) const {
          return node == other.node;
        }

        bool operator!=(const HashListIterator<ValueType>& other) const {
          return !(node == other.node);
        }
      private:
        ListNode<ValueType>* node;
      };
    }
  }
}

#endif