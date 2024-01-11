#ifndef LOD_TREE_GENERATOR_H_
#define LOD_TREE_GENERATOR_H_

#define CASCADE_MUL_FACTOR 2

#include "chunker/lod/lod_node.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace chunker {
  namespace lod {
    /**
     * @brief Generates an LOD quadtree for a terrain object at an arbitrary location
     */
    class LodTreeGenerator {
    public:
      LodTreeGenerator(int size, int chunk_res)
       : size_(size),
         chunk_res_(chunk_res)
      {
        // PoT + 1
        assert(((size) & (size - 1)) == 0);
        assert(((chunk_res) & (chunk_res - 1)) == 0);
        assert(chunk_res_ > 1);
        assert(size_ > 1);
      }



      lod_node* CreateLodTree(const glm::vec3& local_position);
      lod_node* CreateLodTree(const glm::vec3& local_position, int force_divide);

      // distance cap for min subdivision level
      // other cascades are handled internally
      double cascade_factor;

      // max number of chunk subdivisions to perform
    private:
      const int size_;
      const int chunk_res_;
      void CreateLodTree_recurse(int x, int y, int node_size, double cascade_threshold, const glm::vec3& local_position, lod_node* root, int force_divide);
    };
  }
}

#endif // LOD_TREE_GENERATOR_H_