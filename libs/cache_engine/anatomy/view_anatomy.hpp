#pragma once
// D11 / L-76c (2026-06-02) — ViewAnatomy: die VIEW-GATTUNG (Pflanze, genus()==View, non-owning). Referenziert
// einen EXTERNEN Puffer (kein eigener Speicher) und liest über die axis_layout-/axis_accessor-Policy. API:
// bind(data,size)/read(index)→value/size. KEIN insert/erase/clear (non-owning + immutabel). Doku 14 §27.2/§28/§32.

#include "anatomy_base.hpp"     // AnatomyGenus
#include "view_composition.hpp" // ViewComposition / Layout/Accessor-Policies

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// In-process View-Observer (reich; ViewObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct ViewObserverSnapshot {
    std::uint64_t read_count     = 0;
    std::uint64_t read_oob_count = 0;
    std::uint64_t bound_size     = 0;
    std::uint64_t bind_count     = 0;
};

/// ViewAnatomy<Composition> — genus()==View; non-owning V-indexed read-only über die axis_layout/accessor-Policy.
template <class Composition>
class ViewAnatomy {
public:
    using composition_t = Composition;
    using layout_t      = typename Composition::layout_policy;
    using accessor_t    = typename Composition::accessor_policy;
    using element_type  = std::uint64_t;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::View; }            // Pflanze
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 7

    // ── View-Gattungs-API (non-owning) — bind externer Puffer + read über layout/accessor ──
    void bind(element_type const* data, std::size_t size) noexcept {
        data_ = data;
        size_ = (data == nullptr) ? 0 : size;
        ++obs_.bind_count;
        obs_.bound_size = static_cast<std::uint64_t>(size_);
    }
    [[nodiscard]] std::optional<element_type> read(std::uint64_t index) const noexcept {
        ++obs_.read_count;
        std::size_t const off = layout_.index_of(static_cast<std::size_t>(index)); // axis_layout
        if (data_ == nullptr || off >= size_) {
            ++obs_.read_oob_count;
            return std::nullopt;
        }
        return accessor_.access(data_, off); // axis_accessor
    }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    [[nodiscard]] ViewObserverSnapshot observe_all() const noexcept { return obs_; }

private:
    element_type const*          data_ = nullptr; // EXTERN (non-owning)
    std::size_t                  size_ = 0;
    layout_t                     layout_{};
    accessor_t                   accessor_{};
    mutable ViewObserverSnapshot obs_{}; // mutable: read() ist const, zählt aber Zugriffe
};

} // namespace comdare::cache_engine::anatomy
