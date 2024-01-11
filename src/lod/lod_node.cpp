#include "chunker/lod/lod_node.hpp"

namespace chunker {
  namespace lod {
    lod_node* lod_node::lod_node_alloc() {
      return new lod_node();
    }

    void lod_node::lod_node_free(lod_node* node) {
      if (node == nullptr) { return; }
      lod_node_free(node->bl);
      lod_node_free(node->br);
      lod_node_free(node->tl);
      lod_node_free(node->tr);
      delete node;
    }

    size_t lod_node::GetChunkSize(const lod_node* node, size_t tree_res, const glm::vec2& sample_point) {
      glm::vec2 sub_sample_point = sample_point;

      if (node == nullptr) {
        return tree_res * 2;
      }

      size_t half_res = tree_res / 2;

      const lod_node** node_ptr = reinterpret_cast<const lod_node**>(const_cast<lod_node*>(node));
      if (sample_point.y > half_res) {
        node_ptr += 2;
        sub_sample_point.y -= half_res;
      }

      if (sample_point.x > half_res) {
        node_ptr += 1;
        sub_sample_point.x -= half_res;
      }

      return GetChunkSize(*node_ptr, tree_res / 2, sub_sample_point);
    }

    bool lod_node::CompareTrees(const lod_node* a, const lod_node* b) {
      if (a != b && (a == nullptr || b == nullptr)) {
        return false;
      } else if (a == nullptr && b == nullptr) {
        return true;
      }

      bool is_equal = CompareTrees(a->tl, b->tl);
      is_equal = is_equal && CompareTrees(a->tr, b->tr);
      is_equal = is_equal && CompareTrees(a->bl, b->bl);
      is_equal = is_equal && CompareTrees(a->br, b->br);

      return is_equal;
    }
  }
}