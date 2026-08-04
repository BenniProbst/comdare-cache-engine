#pragma once
// V41.F.6.1 axis_03m_mapping DirectPlacement MP01 (2026-05-26)
//
// @topic traversal @achse 03m @family MP01 DirectPlacement
// @subaxis MP1 direct_access
//
// **Algorithmus-Pattern:** direktes Slot-zu-Absolute-Offset-Mapping (linear
// packing). Klassisches Array-of-Pointers-Layout aus B-Tree Inner-Nodes
// (Bayer/McCreight 1972). Vorteil: O(N) Lookup, kein Pool-Indirection,
// einfache Cache-Locality.
//
// Standalone-Implementation: dynamische Slot-Tabelle aus pair<slot, absolute_offset> +
// std::find_if linear-scan.
//
// Allocation (A8-S5-03, 2026-08-04): der Tabellen-Speicher kommt REAL ueber das Allokator-ACHSEN-Interface
// (mapping_slot_allocator_t + StdAllocatorAdapter, s. axis_03m_mapping_base.hpp) statt ueber den
// Default-Allokator -- Schnitt-Regel Dossier 20260803-a8_f2 Abschn. 3.4. Fehlerklasse unveraendert:
// [[allocation-failure-exception]] -- seit Posten 64 (2026-08-04) wieder als ECHTER Wurf: die
// Achsen-Strategie meldet OOM per nullptr, der StdAllocatorAdapter uebersetzt ihn in std::bad_alloc.

#include "axis_03m_mapping_base.hpp"
#include "axis_03m_mapping_subaxes_mp1_to_mp2.hpp"
#include "concepts/axis_03m_mapping_concept.hpp"
#include "concepts/axis_03m_mapping_cache_engine_permutation_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/mapping/axis_03m_mapping_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::mapping {

class DirectPlacement : public MappingBase<DirectPlacement> {
public:
    static constexpr bool enabled = flags::direct_placement_enabled;

    using slot_index_type = std::uint16_t;
    using offset_type     = std::size_t;
    using size_type       = std::size_t;
    using topic_tag       = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag        = subaxes::direct_access_tag;
    using family_id       = std::integral_constant<int, 1>; // MP01
    /// A8-S5-03 Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse (nicht deklarativ,
    /// sondern real -- mappings_ traegt den StdAllocatorAdapter dieses allocator_, s. Member unten).
    using allocator_type = mapping_slot_allocator_t;

private:
    using entry_type  = std::pair<slot_index_type, offset_type>;
    using entry_alloc = typename allocator_type::template StdAllocatorAdapter<entry_type>;

public:
    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "direct_placement"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::mapping::DirectPlacement",
                                  "axes/mapping/axis_03m_mapping_direct_placement.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "DirectPlacement (prt-art INode::placement_page_ direct-Pointer)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DIRECT_PLACEMENT"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool is_pool_relative() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_reverse_lookup() noexcept { return true; } // linear-scan
    [[nodiscard]] static constexpr bool requires_pool_base() noexcept { return false; }

    // -- A8-S5-03 Lebensdauer-Vertrag der Achsen-Verdrahtung --------------------------------------------
    // Der StdAllocatorAdapter haelt einen Zeiger auf allocator_ (Wert-Adapter, EBO-freundlich). Daraus folgt
    // GENAU dieselbe Regel wie im Referenz-Muster btree_node_pool_store.hpp:19/:84-105:
    //   (a) allocator_ MUSS vor mappings_ deklariert sein (Member-Reihenfolge unten),
    //   (b) mappings_ wird IMMER mit allocator_.as_std_allocator<entry_type>() konstruiert (der Adapter ist
    //       nicht default-konstruierbar -- es gibt kein stilles Zurueckfallen auf einen Default-Allokator),
    //   (c) die Kopie REBINDET auf das EIGENE allocator_ (sonst zeigte der Adapter der Kopie auf die Quelle),
    //   (d) Move wird BEWUSST nicht deklariert: die benutzerdeklarierte Kopie unterdrueckt den impliziten Move,
    //       ein std::move degradiert damit zur (korrekt rebindenden) Kopie statt den Fremd-Adapter zu stehlen.
    // std::vector kopiert bei propagate_on_container_copy_assignment=false (Default fuer allocator_traits) den
    // Allokator NICHT mit -- die Ziel-Tabelle behaelt in operator= ihren eigenen Adapter. Genau das ist gewollt.
    DirectPlacement() : mappings_(allocator_.template as_std_allocator<entry_type>()) {}
    DirectPlacement(DirectPlacement const& o)
        : allocator_(o.allocator_), mappings_(o.mappings_, allocator_.template as_std_allocator<entry_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        // Memento-Symmetrie (analog btree_node_pool_store.hpp:91): die transiente Kopier-Allokation der Vollkopie
        // ist kein Mess-Ereignis der Achse -> Statistik auf den Quell-Stand zuruecksetzen.
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    DirectPlacement& operator=(DirectPlacement const& o) {
        if (this != &o) {
            mappings_ = o.mappings_; // Allokator propagiert NICHT -> dieses Objekt behaelt seinen Adapter
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~DirectPlacement() = default;

    [[nodiscard]] bool operator==(DirectPlacement const& other) const noexcept {
        return mappings_.size() == other.mappings_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]] -- A8-S5-03 PRAEZISIERT (Auflage 11, Fehlerklassen),
    /// Posten 70 NACHGEFUEHRT (2026-08-04): Der Wachstums-Fehlerpfad laeuft seit dem Schnitt ueber die
    /// Allokator-ACHSE, nicht mehr ueber operator new. Die Achsen-Strategie meldet OOM per nullptr
    /// (axis_06_allocator_exgen.hpp -> portable_aligned_alloc); der StdAllocatorAdapter reicht diesen
    /// nullptr seit POSTEN 64 NICHT mehr durch, sondern uebersetzt ihn an genau EINER Stelle in
    /// std::bad_alloc (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate). Damit ist
    /// register_slot wieder ein echter werfender Pfad -- KEIN UB-Pfad mehr, in dem der besitzende
    /// Container in Nullspeicher konstruiert. Fehlerklasse der Achse bleibt kOrganAxisErrorFloor
    /// (MappingBase::error_classes, FK-5): OOM ist weiter ein Failed-Fall, nur der Traeger ist die
    /// Versorger-Achse. Der frueher hier notierte offene Punkt "Konversion nullptr -> Wurf gehoert ins
    /// Adapter-ZIEL-Interface" ist damit ERLEDIGT (Posten 64).
    void register_slot(slot_index_type s, offset_type o) {
        auto it = std::find_if(mappings_.begin(), mappings_.end(), [s](auto const& m) { return m.first == s; });
        if (it != mappings_.end()) {
            it->second = o; // update
        } else {
            mappings_.emplace_back(s, o);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (mappings_.size() > stats_.peak_mapped) stats_.peak_mapped = mappings_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<offset_type> resolve_offset(slot_index_type s) const {
        auto it = std::find_if(mappings_.begin(), mappings_.end(), [s](auto const& m) { return m.first == s; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        stats_.total_indirection_steps += 1; // MP01: 1 Adress-Aufloesung (Offset absolut gespeichert, keine Rebase)
        if (it != mappings_.end())
            ++stats_.total_resolve_hit_count;
        else
            ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return it->second;
    }

    [[nodiscard]] std::optional<slot_index_type> reverse_lookup(offset_type o) const {
        auto it = std::find_if(mappings_.begin(), mappings_.end(), [o](auto const& m) { return m.second == o; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_reverse_lookup_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return it->first;
    }

    [[nodiscard]] size_type mapped_count() const noexcept { return mappings_.size(); }
    void                    clear() noexcept { mappings_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::MappingStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

    /// A8-S5-03 VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128
    /// store_allocator_statistics): die Statistik der Versorger-Strategie DIESES Organs. Damit ist die
    /// Form-B-Aussage der S5-Gate-Wache am Objekt pruefbar -- ein deklarierter allocator_type ohne reale
    /// Verdrahtung bliebe hier auf 0 stehen (Form-B-Grenze, s. tests/unit/s5_family_alloc_conformance.hpp:31).
    /// NICHT im T6-Mess-Pfad: T6 misst den Allokator DER KOMPOSITION, nicht diesen privaten Versorger.
    [[nodiscard]] typename allocator_type::snapshot_t mapping_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ VOR mappings_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar oben).
    allocator_type                       allocator_{};
    std::vector<entry_type, entry_alloc> mappings_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::MappingStatistics stats_{};
    mutable observer_t                  observer_{};
#endif
};

} // namespace comdare::cache_engine::mapping

namespace comdare::cache_engine::mapping {
static_assert(concepts::MappingVariant<DirectPlacement>);
static_assert(concepts::CacheEngineMappingPermutationStrategy<DirectPlacement>);
} // namespace comdare::cache_engine::mapping
