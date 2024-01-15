#ifndef CHUNKER_TYPE_H_
#define CHUNKER_TYPE_H_

#include "chunker/ChunkIdentifier.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

// "chunk" function that accepts some identifying data, and returns a vector of chunk identifiers
// "stitch" function that accepts a map from ids -> chunk types and returns some finalized type

namespace chunker {
  namespace traits {
    namespace impl_ {
      struct chunker_type_impl {
        template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType,
        typename Chunk = std::is_same<std::vector<ChunkIdentifier>, decltype(std::declval<ChunkerType&>().Chunk(std::declval<JobType&>()))>>
        static std::true_type test(int);

        template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType, typename...>
        static std::false_type test(...);
      };

      struct stitch_type_impl {
        template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType,
        typename Stitch = std::is_same<ReturnType, decltype(std::declval<ChunkerType&>().Stitch(std::declval<JobType&>(), std::declval<std::vector<std::shared_ptr<ChunkType>>&>()))>>
        static std::true_type test(int);

        template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType, typename...>
        static std::false_type test(...);
      };
    }

    // test
    template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType>
    struct chunker_type : decltype(impl_::chunker_type_impl::test<ChunkerType, ChunkType, JobType, ReturnType>(0)) {};

    template <typename ChunkerType, typename ChunkType, typename JobType, typename ReturnType>
    struct stitcher_type : decltype(impl_::stitch_type_impl::test<ChunkerType, ChunkType, JobType, ReturnType>(0)) {};
  }
}

#endif // CHUNKER_TYPE_H_