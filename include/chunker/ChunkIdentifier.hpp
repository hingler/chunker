#ifndef CHUNK_IDENTIFIER_H_
#define CHUNK_IDENTIFIER_H_

#include <algorithm>
#include <functional>

#include "chunker/lod/lod_node.hpp"
#include "chunker/util/Fraction.hpp"

#include "debug/Logger.hpp"

// d'oh!
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace chunker {
  struct ChunkNeighbors {
    // edges
    util::Fraction l;
    util::Fraction r;
    util::Fraction u;
    util::Fraction d;

    // corners
    util::Fraction tl;
    util::Fraction tr;
    util::Fraction bl;
    util::Fraction br;

    bool operator==(const ChunkNeighbors& rhs) const {
      return (
        l == rhs.l
        && r == rhs.r
        && u == rhs.u
        && d == rhs.d
        && tl == rhs.tl
        && tr == rhs.tr
        && bl == rhs.bl
        && br == rhs.br
      );
    }
  };

  struct ChunkIdentifier {
    long x;
    long y;

    // start deprecating this
    size_t size;
    size_t chunk_res;

    // vec2 specifying x/y dims

    
    util::Fraction scale;
    glm::u64vec2 sample_dims;
    // tba: swap scale over to double
    // (add a sample param for it :3 i think only collider and splat are using it)
    // (concern: pixel perfect :[)

    ChunkNeighbors neighbors;

    ChunkIdentifier() {
      x = 0;
      y = 0;
      size = 0;
      chunk_res = 0;

      scale = 1;
      sample_dims.x = 0;
      sample_dims.y = 0;

      neighbors.bl = 0;
      neighbors.l = 0;
      neighbors.tl = 0;
      neighbors.br = 0;
      neighbors.r = 0;
      neighbors.tr = 0;
      neighbors.u = 0;
      neighbors.d = 0;
    }

    // dont like that this is locked to pot - eventually might want to break out of that
    
    ChunkIdentifier(const glm::i64vec2& global_offset, const glm::ivec2& tree_offset, size_t chunk_res, const chunker::util::Fraction& scale, size_t tree_res, const lod::lod_node* tree) {
      // idea: should be able to offset in generation by a simple amount (tba)

      // global offset can be totally arbitrary
      this->x = global_offset.x;
      this->y =  global_offset.y;
      this->scale = scale;
      this->sample_dims = glm::ivec2(chunk_res);

      size_t chunk_size = static_cast<size_t>(scale * chunk_res);
      util::Fraction base_scale(tree_res, chunk_res);

      // lod calculations
      glm::vec2 near_corner = glm::vec2(tree_offset.x - 0.5f, tree_offset.y - 0.5f);
      glm::vec2 far_corner = glm::vec2(tree_offset.x + chunk_size + 0.5f, tree_offset.y + chunk_size + 0.5f);

      float half_size = static_cast<float>(chunk_size / 2);

      // take eight lod samples
      // figured it out - offset and tree no longer match!

      // print these out next
      // want to get consistent generatioN!!!
      // (the issue is specifically corner cases, and how this neighbor table is gen'd)
      neighbors.bl = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, near_corner)                           , scale);
      neighbors.tr = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, far_corner)                            , scale);
      neighbors.tl = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, glm::vec2(near_corner.x, far_corner.y)), scale);
      neighbors.br = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, glm::vec2(far_corner.x, near_corner.y)), scale);
      neighbors.l = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, near_corner + glm::vec2(0.0, half_size)), scale);
      neighbors.r = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, far_corner - glm::vec2(0.0, half_size)) , scale);
      neighbors.u = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, far_corner - glm::vec2(half_size, 0.0)) , scale);
      neighbors.d = std::max(lod::lod_node::GetChunkStep(tree, base_scale, tree_res, near_corner + glm::vec2(half_size, 0.0)), scale);
    }

    // deprecate :3
    ChunkIdentifier(long x, long y, long tree_x, long tree_y, size_t chunk_size, size_t chunk_res, size_t tree_res, const lod::lod_node* tree) {
      this->x = x;
      this->y = y;
      this->size = chunk_size;
      this->chunk_res = chunk_res;

      // lod calculations
      glm::vec2 near_corner = glm::vec2(tree_x - 0.5f, tree_y - 0.5f);
      glm::vec2 far_corner = glm::vec2(tree_x + chunk_size + 0.5f, tree_y + chunk_size + 0.5f);

      float half_size = static_cast<float>(chunk_size / 2);

      // take eight lod samples
      // figured it out - offset and tree no longer match!
      neighbors.bl = std::max(lod::lod_node::GetChunkSize(tree, tree_res, near_corner)                           , chunk_size);
      neighbors.tr = std::max(lod::lod_node::GetChunkSize(tree, tree_res, far_corner)                            , chunk_size);
      neighbors.tl = std::max(lod::lod_node::GetChunkSize(tree, tree_res, glm::vec2(near_corner.x, far_corner.y)), chunk_size);
      neighbors.br = std::max(lod::lod_node::GetChunkSize(tree, tree_res, glm::vec2(far_corner.x, near_corner.y)), chunk_size);
      neighbors.l = std::max(lod::lod_node::GetChunkSize(tree, tree_res, near_corner + glm::vec2(0.0, half_size)), chunk_size);
      neighbors.r = std::max(lod::lod_node::GetChunkSize(tree, tree_res, far_corner - glm::vec2(0.0, half_size)) , chunk_size);
      neighbors.u = std::max(lod::lod_node::GetChunkSize(tree, tree_res, far_corner - glm::vec2(half_size, 0.0)) , chunk_size);
      neighbors.d = std::max(lod::lod_node::GetChunkSize(tree, tree_res, near_corner + glm::vec2(half_size, 0.0)), chunk_size);
    }

    size_t GetStepSize() const {
      return (size / chunk_res);
    }

    // tba: need specifiers for chunk edges
    // (probably just eight ints specifying the chunk's eight neighbors as these are relevant for generation as well)

    bool operator==(const ChunkIdentifier& rhs) const {
      return (rhs.x == x && rhs.y == y && rhs.size == size && rhs.chunk_res == chunk_res && rhs.scale == scale && rhs.sample_dims == sample_dims && rhs.neighbors == neighbors);
    }
  };
}

namespace std {
  template<>
  struct hash<chunker::ChunkNeighbors> {
    hash<chunker::util::Fraction> fract_hash;
    size_t operator()(const chunker::ChunkNeighbors& n) const {
      return (
        fract_hash(n.l)
        ^ fract_hash(n.r)
        ^ fract_hash(n.u)
        ^ fract_hash(n.d)
        ^ fract_hash(n.tl)
        ^ fract_hash(n.tr)
        ^ fract_hash(n.bl)
        ^ fract_hash(n.br)
      );
    }
  };

  template<>
  struct hash<chunker::ChunkIdentifier> {
    std::hash<long> long_hash;
    std::hash<size_t> size_t_hash;
    std::hash<glm::u64vec2> vec_hash;
    std::hash<chunker::util::Fraction> fract_hash;
    std::hash<chunker::ChunkNeighbors> neighbor_hash;
    size_t operator()(const chunker::ChunkIdentifier& identifier) const { 
      // this is fine for now i think
      return ((long_hash(identifier.x) ^ long_hash(identifier.y) + vec_hash(identifier.sample_dims)) * identifier.size ^ identifier.chunk_res) ^ fract_hash(identifier.scale) ^ neighbor_hash(identifier.neighbors);
    }
  };
}

#endif // CHUNK_IDENTIFIER_H_