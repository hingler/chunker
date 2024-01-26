#include "chunker/lod/LodTreeGenerator.hpp"

namespace chunker {
  namespace lod {
    lod_node* LodTreeGenerator::CreateLodTree(const glm::vec3& local_position) {
      return CreateLodTree(local_position, 0);
    }

    lod_node* LodTreeGenerator::CreateLodTree(const glm::vec3& local_position, int force_divide) {
      return CreateLodTree(local_position, force_divide, size_, chunk_res_, cascade_factor, 0);
    }
    
    // add lod
    lod_node* LodTreeGenerator::CreateLodTree(const glm::vec3& local_position, int force_divide, int size, int chunk_size, double cascade_factor, int lod_bias) {
      assert(((size) & (size - 1)) == 0);
      assert(((chunk_size) & (chunk_size - 1)) == 0);
      assert(chunk_size > 1);
      assert(size > 1);
      auto* node = lod_node::lod_node_alloc();
      double cascade_real = cascade_factor / CASCADE_MUL_FACTOR;

      // handle lod bias
      // -> scale down chunk size

      // arb
      int eff_lod_bias = std::clamp(lod_bias, -3, 3);
      int eff_chunk_size = chunk_size;
      while (eff_lod_bias < 0) {
        eff_chunk_size *= 2;
        eff_lod_bias++;
      }

      while (eff_lod_bias > 0 && eff_chunk_size > 1) {
        eff_chunk_size /= 2;
        eff_lod_bias--;
      }

      // scale up cascade
      size_t cascade_mul = size;
      while (cascade_mul > chunk_size) {
        cascade_real *= CASCADE_MUL_FACTOR;
        cascade_mul >>= 1;
      }

      CreateLodTree_recurse(
        0,
        0,
        size,
        eff_chunk_size,
        cascade_real,
        local_position,
        node,
        force_divide
      );

      return node;
    }

     void LodTreeGenerator::CreateLodTree_recurse(
      int x, 
      int y,
      int node_size,
      int chunk_size,
      double cascade_threshold,
      const glm::vec3& local_position,
      lod_node* root,
      int force_divide) 
    {
      // no longer descend
      if (node_size <= chunk_size) {
        return;
      }


      float dist_to_chunk;
      float x_f = static_cast<float>(x);
      float y_f = static_cast<float>(y);

      // side note: we need to map z to height :(
      if (local_position.x < x || local_position.x > x + node_size || local_position.z < y || local_position.z > y + node_size) {
        glm::vec3 closest_point(glm::clamp(local_position.x, x_f, x_f + node_size), local_position.y, glm::clamp(local_position.z, y_f, y_f + node_size));
        dist_to_chunk = glm::length(closest_point - local_position);
      } else {
        // ignore z
        dist_to_chunk = 0.0;
      }

      // use force_divide to require node to split
      if (dist_to_chunk > cascade_threshold && force_divide <= 0) {
        return;
      }

      root->bl = lod_node::lod_node_alloc();
      root->br = lod_node::lod_node_alloc();
      root->tl = lod_node::lod_node_alloc();
      root->tr = lod_node::lod_node_alloc();

      double new_cascade_threshold = cascade_threshold / CASCADE_MUL_FACTOR;
      int new_node_size = node_size / 2;

      CreateLodTree_recurse(x,                 y,                 new_node_size, chunk_size, new_cascade_threshold, local_position, root->bl, force_divide - 1);
      CreateLodTree_recurse(x + new_node_size, y,                 new_node_size, chunk_size, new_cascade_threshold, local_position, root->br, force_divide - 1);
      CreateLodTree_recurse(x,                 y + new_node_size, new_node_size, chunk_size, new_cascade_threshold, local_position, root->tl, force_divide - 1);
      CreateLodTree_recurse(x + new_node_size, y + new_node_size, new_node_size, chunk_size, new_cascade_threshold, local_position, root->tr, force_divide - 1);
    }
  }
}