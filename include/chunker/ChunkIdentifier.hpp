#ifndef CHUNK_IDENTIFIER_H_
#define CHUNK_IDENTIFIER_H_

#include <algorithm>
#include <functional>

#include "chunker/lod/lod_node.hpp"

// d'oh!
#include <glm/glm.hpp>

namespace chunker {
  struct ChunkNeighbors {
    // edges
    size_t l;
    size_t r;
    size_t u;
    size_t d;

    // corners
    size_t tl;
    size_t tr;
    size_t bl;
    size_t br;

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
    size_t size;
    size_t chunk_res;
    ChunkNeighbors neighbors;

    ChunkIdentifier() {
      x = 0;
      y = 0;
      size = 0;
      chunk_res = 0;

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
      return (rhs.x == x && rhs.y == y && rhs.size == size && rhs.chunk_res == chunk_res && rhs.neighbors == neighbors);
    }
  };
}

namespace std {
  template<>
  struct hash<chunker::ChunkNeighbors> {
    size_t operator()(const chunker::ChunkNeighbors& n) const {
      return (
        n.l
        ^ n.r
        ^ n.u
        ^ n.d
        ^ n.tl
        ^ n.tr
        ^ n.bl
        ^ n.br
      );
    }
  };

  template<>
  struct hash<chunker::ChunkIdentifier> {
    size_t operator()(const chunker::ChunkIdentifier& identifier) const {
      // this is fine for now i think
      return (((identifier.x << 24) | identifier.y) * identifier.size ^ identifier.chunk_res) ^ hash<chunker::ChunkNeighbors>{}(identifier.neighbors);
    }
  };
}

#endif // CHUNK_IDENTIFIER_H_