#pragma once
// V41.F.6.1 R5.B axis_06_allocator PoolResourceAllocator (2026-05-29)
//
// @topic allocator
// @achse 6
// @family A22 (std::pmr — Halpern N3916, C++17 pool variant)
// @subaxis AA2 size_class_schema
//
// **Algorithmus:** besitzt einen EIGENEN std::pmr::unsynchronized_pool_resource (size-class-Pools
// mit Block-Wiederverwendung, upstream = new_delete). Im Gegensatz zu StdMalloc (System-malloc) und
// zum default-konstruierten PmrResourceAllocator (new_delete = System) ist dieser Wrapper
// VERHALTENS-DISTINKT von System-malloc OHNE externes Linking: viele kleine, gleichgrosse
// Allokationen werden aus vorallokierten Chunks bedient + bei deallocate in die Free-List des
// Size-Class-Pools zurueckgegeben → andere Latenz-Charakteristik als der globale Allocator.
//
// **Zweck (R5.B):** macht die Allocator-Achse BEHAVIORAL operativ — der erste axis_06-Wrapper, der
// sich ohne Dependency-Linking real von System-malloc unterscheidet. Damit ist eine NICHT-hohle
// 2-Achsen-F15-Messung (search_algo × allocator) moeglich (Doku 22 §3.1, [[std-map-unified-interface]]).
//
// **Provenienz / Lizenz:** Standardbibliothek (std::pmr), eigene C++23-Komposition → is_original=false.
// Kopierbarkeit: pool_resource ist non-copyable/non-movable → via std::shared_ptr gehalten; Kopien
// TEILEN den Pool (korrekte PMR-is_equal-Semantik).
//
// **Allocation-Failure (A1-Wurf-Vertrag, 2026-08-06) -- KORREKTUR EINER ABWEICHUNG:** bis zu diesem
// Schnitt war dieser Wrapper die EINZIGE Strategie der Achse 6, die den achsen-uniformen Vertrag
// "OOM == nullptr, failure_count VOR der Rueckgabe gezaehlt" nicht wahrte: `resource_->allocate` wirft
// std::bad_alloc, und der Wurf lief ungefangen durch (die Kopfzeile schrieb das frueher sogar als
// Absicht fest). Das Concept AllocatorStrategy erzwingt die Konvention nicht typsystemisch, die
// Abweichung blieb also compile-clean unsichtbar -- mit der Folge, dass das Fehlersignal, das ein
// Aufrufer von `alloc_.allocate(...)` sieht, vom gebundenen Strategie-Typ abhing. Genau das
// widerspricht dem Ziel EINES Wurf-Vertrags. Der Wrapper faengt jetzt -- wie die strukturgleiche
// Schwester-Strategie PmrResourceAllocator (axis_06_allocator_pmr_resource.hpp) es seit jeher tut --
// und uebersetzt zurueck auf nullptr. Nach aussen aendert sich fuer besitzende Container NICHTS: der
// StdAllocatorAdapter (Posten 64) bzw. allocate_or_throw (Posten 74) macht daraus wieder std::bad_alloc.
// Der Unterschied ist, dass es jetzt EINE Uebersetzungsstelle gibt statt zweier Konventionen.
//
// F-B (GO4/#8, 2026-07-12): Page-Hint der NUMA/Page-Unterachse (alloc_hw_config.hpp) → der Koerper ist
// jetzt das CRTP-Body-Template PoolResourceAllocatorBody<Derived, AllocHwConfig>: ein nicht-nativer
// Page-Hint (4k/2m) setzt COMPILE-TIME die pmr::pool_options (largest_required_pool_block = Page-Bytes),
// d.h. Bloecke bis zur Page-Groesse werden aus Size-Class-Pools bedient statt upstream zu gehen (dTLB-/
// huge-page-Motivation, thesis 02_fundamentals). Das konkrete Registry-Blatt `PoolResourceAllocator`
// bleibt eine konkrete Klasse mit Default Native (KEIN Hint -> default-konstruierte pool_options,
// byte-identisch); Page-Varianten = distinkte Organ-Instanzen mit explizitem NTTP (KF-6/KF-8-Mechanik).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <new> // A1-Wurf-Vertrag: std::bad_alloc -- der pmr-Wurf, den dieser Wrapper zurueckuebersetzt
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)

namespace comdare::cache_engine::alloc {

/**
 * @brief PoolResourceAllocatorBody — eigener std::pmr::unsynchronized_pool_resource (A22 pool-Variante)
 * @topic allocator @achse 6 @subaxis AA2 size_class_schema_tag
 * F-B: HwCfg = NUMA/Page-Unterachse (nur der Page-Hint wird hier konsumiert → pool_options).
 */
template <typename Derived, AllocHwConfig HwCfg = AllocHwConfig{}>
class PoolResourceAllocatorBody
    : public AllocatorStrategyBase<Derived, ::comdare::cache_engine::cacheline::CacheLineConfig{}, HwCfg> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;

    static constexpr bool enabled = flags::pool_enabled;

    using topic_tag = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag  = subaxes::size_class_schema_tag;
    using family_id = std::integral_constant<int, 22>; // A22 std::pmr-Familie (pool-Variante)

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; } // unsynchronized
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }    // IST ein pmr-resource
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "pool_resource"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "std::pmr::unsynchronized_pool_resource (eigener Size-Class-Pool, Halpern N3916)";
    }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    /// A1-WURF-VERTRAG (2026-08-06), 1. Bump: v1.0.0c -> v1.0.1c, weil der FEHLSCHLAG-Vertrag der Achse sich
    /// geaendert hat, ohne eine Registry-/XML-Flaeche zu bewegen -- ohne Bump wuerde der inkrementelle
    /// Tier-Binary-Cache alte Binaries weiterverwenden. Volle Begruendung samt Frozen-Neutralitaets-Beweis:
    /// axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP".
    /// 2. Bump (2026-08-06): v1.0.1c -> v1.0.2c -- die reallocate()-Statistik-Korrektur unten bekam
    /// nachtraeglich einen Bump (Owner-Entscheid: "heute unerreichbar" entlastet nicht, s. dort).
    /// Volle Begruendung: axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP, 2. BUMP".
    static constexpr std::string_view               algo_version = "v1.0.2c";
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "POOL"; }

    // Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc() noexcept {
        return true;
    } // pmr allocate(bytes, alignment)
    [[nodiscard]] static constexpr bool                        requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    /// R7.4: BESITZT die memory_resource selbst (eigener unsynchronized_pool_resource via shared_ptr,
    /// Lebensdauer an die Wrapper-Instanz gebunden) -> Owned. Grenzt POOL gegen PMR (Borrowed) ab.
    [[nodiscard]] static constexpr concepts::ResourceOwnership resource_ownership() noexcept {
        return concepts::ResourceOwnership::Owned;
    }

    PoolResourceAllocatorBody() : resource_(make_resource_()) {}

    // operator==: zwei Allokatoren sind GLEICH gdw. sie denselben Pool teilen (PMR-is_equal-Semantik).
    [[nodiscard]] bool operator==(PoolResourceAllocatorBody const& other) const noexcept {
        return resource_.get() == other.resource_.get();
    }

    /// A1-Wurf-Vertrag: der pmr-Wurf wird HIER gefangen und auf die Achsen-Semantik (OOM == nullptr)
    /// zurueckuebersetzt -- identisches Idiom wie PmrResourceAllocator::allocate. Die Zaehlung steht
    /// VOR der Rueckgabe (Ehrlichkeits-Auflage aus Posten 64): der failure_count ist gesetzt, bevor ein
    /// Adapter daraus wieder std::bad_alloc macht. Erfolgs-Pfad byte-identisch zum Stand davor.
    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p = nullptr;
        try {
            p = resource_->allocate(bytes, alignment);
        } catch (std::bad_alloc const&) { p = nullptr; }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    /// pmr deallocate verlangt IDENTISCHE bytes+alignment wie bei allocate (Aufrufer-Pflicht).
    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        resource_->deallocate(p, bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use)
            stats_.total_bytes_in_use -= aligned_bytes;
        else
            stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes;
        (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    } // Statistik-Reset (NICHT Pool-Release)
    // Phase 0.3 (Memento, VERVOLLSTAENDIGUNG 2026-08-05 / A8-S5 01c Vorlauf 0): Statistik auf einen zuvor
    // via statistics() gezogenen Snapshot zuruecksetzen -- spiegelbildlich zu reset(). Die Deklaration ist
    // PFLICHT, kein Komfort: AllocatorStrategyBase::restore_statistics ist ein CRTP-WEITERLEITER
    // (derived().restore_statistics(s)). Fehlt der eigene Member, findet der Weiterleiter nur sich SELBST
    // wieder -- unbeschraenkte Selbst-Rekursion, die g++ 15.3 unter -O3 als Endlosschleife ('jmp .')
    // emittiert und unter -O1 als rekursiven Selbstaufruf. Bis 2026-08-05 trug ausschliesslich
    // ExgenAllocator diesen Member; jede andere Strategie haette den Memento-Pfad (Copy-Ctor der
    // strategie-besitzenden Stores/Puffer) zum Haenger gemacht. Die Basis pinnt das jetzt zusaetzlich
    // compile-hart (self-proving static_assert im Weiterleiter).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

    // HINWEIS: KEIN zero_allocate / ZeroingStrategy — analog PmrResourceAllocator. Der
    // typisierte ZeroAllocateRoundtrip-Test gibt zero_allocate-Speicher per std::free frei
    // (calloc-Vertrag); Pool-Speicher ist NICHT std::free-faehig. PMR-basierte Allokatoren
    // erfuellen ZeroingStrategy daher bewusst nicht (das if-constexpr-Guard ueberspringt sie).

    // Sub-Concept: ReallocatingStrategy (alloc-new aus Pool + memcpy + dealloc-old in Pool;
    // der Test gibt das Ergebnis per m.deallocate frei → konsistent mit dem Pool).
    //
    // STATISTIK-SYMMETRIE (A1-Nachbesserung 2026-08-06, Review-Befund "Phantom-Bytes") -- der
    // KANONISCHE Ort der Begruendung fuer die ganze Achse (die 23 Schwester-Strategien tragen dieselbe
    // Korrektur mit Verweis hierher):
    //
    // BEFUND: allocate() bucht ALIGNED (aligned_bytes = aufgerundet auf alignment), deallocate() bucht
    // ALIGNED gegen -- reallocate() buchte den ALTEN Block aber ROH (old_bytes) gegen und den NEUEN
    // wieder ALIGNED (aligned_new). Je reallocate blieben damit (aligned_old - old_bytes) Bytes in
    // total_bytes_in_use stehen, die real nicht mehr gehalten werden. Bei alignment-teilbaren Groessen
    // (der bisher einzige gepruefte Fall, 64/128 @ 16) ist die Differenz 0 -- der Fehler war deshalb
    // unsichtbar und wird jetzt mit alignment-UNGLEICHEN Werten (65 @ 16 -> 80) gepinnt
    // (test_a1_wurf_vertrag_allokator_store, Abschnitt (4c)).
    //
    // MESSWIRKUNG UNTER DEN HEUTIGEN AUFRUFERN -- ehrlich geprueft, nicht behauptet: KEINE.
    // total_bytes_in_use ist eine T6-Groesse, die Korrektur waere also grundsaetzlich messwirksam. Kein
    // aufgezeichneter Messwert kann sich HEUTE bewegen, weil reallocate() auf dem gesamten Mess-Pfad NIE
    // gerufen wird: `grep -rn "\.reallocate(\|->reallocate(" libs/ apps/ modules/ benchmarks/ tools/
    // adapters/ deploy/` liefert GENAU EINEN Treffer, und der ist die Concept-Deklaration selbst
    // (axis_06_allocator_reallocating_strategy_concept.hpp:35). Alle Aufrufer sind Tests.
    //
    // TROTZDEM BUMP (2. Bump, 2026-08-06, Owner-Entscheid nach Lens-Pass -- Umkehr der Erst-Fassung
    // dieses Abschnitts): "kein Aufrufer heute" wurde zuerst als Bump-Ausschluss gewertet. Der Owner hat
    // das verworfen: "HEUTE UNERREICHBAR ENTLASTET NICHT" -- reallocate() ist eine offiziell im
    // Typsystem gefuehrte Achsen-Faehigkeit (ReallocatingStrategy-Concept), kein totes Feature. Ein
    // KUENFTIGER Konsument koennte sie in den Mess-Pfad ziehen und dabei ein VOR dieser Korrektur unter
    // unveraendertem Versions-Stand gecachtes Binary mit dem Phantom-Byte-Fehler weiterverwenden --
    // ein Cache-IDENTITAETS-Risiko, kein Kosmetikposten. Die 24 Strategien mit eigener
    // reallocate()-Implementierung gehen deshalb auf v1.0.2c; PmrResourceAllocator und
    // VampirNfpAllocator implementieren kein reallocate() und bleiben auf v1.0.1c stehen. Volle
    // Begruendung: axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP, 2. BUMP".
    //
    // Der zusaetzliche else-Zweig (Klemmung auf 0) ist NICHT neu erfunden: er spiegelt exakt die Form
    // in deallocate() darueber. Vorher fehlte er hier -- die Gegenbuchung war also auch in ihrer FORM
    // die einzige, die aus der Reihe fiel.
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t alignment) {
        void* np = nullptr;
        try {
            np = resource_->allocate(new_bytes, alignment);
        } catch (std::bad_alloc const&) { np = nullptr; }
        // A1-Wurf-Vertrag: scheitert die NEUE Vergabe, bleibt der ALTE Block gueltig und unangetastet
        // (realloc-Vertrag) -- der Aufrufer verliert seinen Speicher nicht. Muster: exgen reallocate.
        if (np == nullptr) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.failure_count;
            observer_.notify(stats_);
#endif
            return nullptr;
        }
        if (p != nullptr) {
            std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
            std::memcpy(np, p, copy_bytes);
            resource_->deallocate(p, old_bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
            // A1-Nachbesserung 2026-08-06 (Statistik-Symmetrie der Achse): die Gegenbuchung des ALTEN
            // Blocks rechnet ALIGNED -- genau wie seine Buchung in allocate() und wie die Gegenbuchung
            // in deallocate(). Die rohe old_bytes liess je reallocate Phantom-Bytes stehen. Volle
            // Begruendung: axis_06_allocator_pool_resource.hpp, Abschnitt reallocate.
            std::size_t aligned_old = ((old_bytes + alignment - 1) / alignment) * alignment;
            if (aligned_old <= stats_.total_bytes_in_use)
                stats_.total_bytes_in_use -= aligned_old;
            else
                stats_.total_bytes_in_use = 0;
            ++stats_.deallocation_count;
#endif
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

    /// Pool-spezifisch: Zugriff auf das underlying memory_resource (Symmetrie zu PmrResourceAllocator).
    [[nodiscard]] std::pmr::memory_resource* underlying_resource() const noexcept { return resource_.get(); }

    /// F-B: die tatsaechlich wirksamen pool_options des Resources (Konsum-Beweis: ein nicht-nativer
    /// Page-Hint veraendert largest_required_pool_block gegenueber dem default-konstruierten Pool).
    [[nodiscard]] std::pmr::pool_options pool_options_in_effect() const noexcept { return resource_->options(); }

private:
    /// F-B: Page-Hint (compile-time) → pool_options. Native (0) = default-konstruierter Pool — dieser
    /// if-constexpr-Zweig ist AST-identisch zum Stand vor F-B (byte-identisches Default-Verhalten).
    [[nodiscard]] static std::shared_ptr<std::pmr::unsynchronized_pool_resource> make_resource_() {
        constexpr std::size_t page_hint_bytes = AllocHwAware<HwCfg>::alloc_hw_page_bytes();
        if constexpr (page_hint_bytes == 0) {
            return std::make_shared<std::pmr::unsynchronized_pool_resource>();
        } else {
            return std::make_shared<std::pmr::unsynchronized_pool_resource>(
                std::pmr::pool_options{/*max_blocks_per_chunk=*/0,
                                       /*largest_required_pool_block=*/page_hint_bytes});
        }
    }

    std::shared_ptr<std::pmr::unsynchronized_pool_resource> resource_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::AllocationStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

/// Das konkrete Registry-Blatt (AllVendors-mp_list, unveraendert): Default Native (kein Page-Hint) =
/// default-konstruierte pool_options — Verhalten byte-identisch zum Stand vor F-B. Page-Varianten =
/// distinkte Organ-Instanzen mit explizitem NTTP, NIE Runtime-Switch im Hot-Path.
class PoolResourceAllocator : public PoolResourceAllocatorBody<PoolResourceAllocator> {
public:
    using PoolResourceAllocatorBody<PoolResourceAllocator>::PoolResourceAllocatorBody;
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::alloc::PoolResourceAllocator",
                                  "axes/alloc/axis_06_allocator_pool_resource.hpp");
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<PoolResourceAllocator>,
              "Pflicht: PoolResourceAllocator muss AllocatorStrategy erfuellen (Standard-PMR-API)");
static_assert(concepts::CacheEnginePermutationStrategy<PoolResourceAllocator>,
              "Pflicht: PoolResourceAllocator muss CacheEnginePermutationStrategy erfuellen");
static_assert(!concepts::ZeroingStrategy<PoolResourceAllocator>,
              "Erwartet: PoolResourceAllocator bietet KEIN zero_allocate (Pool-Speicher ist nicht std::free-faehig, "
              "analog PmrResourceAllocator)");
static_assert(concepts::ReallocatingStrategy<PoolResourceAllocator>,
              "Optional: PoolResourceAllocator bietet reallocate (Pool alloc-copy-free Pattern)");
// F-B-Neutralitaet: das Registry-Blatt traegt die Unterachse (Concept) UND behaelt Native (kein Page-Hint).
static_assert(AllocHwConfigurable<PoolResourceAllocator>);
static_assert(PoolResourceAllocator::alloc_hw_page_bytes() == 0,
              "F-B-Neutralitaet: Default-Pool muss byte-identisch ohne Page-Hint bleiben (Native)");
} // namespace comdare::cache_engine::alloc
