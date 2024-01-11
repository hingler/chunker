#include "chunker/lod/LodTreeGenerator.hpp"

namespace chunker {
  namespace lod {
    lod_node* LodTreeGenerator::CreateLodTree(const glm::vec3& local_position) {
      return CreateLodTree(local_position, 0);
    }
    
    lod_node* LodTreeGenerator::CreateLodTree(const glm::vec3& local_position, int force_divide) {
      auto* node = lod_node::lod_node_alloc();
      int size = size_;
      double cascade_real = cascade_factor / CASCADE_MUL_FACTOR;
      while (size > chunk_res_) {
        cascade_real *= CASCADE_MUL_FACTOR;
        size >>= 1;
      }

      CreateLodTree_recurse(
        0,
        0,
        size_,
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
      double cascade_threshold,
      const glm::vec3& local_position,
      lod_node* root,
      int force_divide) 
    {
      // no longer descend
      if (node_size <= chunk_res_) {
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

      CreateLodTree_recurse(x,                 y,                 new_node_size, new_cascade_threshold, local_position, root->bl, force_divide - 1);
      CreateLodTree_recurse(x + new_node_size, y,                 new_node_size, new_cascade_threshold, local_position, root->br, force_divide - 1);
      CreateLodTree_recurse(x,                 y + new_node_size, new_node_size, new_cascade_threshold, local_position, root->tl, force_divide - 1);
      CreateLodTree_recurse(x + new_node_size, y + new_node_size, new_node_size, new_cascade_threshold, local_position, root->tr, force_divide - 1);
    }
  }
}