#pragma once
// V41.F.6.1 axis_03m_mapping PoolRelative MP02 (2026-05-26)
//
// @topic traversal @achse 03m @family MP02 PoolRelative
// @subaxis MP2 pool_relative_access
//
// **Algorithmus-Pattern:** Persistent-Data-Structure mit Snapshot-Versioning
// (Driscoll/Sarnak/Sleator/Tarjan: "Making Data Structures Persistent." JCSS
// 38(1):86-124, 1989). Alle Offsets sind relativ zu einem pool_base_address
// gespeichert.
//
// Standalone-Implementation: dynamische Slot-Tabelle aus pair<slot, relative_offset>. Vorteil:
// Pool kann komplett umalloziert werden ohne Mapping-Tabelle anzufassen
// — nur pool_base_ wird neu gesetzt (O(1) Rebase statt O(N) Translation).
//
// Constructor benoetigt pool_base_address (Pflicht-Sonderfall:
// requires_pool_base()=true).
//
// Allocation (A8-S5-03, 2026-08-04): der Tabellen-Speicher kommt REAL ueber das Allokator-ACHSEN-Interface
// (mapping_slot_allocator_t + StdAllocatorAdapter, s. axis_03m_mapping_base.hpp) statt ueber den
// Default-Allokator -- Schnitt-Regel Dossier 20260803-a8_f2 Abschn. 3.4. Fehlerklasse: s. register_slot
// (seit Posten 64, 2026-08-04, wieder ein echter std::bad_alloc-Wurf ueber den StdAllocatorAdapter).

#include "axis_03m_mapping_base.hpp"
#include "axis_03m_mapping_subaxes_mp1_to_mp2.hpp"
#include "concepts/axis_03m_mapping_concept.hpp"
#include "concepts/axis_03m_mapping_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03m_mapping_pool_rebasable_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <organ_axes/mapping/axis_03m_mapping_flags.hpp>
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

class PoolRelative : public MappingBase<PoolRelative> {
public:
    static constexpr bool enabled = flags::pool_relative_enabled;

    using slot_index_type = std::uint16_t;
    using offset_type     = std::size_t;
    using size_type       = std::size_t;
    using topic_tag       = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag        = subaxes::pool_relative_access_tag;
    using family_id       = std::integral_constant<int, 2>; // MP02
    /// A8-S5-03 Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse (real verdrahtet,
    /// s. Member unten + mapping_allocator_statistics()).
    using allocator_type = mapping_slot_allocator_t;

private:
    using entry_type  = std::pair<slot_index_type, offset_type>;
    using entry_alloc = typename allocator_type::template StdAllocatorAdapter<entry_type>;

public:
    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "pool_relative"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::mapping::PoolRelative",
                                  "organ_axes/mapping/axis_03m_mapping_pool_relative.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PoolRelative (prt-art CustomAlignedStructure pool-relative)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "POOL_RELATIVE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool is_pool_relative() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_reverse_lookup() noexcept { return true; }
    /// SONDERFALL: Constructor benoetigt pool_base_address.
    [[nodiscard]] static constexpr bool requires_pool_base() noexcept { return true; }

    // -- A8-S5-03 Lebensdauer-Vertrag der Achsen-Verdrahtung (identisch DirectPlacement, Muster
    //    btree_node_pool_store.hpp:19/:84-105): allocator_ vor mappings_; mappings_ IMMER mit dem Adapter
    //    dieses allocator_ konstruiert; die Kopie REBINDET auf ihr eigenes allocator_; Move bewusst nicht
    //    deklariert (degradiert zur rebindenden Kopie statt den Fremd-Adapter zu stehlen).
    //    NICHT mehr noexcept: die Tabellen-Konstruktion laeuft jetzt ueber die Versorger-Achse.
    /// Default-Constructor (pool_base=0 fuer Test-Builds).
    PoolRelative() : pool_base_(0), mappings_(allocator_.template as_std_allocator<entry_type>()) {}
    explicit PoolRelative(offset_type pool_base)
        : pool_base_(pool_base), mappings_(allocator_.template as_std_allocator<entry_type>()) {}
    PoolRelative(PoolRelative const& o)
        : allocator_(o.allocator_), pool_base_(o.pool_base_),
          mappings_(o.mappings_, allocator_.template as_std_allocator<entry_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        // Memento-Symmetrie (analog btree_node_pool_store.hpp:91): transiente Kopier-Allokation verwerfen.
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    PoolRelative& operator=(PoolRelative const& o) {
        if (this != &o) {
            pool_base_ = o.pool_base_;
            mappings_  = o.mappings_; // Allokator propagiert NICHT -> eigener Adapter bleibt
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~PoolRelative() = default;

    [[nodiscard]] bool operator==(PoolRelative const& other) const noexcept {
        return mappings_.size() == other.mappings_.size() && pool_base_ == other.pool_base_;
    }

    /// SONDERFALL [[allocation-failure-exception]] -- A8-S5-03 PRAEZISIERT (Auflage 11, Fehlerklassen),
    /// Posten 70 NACHGEFUEHRT (2026-08-04): Der Wachstums-Fehlerpfad laeuft seit dem Schnitt ueber die
    /// Allokator-ACHSE. Die Strategie meldet OOM per nullptr (axis_06_allocator_exgen.hpp); der
    /// StdAllocatorAdapter reicht ihn seit POSTEN 64 NICHT mehr durch, sondern uebersetzt ihn an genau
    /// EINER Stelle in std::bad_alloc (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate)
    /// -- register_slot ist damit wieder ein echter werfender Pfad statt eines UB-Pfads. Fehlerklasse der
    /// Achse unveraendert kOrganAxisErrorFloor (MappingBase::error_classes, FK-5); der frueher hier
    /// notierte offene Punkt zur nullptr-Konversion ist damit ERLEDIGT.
    /// Stored: offset relativ zu pool_base_ (positiv). Negativ-Offsets sind verboten.
    void register_slot(slot_index_type s, offset_type absolute_offset) {
        offset_type relative = (absolute_offset >= pool_base_) ? (absolute_offset - pool_base_) : 0;
        auto        it = std::find_if(mappings_.begin(), mappings_.end(), [s](auto const& m) { return m.first == s; });
        if (it != mappings_.end()) {
            it->second = relative;
        } else {
            mappings_.emplace_back(s, relative);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (mappings_.size() > stats_.peak_mapped) stats_.peak_mapped = mappings_.size();
        observer_.notify(stats_);
#endif
    }

    /// Liefert absoluten Offset = pool_base_ + relativ.
    [[nodiscard]] std::optional<offset_type> resolve_offset(slot_index_type s) const {
        auto it = std::find_if(mappings_.begin(), mappings_.end(), [s](auto const& m) { return m.first == s; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        stats_.total_indirection_steps +=
            2; // MP02: 1 Lookup + 1 pool_base-Rebase-Translation (is_pool_relative()==true, jetzt gemessen statt nur strukturell praediziert)
        if (it != mappings_.end())
            ++stats_.total_resolve_hit_count;
        else
            ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return pool_base_ + it->second;
    }

    [[nodiscard]] std::optional<slot_index_type> reverse_lookup(offset_type absolute_offset) const {
        if (absolute_offset < pool_base_) return std::nullopt;
        offset_type relative = absolute_offset - pool_base_;
        auto        it       = std::find_if(mappings_.begin(), mappings_.end(),
                                            [relative](auto const& m) { return m.second == relative; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_reverse_lookup_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return it->first;
    }

    [[nodiscard]] size_type mapped_count() const noexcept { return mappings_.size(); }
    void                    clear() noexcept { mappings_.clear(); }

    /// Pool-Relative-spezifisch: aktueller pool_base_address.
    [[nodiscard]] offset_type pool_base() const noexcept { return pool_base_; }

    /// Pool-Relative-spezifisch: pool_base umsetzen (Reallocation-Faehigkeit).
    /// Alle relativen Offsets bleiben semantisch korrekt — nur die absolute
    /// Position aendert sich.
    void rebase(offset_type new_pool_base) noexcept { pool_base_ = new_pool_base; }

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

    /// A8-S5-03 VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Organs -- macht die Form-B-Aussage der S5-Gate-Wache am
    /// Objekt pruefbar (ein bloss deklarierter allocator_type bliebe hier auf 0 stehen).
    /// NICHT im T6-Mess-Pfad: T6 misst den Allokator DER KOMPOSITION, nicht diesen privaten Versorger.
    [[nodiscard]] typename allocator_type::snapshot_t mapping_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ VOR mappings_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar oben).
    allocator_type                       allocator_{};
    offset_type                          pool_base_;
    std::vector<entry_type, entry_alloc> mappings_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::MappingStatistics stats_{};
    mutable observer_t                  observer_{};
#endif
};

} // namespace comdare::cache_engine::mapping

namespace comdare::cache_engine::mapping {
static_assert(concepts::MappingVariant<PoolRelative>);
static_assert(concepts::CacheEngineMappingPermutationStrategy<PoolRelative>);
static_assert(concepts::PoolRebasableStrategy<PoolRelative>);
} // namespace comdare::cache_engine::mapping
