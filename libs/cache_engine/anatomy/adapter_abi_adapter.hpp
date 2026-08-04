#pragma once
// D4b / L-75 (2026-06-02) — AdapterAbiAdapter: Runtime-ABI-Adapter der CONTAINER-Gattung (Adapter/Wirbelloses),
// analog SearchAlgorithmAbiAdapter für SearchAlgorithm. Bridge eine AdapterAnatomy<Composition> (Compile-Time-
// Concept) zur Runtime-ABI-Schicht (IAnatomyBase → IExecutionEngine) + dem Container-Antrieb (IAdapterTier).
//
// Eine generierte Container-Permutations-.dll exportiert EXAKT EINEN solchen Adapter via comdare_create_anatomy()
// (gibt IAnatomyBase* — der Loader ist gattungs-agnostisch, der Container-Dock fragt dynamic_cast<IAdapterTier*>).
// Cross-Genus-Adapter sind type-system-mathematisch unmöglich (Doku 14 §32) → static_assert genus()==Adapter.

// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE): der Adapter erbt ZUSAETZLICH IAdapterTierV2 und
// liefert darueber die per-Achsen-Wire-Form AdapterObserverAggregate<11> -- inklusive der beiden
// Symmetrie-Schliessungen observable_axis_count und underflow_count (Katalog Sektion 2 Punkt 5).
// APPEND-ONLY: IAdapterTier und AdapterObserverSnapshotV1 bleiben unveraendert (neue Basis HINTEN).

#include "anatomy_base.hpp"    // IAnatomyBase + AnatomyConcept
#include "adapter_anatomy.hpp" // AdapterAnatomy / AdapterObserverSnapshot
#include "adapter_tier.hpp"    // IAdapterTier + AdapterObserverSnapshotV1
#include "adapter_tier_v2.hpp" // E-24 C6: IAdapterTierV2 + AdapterObserverAggregate<11>
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// AdapterAbiAdapter<A> — generischer Runtime-ABI-Adapter für die Container-Gattung (Adapter in Tier-Metapher).
/// Vorbedingung: A erfüllt AnatomyConcept UND A::genus() == AnatomyGenus::Adapter (Compile-Zeit-Validierung).
///
/// #87+#90 (2026-06-03, Doku 14 §28): die Adapter-Tier-Unterklasse hat 13 Achsen (12 geteilt/delegiert +
/// inner_container), KEINE „ordering"-Achse. Der Adapter treibt put/get über die DLL-Grenze; get() == FIFO-
/// Default (pop_front), die Disziplin FIFO/LIFO ist API-Nutzung (§26.4). Ein Adapter ist unbeschränkt.
template <AnatomyConcept A>
class AdapterAbiAdapter final : public IAnatomyBase, public IAdapterTier, public IAdapterTierV2 {
    static_assert(A::genus() == AnatomyGenus::Adapter,
                  "AdapterAbiAdapter erwartet eine Container-Gattung-Anatomie (AnatomyGenus::Adapter). "
                  "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    using element_type = typename A::element_type;

    // ── IExecutionEngine-Pflicht (Wurzel-Schicht; engine_kind() ist in IAnatomyBase final = Anatomy) ──
    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }

    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }

    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    // ── IAnatomyBase-Pflicht (Anatomie-Schicht) ──
    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id() const noexcept override { return A::paper_id(); }
    [[nodiscard]] AnatomyGenus     genus() const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t      organ_count() const noexcept override { return A::organ_count(); }

    // ── IAdapterTier-Pflicht (Container-Antrieb + Observer durch die ABI-Grenze) ──
    void tier_put(std::uint64_t value) noexcept override { anatomy_.put(static_cast<element_type>(value)); }
    [[nodiscard]] bool tier_get(std::uint64_t* out_value) noexcept override {
        auto r = anatomy_.get();
        if (!r) return false;
        if (out_value != nullptr) *out_value = static_cast<std::uint64_t>(*r);
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(anatomy_.size());
    }
    void tier_clear() noexcept override { anatomy_.clear(); }

    void tier_observe_container(AdapterObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        AdapterObserverSnapshot const s = anatomy_.observe_all();
        AdapterObserverSnapshotV1     v{};
        v.push_count        = s.push_count;
        v.pop_count         = s.pop_count;
        v.front_reads       = s.front_reads;
        v.back_reads        = s.back_reads;
        v.current_occupancy = s.current_occupancy;
        v.peak_occupancy    = s.peak_occupancy;
        // E-24 C6 (Katalog Sektion 2 Punkt 7, Kommentar-Nachzug auf das Ist): LIVE sind es 11 Slots
        // (10 geteilt/delegiert + inner_container, INC-2d) -- die frozen Legacy-Konstante
        // kAdapterCompositionSlotCount steht weiter bei 13 und ist NICHT die Quelle. Der Wert selbst
        // kam schon immer aus A::organ_count(), also aus der LIVEN Komposition; nur der Kommentar war stale.
        v.organ_count = A::organ_count(); // 11 (10 geteilt/delegiert + inner_container, live)
        *out          = v;
    }

    // -- IAdapterTierV2 (E-24 C6: die PER-ACHSEN-Wire-Form + die beiden Symmetrie-Schliessungen) ----
    /// Projiziert observe_axes() in den flachen Gattungs-Wire-POD. observable_axis_count quert damit
    /// ERSTMALS die Adapter-Grenze (im V1-POD fehlte das Feld als einzigem der vier), und
    /// underflow_count steht in der inner_container-Zeile.
    void tier_observe_container_axes(AdapterObserverAggregateWire* out) const noexcept override {
        if (out == nullptr) return;
        AdapterObserverAggregateWire w{};
        fill_adapter_observer_aggregate(anatomy_, w);
        *out = w;
    }

private:
    // ASCII-Transliteration mitgezogen, weil die Zeile in diesem Commit ohnehin angefasst wird
    // (13 -> 11 live); Inhalt unveraendert.
    A anatomy_{}; // unbeschraenkter Container-Adapter (11 Achsen live, inner_container real getrieben)
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

} // namespace comdare::cache_engine::anatomy
