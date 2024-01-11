#ifndef LIST_NODE_H_
#define LIST_NODE_H_

namespace chunker {
  namespace util {

    namespace impl {
      template <typename KeyType>
      struct ListNode {
        ListNode<KeyType>* prev;
        ListNode<KeyType>* next;
        KeyType value;
      };
    }
  }
}

#endif // LIST_NODE_H_