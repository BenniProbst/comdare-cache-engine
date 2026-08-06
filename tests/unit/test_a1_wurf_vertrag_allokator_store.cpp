// A1-SCHEIBE "Wurf-Vertrag" (Owner-Posten 71/72/73/74, Ledger-Nachtrag 05.08.2026 abend-10) --
// EIN konsistenter Fehlschlag-Vertrag ueber die Allokator- und Store-Stellen, negativ belegt.
//
// AUSGANGSLAGE. Posten 64 hat den Fehlschlag-Vertrag an EINER Stelle uebersetzt: im StdAllocatorAdapter
// wird der OOM-nullptr der Achse zu std::bad_alloc. Diese TU belegt die vier Stellen, an denen dieselbe
// Uebersetzung bis 06.08.2026 FEHLTE oder gegenlaeufig war -- und die deshalb je nach Zugriffsweg ein
// ANDERES Fehlersignal lieferten. Genau diese Uneinheitlichkeit ist der Gegenstand:
//
//   (F1/Posten 71) PmrResourceAdapter::do_allocate reichte den nullptr ungeprueft an pmr-Container
//                  durch. std::pmr::memory_resource::allocate ist standardvertraglich WERFEND, ein
//                  pmr-Container prueft daher nicht nach -> Konstruktion in Nullspeicher, UB.
//   (F2/Posten 72) StdAllocatorAdapter<T>::allocate rechnete n*sizeof(T) UNGEWACHT. Ein direkter
//                  Aufruf mit n > SIZE_MAX/sizeof(T) laesst das Produkt umlaufen; die Strategie
//                  "gelingt" mit zu kleinem Puffer -> Heap-Overflow beim SCHREIBEN, nicht Absturz
//                  bei der Vergabe. Standard-Praezedenz: std::bad_array_new_length.
//   (F3/Posten 73) node_width_bytes trug das Line-Groessen-Literal 64 in der Knoten-BREITEN-Unterachse.
//                  ANDERE Entscheid-Klasse (Konsistenz, kein Wurf-Vertrag) -> hier nur der
//                  Neutralitaets-Beweis, damit die Substitution nicht stillschweigend etwas bewegt.
//   (F4/F5/P. 74) LayoutAwareChunkedStore haelt die Strategie ROH (Template-Parameter A) und kam an der
//                  Posten-64-Uebersetzung VORBEI: append_slot memsetzte in den nullptr, copy_from_
//                  memcpyte hinein. copy_from_ hatte zusaetzlich ein LECK -- wirft die Vergabe in
//                  Iteration k, laeuft auf dem Kopier-Ktor-Pfad NIE ein Destruktor fuer *this, und der
//                  vector-Destruktor von chunks_ raeumt nur die POD-Structs, nicht ihre Puffer.
//   (F6/NEU)       PoolResourceAllocator war die EINZIGE Strategie der Achse 6, die OOM als Wurf statt
//                  als nullptr meldete -- die Abweichung von der Konvention aller ~25 Schwestern.
//
// NEUER VERTRAG. Achsen-INNEN gilt ausnahmslos "OOM == nullptr, failure_count VOR der Rueckgabe";
// achsen-AUSSEN wirft genau eine Uebersetzungsstelle je Zugriffsweg (StdAllocatorAdapter,
// PmrResourceAdapter, allocate_or_throw) -- alle drei in axis_06_allocator_strategy_base.hpp.
//
// NACHBESSERUNG 2026-08-06 (drei Review-Befunde, in DIESER TU sichtbar):
//   (N1) VERSIONS-SICHTBARKEIT -- die Vertrags-Aenderung bewegte keine Registry-/XML-Flaeche und war
//        damit fuer den inkrementellen Tier-Binary-Cache UNSICHTBAR. Alle 26 Strategien der Achse 6
//        tragen jetzt den PATCH-Bump v1.0.0c -> v1.0.1c (Begruendung + Frozen-Neutralitaets-Beweis:
//        axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP").
//   (N2) PMR-ZERO-SIZE -- do_allocate trug ein `&& bytes != 0` und liess damit genau den einen nullptr
//        stehen, den [mem.res.public] verbietet. Der Vorbehalt ist weg; das Orakel in (2) unten heisst
//        jetzt "NIE nullptr" statt "wirft nicht" und rief vorher deallocate(nullptr, 0) -- das ist
//        entfernt. Std-Adapter und allocate_or_throw BEHALTEN ihre Zero-Size-Ausnahme (dort zulaessig),
//        und genau diese Trennung wird in (2) mitgeprueft.
//   (N3) CONCEPT-REGRESS -- der Store-Kopf verlangte nur AllocatorStrategy, konsumierte aber VIER
//        Achsen-Faehigkeiten: allocate_or_throw, die Std-Container-Naht des Chunk-INDEX
//        (StdAllocatorAdapter/as_std_allocator), die Wert-Semantik der roh gehaltenen Strategie
//        (A{} / alloc_ = A{}) und die T6-Route (snapshot_t/statistics). Alle vier stehen jetzt als
//        Sub-Concepts in der ALLOKATOR-Achse (Schnitt-Regel) und einzeln im Kopf-Constraint des
//        Stores; (1b) belegt JEDEN Term positiv UND negativ -- und zwar AM STORE-KOPF, nicht nur am
//        Concept (Sonde StoreKopfBindetAllokator). Registrierte Varianten unberuehrt: die Member
//        sitzen an der CRTP-Wurzel bzw. sind dort seit Posten 80 Pflicht.
//   (N4) ORAKEL-HAERTUNG (Review-Befund 06.08.2026, zwei Stellen): (a) die Zusatz-Forderung "und zwar
//        per Zeiger, nicht per Wurf" an der REALEN Strategie bei bytes == 0 ist ENTFALLEN -- sie war
//        ueberscharf (C11 7.22.3/1 laesst fuer size == 0 auch NULL zu, der daraus folgende Wurf ist
//        vertragskonform) und ueberfluessig (der Nicht-pauschal-Satz steht eine Zeile hoeher). Das
//        Orakel heisst dort jetzt nur noch "Wurf ODER Nichtnull". (b) der Fehlschlag-Pfad der
//        pmr-tragenden Strategien wird ZUSAETZLICH deterministisch und sanitizer-fest belegt --
//        WerfendeResource (immer werfendes memory_resource) ueber PmrResourceAllocator, die einzige
//        Strategie der Achse mit injizierbarem Resource. Die absurd grosse Anfrage bleibt NUR dort,
//        wo es keine Injektions-Naht gibt (PoolResourceAllocator, Resource ist Owned) -- mit
//        benannter Sanitizer-Grenze statt stillem Vorbehalt.
//
// WAS DIESE TU BEWEIST (literal, nicht behauptet):
//   (1) KONFORMITAET DER ORAKEL   -- beide Stub-Strategien sind vollwertige Achsen-Varianten an
//                                    AllocatorStrategyBase, kein Fremdkoerper neben der Achse.
//   (1b) CONCEPT-NAHT             -- ThrowTranslatingStrategy ist ECHT staerker als AllocatorStrategy
//                                    (Gegenprobe-Typ OhneWurfUebersetzung) und von jeder Achsen-
//                                    Strategie erfuellt.
//   (2) F1 NEGATIV + POSITIV      -- pmr::vector ueber as_pmr_resource() wirft bad_alloc statt in
//                                    Nullspeicher zu bauen; die PMR-Naht liefert NIE nullptr (auch nicht
//                                    bei bytes == 0), Std/roh behalten ihre Zero-Size-Ausnahme; reale
//                                    Strategie laeuft.
//   (3) F2 NEGATIV + POSITIV      -- allocate(n) mit umlaufendem n*sizeof(T) wirft bad_array_new_length;
//                                    realistische n laufen unveraendert durch.
//   (4) F6 KONVENTIONS-ANGLEICH   -- PoolResourceAllocator liefert bei OOM nullptr (kein Wurf) und zaehlt;
//                                    Erfolgs-Pfad + reallocate-Erfolg unveraendert; alter Block ueberlebt.
//   (5) F4 STORE-WURF             -- append_slot wirft bad_alloc statt in Nullspeicher zu memsetzen, und
//                                    laesst den Store dabei UNVERAENDERT (starke Ausnahme-Garantie).
//   (6) F5 KEIN LECK              -- der Kopier-Ktor wirft mitten in copy_from_ und gibt JEDEN bereits
//                                    materialisierten Chunk zurueck: die Live-Block-Bilanz der Achse
//                                    kehrt exakt auf den Stand VOR dem Kopierversuch zurueck.
//   (7) F3 NEUTRALITAET           -- der Literal-Bezug ist wertgleich zum frueheren Default-Argument.
//
// WARUM ZWEI ORAKEL: (2)/(3)/(5) brauchen eine IMMER erschoepfte Quelle (Muster
// test_h64_allocator_adapter_failure_contract). (6) braucht das Gegenteil -- eine Quelle, die ECHT
// allokiert, sich dabei ZAEHLEN laesst und erst nach einem gesetzten Budget kippt; nur so ist "kein
// Leck" eine Bilanz-Aussage statt einer Behauptung. Die Zaehler sind static, weil der Store seine
// Strategie als WERT haelt: die Kopie legt ihr EIGENES alloc_ an, die Bilanz muss aber beide sehen.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.
// ASCII-only.

#include <axes/alloc/axis_06_allocator_exgen.hpp> // die reale Achsen-Default-Strategie (Positiv-Kontrolle)
#include <axes/alloc/axis_06_allocator_pmr_resource.hpp> // injizierbare Resource -> deterministisches Wurf-Orakel
#include <axes/alloc/axis_06_allocator_pool_resource.hpp>
#include <axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <axes/alloc/axis_06_allocator_subaxes_aa1_to_aa7.hpp>
#include <axes/alloc/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/cacheline/cacheline_line_bytes.hpp>
#include <axes/cacheline/node_width_config.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <topics/allocator/concepts/topic_allocator_concept.hpp>
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>

#include <concepts> // A1-Nachbesserung: std::default_initializable/std::assignable_from in den Isolations-Saetzen
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory_resource>
#include <new>
#include <string_view>
#include <type_traits>
#include <vector>

namespace alloc     = ::comdare::cache_engine::alloc;
namespace acpts     = ::comdare::cache_engine::alloc::concepts;
namespace topics    = ::comdare::cache_engine::topics;
namespace nd        = ::comdare::cache_engine::node;
namespace ml        = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace cacheline = ::comdare::cache_engine::cacheline;

namespace {

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::printf("  %s %s\n", ok ? "[OK] " : "[ERR]", what);
    if (!ok) ++g_fail;
}

// -- ORAKEL A: die Speicherquelle ist IMMER erschoepft ---------------------------------------------
// Bildet den OOM-Fall der realen Vendoren nach, ohne ihn erzwingen zu muessen. Zaehlt den
// failure_count VOR dem return -- genau wie ExgenAllocator::allocate es tut (Ehrlichkeits-Auflage).
class ErschoepfteStubStrategie : public alloc::AllocatorStrategyBase<ErschoepfteStubStrategie> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = alloc::subaxes::synchronization_tag;
    using family_id  = std::integral_constant<int, 0>; // 0 = KEINE Vendor-Familie: reiner Test-Stub

    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "test_stub_erschoepft_a1"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Test-Stub: Speicherquelle immer erschoepft (A1-Wurf-Vertrag-Orakel)";
    }

    [[nodiscard]] static constexpr bool                     has_native_aligned_alloc() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr acpts::ProgressGuarantee progress_guarantee() noexcept {
        return acpts::ProgressGuarantee::WaitFree;
    }

    [[nodiscard]] bool operator==(ErschoepfteStubStrategie const&) const noexcept { return true; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        (void)bytes;
        (void)alignment;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.failure_count;
        observer_.notify(stats_);
#endif
        return nullptr;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        (void)p;
        (void)bytes;
        (void)alignment;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = acpts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

private:
    snapshot_t stats_{};
    observer_t observer_{};
#endif
};

// -- ORAKEL B: echte Vergaben, gedeckelt und BILANZIERT --------------------------------------------
// Fuer den Leck-Beweis (6) reicht eine immer-leere Quelle nicht: es muss etwas da sein, das leckt.
// Diese Strategie allokiert REAL (portable_aligned_alloc, Alignment-treu -- der Store verlangt
// Cache-Line-Alignment), fuehrt eine LIVE-Bilanz und kippt, sobald das Budget aufgebraucht ist.
// static, weil der Store seine Strategie als WERT haelt (die Kopie hat ihr eigenes alloc_).
class BudgetStubStrategie : public alloc::AllocatorStrategyBase<BudgetStubStrategie> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = alloc::subaxes::synchronization_tag;
    using family_id  = std::integral_constant<int, 0>;

    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "test_stub_budget_a1"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Test-Stub: echte, gedeckelte und bilanzierte Vergaben (A1-Leck-Orakel)";
    }

    [[nodiscard]] static constexpr bool                     has_native_aligned_alloc() noexcept { return true; }
    [[nodiscard]] static constexpr bool                     requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr acpts::ProgressGuarantee progress_guarantee() noexcept {
        return acpts::ProgressGuarantee::WaitFree;
    }

    [[nodiscard]] bool operator==(BudgetStubStrategie const&) const noexcept { return true; }

    // -- die Bilanz-Naht (static: ueber ALLE Instanzen, s. Klassen-Kopf) ---------------------------
    static inline std::size_t s_budget = std::numeric_limits<std::size_t>::max();
    static inline long long   s_live   = 0; // aktuell gehaltene Bloecke (der Leck-Detektor)

    static void                    unbeschraenkt() noexcept { s_budget = std::numeric_limits<std::size_t>::max(); }
    static void                    budget_setzen(std::size_t n) noexcept { s_budget = n; }
    [[nodiscard]] static long long live() noexcept { return s_live; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        if (s_budget == 0) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.failure_count;
            observer_.notify(stats_);
#endif
            return nullptr; // Achsen-Vertrag: OOM == nullptr
        }
        void* const p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        if (p == nullptr) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.failure_count;
            observer_.notify(stats_);
#endif
            return nullptr;
        }
        --s_budget;
        ++s_live;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.allocation_count;
        stats_.total_bytes_allocated += bytes;
        stats_.total_bytes_in_use += bytes;
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        (void)alignment;
        if (p == nullptr) return;
        ::comdare::cache_engine::allocator::portable_aligned_free(p);
        --s_live;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.deallocation_count;
        if (bytes <= stats_.total_bytes_in_use)
            stats_.total_bytes_in_use -= bytes;
        else
            stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = acpts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

private:
    snapshot_t stats_{};
    observer_t observer_{};
#endif
};

// -- ORAKEL C: eine memory_resource, die IMMER wirft (A1-Nachbesserung 2026-08-06) -----------------
// WOZU (Review-Befund "OOM-Orakel 1<<60"): der Fehlschlag-Pfad der pmr-tragenden Strategien wurde
// bisher AUSSCHLIESSLICH ueber eine absurd grosse Anfrage an die ECHTE Resource erzwungen. Das ist ein
// UMWELT-Orakel, kein Vertrags-Orakel: unter ASan bricht eine Anfrage jenseits kMaxAllowedMallocSize
// als harter Sanitizer-Fehler ab (allocator_may_return_null=0 ist Default), statt std::bad_alloc zu
// werfen -- die Wache stuerbe, statt zu pruefen. Und kleiner darf die Zahl nicht werden: unter
// Linux-Overcommit GELINGEN Anfragen unterhalb dieser Schwelle oft.
// Diese Resource loest das an der Stelle, an der die Achse eine INJEKTION zulaesst: PmrResourceAllocator
// nimmt ein fremdes memory_resource (axis_06_allocator_pmr_resource.hpp:96) und traegt das IDENTISCHE
// Uebersetzungs-Idiom wie PoolResourceAllocator (try/catch -> nullptr + failure_count VOR der Rueckgabe;
// die Pool-Kopfzeile nennt die Schwester ausdruecklich als Muster). Der Wurf ist damit deterministisch,
// ohne jede Allokation, sanitizer-neutral und plattform-unabhaengig.
class WerfendeResource final : public std::pmr::memory_resource {
public:
    [[nodiscard]] unsigned long long wuerfe() const noexcept { return wuerfe_; }

private:
    void* do_allocate(std::size_t, std::size_t) override {
        ++wuerfe_;
        throw std::bad_alloc{};
    }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    [[nodiscard]] bool do_is_equal(std::pmr::memory_resource const& other) const noexcept override {
        return this == &other;
    }
    unsigned long long wuerfe_ = 0;
};

// -- GEGENPROBE-BAUKASTEN der Kopf-Constraint-Naht (A1-Nachbesserung 2026-08-06) -------------------
//
// WAS DIE ERST-FASSUNG NICHT BEWIES (Review-Befund 06.08.2026): sie hatte EINEN Gegenprobe-Typ ohne
// allocate_or_throw und zeigte damit "ThrowTranslatingStrategy ist echt staerker als AllocatorStrategy".
// Das ist eine Aussage ueber das CONCEPT -- nicht ueber den STORE-KOPF. Wer den Term
// `&& ThrowTranslatingStrategy<A>` aus dem Kopf wieder herausnaehme, liesse jeden dieser static_asserts
// gruen: die Regression waere unsichtbar. Und drei WEITERE Rumpf-Anforderungen (Std-Container-Naht,
// Wert-Semantik, T6-Route) standen ueberhaupt nicht im Kopf.
//
// WAS DIESER BAUKASTEN BEWEIST: fuer JEDEN der vier Allokator-Terme gibt es einen Typ, der die anderen
// drei erfuellt und GENAU an diesem einen scheitert -- und der STORE-KOPF SELBST weist ihn ab (Sonde
// StoreKopfBindetAllokator unten, ein requires-Ausdruck ueber die Bildung des Store-Template-Ids).
// Faellt ein Term aus dem Kopf, wird der zugehoerige Negativ-Satz ROT. Die Positiv-Kontrolle
// VollstaendigeFremdStrategie schliesst die Gegenrichtung: ein Typ MIT allen vier Nahten wird
// angenommen -- die Negativ-Saetze scheitern also am genannten Term und nicht an etwas anderem.
//
// KEINE dieser Klassen haengt an AllocatorStrategyBase, keine steht in einer Registry, keine wird je
// instanziiert. Sie sind reine Typ-Sonden. Der Baukasten ist bewusst additiv (Basis + Mixins): so ist
// je Negativ-Typ an EINER Zeile ablesbar, welcher Baustein fehlt.

/// Die Standard-Schnittmenge AllocatorStrategy, vollstaendig und ohne jede Achsen-Zusatz-Faehigkeit.
struct FremdBasis {
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using value_type = std::byte;
    using size_type  = std::size_t;

    [[nodiscard]] void* allocate(std::size_t, std::size_t) { return nullptr; }
    void                deallocate(void*, std::size_t, std::size_t) noexcept {}
    [[nodiscard]] bool  operator==(FremdBasis const&) const noexcept { return true; }
};

/// Mixin 1 -- die achsen-eigene Wurf-Uebersetzung (ThrowTranslatingStrategy).
struct WurfNahtMixin {
    [[nodiscard]] void* allocate_or_throw(std::size_t, std::size_t) { return nullptr; }
};

/// Mixin 2 -- die Standard-Container-Naht (StdAllocatorAdaptingStrategy). Formtreu zur CRTP-Wurzel:
/// Adapter-Typ OHNE Default-Ktor + noexcept-Fabrik.
struct StdNahtMixin {
    template <typename T>
    class StdAllocatorAdapter {
    public:
        using value_type = T;
        explicit StdAllocatorAdapter(StdNahtMixin*) noexcept {}
        template <typename U>
        StdAllocatorAdapter(StdAllocatorAdapter<U> const&) noexcept {}
        [[nodiscard]] T* allocate(std::size_t) { return nullptr; }
        void             deallocate(T*, std::size_t) noexcept {}
        template <typename U>
        [[nodiscard]] bool operator==(StdAllocatorAdapter<U> const&) const noexcept {
            return true;
        }
    };
    template <typename T>
    [[nodiscard]] StdAllocatorAdapter<T> as_std_allocator() noexcept {
        return StdAllocatorAdapter<T>(this);
    }
};

/// Mixin 3 -- die T6-Mess-Route (StatisticsReportingStrategy). Steht wie an den echten Strategien
/// unter demselben Schalter wie ihr Konsum.
struct StatistikRouteMixin {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = acpts::AllocationStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return {}; }
#endif
};

/// POSITIV-KONTROLLE: alle vier Nahten vorhanden -> der Store-Kopf nimmt den Typ, obwohl er NICHT an
/// der CRTP-Wurzel haengt. Das ist zugleich die praezise Form der Rueckwaertsvertraeglichkeits-Aussage:
/// der Kopf verlangt FAEHIGKEITEN, keine Abstammung.
struct VollstaendigeFremdStrategie : FremdBasis, WurfNahtMixin, StdNahtMixin, StatistikRouteMixin {};

/// NEGATIV 1 -- alles ausser der Wurf-Uebersetzung (der Typ der Erst-Fassung, jetzt aus dem Baukasten).
struct OhneWurfUebersetzung : FremdBasis, StdNahtMixin, StatistikRouteMixin {};

/// NEGATIV 2 -- alles ausser der Standard-Container-Naht (kein StdAllocatorAdapter/as_std_allocator).
struct OhneStdContainerNaht : FremdBasis, WurfNahtMixin, StatistikRouteMixin {};

/// NEGATIV 3 -- alles ausser der Wert-Semantik: kein Default-Ktor. Kopier-Konstruierbarkeit und
/// Zuweisbarkeit bleiben ERHALTEN, sonst scheiterte der Typ schon an AllocatorStrategy und der Satz
/// waere nicht mehr trennscharf.
struct OhneWertsemantik : FremdBasis, WurfNahtMixin, StdNahtMixin, StatistikRouteMixin {
    OhneWertsemantik()                                   = delete;
    OhneWertsemantik(OhneWertsemantik const&)            = default;
    OhneWertsemantik& operator=(OhneWertsemantik const&) = default;
};

#ifdef COMDARE_CE_ENABLE_STATISTICS
/// NEGATIV 4 -- alles ausser der T6-Route (kein snapshot_t/statistics()). Nur im Statistik-Bau eine
/// Gegenprobe: ohne COMDARE_CE_ENABLE_STATISTICS fordert das Sub-Concept die Route nicht (und der
/// Store konsumiert sie nicht) -- dann WAERE dieser Typ zulaessig, und der Satz waere falsch.
struct OhneStatistikRoute : FremdBasis, WurfNahtMixin, StdNahtMixin {};
#endif

// Die Stores der Wache: Node4 (Kapazitaet 4 -> mehrere Chunks bei kleiner Slot-Zahl) x CacheLineAligned.
using ErschoepfterStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, ErschoepfteStubStrategie>;
using BudgetStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, BudgetStubStrategie>;

/// DIE SONDE AUF DEN STORE-KOPF SELBST (A1-Nachbesserung 2026-08-06). Die Bildung des Template-Ids
/// prueft die requires-Klausel des Klassen-Templates; sind ihre Terme nicht erfuellt, ist der Ausdruck
/// im unmittelbaren Kontext ungueltig und das requires liefert false -- OHNE dass der Store-Rumpf
/// instanziiert wird. Damit misst dieser Praedikats-Ausdruck GENAU das, was ein
/// `static_assert(Concept<A>)` NICHT misst: ob der Term am KOPF DES KONSUMENTEN wirklich steht.
template <class A>
concept StoreKopfBindetAllokator =
    requires { typename nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, A>; };

// (7) F3 -- Posten 73, compile-hart: der Bezug aus der Line-Groessen-Einzelquelle ist wertgleich zum
// frueher hartkodierten Default-Argument 64. Die Aussage steht hier ZUSAETZLICH zum Selbstbeweis in
// node_width_config.hpp, damit sie in der Wache dieser Scheibe sichtbar ist.
static_assert(cacheline::kDefaultLineBytes == 64);
static_assert(
    cacheline::NodeWidthAware<cacheline::NodeWidthConfig{cacheline::NodeWidthInLines::W16}>::node_width_bytes() ==
    16u * 64u);
static_assert(
    cacheline::NodeWidthAware<cacheline::NodeWidthConfig{cacheline::NodeWidthInLines::W16}>::node_width_bytes(
        cacheline::kDefaultLineBytes) ==
    cacheline::NodeWidthAware<cacheline::NodeWidthConfig{cacheline::NodeWidthInLines::W16}>::node_width_bytes());

} // namespace

int main() {
    std::printf("== (1) KONFORMITAET: beide Orakel sind echte Achsen-Varianten, keine Fremdkoerper ==\n");
    {
        static_assert(acpts::AllocatorStrategy<ErschoepfteStubStrategie>);
        static_assert(acpts::CacheEnginePermutationStrategy<ErschoepfteStubStrategie>);
        static_assert(topics::AxisBaseConcept<ErschoepfteStubStrategie>);
        static_assert(topics::OrganAxisConcept<ErschoepfteStubStrategie>);
        static_assert(acpts::AllocatorStrategy<BudgetStubStrategie>);
        static_assert(acpts::CacheEnginePermutationStrategy<BudgetStubStrategie>);
        static_assert(topics::AxisBaseConcept<BudgetStubStrategie>);
        static_assert(topics::OrganAxisConcept<BudgetStubStrategie>);
        check("ErschoepfteStubStrategie erfuellt AllocatorStrategy",
              acpts::AllocatorStrategy<ErschoepfteStubStrategie>);
        check("BudgetStubStrategie erfuellt AllocatorStrategy", acpts::AllocatorStrategy<BudgetStubStrategie>);
        check("beide erfuellen OrganAxisConcept",
              topics::OrganAxisConcept<ErschoepfteStubStrategie> && topics::OrganAxisConcept<BudgetStubStrategie>);

        // (1b) CONCEPT-REGRESS GESCHLOSSEN (A1-Nachbesserung 2026-08-06): der Store-Kopf verlangte nur
        // AllocatorStrategy, konsumierte aber VIER Achsen-Faehigkeiten. Sie heissen jetzt
        // ThrowTranslatingStrategy / StdAllocatorAdaptingStrategy / ValueSemanticStrategy /
        // StatisticsReportingStrategy und liegen samt Begruendung in der ALLOKATOR-Achse
        // (generalisierte Schnitt-Regel). Drei Saetze je Term: das Sub-Concept ist ECHT staerker als
        // AllocatorStrategy, jede Achsen-Strategie erfuellt es geerbt, und der STORE-KOPF weist den
        // Gegenprobe-Typ tatsaechlich ab.
        static_assert(acpts::ThrowTranslatingStrategy<ErschoepfteStubStrategie>);
        static_assert(acpts::ThrowTranslatingStrategy<BudgetStubStrategie>);
        static_assert(acpts::ThrowTranslatingStrategy<alloc::ExgenAllocator>);
        static_assert(acpts::ThrowTranslatingStrategy<alloc::PoolResourceAllocator>);
        static_assert(acpts::StdAllocatorAdaptingStrategy<alloc::ExgenAllocator>);
        static_assert(acpts::StdAllocatorAdaptingStrategy<alloc::PoolResourceAllocator>);
        static_assert(acpts::ValueSemanticStrategy<alloc::ExgenAllocator>);
        static_assert(acpts::ValueSemanticStrategy<alloc::PoolResourceAllocator>);
        static_assert(acpts::StatisticsReportingStrategy<alloc::ExgenAllocator>);
        static_assert(acpts::StatisticsReportingStrategy<alloc::PoolResourceAllocator>);
        check("registrierte Varianten erfuellen ALLE VIER Allokator-Terme des Store-Kopfes (Erbe der "
              "CRTP-Wurzel)",
              acpts::ThrowTranslatingStrategy<alloc::ExgenAllocator> &&
                  acpts::StdAllocatorAdaptingStrategy<alloc::ExgenAllocator> &&
                  acpts::ValueSemanticStrategy<alloc::ExgenAllocator> &&
                  acpts::StatisticsReportingStrategy<alloc::ExgenAllocator> &&
                  acpts::ThrowTranslatingStrategy<alloc::PoolResourceAllocator> &&
                  acpts::StdAllocatorAdaptingStrategy<alloc::PoolResourceAllocator> &&
                  acpts::ValueSemanticStrategy<alloc::PoolResourceAllocator> &&
                  acpts::StatisticsReportingStrategy<alloc::PoolResourceAllocator>);

        // POSITIV-KONTROLLE ZUERST -- ohne sie waeren die Negativ-Saetze wertlos: ein Typ MIT allen vier
        // Nahten, aber OHNE Abstammung von AllocatorStrategyBase, wird vom Store-Kopf ANGENOMMEN. Die
        // Negativ-Typen unterscheiden sich von ihm in GENAU EINEM Baustein -- ihre Ablehnung kann also
        // keine andere Ursache haben.
        static_assert(acpts::AllocatorStrategy<VollstaendigeFremdStrategie>);
        static_assert(acpts::ThrowTranslatingStrategy<VollstaendigeFremdStrategie>);
        static_assert(acpts::StdAllocatorAdaptingStrategy<VollstaendigeFremdStrategie>);
        static_assert(acpts::ValueSemanticStrategy<VollstaendigeFremdStrategie>);
        static_assert(acpts::StatisticsReportingStrategy<VollstaendigeFremdStrategie>);
        static_assert(StoreKopfBindetAllokator<VollstaendigeFremdStrategie>,
                      "der Store-Kopf verlangt FAEHIGKEITEN, keine Abstammung -- ein vollstaendiger "
                      "Fremd-Allokator MUSS bindbar bleiben (Rueckwaertsvertraeglichkeits-Aussage).");
        static_assert(StoreKopfBindetAllokator<alloc::ExgenAllocator>);
        static_assert(StoreKopfBindetAllokator<alloc::PoolResourceAllocator>);
        check("POSITIV: vollstaendiger Fremd-Allokator (ohne CRTP-Abstammung) bindet am Store-Kopf",
              StoreKopfBindetAllokator<VollstaendigeFremdStrategie> &&
                  StoreKopfBindetAllokator<alloc::ExgenAllocator>);

        // TERM 1 -- Wurf-Uebersetzung.
        static_assert(acpts::AllocatorStrategy<OhneWurfUebersetzung>,
                      "der Gegenprobe-Typ MUSS AllocatorStrategy erfuellen -- nur dann belegt er, dass das "
                      "Sub-Concept ECHT staerker ist und nicht bloss dieselbe Menge anders schreibt.");
        static_assert(!acpts::ThrowTranslatingStrategy<OhneWurfUebersetzung>);
        static_assert(acpts::StdAllocatorAdaptingStrategy<OhneWurfUebersetzung> &&
                          acpts::ValueSemanticStrategy<OhneWurfUebersetzung> &&
                          acpts::StatisticsReportingStrategy<OhneWurfUebersetzung>,
                      "ISOLATION: er darf NUR an der Wurf-Uebersetzung scheitern.");
        static_assert(!StoreKopfBindetAllokator<OhneWurfUebersetzung>,
                      "REGRESS-WACHE: faellt `&& ThrowTranslatingStrategy<A>` aus dem Store-Kopf, wird "
                      "dieser Satz rot -- die Concept-Saetze darueber wuerden es NICHT.");
        check("TERM 1 (allocate_or_throw): Sub-Concept echt staerker, Store-Kopf weist ab",
              acpts::AllocatorStrategy<OhneWurfUebersetzung> &&
                  !acpts::ThrowTranslatingStrategy<OhneWurfUebersetzung> &&
                  !StoreKopfBindetAllokator<OhneWurfUebersetzung>);

        // TERM 2 -- Standard-Container-Naht (daran haengt der Chunk-INDEX seit A8-S5-02a).
        static_assert(acpts::AllocatorStrategy<OhneStdContainerNaht>);
        static_assert(!acpts::StdAllocatorAdaptingStrategy<OhneStdContainerNaht>);
        static_assert(acpts::ThrowTranslatingStrategy<OhneStdContainerNaht> &&
                          acpts::ValueSemanticStrategy<OhneStdContainerNaht> &&
                          acpts::StatisticsReportingStrategy<OhneStdContainerNaht>,
                      "ISOLATION: er darf NUR an der Std-Container-Naht scheitern.");
        static_assert(!StoreKopfBindetAllokator<OhneStdContainerNaht>,
                      "REGRESS-WACHE: faellt `&& StdAllocatorAdaptingStrategy<A>` aus dem Store-Kopf, "
                      "wird dieser Satz rot.");
        check("TERM 2 (StdAllocatorAdapter/as_std_allocator): Store-Kopf weist ab",
              acpts::AllocatorStrategy<OhneStdContainerNaht> &&
                  !acpts::StdAllocatorAdaptingStrategy<OhneStdContainerNaht> &&
                  !StoreKopfBindetAllokator<OhneStdContainerNaht>);

        // TERM 3 -- Wert-Semantik (`mutable A alloc_{}` + `alloc_ = A{}` im operator=).
        static_assert(acpts::AllocatorStrategy<OhneWertsemantik>);
        static_assert(!acpts::ValueSemanticStrategy<OhneWertsemantik>);
        static_assert(acpts::ThrowTranslatingStrategy<OhneWertsemantik> &&
                          acpts::StdAllocatorAdaptingStrategy<OhneWertsemantik> &&
                          acpts::StatisticsReportingStrategy<OhneWertsemantik>,
                      "ISOLATION: er darf NUR an der Wert-Semantik scheitern.");
        static_assert(std::assignable_from<OhneWertsemantik&, OhneWertsemantik> &&
                          !std::default_initializable<OhneWertsemantik>,
                      "und zwar GENAU am Default-Ktor, nicht an der Zuweisbarkeit.");
        static_assert(!StoreKopfBindetAllokator<OhneWertsemantik>,
                      "REGRESS-WACHE: faellt `&& ValueSemanticStrategy<A>` aus dem Store-Kopf, wird "
                      "dieser Satz rot.");
        check("TERM 3 (A{} / alloc_ = A{}): Store-Kopf weist ab",
              acpts::AllocatorStrategy<OhneWertsemantik> && !acpts::ValueSemanticStrategy<OhneWertsemantik> &&
                  !StoreKopfBindetAllokator<OhneWertsemantik>);

#ifdef COMDARE_CE_ENABLE_STATISTICS
        // TERM 4 -- T6-Mess-Route. Nur im Statistik-Bau eine Gegenprobe (s. Typ-Kommentar).
        static_assert(acpts::AllocatorStrategy<OhneStatistikRoute>);
        static_assert(!acpts::StatisticsReportingStrategy<OhneStatistikRoute>);
        static_assert(acpts::ThrowTranslatingStrategy<OhneStatistikRoute> &&
                          acpts::StdAllocatorAdaptingStrategy<OhneStatistikRoute> &&
                          acpts::ValueSemanticStrategy<OhneStatistikRoute>,
                      "ISOLATION: er darf NUR an der T6-Route scheitern.");
        static_assert(!StoreKopfBindetAllokator<OhneStatistikRoute>,
                      "REGRESS-WACHE: faellt `&& StatisticsReportingStrategy<A>` aus dem Store-Kopf, "
                      "wird dieser Satz rot.");
        check("TERM 4 (snapshot_t/statistics): Store-Kopf weist ab",
              acpts::AllocatorStrategy<OhneStatistikRoute> && !acpts::StatisticsReportingStrategy<OhneStatistikRoute> &&
                  !StoreKopfBindetAllokator<OhneStatistikRoute>);
#endif
    }

    std::printf("== (2) F1/Posten 71: PmrResourceAdapter uebersetzt nullptr -> std::bad_alloc ==\n");
    {
        ErschoepfteStubStrategie strategie{};
        auto                     resource = strategie.as_pmr_resource();

        // NEGATIV-FALL am REALEN Konsumenten-Muster (std::pmr::vector ueber as_pmr_resource()).
        bool geworfen = false;
        try {
            std::pmr::vector<std::uint64_t> v{&resource};
            v.push_back(0xC0FFEEu); // erste Belegung -> do_allocate -> nullptr aus der Strategie
            std::printf("  [INFO] push_back kehrte OHNE Wurf zurueck (size=%zu)\n", v.size());
        } catch (std::bad_alloc const& e) {
            geworfen = true;
            std::printf("  [INFO] gefangen: std::bad_alloc -- what()='%s'\n", e.what());
        }
        check("pmr::vector::push_back auf erschoepfter Strategie wirft std::bad_alloc", geworfen);

        // ZERO-SIZE AM PMR-WEG -- KORRIGIERTES ORAKEL (A1-Nachbesserung 2026-08-06). Die Erst-Fassung
        // dieser Wache pruefte hier "wirft NICHT" und rief anschliessend deallocate(nullptr, 0): sie
        // ZEMENTIERTE damit genau den Vertragsbruch, den sie haette fangen muessen. [mem.res.public]
        // kennt an dieser Naht keinen nullptr -- in KEINEM Fall, auch nicht bei bytes == 0 (der
        // pmr-Container prueft nicht nach, er speichert den Zeiger und rechnet mit ihm weiter). Das
        // richtige Orakel ist deshalb "NIE nullptr" = Zeiger ODER Wurf; eine erschoepfte Strategie kann
        // fuer 0 Bytes nichts liefern und MUSS hier folglich werfen.
        bool  geworfen_0 = false;
        void* p0         = reinterpret_cast<void*>(~std::uintptr_t{0});
        try {
            p0 = resource.allocate(0, alignof(std::max_align_t));
            std::printf("  [INFO] do_allocate(0) lieferte %s\n", p0 == nullptr ? "nullptr" : "einen Zeiger");
            if (p0 != nullptr) resource.deallocate(p0, 0, alignof(std::max_align_t));
        } catch (std::bad_alloc const&) { geworfen_0 = true; }
        check("PMR-Naht liefert NIE nullptr -- auch nicht bei bytes == 0", geworfen_0 || p0 != nullptr);
        check("erschoepfte Strategie bei bytes == 0 -> Wurf statt nullptr an den pmr-Container", geworfen_0);

        // GEGENSTUECK: die zwei ANDEREN Wege BEHALTEN ihre Zero-Size-Ausnahme -- dieselbe erschoepfte
        // Strategie, derselbe nullptr, dort aber vertraglich zulaessig. Ohne diese beiden Saetze waere
        // die Aenderung oben nicht von einem globalen Zero-Size-Umbau zu unterscheiden.
        bool           geworfen_std0 = false;
        std::uint64_t* p_std0        = reinterpret_cast<std::uint64_t*>(~std::uintptr_t{0});
        try {
            auto std_adapter = strategie.as_std_allocator<std::uint64_t>();
            p_std0           = std_adapter.allocate(0);
        } catch (std::bad_alloc const&) { geworfen_std0 = true; }
        check("StdAllocatorAdapter behaelt die Zero-Size-Ausnahme (n == 0 wirft NICHT)", !geworfen_std0);
        check("StdAllocatorAdapter reicht den nullptr bei n == 0 durch", !geworfen_std0 && p_std0 == nullptr);

        bool  geworfen_roh0 = false;
        void* p_roh0        = reinterpret_cast<void*>(~std::uintptr_t{0});
        try {
            p_roh0 = strategie.allocate_or_throw(0, alignof(std::max_align_t));
        } catch (std::bad_alloc const&) { geworfen_roh0 = true; }
        check("allocate_or_throw behaelt die Zero-Size-Ausnahme (bytes == 0 wirft NICHT)", !geworfen_roh0);
        check("allocate_or_throw reicht den nullptr bei bytes == 0 durch", !geworfen_roh0 && p_roh0 == nullptr);

        // POSITIV-KONTROLLE: sonst waere ein Adapter, der IMMER wirft, ebenfalls gruen.
        alloc::ExgenAllocator echt{};
        auto                  echt_resource = echt.as_pmr_resource();
        bool                  geworfen_echt = false;
        std::size_t           groesse       = 0;
        try {
            std::pmr::vector<std::uint64_t> v{&echt_resource};
            for (std::uint64_t i = 0; i < 64; ++i) v.push_back(i * 3u + 1u);
            groesse = v.size();
            check("Inhalt korrekt (v[63] == 190)", v[63] == 190u);
        } catch (std::bad_alloc const&) { geworfen_echt = true; }
        check("64x push_back ueber die reale Strategie wirft NICHT", !geworfen_echt);
        check("Groesse korrekt (64)", groesse == 64u);

        // dieselbe "NIE nullptr"-Aussage an der REALEN Strategie. Das Orakel ist "Wurf ODER Nichtnull" --
        // und NUR das (Review-Befund 06.08.2026, Korrektur der Erst-Fassung):
        //
        // Die Erst-Fassung pruefte hier ZUSAETZLICH "und zwar per Zeiger, nicht per Wurf". Dieser Zusatz
        // war UEBERSCHARF und haette an einer anderen Plattform grundlos gebissen: ExgenAllocator::allocate
        // fuehrt auf portable_aligned_alloc, und C11 laesst fuer size == 0 ausdruecklich BEIDES zu -- einen
        // eindeutigen Zeiger ODER NULL (7.22.3/1, "either a null pointer or a pointer to the allocated
        // space"). Meldet die Plattform dort NULL, uebersetzt PmrResourceAdapter::do_allocate das
        // VERTRAGSKONFORM in std::bad_alloc ([mem.res.public] kennt an dieser Naht keinen nullptr) -- der
        // Wurf waere dann das RICHTIGE Verhalten und die Wache trotzdem rot. Ein Test, der konformes
        // Verhalten rot faerbt, misst die Plattform statt den Vertrag.
        //
        // Der Zusatz war ausserdem UEBERFLUESSIG: "die Naht wirft nicht pauschal" ist eine Zeile hoeher
        // bereits belegt (64x push_back ueber dieselbe reale Strategie, ohne Wurf). Der beobachtete Ausgang
        // wird nur noch BERICHTET, nicht mehr gefordert.
        bool  geworfen_echt0 = false;
        void* p_echt0        = reinterpret_cast<void*>(~std::uintptr_t{0});
        try {
            p_echt0 = echt_resource.allocate(0, alignof(std::max_align_t));
            if (p_echt0 != nullptr) echt_resource.deallocate(p_echt0, 0, alignof(std::max_align_t));
        } catch (std::bad_alloc const&) { geworfen_echt0 = true; }
        std::printf("  [INFO] reale Strategie, do_allocate(0): %s (beide Ausgaenge sind vertragskonform)\n",
                    geworfen_echt0 ? "Wurf" : (p_echt0 == nullptr ? "nullptr" : "Zeiger"));
        check("reale Strategie an der PMR-Naht: bytes == 0 liefert NIE nullptr", geworfen_echt0 || p_echt0 != nullptr);
    }

    std::printf("== (3) F2/Posten 72: n*sizeof(T)-Ueberlauf -> std::bad_array_new_length ==\n");
    {
        alloc::ExgenAllocator echt{};
        auto                  adapter = echt.as_std_allocator<std::uint64_t>();

        // Das kleinste n, dessen Produkt mit sizeof(uint64_t) NICHT mehr in size_t passt.
        constexpr std::size_t kUmlaufN        = (std::numeric_limits<std::size_t>::max)() / sizeof(std::uint64_t) + 1u;
        bool                  geworfen        = false;
        bool                  richtige_klasse = false;
        try {
            std::uint64_t* p = adapter.allocate(kUmlaufN);
            std::printf("  [INFO] allocate(umlaufendes n) lieferte %s -- KEINE Wache!\n",
                        p == nullptr ? "nullptr" : "einen Zeiger");
        } catch (std::bad_array_new_length const& e) {
            geworfen        = true;
            richtige_klasse = true;
            std::printf("  [INFO] gefangen: std::bad_array_new_length -- what()='%s'\n", e.what());
        } catch (std::bad_alloc const&) {
            geworfen = true; // bad_array_new_length IST ein bad_alloc; diese Zweigung trennt die Klassen
        }
        check("allocate(n) mit umlaufendem n*sizeof(T) wirft", geworfen);
        check("und zwar std::bad_array_new_length (Standard-Praezedenz)", richtige_klasse);

        // POSITIV-KONTROLLE: realistische Groessen laufen unveraendert durch.
        bool geworfen_ok = false;
        try {
            std::uint64_t* p = adapter.allocate(256);
            check("realistische Vergabe liefert einen Zeiger", p != nullptr);
            adapter.deallocate(p, 256);
        } catch (std::bad_alloc const&) { geworfen_ok = true; }
        check("allocate(256) wirft NICHT (Erfolgs-Pfad unveraendert)", !geworfen_ok);
    }

    std::printf("== (4) F6: PoolResourceAllocator haelt jetzt den achsen-uniformen nullptr-Vertrag ==\n");
    {
        // (4a) DETERMINISTISCHES ORAKEL des Uebersetzungs-Idioms (A1-Nachbesserung 2026-08-06, s.
        // WerfendeResource): pmr-Wurf -> nullptr + failure_count VOR der Rueckgabe. Kein Byte alloziert,
        // kein Umwelt-Verhalten befragt, sanitizer-neutral. Gegenstand ist die Schwester-Strategie
        // PmrResourceAllocator, weil sie als EINZIGE der Achse ihr memory_resource von aussen nimmt --
        // das Idiom ist Zeile fuer Zeile dasselbe, das (4b) am Pool bestaetigt.
        WerfendeResource            werfend{};
        alloc::PmrResourceAllocator ueber_werfend{&werfend};
        bool                        pmr_geworfen = false;
        void*                       p_pmr        = reinterpret_cast<void*>(~std::uintptr_t{0});
        try {
            p_pmr = ueber_werfend.allocate(64, 16);
        } catch (std::bad_alloc const&) { pmr_geworfen = true; }
        check("Uebersetzung deterministisch: pmr-Wurf dringt NICHT nach aussen", !pmr_geworfen);
        check("Uebersetzung deterministisch: pmr-Wurf wird zu nullptr", !pmr_geworfen && p_pmr == nullptr);
        check("die Fake-Resource hat wirklich geworfen (Orakel selbst geprueft)", werfend.wuerfe() == 1u);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        check("failure_count VOR der Rueckgabe gezaehlt (Ehrlichkeits-Auflage)",
              ueber_werfend.statistics().failure_count == 1u);
#endif

        // (4b) DER EIGENTLICHE GEGENSTAND: PoolResourceAllocator selbst. Seine Resource ist BESITZEND
        // (ResourceOwnership::Owned) und nicht von aussen setzbar -- hier gibt es keine Injektions-Naht,
        // der Fehlschlag muss ueber eine reale, garantiert scheiternde Anfrage kommen. Die Zahl bleibt
        // deshalb absurd gross (kleiner waere sie unter Overcommit nicht mehr garantiert). GRENZE, ehrlich
        // benannt statt still gelassen: unter ASan ist dieser eine Aufruf ein harter Sanitizer-Abbruch
        // statt eines Wurfs. Er ist heute unkritisch -- der sanitize:asan-ubsan-Job baut und laeuft GENAU
        // fuenf Targets (.gitlab-ci.yml:182-183), diese TU ist keines davon. Wer sie dort aufnimmt, muss
        // vorher eine Upstream-Naht an PoolResourceAllocatorBody schaffen; bis dahin traegt (4a) den
        // sanitizer-festen Teil der Aussage.
        alloc::PoolResourceAllocator pool{};

        // Erfolgs-Pfad zuerst -- er MUSS unangetastet sein.
        void* ok = pool.allocate(64, 16);
        check("Erfolgs-Pfad unveraendert: allocate(64,16) liefert einen Zeiger", ok != nullptr);
        if (ok != nullptr) pool.deallocate(ok, 64, 16);

        // OOM deterministisch erzwingen: eine Groesse jenseits des Adressraums geht am Size-Class-Pool
        // vorbei direkt upstream (new_delete_resource) -> operator new schlaegt fehl -> pmr wirft.
        // Vor dem A1-Schnitt lief dieser Wurf durch; jetzt wird er auf nullptr zurueckuebersetzt.
        constexpr std::size_t kAbsurd  = std::size_t{1} << 60;
        bool                  geworfen = false;
        void*                 p_oom    = reinterpret_cast<void*>(~std::uintptr_t{0});
        try {
            p_oom = pool.allocate(kAbsurd, 16);
        } catch (std::bad_alloc const&) { geworfen = true; }
        check("allocate(OOM) wirft NICHT mehr (Achsen-Vertrag statt pmr-Vertrag)", !geworfen);
        check("allocate(OOM) liefert nullptr", !geworfen && p_oom == nullptr);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        auto const s = pool.statistics();
        std::printf("  [INFO] Pool: allocation_count=%llu failure_count=%llu\n",
                    static_cast<unsigned long long>(s.allocation_count),
                    static_cast<unsigned long long>(s.failure_count));
        check("failure_count >= 1 (Zaehlung VOR der Rueckgabe -- Ehrlichkeit bleibt)", s.failure_count >= 1u);
        check("allocation_count >= 1 (der Erfolgs-Pfad zaehlt weiter)", s.allocation_count >= 1u);
#endif

        // reallocate: Erfolg unveraendert; bei Fehlschlag ueberlebt der ALTE Block (realloc-Vertrag).
        void* alt = pool.allocate(64, 16);
        check("reallocate-Vorbedingung: alter Block vorhanden", alt != nullptr);
        if (alt != nullptr) {
            void* neu = pool.reallocate(alt, 64, 128, 16);
            check("reallocate-Erfolg unveraendert (Zeiger != nullptr)", neu != nullptr);
            if (neu != nullptr) {
                bool  r_geworfen = false;
                void* fehl       = reinterpret_cast<void*>(~std::uintptr_t{0});
                try {
                    fehl = pool.reallocate(neu, 128, kAbsurd, 16);
                } catch (std::bad_alloc const&) { r_geworfen = true; }
                check("reallocate(OOM) wirft NICHT", !r_geworfen);
                check("reallocate(OOM) liefert nullptr", !r_geworfen && fehl == nullptr);
                pool.deallocate(neu, 128, 16); // der ALTE Block ist gueltig geblieben -> freigebbar
            }
        }
    }

    std::printf("== (5) F4/Posten 74: append_slot wirft, statt in Nullspeicher zu memsetzen ==\n");
    {
        ErschoepfterStore store{};
        bool              geworfen = false;
        try {
            store.append_slot(1u, 100u); // erster Chunk -> alloc_.allocate_or_throw -> nullptr -> Wurf
            std::printf("  [INFO] append_slot kehrte OHNE Wurf zurueck\n");
        } catch (std::bad_alloc const& e) {
            geworfen = true;
            std::printf("  [INFO] gefangen: std::bad_alloc -- what()='%s'\n", e.what());
        }
        check("append_slot auf erschoepfter Strategie wirft std::bad_alloc", geworfen);
        check("der Store blieb leer (slot_count == 0)", store.slot_count() == 0u);
        check("kein Chunk eingetragen (chunk_count == 0)", store.chunk_count() == 0u);
        check("kein Chunk gezaehlt (chunk_alloc_count == 0)", store.chunk_alloc_count() == 0u);
    }

    std::printf("== (5b) F4-Zwilling: append_slot leckt nicht, wenn der INDEX-Eintrag wirft ==\n");
    {
        // append_slot hat ZWEI Wurf-Quellen: die Record-Vergabe (oben belegt) und das Wachstum des
        // Chunk-INDEX in push_back, das seit dem HERZ-Schnitt ebenfalls ueber die Achse laeuft. Zwischen
        // beiden haelt NUR die lokale Chunk-Variable den frischen Block. Budget 1 trifft genau diese
        // Luecke: die Record-Vergabe gelingt, der Index-Eintrag scheitert.
        BudgetStubStrategie::unbeschraenkt();
        BudgetStubStrategie::s_live = 0;

        BudgetStore     store{};
        long long const live_vorher = BudgetStubStrategie::live();
        BudgetStubStrategie::budget_setzen(1);
        bool geworfen = false;
        try {
            store.append_slot(1u, 100u);
            std::printf("  [INFO] append_slot gelang OHNE Wurf -- Budget zu gross gewaehlt\n");
        } catch (std::bad_alloc const&) { geworfen = true; }
        BudgetStubStrategie::unbeschraenkt();

        std::printf("  [INFO] Live-Bloecke vorher=%lld nachher=%lld\n", live_vorher, BudgetStubStrategie::live());
        check("append_slot wirft, wenn der Index-Eintrag scheitert", geworfen);
        check("KEIN LECK: der bereits vergebene Record-Block ist zurueckgegeben",
              BudgetStubStrategie::live() == live_vorher);
        check("starke Ausnahme-Garantie: Store unveraendert (slot_count == 0)", store.slot_count() == 0u);
        check("starke Ausnahme-Garantie: chunk_alloc_count == 0", store.chunk_alloc_count() == 0u);
    }

    std::printf("== (6) F5/Posten 74: Kopier-Ktor leckt nicht, wenn er mitten in copy_from_ wirft ==\n");
    {
        BudgetStubStrategie::unbeschraenkt();
        BudgetStubStrategie::s_live = 0;

        // Quelle mit MEHREREN Chunks (Node4 -> Kapazitaet 4 Records je Chunk).
        BudgetStore quelle{};
        for (std::uint64_t i = 0; i < 12; ++i) quelle.append_slot(i, i * 10u);
        check("Quelle gefuellt (12 Slots)", quelle.slot_count() == 12u);
        check("Quelle hat mehr als einen Chunk", quelle.chunk_count() > 1u);

        long long const live_vorher = BudgetStubStrategie::live();
        std::printf("  [INFO] Live-Bloecke der Achse VOR dem Kopierversuch: %lld (chunks=%zu)\n", live_vorher,
                    quelle.chunk_count());
        check("Bilanz-Vorbedingung: es sind ueberhaupt Bloecke offen", live_vorher > 0);

        // Budget so setzen, dass der Index-reserve UND mindestens ein Chunk gelingen, der naechste aber
        // NICHT -- nur dann steht beim Wurf wirklich etwas Materialisiertes im Weg (das potenzielle Leck).
        BudgetStubStrategie::budget_setzen(2);
        bool geworfen = false;
        try {
            BudgetStore kopie{quelle}; // Kopier-Ktor -> copy_from_ -> Wurf in Iteration 2
            std::printf("  [INFO] Kopie gelang OHNE Wurf (chunks=%zu) -- Budget zu gross gewaehlt\n",
                        kopie.chunk_count());
        } catch (std::bad_alloc const& e) {
            geworfen = true;
            std::printf("  [INFO] gefangen: std::bad_alloc -- what()='%s'\n", e.what());
        }
        BudgetStubStrategie::unbeschraenkt();

        long long const live_nachher = BudgetStubStrategie::live();
        std::printf("  [INFO] Live-Bloecke der Achse NACH dem gescheiterten Kopierversuch: %lld\n", live_nachher);
        check("der Kopier-Ktor wirft std::bad_alloc", geworfen);
        check("KEIN LECK: die Live-Bilanz kehrt exakt auf den Stand vor dem Kopierversuch zurueck",
              live_nachher == live_vorher);
        check("die Quelle ist unversehrt (12 Slots)", quelle.slot_count() == 12u);
        check("die Quelle liest korrekt zurueck (Slot 11 -> 110)", quelle.value_at(11) == 110u);

        // POSITIV-KONTROLLE: mit Budget kopiert derselbe Pfad vollstaendig und korrekt.
        {
            BudgetStore kopie{quelle};
            check("Kopie mit Budget gelingt (12 Slots)", kopie.slot_count() == 12u);
            check("Kopie ist inhaltsgleich (Slot 7 -> 70)", kopie.value_at(7) == 70u);
        }
        check("nach Ablauf beider Stores bleibt kein Block offen", BudgetStubStrategie::live() == live_vorher);
    }

    std::printf("== (7) F3/Posten 73: der Literal-Bezug ist wertgleich (compile-hart, s. static_asserts) ==\n");
    {
        using W16 = cacheline::NodeWidthAware<cacheline::NodeWidthConfig{cacheline::NodeWidthInLines::W16}>;
        std::printf("  [INFO] kDefaultLineBytes=%zu  node_width_bytes(W16)=%zu\n", cacheline::kDefaultLineBytes,
                    W16::node_width_bytes());
        check("kDefaultLineBytes == 64 (Achsen-Default unbewegt)", cacheline::kDefaultLineBytes == 64u);
        check("node_width_bytes(W16) == 16*64 (wertgleich zum frueheren Literal-Default)",
              W16::node_width_bytes() == 16u * 64u);
    }

    if (g_fail == 0) {
        std::printf("A1 WURF-VERTRAG (Posten 71/72/73/74 + Pool-Konvention): ALLE OK\n");
    } else {
        std::printf("A1 WURF-VERTRAG (Posten 71/72/73/74 + Pool-Konvention): %d FEHLER\n", g_fail);
    }
    return g_fail == 0 ? 0 : 1;
}
