#pragma once
// V41.F.6.1.A CRTP-Basis-Klasse fuer Allocator-Achse 6 (2026-05-25, W1-revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// CRTP-Basis-Klasse mit Concept-Guard fuer alle Allokator-Familien.
//
// Pattern (siehe docs/architektur/11_konzept_achsen_extension_visitor_pattern.md §3.6):
//   - static_assert Concept-Check im Konstruktor (NICHT als template-constraint —
//     wegen CRTP-Henne-Ei: Derived ist incomplete bei Basis-Instantiation)
//   - Default-Methoden via CRTP-Inlining (Zero-Cost, KEINE virtual)
//
// API ist Standard-konform (PMR-Naming):
//   - allocate(bytes, alignment)
//   - deallocate(p, bytes, alignment)  noexcept
//
// Vendor-spezifische Sub-Refinements (calloc/realloc/usable_size/...) sind als
// separate Concepts vorhanden (siehe concepts/axis_06_allocator_*_strategy_concept.hpp).

#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "alloc_hw_config.hpp" // F-B: NUMA/Page->allocator-Unterachse (GO4/#8, 2026-07-12)
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived> = topics::Axis-Dach + AxisBase (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum
#include <axes/cacheline/cacheline_config.hpp> // KF-5: per-Organ Cache-Line-Unterachse

#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <new> // Posten 64: std::bad_alloc -- der Standard-Allokator-Vertrag des StdAllocatorAdapter
#include <type_traits>

namespace comdare::cache_engine::alloc {

/**
 * @brief AllocatorStrategyBase - CRTP-Basis fuer alle Allokator-Familien
 * @topic allocator
 * @achse 6
 *
 * HINWEIS V41.F.6.1.A: Template-Constraint `requires AllocatorStrategy<Derived>` ist
 * bei CRTP NICHT moeglich — Derived ist zur Zeit der Basis-Klassen-Instanziierung
 * noch incomplete. Loesung: Concept-Check NUR per static_assert im Konstruktor
 * (Derived ist dann vollstaendig).
 *
 * **CRTP-Pattern:** Default-Methoden delegieren via static_cast an Derived.
 * Compile-Time-Polymorphie, KEINE virtual call, Inlining-faehig.
 *
 * **V41.F.6.1.R7.4 (2026-05-29):** Adapter-Methoden as_std_allocator<T>() + as_pmr_resource()
 * implementiert (vorher static_assert-Stubs). Beide liefern WERT-basierte Adapter, die an die
 * allocate/deallocate-API der Strategie weiterleiten — KEINE Basis-Datenmember (Empty-Base-
 * Optimization + Wrapper-Groesse bleiben erhalten; der Adapter haelt nur einen Derived*).
 */
// KF-5 (2026-06-02): zusätzlicher defaulted NTTP CacheLineCfg + Erbe von CacheLineAware<Cfg> — macht JEDEN
// Allokator-Wrapper cacheline-fähig (cacheline_config/cacheline_alignment/cacheline_prefetch). Default {} =
// B64/None/None = unverändertes Verhalten (nicht-brechend, ODR-sicher: Default ist ein Literal, kein Makro).
// Die per-Binary-Bäckung wählt der Codegen über eine DISTINKTE Organ-Instanz (KF-6/KF-8), nicht über den Default.
// F-B (GO4/#8, 2026-07-12): dritter defaulted NTTP AllocHwCfg + AllocHwAware<Cfg> → jeder Allokator-Wrapper
// trägt die NUMA/Page-Unterachse (alloc_hw.numa_node {auto,0,1} / alloc_hw.page {4k,2m}; alloc_hw_config.hpp).
// Default {} = Auto/Native = keine Vorgabe → Verhalten/Bytes unverändert (exakt das KF-5-/node_width-Muster:
// Blätter bleiben konkrete Klassen, Registry-mp_list unberührt, golden-/Gate-1-neutral). Konsum:
// axis_06_allocator_numalloc.hpp (Node-Bindung) + axis_06_allocator_pool_resource.hpp (Page-Hint→pool_options).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
              ::comdare::cache_engine::cacheline::CacheLineConfig{},
          AllocHwConfig AllocHwCfg = AllocHwConfig{}>
class AllocatorStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived>,
                              public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg>,
                              public AllocHwAware<AllocHwCfg> {
public:
    /// Concept-Check im Konstruktor: Pflicht-Set AllocatorStrategy + CacheEnginePermutationStrategy + AxisBase
    constexpr AllocatorStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::AllocatorStrategy<Derived>, "Derived must satisfy AllocatorStrategy concept "
                                                            "(see concepts/axis_06_allocator_concept.hpp)");
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>,
                      "Derived must satisfy CacheEnginePermutationStrategy concept "
                      "(see concepts/axis_06_allocator_cache_engine_permutation_concept.hpp)");
        static_assert(
            ::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Derived must satisfy AxisBaseConcept (get_compiler() Pflicht-API). "
            "AllocatorStrategyBase erbt AxisBase via OrganAxis — Derived bekommt Default 'original' automatisch.");
        static_assert(::comdare::cache_engine::topics::OrganAxisConcept<Derived>,
                      "Derived must satisfy OrganAxisConcept: Organ-Achse unterm gemeinsamen Dach topics::Axis "
                      "(axis_kind()==organ, EBO-neutral). INC-1a: AllocatorStrategyBase haengt via OrganAxis am Dach.");
    }

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.P2.C ENTFERNT: has_original_paper_code + is_original_module
    // Kommen jetzt generisch via AxisBase Default (is_original_module = false).
    // Paper-Wrappers (z.B. MimallocAllocator) ueberschreiben via Mixin-Inheritance.
    // ───────────────────────────────────────────────────────────────────────

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Cross-Axis-Default resource_ownership() ([[cross-axis-defaults-no-bloat]]):
    // Das Besitz-Modell des memory_resource-Objekts ist fuer die GANZE malloc-Familie (libc +
    // alle Vendor-Allokatoren) None — sie verwalten kein std::pmr::memory_resource. Der Default
    // gehoert daher EINMAL hierher in die CRTP-Wurzel, NICHT 25x in die Wrapper. Nur die beiden
    // PMR-Familien-Wrapper ueberschreiben (PoolResourceAllocator=Owned, PmrResourceAllocator=Borrowed).
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr concepts::ResourceOwnership resource_ownership() noexcept {
        return concepts::ResourceOwnership::None;
    }

    // ───────────────────────────────────────────────────────────────────────
    // CRTP-Delegate-Methoden zu Derived (Standard-PMR-Naming)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        return derived().allocate(bytes, alignment);
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
        derived().deallocate(p, bytes, alignment);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // -- SELBST-REKURSIONS-WACHE der drei Statistik-Weiterleiter (A8-S5 01c Vorlauf 0, 2026-08-05) -------
    //
    // WAS HIER SCHIEFGEHEN KANN (und bis heute schiefging): jeder Weiterleiter unten ruft
    // `derived().<name>(...)`. Deklariert die Strategie KEINEN eigenen Member dieses Namens, dann findet
    // die Namenssuche in Derived nur den GEERBTEN Basis-Member wieder -- der Weiterleiter ruft SICH SELBST.
    // Das ist keine Diagnose wert, die der Compiler von sich aus stellt: g++ 15.3 uebersetzt die
    // unbeschraenkte Selbst-Rekursion unter -O3 als Endlosschleife ('jmp .', Tail-Call-Faltung) und unter
    // -O1 als rekursiven Selbstaufruf bis zum Stack-Ueberlauf. ASan/UBSan melden NICHTS -- es ist kein
    // Speicherfehler, sondern ein Programm, das nie zurueckkehrt. Genau dieser Befund wurde in Scheibe 01b
    // an AxisBoundBuffer<MimallocAllocator,T> beobachtet und dort (mangels Ursache) als moeglicher
    // Compiler-/UB-Verdacht notiert; er ist WEDER das eine NOCH das andere, sondern diese fehlende
    // Deklaration: bis 2026-08-05 trug ausschliesslich ExgenAllocator ein eigenes restore_statistics.
    //
    // WARUM ALS static_assert IM RUMPF und nicht als Concept-Constraint: der Rumpf einer Member-Funktion
    // eines Klassen-Templates wird erst bei BENUTZUNG instanziiert -- erst dann ist Derived vollstaendig
    // (CRTP-Henne-Ei, dieselbe Begruendung wie bei den Ctor-Guards oben). Der Beweis ist self-proving statt
    // geraten: ist der Klassen-Teil des Member-Zeigers die BASIS, hat Derived nichts Eigenes deklariert.
    template <class MemPtr>
    static constexpr bool kIsBaseForwarder = std::is_same_v<MemPtr, void (AllocatorStrategyBase::*)() noexcept>;

    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        static_assert(!std::is_same_v<decltype(&Derived::statistics),
                                      concepts::AllocationStatistics (AllocatorStrategyBase::*)() const noexcept>,
                      "axis_06: die Strategie deklariert kein EIGENES statistics() -- der CRTP-Weiterleiter der "
                      "Basis wuerde sich selbst aufrufen (unbeschraenkte Rekursion). Eigenen Member nachziehen.");
        return derived_const().statistics();
    }

    void reset() noexcept {
        // V41.F.6.1.A User-Klarstellung: reset() = Statistik-Reset, NICHT Pool-Reset!
        static_assert(!kIsBaseForwarder<decltype(&Derived::reset)>,
                      "axis_06: die Strategie deklariert kein EIGENES reset() -- der CRTP-Weiterleiter der Basis "
                      "wuerde sich selbst aufrufen (unbeschraenkte Rekursion). Eigenen Member nachziehen.");
        derived().reset();
    }

    // Phase 0.3 (Hebel B, Memento-Pattern GoF): Statistik auf einen zuvor via statistics() gezogenen Snapshot
    // zuruecksetzen — spiegelbildlich zu reset(). Nutzung: ein strategie-besitzender Store (z.B. TreeNodePoolStore)
    // verwirft damit im Copy-Ctor/Assign die durch die COW-Vollkopie entstandene transiente Re-Allokations-
    // Pollution (die Zwei-Phasen-Mess-Doppelzaehlung), sodass T6 = save-Stand + measure-Delta bleibt.
    void restore_statistics(concepts::AllocationStatistics const& s) noexcept {
        static_assert(
            !std::is_same_v<decltype(&Derived::restore_statistics),
                            void (AllocatorStrategyBase::*)(concepts::AllocationStatistics const&) noexcept>,
            "axis_06: die Strategie deklariert kein EIGENES restore_statistics() -- der CRTP-Weiterleiter der "
            "Basis wuerde sich selbst aufrufen (unbeschraenkte Rekursion; -O3 emittiert 'jmp .'). Das ist der "
            "Memento-Pfad JEDES strategie-besitzenden Stores/Puffers: ohne den Member haengt der Copy-Ctor. "
            "Eigenen Member nachziehen (Vorlage: axis_06_allocator_exgen.hpp).");
        derived().restore_statistics(s);
    }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Adapter-Methoden — Allocator-Achse an std::allocator / std::pmr anbinden.
    // WERT-basiert (kein Basis-Datenmember → EBO + Wrapper-Groesse bleiben erhalten).
    // ───────────────────────────────────────────────────────────────────────

    /**
     * @brief StdAllocatorAdapter<T> — erfuellt die std::allocator-Named-Requirements (C++23) und
     *        leitet allocate/deallocate an die zugrundeliegende Achsen-Strategie weiter.
     *        Nutzbar mit std::vector<T, StdAllocatorAdapter<T>>, std::allocator_traits, rebind.
     *
     * **FEHLSCHLAG-VERTRAG (Posten 64, 2026-08-04) -- die EINE Uebersetzungsstelle:**
     * Die Achsen-Strategie meldet OOM per **nullptr** (Achsen-Semantik, z.B. ExgenAllocator::allocate
     * -> portable_aligned_alloc). Die std::allocator-Named-Requirements verlangen dagegen, dass
     * `allocate()` bei Fehlschlag **WIRFT** (C++23 [allocator.requirements]: "Throws: bad_alloc if the
     * storage cannot be obtained"); ein besitzender Container wie std::vector prueft den Rueckgabewert
     * NICHT und konstruiert bei nullptr in Nullspeicher -- UB. Genau hier, im Adapter, wird die
     * Achsen-Semantik in die Standard-Semantik uebersetzt: EINE Stelle statt einer Pruefung je
     * Konsument. Damit sind die `[[allocation-failure-exception]]`-Aussagen der besitzenden Organe
     * (mapping/value_handle/cache_traversal + die vier Pool-Stores) wieder WAHR -- der Wurf kommt
     * jetzt vom Adapter statt vom Default-Allokator, die Fehlerklasse bleibt der FK-5-Boden.
     *
     * **EHRLICHKEIT BLEIBT (Auflage):** der `failure_count` der Strategie ist zum Zeitpunkt des Wurfs
     * BEREITS gezaehlt -- die Strategie zaehlt ihn VOR dem `return nullptr` (axis_06_allocator_exgen.hpp).
     * Der Wurf verdeckt also keinen Messwert, er ersetzt nur den UB-Pfad dahinter.
     *
     * **ZERO-SIZE UNVERAENDERT:** fuer `n == 0` bleibt der Rueckgabewert der Strategie unangetastet
     * durchgereicht (der Standard fordert dort keinen Nicht-Null-Zeiger, und kein Konsument
     * dereferenziert ihn) -- die Zero-Size-Wachen der Organe bleiben damit gueltig.
     */
    template <typename T>
    class StdAllocatorAdapter {
    public:
        using value_type = T;
        explicit StdAllocatorAdapter(Derived* strat) noexcept : strat_(strat) {}
        template <typename U>
        StdAllocatorAdapter(StdAllocatorAdapter<U> const& other) noexcept : strat_(other.strat_) {}

        [[nodiscard]] T* allocate(std::size_t n) {
            void* const p = strat_->allocate(n * sizeof(T), alignof(T));
            // Fehlschlag-Vertrag (s. Klassen-Doku): nullptr aus der Strategie -> std::bad_alloc.
            // n == 0 bleibt bewusst ausgenommen (Zero-Size-Verhalten unveraendert).
            if (p == nullptr && n != 0) throw std::bad_alloc{};
            return static_cast<T*>(p);
        }
        void deallocate(T* p, std::size_t n) noexcept { strat_->deallocate(p, n * sizeof(T), alignof(T)); }
        template <typename U>
        [[nodiscard]] bool operator==(StdAllocatorAdapter<U> const& other) const noexcept {
            return strat_ == other.strat_;
        }

    private:
        template <typename U>
        friend class StdAllocatorAdapter;
        Derived* strat_;
    };

    /**
     * @brief PmrResourceAdapter — std::pmr::memory_resource ueber die Achsen-Strategie. Konkreter,
     *        kopierbarer Wert-Typ (haelt nur Derived*); der Aufrufer haelt ihn am Leben und
     *        uebergibt &resource an pmr-Container.
     */
    class PmrResourceAdapter final : public std::pmr::memory_resource {
    public:
        explicit PmrResourceAdapter(Derived* strat) noexcept : strat_(strat) {}

    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override {
            return strat_->allocate(bytes, alignment);
        }
        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
            strat_->deallocate(p, bytes, alignment);
        }
        [[nodiscard]] bool do_is_equal(std::pmr::memory_resource const& other) const noexcept override {
            auto const* o = dynamic_cast<PmrResourceAdapter const*>(&other);
            return o != nullptr && o->strat_ == strat_;
        }
        Derived* strat_;
    };

    /// Liefert einen std::allocator-kompatiblen Adapter (Wert) fuer Element-Typ T.
    template <typename T>
    [[nodiscard]] StdAllocatorAdapter<T> as_std_allocator() noexcept {
        return StdAllocatorAdapter<T>(&derived());
    }

    /// Liefert einen pmr-memory_resource-Adapter (Wert). Aufrufer haelt ihn am Leben und uebergibt
    /// dessen Adresse an pmr-Container (z.B. std::pmr::polymorphic_allocator).
    [[nodiscard]] PmrResourceAdapter as_pmr_resource() noexcept { return PmrResourceAdapter(&derived()); }

public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: der Allokator kann real scheitern (OOM ist der Lehrbuch-Fall von Failed).
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    [[nodiscard]] Derived&       derived() noexcept { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const& derived_const() const noexcept { return static_cast<Derived const&>(*this); }
};

} // namespace comdare::cache_engine::alloc
