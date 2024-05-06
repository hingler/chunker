#ifndef LIST_NODE_H_
#define LIST_NODE_H_

namespace chunker {
  namespace util {

    namespace impl {
      template <typename KeyType>
      struct ListNode {
        KeyType value;
        ListNode<KeyType>* prev = nullptr;
        ListNode<KeyType>* next = nullptr;
      };
    }
  }
}

#endif // LIST_NODE_H_
