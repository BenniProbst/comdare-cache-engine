#pragma once
// V41.F.6.1.F1 axis_02 PathCompressionNone Default-Wrapper (Skelett-Stufe-A)

#include "axis_02_path_compression_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_02_path_compression {

/// PathCompressionNone — Default-Variante: keine Compression (Roh-Pfad).
class PathCompressionNone : public PathCompressionBase<PathCompressionNone> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using family_id = std::integral_constant<int, 1>;

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "path_compression_none"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "PathCompressionNone (raw path, no compression)"; }

    [[nodiscard]] double compression_ratio() const noexcept { return 1.0; }  // raw = 1:1
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_02_path_compression {
    static_assert(concepts::PathCompressionStrategy<PathCompressionNone>);
}
