#ifndef CHUNK_MANAGER_H_
#define CHUNK_MANAGER_H_

#include "chunker/traits/chunk_gen_type.hpp"
#include "chunker/lod/LodTreeGenerator.hpp"
#include "chunker/ChunkIdentifier.hpp"

#include "chunker/TypedChunkThreadPool.hpp"

#include <glm/glm.hpp>

#include <algorithm>

// how to constrain chunk gen?
// - type trait - we need a method which handles "generation" (accepting x/y and lod, returning returntype)

// params for chunk identifier?
// - x
// - y
// - for mesh: we need lod (how best to do? thinking just arb data would work)
// - how can we generify lod?
//   - like the idea of still incorporating "cascades" more generally
//   - thinking we can

namespace chunker {
  // takes responsibility for orchestrating chunk generation
  template <typename ChunkGenFactory, typename ChunkGenerator, typename ChunkType>
  class ChunkManager {
    typedef chunker::TypedChunkThreadPool<ChunkGenFactory, ChunkGenerator, ChunkType> PoolType;
    static_assert(chunker::traits::chunk_gen_type<ChunkGenerator, ChunkType>::value);
    static_assert(chunker::traits::chunk_gen_factory_type<ChunkGenFactory, ChunkGenerator>::value);
   public:
    // tba: want some better LOD configuration (ie: constant LOD for generation for splats)
    // - putting that info in "cascade factor" makes sense
    // - add a flag to indicate LOD bias (default scale is 1:1, only decrease rn but eventually we can increase too)

    /**
     * @brief Construct a new Chunk Manager object
     * 
     * @param chunk_generator - the underlying generator which will power our chunk generation.
     * @param thread_count - the number of threads to use for chunk generation
     * @param max_gen_distance - the max distance to generate tiles from
     * @param min_chunk_size - min size, in units, of a given chunk
     * @param cascade_factor - rate at which to decrease LOD as we back away from player
     */
    ChunkManager(
      std::shared_ptr<ChunkGenFactory> chunk_factory,
      size_t thread_count,
      double max_gen_distance,
      size_t min_chunk_size,
      double cascade_factor,
      // tba: remove default
      long lod_bias = 0
    ) : factory(chunk_factory),
        gen_dist_(max_gen_distance),
        // ensure tree size is at least 4x the max gen distance
        tree_size_(static_cast<long>(1) << static_cast<long>(ceil(log2(max_gen_distance)) + 2)),
        min_chunk_size_(min_chunk_size),
        cascade_factor_(cascade_factor),
        last_tree_(nullptr),
        tree_gen_(tree_size_, min_chunk_size_ << std::min(-lod_bias, 0L)),
        thread_pool_(thread_count, factory),
        chunk_count_(0) 
    {
      tree_gen_.cascade_factor = cascade_factor;
    }

    bool UpdateChunkData(const glm::vec3& local_position) {
      // figure out the generation center, based on origin
      long nudge_factor = tree_size_ >> MAX_CHUNK_SIZE_FACTOR;

      // relative to bottom left corner of tree
      glm::vec3 relative_pos(local_position);

      // maintains offset of bottom left corner of tree
      glm::ivec2 offset(0, 0);

      while (relative_pos.x > nudge_factor) {
        relative_pos.x -= nudge_factor;
        offset.x += nudge_factor;
      }

      while (relative_pos.x < -nudge_factor) {
        relative_pos.x += nudge_factor;
        offset.x -= nudge_factor;
      }

      while (relative_pos.z > nudge_factor) {
        relative_pos.z -= nudge_factor;
        offset.y += nudge_factor;
      }

      while (relative_pos.z < -nudge_factor) {
        relative_pos.z += nudge_factor;
        offset.y -= nudge_factor;
      }

      // we need to recenter relative pos within the tree

      long half_size = tree_size_ / 2;

      relative_pos.x += half_size;
      relative_pos.z += half_size;

      // recenter - this tracks bottom left corner of the tree now
      offset.x -= half_size;
      offset.y -= half_size;

      
      // bias impl:
      // - multiply min chunk size
      chunker::lod::lod_node* tree = tree_gen_.CreateLodTree(relative_pos, MAX_CHUNK_SIZE_FACTOR);
      if (last_tree_ != nullptr) {
        bool trees_equal = chunker::lod::lod_node::CompareTrees(tree, last_tree_);
        if (trees_equal) {
          chunker::lod::lod_node::lod_node_free(tree);
          return true;
        }
      }

      // update chunks based on tree state
      // manager will retain responsibility for stepping down the chunk tree
      // pass offset to control how the genned chunks are offset
      UpdateChunks(tree, offset);
      chunker::lod::lod_node::lod_node_free(last_tree_);
      last_tree_ = tree;
      return false;
    }

    size_t GetChunkCount() {
      return chunk_count_;
    }

    void wait() {
      thread_pool_.Wait();
    }

    typename PoolType::CacheType::iterator begin() {
      if (!thread_pool_.Empty()) {
        thread_pool_.Wait();
      }

      return thread_pool_.BoundedIterator(chunk_count_);
    }

    typename PoolType::CacheType::iterator end() {
      if (!thread_pool_.Empty()) {
        thread_pool_.Wait();
      }
      
      return thread_pool_.End();
    }

   private:
    static const long MAX_CHUNK_SIZE_FACTOR = 3;

    void UpdateChunks(const chunker::lod::lod_node* tree, const glm::ivec2 offset) {
      // prep thread pool
      // specify offset
      // specify current node size
      // pass in the whole tree (for tree gen)
      size_t node_size = static_cast<size_t>(tree_size_);
      // tba: cache chunk count somewhere
      // (alt: we obviously can't generate index data here!!)
      chunk_count_ = UpdateChunks_Recurse(offset.x, offset.y, 0, 0, node_size, tree, tree);
      thread_pool_.Wake();
    }

    // offsets are world space
    size_t UpdateChunks_Recurse(
      long offset_x,
      long offset_y,
      long tree_x,
      long tree_y,
      size_t node_size,
      const chunker::lod::lod_node* node,
      const chunker::lod::lod_node* tree
    ) {
      if (node->tl == nullptr) {
        // no children - generate this node
        // need to encode origin of tree
        chunker::ChunkIdentifier identifier(offset_x, offset_y, tree_x, tree_y, node_size, min_chunk_size_, tree_size_, tree);
        thread_pool_.Enqueue(identifier);
        // enqueue
        return 1;
      } else {
        assert(node->tr != nullptr);
        assert(node->bl != nullptr);
        assert(node->br != nullptr);
        long half_size = node_size >> 1;

        size_t chunk_count = 0;
        chunk_count += UpdateChunks_Recurse(offset_x,             offset_y,             tree_x,             tree_y,             half_size, node->bl, tree);
        chunk_count += UpdateChunks_Recurse(offset_x + half_size, offset_y,             tree_x + half_size, tree_y,             half_size, node->br, tree);
        chunk_count += UpdateChunks_Recurse(offset_x,             offset_y + half_size, tree_x,             tree_y + half_size, half_size, node->tl, tree);
        chunk_count += UpdateChunks_Recurse(offset_x + half_size, offset_y + half_size, tree_x + half_size, tree_y + half_size, half_size, node->tr, tree);
        return chunk_count;
      }
    }

    std::shared_ptr<ChunkGenFactory> factory;
    double gen_dist_;
    long tree_size_;
    size_t min_chunk_size_;
    double cascade_factor_;

    // mem leak here lole
    chunker::lod::lod_node* last_tree_ = nullptr;
    chunker::lod::LodTreeGenerator tree_gen_;
    chunker::TypedChunkThreadPool<ChunkGenFactory, ChunkGenerator, ChunkType> thread_pool_;

    size_t chunk_count_;

    // tba: need a thread pool
  };
}

#endif // CHUNK_MANAGER_H_