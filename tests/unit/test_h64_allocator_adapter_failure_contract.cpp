// HAERTUNG Posten 64 (Ledger-Nachtrag 04.08.2026 abend-9/abend-10) -- der FEHLSCHLAG-VERTRAG des
// StdAllocatorAdapter, negativ belegt.
//
// AUSGANGSLAGE (der UB-Pfad, den diese TU schliesst): die Allokator-ACHSE meldet OOM per nullptr
// (Achsen-Semantik, z.B. ExgenAllocator::allocate -> portable_aligned_alloc). Der StdAllocatorAdapter
// castete diesen nullptr bis 04.08.2026 UNGEPRUEFT auf T* weiter. Ein BESITZENDER Container -- seit dem
// A8-S5-Schnitt sind das mapping-, value_handle-, cache_traversal-Organe und die vier Pool-Stores --
// prueft den Rueckgabewert von allocate() NICHT (er darf es nach Standard nicht muessen) und
// konstruiert dann in Nullspeicher: undefiniertes Verhalten, still, mitten im Mess-Pfad.
//
// NEUER VERTRAG (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate): liefert die
// Strategie nullptr bei n != 0, wirft der Adapter std::bad_alloc. Das ist exakt die Aussage der
// std::allocator-Named-Requirements (C++23 [allocator.requirements]) und stellt zugleich die
// Vor-Scrub-Semantik der Organe definiert wieder her -- deren [[allocation-failure-exception]]-Notizen
// ("kann std::bad_alloc werfen") sind damit wieder WAHR, nur mit neuer Kausalitaet: der Wurf kommt vom
// ADAPTER, nicht mehr vom Default-Allokator.
//
// WAS DIESE TU BEWEIST (literal, nicht behauptet):
//   (1) KONFORMITAET DES ORAKELS. Die Stub-Strategie ist KEIN Attrappen-Typ neben der Achse, sondern
//       eine vollwertige Achsen-Variante: sie haengt an AllocatorStrategyBase und erfuellt alle vier
//       Pflicht-Concepts. Ohne diesen Abschnitt bewiese der Rest nur etwas ueber einen Fremdkoerper.
//   (2) NEGATIV-FALL. std::vector<T, StdAllocatorAdapter<T>>::push_back auf einer Strategie, die
//       IMMER nullptr liefert, wirft std::bad_alloc -- statt in Nullspeicher zu konstruieren.
//   (3) EHRLICHKEIT BLEIBT. Der failure_count der Strategie ist beim Wurf BEREITS gezaehlt. Der
//       Wurf verdeckt keinen Messwert; er ersetzt nur den UB-Pfad dahinter.
//   (4) ZERO-SIZE UNVERAENDERT. allocate(0) laeuft weiter durch, ohne zu werfen (die Zero-Size-Wachen
//       der Organe bleiben gueltig).
//   (5) POSITIV-KONTROLLE. Dieselbe Naht mit der REALEN Achsen-Default-Strategie (ExgenAllocator)
//       allokiert weiter normal: kein Wurf, korrekte Werte, failure_count == 0. Ohne diesen Abschnitt
//       waere ein Adapter, der IMMER wirft, ebenfalls "gruen".
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.
// ASCII-only.

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp> // die reale Achsen-Default-Strategie (Positiv-Kontrolle)
#include <organ_axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <organ_axes/alloc/axis_06_allocator_subaxes_aa1_to_aa7.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/concepts/topic_allocator_concept.hpp>
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>

#include <measurement/measurable_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <new>
#include <string_view>
#include <type_traits>
#include <vector>

namespace alloc  = ::comdare::cache_engine::alloc;
namespace acpts  = ::comdare::cache_engine::alloc::concepts;
namespace topics = ::comdare::cache_engine::topics;

namespace {

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::printf("  %s %s\n", ok ? "[OK] " : "[ERR]", what);
    if (!ok) ++g_fail;
}

// -- Das ORAKEL: eine Achsen-Variante, deren Speicherquelle IMMER erschoepft ist -------------------
// Sie bildet den OOM-Fall der realen Vendoren nach, ohne ihn erzwingen zu muessen (echtes OOM ist
// weder deterministisch noch in einer Test-Suite zumutbar). Wichtig fuer (3): sie zaehlt den
// failure_count VOR dem return -- genau wie ExgenAllocator::allocate es tut.
class ErschoepfteStubStrategie : public alloc::AllocatorStrategyBase<ErschoepfteStubStrategie> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = alloc::subaxes::synchronization_tag;
    using family_id  = std::integral_constant<int, 0>; // 0 = KEINE Vendor-Familie: reiner Test-Stub

    static constexpr std::string_view algo_version = "1.0.0";

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "test_stub_erschoepft"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Test-Stub: Speicherquelle immer erschoepft (Posten-64-Orakel)";
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

    /// Immer erschoepft. Die Zaehlung steht VOR dem return -- die Ehrlichkeits-Auflage aus Posten 64.
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

} // namespace

int main() {
    std::printf("== (1) KONFORMITAET: das Orakel ist eine echte Achsen-Variante, kein Fremdkoerper ==\n");
    {
        static_assert(acpts::AllocatorStrategy<ErschoepfteStubStrategie>);
        static_assert(acpts::CacheEnginePermutationStrategy<ErschoepfteStubStrategie>);
        static_assert(topics::AxisBaseConcept<ErschoepfteStubStrategie>);
        static_assert(topics::OrganAxisConcept<ErschoepfteStubStrategie>);
        check("ErschoepfteStubStrategie erfuellt AllocatorStrategy",
              acpts::AllocatorStrategy<ErschoepfteStubStrategie>);
        check("ErschoepfteStubStrategie erfuellt CacheEnginePermutationStrategy",
              acpts::CacheEnginePermutationStrategy<ErschoepfteStubStrategie>);
        check("ErschoepfteStubStrategie erfuellt AxisBaseConcept", topics::AxisBaseConcept<ErschoepfteStubStrategie>);
        check("ErschoepfteStubStrategie erfuellt OrganAxisConcept", topics::OrganAxisConcept<ErschoepfteStubStrategie>);
    }

    std::printf("== (2) NEGATIV-FALL: vector::push_back wirft std::bad_alloc statt in Nullspeicher zu bauen ==\n");
    ErschoepfteStubStrategie strategie{};
    using StubAdapter = ErschoepfteStubStrategie::StdAllocatorAdapter<std::uint64_t>;
    {
        std::vector<std::uint64_t, StubAdapter> v{strategie.as_std_allocator<std::uint64_t>()};
        bool                                    geworfen = false;
        try {
            v.push_back(0xDEADBEEFu); // erste Belegung -> allocate(1) -> nullptr aus der Strategie
            std::printf("  [INFO] push_back kehrte OHNE Wurf zurueck (size=%zu)\n", v.size());
        } catch (std::bad_alloc const& e) {
            geworfen = true;
            std::printf("  [INFO] gefangen: std::bad_alloc -- what()='%s'\n", e.what());
        }
        check("push_back auf erschoepfter Strategie wirft std::bad_alloc", geworfen);
        check("der Container blieb leer (nichts in Nullspeicher konstruiert)", v.empty());
    }

    std::printf("== (3) EHRLICHKEIT: der failure_count der Strategie ist beim Wurf bereits gezaehlt ==\n");
    {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        auto const s = strategie.statistics();
        std::printf("  [INFO] failure_count=%llu allocation_count=%llu\n",
                    static_cast<unsigned long long>(s.failure_count),
                    static_cast<unsigned long long>(s.allocation_count));
        check("failure_count >= 1 NACH dem Wurf (Zaehlung vor dem Wurf)", s.failure_count >= 1u);
        check("allocation_count == 0 (nichts wurde real vergeben)", s.allocation_count == 0u);
#else
        std::printf("  [INFO] COMDARE_CE_ENABLE_STATISTICS=OFF -- Zaehler-Haelfte entfaellt bauartbedingt\n");
#endif
    }

    std::printf("== (4) ZERO-SIZE UNVERAENDERT: allocate(0) wirft NICHT ==\n");
    {
        auto adapter    = strategie.as_std_allocator<std::uint64_t>();
        bool geworfen_0 = false;
        try {
            std::uint64_t* p = adapter.allocate(0);
            std::printf("  [INFO] allocate(0) lieferte %s\n", p == nullptr ? "nullptr" : "einen Zeiger");
            adapter.deallocate(p, 0);
        } catch (std::bad_alloc const&) { geworfen_0 = true; }
        check("allocate(0) wirft nicht (Zero-Size-Verhalten unveraendert)", !geworfen_0);
    }

    std::printf("== (5) POSITIV-KONTROLLE: die reale Achsen-Default-Strategie allokiert weiter normal ==\n");
    {
        alloc::ExgenAllocator echt{};
        using EchtAdapter = alloc::ExgenAllocator::StdAllocatorAdapter<std::uint64_t>;
        std::vector<std::uint64_t, EchtAdapter> v{echt.as_std_allocator<std::uint64_t>()};
        bool                                    geworfen = false;
        try {
            for (std::uint64_t i = 0; i < 64; ++i) v.push_back(i * 3u + 1u);
        } catch (std::bad_alloc const&) { geworfen = true; }
        check("64x push_back ueber ExgenAllocator wirft NICHT", !geworfen);
        check("Groesse korrekt (64)", v.size() == 64u);
        check("Inhalt korrekt (v[63] == 190)", !v.empty() && v[63] == 190u);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        auto const s = echt.statistics();
        std::printf("  [INFO] Exgen: allocation_count=%llu failure_count=%llu\n",
                    static_cast<unsigned long long>(s.allocation_count),
                    static_cast<unsigned long long>(s.failure_count));
        check("Exgen failure_count == 0 (kein Fehlschlag im Normalpfad)", s.failure_count == 0u);
        check("Exgen allocation_count >= 1 (real allokiert)", s.allocation_count >= 1u);
#endif
    }

    if (g_fail == 0) {
        std::printf("POSTEN-64 FEHLSCHLAG-VERTRAG: ALLE OK\n");
    } else {
        std::printf("POSTEN-64 FEHLSCHLAG-VERTRAG: %d FEHLER\n", g_fail);
    }
    return g_fail == 0 ? 0 : 1;
}
