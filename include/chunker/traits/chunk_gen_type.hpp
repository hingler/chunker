#ifndef CHUNK_GEN_TYPE_H_
#define CHUNK_GEN_TYPE_H_
// accept chunk identifier as a param
// return desired type

#include "chunker/ChunkIdentifier.hpp"

#include <memory>
#include <type_traits>

namespace chunker {
  namespace traits {
    namespace impl_ {
      struct chunk_gen_type_impl {
        template <typename ChunkGenerator, typename ReturnType,
        typename Generate = std::is_same<ReturnType, decltype(std::declval<ChunkGenerator&>().Generate(chunker::ChunkIdentifier()))>>
        static std::true_type test(int);

        template <typename ChunkGenerator, typename ReturnType, typename...>
        static std::false_type test(...);
      };

      struct chunk_gen_factory_type_impl {
        template <typename ChunkGenFactory, typename ChunkGenerator,
        typename Create = std::is_same<std::shared_ptr<ChunkGenerator>, decltype(std::declval<ChunkGenFactory&>().Create())>>
        static std::true_type test(int);

        template <typename ChunkGenFactory, typename ChunkGenerator, typename...>
        static std::false_type test(...);
      };
    }

    template <typename ChunkGenerator, typename ReturnType>
    struct chunk_gen_type : decltype(impl_::chunk_gen_type_impl::test<ChunkGenerator, ReturnType>(0)) {};

    template <typename ChunkGenFactory, typename ChunkGenerator>
    struct chunk_gen_factory_type : decltype(impl_::chunk_gen_factory_type_impl::test<ChunkGenFactory, ChunkGenerator>(0)) {};
  }
}

#endif // CHUNK_GEN_TYPE_H_