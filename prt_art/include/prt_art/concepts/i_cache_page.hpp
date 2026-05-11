#pragma once
// i_cache_page.hpp - REV 5.1 ICachePage (Schicht 3 - physisch)
// Volle Cache-Engine-Implementation liegt unter cache_engine/

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::prt_art {

template <typename K, typename V> class ISearchPage;

/// ResidencyKind - Cache-Whereabouts (Saeule B)
enum class ResidencyKind : std::uint8_t {
    Cached, HeaderCached, Uncached, Evicted, Pinned, Migrated
};

/// FragmentationKind - INode-Fragmentierung (Worst-Case Cache-Line-Grenze)
enum class FragmentationKind : std::uint8_t {
    NotPresent,
    WhollyContained,  ///< INode liegt ganz in dieser ICachePage
    Fragmented        ///< INode an Cache-Line-Grenze gesplittet
};

/// ICachePage<K, V> - physische Cache-Page-Einheit
template <typename Key, typename Value>
class ICachePage {
protected:
    std::span<std::byte> page_bytes_{};
    std::size_t alignment_bytes_ = 64;
    /// Mehrere SearchPages koennen sich eine ICachePage teilen, oder eine SearchPage
    /// kann sich ueber mehrere ICachePages erstrecken (Worst-Case fragmentiert).

public:
    virtual ~ICachePage() = default;

    [[nodiscard]] std::size_t size_bytes() const noexcept { return page_bytes_.size(); }
    [[nodiscard]] std::size_t alignment_bytes() const noexcept { return alignment_bytes_; }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return page_bytes_; }

    [[nodiscard]] virtual ResidencyKind residency() const noexcept = 0;
};

}  // namespace comdare::prt_art
