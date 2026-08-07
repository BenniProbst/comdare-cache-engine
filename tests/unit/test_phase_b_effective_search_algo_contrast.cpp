// A8-S5 PHASE B (2026-08-05) -- DER KONTRAST-BEWEIS der Treiber-Durchbindung.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4.
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt: multiple Allokatoren HINTER dem
// Allokator-Achsen-Interface"). Vorbild der Beweis-FORM: 02a-HERZ
// (tests/unit/test_s5_02a_layout_alloc_conformance.cpp:312-330).
//
// ===================================================================================================
// WAS HIER BEWIESEN WIRD -- UND WARUM EIN ">0" NICHTS BEWIESEN HAETTE
// ===================================================================================================
// Der ABI-Adapter ist der GENUS-ERST-INSTANZIIERUNGS-PUNKT: die einzige Stelle, an der aus einer
// Komposition ein konkretes Tier wird, und damit die einzige erlaubte Ausnahme der generalisierten
// Schnitt-Regel (Owner-KERN 04.08. abend-6). Sein Mess-Workload materialisierte an DREI Stellen
// (run_workload, run_workload_segmented, run_workload_segmented_v2) das Such-Organ mit
//
//     SearchAlgo algo;            // <- die FASSADE: am benannten Achsen-Default gebunden
//
// direkt NEBEN `Allocator alloc;` -- der T6-Wahl derselben Komposition. Der Organ-Algorithmus
// ignorierte also die Allokator-Strategie seiner eigenen Komposition: eine mimalloc-Komposition mass
// ihren Churn an mimalloc und die 256 Organ-Inserts am Achsen-Default. Seit Phase B steht dort
//
//     EffectiveSearchAlgo algo;   // <- search_algo_for_composition_t<SearchAlgo, Composition::allocator>
//
// DER PUNKT DIESES TESTS: die Aussage muss am ALT-Stand FALLEN. Ein "der Zaehler ist > 0" waere
// gruen gewesen, BEVOR die Zeile gedreht wurde -- der Treiber alloziert ja selbst ueber genau diese
// Strategie (Layout-Puffer + Churn). Die Aussage ist deshalb eine EXAKTE GLEICHUNG:
//
//     Zaehler nach run_workload  ==  TREIBER-EIGENANTEIL  +  ORGAN-ANTEIL
//
// Der TREIBER-EIGENANTEIL ist deterministisch und wird hier nicht geraten, sondern aus dem Quelltext
// abgeleitet (1 Layout-Puffer + 2 Churn-Runden a 2048 bei batches=1 -- Warmup + ein Mess-Batch).
// Der ORGAN-ANTEIL wird GEMESSEN, indem dieselben 256 Inserts auf einer direkt materialisierten
// EffectiveSearchAlgo-Instanz gefahren werden. Am ALT-Stand waere er 0 gewesen (die Inserts liefen
// unsichtbar ueber den Achsen-Default) -- die Gleichung braeche.
//
// UND DER GEGENPOL, der die Aussage erst zu einem KONTRAST macht: dieselbe Insert-Folge auf der
// FASSADE bewegt den Zaehler der Kompositions-Strategie um EXAKT 0. Die Fassade alloziert nicht
// weniger, sie alloziert WOANDERS -- genau das ist die stille zweite Strategie, um die es geht.
//
// GEPRUEFTE EBENEN:
//   (K-1) TYP-EBENE       -- Level 0 (Achsen-Default -> Fassade SELBST) und Level 1 (fremde Strategie
//                            -> Rebound-Leaf), plus die Ebenen-Wache (die Fassade traegt NIE den
//                            Rebound-Ausweis) und die name()-Invarianz (T6 leckt nicht in binary_id).
//   (K-2) HERZ-KONTRAST   -- run_workload: exakte Gleichung + Fassaden-Gegenpol (Zaehler-Bewegung 0).
//   (K-3) SEGMENT-KOHAERENZ-- dieselbe Aussage ueber run_workload_segmented_v2 (die dritte Treiber-
//                            Stelle) und run_workload_segmented (die zweite): alle DREI abgedeckt.
//
// COMDARE_MEASUREMENT_ON=1 ist ZWINGEND: die drei Treiber liegen komplett unter diesem Schalter
// (abi_adapter.hpp, Mess-Block) -- bei Messung-AUS gibt es sie gar nicht. Standalone (plain main,
// kein gtest), konsistent mit den anderen contract-Stufen.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <axes/alloc/axis_06_allocator_subaxes_aa1_to_aa7.hpp>
#include <axes/alloc/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>
#include <compositions/art_reference.hpp>
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <new>
#include <string_view>
#include <type_traits>

#include <axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace acpts  = ::comdare::cache_engine::alloc::concepts;
namespace alloc_ = ::comdare::cache_engine::alloc;
namespace an     = ::comdare::cache_engine::anatomy;
namespace comp   = ::comdare::cache_engine::compositions;
namespace lk     = ::comdare::cache_engine::lookup;
namespace lkc    = ::comdare::cache_engine::lookup::composable;
namespace topics = ::comdare::cache_engine::topics;

namespace {

int  g_fail = 0;
void pruefe(char const* was, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", was);
    if (!ok) ++g_fail;
}

// ===================================================================================================
// DIE ZAEHL-STRATEGIE -- eine ECHTE Achsen-Variante, kein Fremdkoerper.
// ===================================================================================================
// Sie ist nach dem Muster der bestehenden Test-Strategie ErschoepfteStubStrategie gebaut
// (tests/unit/test_h64_allocator_adapter_failure_contract.cpp:70-136) -- inklusive der drei
// EIGENEN Statistik-Member, die die h81-Lehre verlangt: `statistics`/`reset`/`restore_statistics`
// sind CRTP-WEITERLEITER der Basis; eine Strategie ohne eigene Member faende ueber derived() nur
// den geerbten Basis-Member wieder = unbeschraenkte Selbst-Rekursion (LEDGER 05.08. nacht-3).
//
// DER EINE UNTERSCHIED zu allen ausgelieferten Strategien -- und der Grund, warum dieser Test eine
// eigene braucht: der Zaehler ist TYP-GLOBAL, nicht instanz-lokal. Der Treiber im ABI-Adapter haelt
// `Allocator alloc;` und `EffectiveSearchAlgo algo;` als ZWEI getrennte Objekte mit je eigener
// Strategie-Instanz; ein instanz-lokaler Zaehler waere von aussen fuer das Organ gar nicht lesbar.
// Der typ-globale Zaehler summiert beide -- und genau ihre SUMME ist die Aussage: der Speicher des
// Organs kommt aus derselben Strategie wie der des Treibers.
class ZaehlStrategie : public alloc_::AllocatorStrategyBase<ZaehlStrategie> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = alloc_::subaxes::synchronization_tag;
    using family_id  = std::integral_constant<int, 0>; // 0 = KEINE Vendor-Familie: reine Test-Strategie

    static constexpr std::string_view algo_version = "1.0.0";

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "test_phase_b_zaehlend"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Test-Strategie: echte Belegung ueber ::operator new + TYP-GLOBALER Zaehler (Phase-B-Kontrast)";
    }

    [[nodiscard]] static constexpr bool                     has_native_aligned_alloc() noexcept { return true; }
    [[nodiscard]] static constexpr bool                     requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr bool                     requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr acpts::ProgressGuarantee progress_guarantee() noexcept {
        return acpts::ProgressGuarantee::WaitFree;
    }

    [[nodiscard]] bool operator==(ZaehlStrategie const&) const noexcept { return true; }

    /// DER TYP-GLOBALE ZAEHLER. Er zaehlt ALLOKATIONS-AUFRUFE (nicht Bytes): die Treiber-Arithmetik
    /// unten ist eine Aufruf-Arithmetik, und Aufrufe sind das, was deterministisch ist.
    static inline std::uint64_t g_allocs = 0;

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        ++g_allocs;
        if (bytes == 0) bytes = 1;
        if (alignment < alignof(std::max_align_t)) alignment = alignof(std::max_align_t);
        return ::operator new(bytes, std::align_val_t{alignment}, std::nothrow);
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        if (bytes == 0) bytes = 1;
        if (alignment < alignof(std::max_align_t)) alignment = alignof(std::max_align_t);
        ::operator delete(p, bytes, std::align_val_t{alignment});
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

// ===================================================================================================
// DIE KOMPOSITION -- Referenz-Basis, EIN Wert getauscht.
// ===================================================================================================
// 17 der 18 Achsen kommen unveraendert aus der ausgelieferten ArtComposition; getauscht wird NUR die
// Allokator-Achse (auf die Zaehl-Strategie) und der Such-Algorithmus (auf ein migriertes Organ, das
// beim Einfuegen real alloziert). Das ist genau die Struktur der generierten Permutations-Binaries.
template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct KontrastComposition {
    using search_algo        = SearchAlgoWrapper;
    using cache_traversal    = comp::ArtComposition::cache_traversal;
    using mapping            = comp::ArtComposition::mapping;
    using path_compression   = comp::ArtComposition::path_compression;
    using node_type          = comp::ArtComposition::node_type;
    using memory_layout      = comp::ArtComposition::memory_layout;
    using allocator          = ZaehlStrategie; // <-- die T6-Wahl DIESER Komposition
    using prefetch           = comp::ArtComposition::prefetch;
    using concurrency        = comp::ArtComposition::concurrency;
    using serialization      = comp::ArtComposition::serialization;
    using value_handle       = comp::ArtComposition::value_handle;
    using index_organization = comp::ArtComposition::index_organization;
    using io_dispatch        = comp::ArtComposition::io_dispatch;
    using migration_policy   = comp::ArtComposition::migration_policy;
    using filter             = comp::ArtComposition::filter;
    using queuing_q1         = comp::ArtComposition::queuing_q1;
    using queuing_q2         = comp::ArtComposition::queuing_q2;
    using persistence_target = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;

    static constexpr std::string_view paper_id = "A8-S5 Phase B: Treiber-Durchbindungs-Kontrast";
    static constexpr std::string_view name     = "PhaseBKontrastComposition";
};

using Fassade  = lk::LinearScanSearchAlgo;
using KontrC   = KontrastComposition<Fassade>;
using KontrAna = an::SearchAlgorithmAnatomy<KontrC>;
using Effektiv = lkc::search_algo_for_composition_t<Fassade, ZaehlStrategie>;

// Ein ZWEITES migriertes Organ mit ANDEREM Allokations-Profil. Es traegt den Differenz-Beweis fuer
// die dritte Treiber-Stelle (s. K-3): run_workload_segmented_v2 treibt ALLE 18 Achsen, dort ist der
// Fremd-Anteil anderer Organe im selben Zaehler. Die DIFFERENZ zweier Kompositionen, die sich NUR
// im Such-Organ unterscheiden, kuerzt diesen Fremd-Anteil exakt heraus -- und ist am Alt-Stand
// zwangslaeufig 0, weil dort BEIDE Organe unsichtbar am Achsen-Default liefen.
using Fassade2  = lk::VectorU16U16SearchAlgo;
using KontrC2   = KontrastComposition<Fassade2>;
using KontrAna2 = an::SearchAlgorithmAnatomy<KontrC2>;
using Effektiv2 = lkc::search_algo_for_composition_t<Fassade2, ZaehlStrategie>;
static_assert(!std::is_same_v<Fassade, Fassade2>);
static_assert(std::is_same_v<typename Effektiv2::allocator_type, ZaehlStrategie>);

// ===================================================================================================
// (K-1) TYP-EBENE -- compile-hart, vor jedem Laufzeit-Wort.
// ===================================================================================================
static_assert(acpts::AllocatorStrategy<ZaehlStrategie>,
              "Phase-B-Kontrast: die Zaehl-Strategie ist keine gueltige Achsen-Variante -- dann probt sie "
              "nicht die Achse, sondern an ihr vorbei.");
static_assert(acpts::CacheEnginePermutationStrategy<ZaehlStrategie>);
static_assert(topics::OrganAxisConcept<ZaehlStrategie>);
static_assert(!std::is_same_v<ZaehlStrategie, alloc_::ExgenAllocator>,
              "Phase-B-Kontrast VAKUOOS: die Zaehl-Strategie IST der Achsen-Default -- dann liefe der Test "
              "auf Level 0 und der Kontrast verglichen zwei Mal dasselbe.");

// Level 0: am eigenen Achsen-Default liefert die Naht die Fassade SELBST (der golden-Pfad).
static_assert(std::is_same_v<lkc::search_algo_for_composition_t<Fassade, typename Fassade::allocator_type>, Fassade>,
              "PHASE B Level-0: die Naht liefert am Achsen-Default nicht die Fassade selbst -- damit laege ein "
              "anderer Typ (anderer Symbolname) auf dem golden-Pfad.");
// Level 1: bei fremder Strategie der Rebound-Leaf -- und der traegt sie auch wirklich.
static_assert(!std::is_same_v<Effektiv, Fassade>,
              "PHASE B GATE TOT: die Naht liefert auch bei FREMDER Strategie die Fassade -- dann materialisiert "
              "der Treiber weiter das Organ am Achsen-Default und dieser ganze Test pinnt nichts.");
static_assert(std::is_same_v<typename Effektiv::allocator_type, ZaehlStrategie>,
              "PHASE B DURCHBINDUNG VERFEHLT: der effektive Organ-Typ traegt nicht die T6-Wahl der Komposition.");
// Ebenen-Trennung: die Identitaets-Ebene traegt NIE den Rebound-Ausweis (sonst drehten Emitter-Typnamen).
static_assert(!lkc::IsReboundSearchAlgoLeaf<Fassade>);
static_assert(lkc::IsReboundSearchAlgoLeaf<Effektiv>);
// Und der serialize-/binary_id-Schluessel bleibt allokator-invariant.
static_assert(lkc::search_algo_name_is_allocator_invariant_v<Fassade, ZaehlStrategie>,
              "PHASE B name()-INVARIANZ verletzt: die T6-Wahl leckt in den serialize-Schluessel -- dieselbe "
              "Komposition serialisierte dann je nach Allokator unter einem anderen Namen.");
static_assert(Fassade::family_id::value == Effektiv::family_id::value);
static_assert(Fassade::algo_version == Effektiv::algo_version);

// ===================================================================================================
// DIE TREIBER-ARITHMETIK -- aus dem Quelltext abgeleitet, nicht geraten.
// ===================================================================================================
// abi_adapter.hpp, run_workload: EIN Layout-Puffer ueber `alloc.allocate(kLbufBytes, kLineBytes)` --
// seit B14-NB2/NB3 kommt BEIDES (Groesse wie Alignment) aus der cacheline-Unterachse, nicht mehr aus dem
// Literal 64; am Achsen-Default sind die Zahlen unveraendert (kLineBytes == 64). Die Allokation selbst laeuft
// seit B14-NB3 ueber detail::ScanBufferGuard (RAII) -- an der ZAHL der Allocate-Aufrufe aendert das nichts,
// und nur die zaehlt die Arithmetik unten. Danach
// `do_batch` je einmal als Warmup und je Mess-Batch; jeder Batch fuehrt kChurn=2048 `alloc.allocate`-
// Aufrufe (Segment 2). Bei batches=1 sind das also 1 + 2*2048 Aufrufe -- der EIGENANTEIL des
// Treibers, der schon vor Phase B ueber die Kompositions-Strategie lief. Genau er ist der Grund,
// warum ein ">0"-Test hier nichts beweist.
constexpr std::uint64_t kChurn        = 2048;
constexpr std::uint64_t kTreiberEigen = 1u + 2u * kChurn;

/// Der ORGAN-ANTEIL, gemessen statt behauptet: dieselben 256 Inserts wie im Treiber-Setup, auf einer
/// direkt materialisierten Instanz des effektiven Organ-Typs.
template <class Organ>
[[nodiscard]] std::uint64_t organ_anteil_von() {
    using K                    = typename Organ::key_type;
    std::uint64_t const vorher = ZaehlStrategie::g_allocs;
    {
        Organ o;
        for (int k = 0; k < 256; ++k) { o.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u); }
    }
    return ZaehlStrategie::g_allocs - vorher;
}

} // namespace

int main() {
    std::printf("== A8-S5 PHASE B -- KONTRAST-BEWEIS der Treiber-Durchbindung ==\n");
    std::printf("   Fassade   : %.*s (gebunden an den Achsen-Default)\n", static_cast<int>(Fassade::name().size()),
                Fassade::name().data());
    std::printf("   Effektiv  : derselbe Name (%.*s), gebunden an %.*s\n", static_cast<int>(Effektiv::name().size()),
                Effektiv::name().data(), static_cast<int>(ZaehlStrategie::name().size()),
                ZaehlStrategie::name().data());

    std::printf("\n== (K-1) TYP-EBENE (compile-hart oben; hier nur der Vollzugs-Vermerk) ==\n");
    pruefe("Level 0 liefert die Fassade selbst, Level 1 den Rebound-Leaf mit der T6-Wahl", true);

    std::printf("\n== (K-2) HERZ-KONTRAST am Mess-Pfad: run_workload ==\n");
    // (a) Der ORGAN-ANTEIL, gemessen. Und der GEGENPOL: dieselbe Folge auf der FASSADE.
    std::uint64_t const organ_anteil    = organ_anteil_von<Effektiv>();
    std::uint64_t const fassaden_anteil = organ_anteil_von<Fassade>();
    std::printf("   Organ-Anteil (256 Inserts auf EffectiveSearchAlgo) : %llu Allokationen\n",
                static_cast<unsigned long long>(organ_anteil));
    std::printf("   Fassaden-Anteil (dieselbe Folge auf der Fassade)   : %llu Allokationen\n",
                static_cast<unsigned long long>(fassaden_anteil));
    pruefe("das effektive Organ alloziert REAL ueber die Kompositions-Strategie (Organ-Anteil > 0)", organ_anteil > 0);
    pruefe("DER KONTRAST: dieselbe Folge auf der FASSADE bewegt den Zaehler der Kompositions-Strategie um "
           "EXAKT 0 (sie alloziert nicht weniger -- sie alloziert WOANDERS)",
           fassaden_anteil == 0);

    // (b) Der Treiber. Nullpunkt frisch, damit die Gleichung eine Gleichung ist.
    an::SearchAlgorithmAbiAdapter<KontrAna> tier;
    std::int64_t                            latenzen[4] = {};
    ZaehlStrategie::g_allocs                            = 0;
    std::uint64_t const proben   = tier.run_workload(/*ops_per_batch=*/64, /*batches=*/1, /*seed=*/0xA8B5u, latenzen,
                                                     /*out_capacity=*/1);
    std::uint64_t const gemessen = ZaehlStrategie::g_allocs;
    std::printf("   run_workload lieferte %llu Probe(n); Zaehler-Delta = %llu\n",
                static_cast<unsigned long long>(proben), static_cast<unsigned long long>(gemessen));
    std::printf("   Erwartung  = Treiber-Eigenanteil %llu + Organ-Anteil %llu = %llu\n",
                static_cast<unsigned long long>(kTreiberEigen), static_cast<unsigned long long>(organ_anteil),
                static_cast<unsigned long long>(kTreiberEigen + organ_anteil));
    pruefe("run_workload lieferte die erwartete Probenzahl (1)", proben == 1u);
    pruefe("der Treiber-Eigenanteil allein reicht NICHT aus (Delta ECHT groesser) -- am ALT-Stand waren "
           "beide EXAKT gleich, genau dort faellt die Aussage",
           gemessen > kTreiberEigen);
    pruefe("EXAKTE GLEICHUNG: Delta == Treiber-Eigenanteil + Organ-Anteil (das Organ materialisiert im "
           "Treiber GENAU den effektiven Typ, nicht mehr und nicht weniger)",
           gemessen == kTreiberEigen + organ_anteil);

    std::printf("\n== (K-3) SEGMENT-KOHAERENZ: dieselbe Aussage an den beiden anderen Treiber-Stellen ==\n");
    {
        an::ComdareSegmentLatencyV1 seg1{};
        ZaehlStrategie::g_allocs = 0;
        std::uint64_t const n    = tier.run_workload_segmented(64, 1, 0xA8B5u, &seg1);
        std::uint64_t const d    = ZaehlStrategie::g_allocs;
        std::printf("   run_workload_segmented    : n=%llu, Delta=%llu (Erwartung %llu)\n",
                    static_cast<unsigned long long>(n), static_cast<unsigned long long>(d),
                    static_cast<unsigned long long>(kTreiberEigen + organ_anteil));
        pruefe("run_workload_segmented: Delta == Treiber-Eigenanteil + Organ-Anteil",
               d == kTreiberEigen + organ_anteil);
        pruefe("run_workload_segmented: Treiber-Eigenanteil allein reicht NICHT (Alt-Stand-Falle)", d > kTreiberEigen);
    }
    {
        // V2 treibt ALLE 18 Achsen -- neben dem Such-Organ allozieren dort weitere Organe ueber
        // DIESELBE Strategie. Eine Gleichung "Treiber + Organ" waere hier eine Unter-Behauptung, ein
        // ">=" eine ZAHNLOSE (es waere am Alt-Stand ebenfalls gruen gewesen, weil der Fremd-Anteil
        // allein schon groesser ist). Deshalb der DIFFERENZ-BEWEIS: zwei Kompositionen, die sich
        // NUR im Such-Organ unterscheiden, kuerzen den Fremd-Anteil exakt heraus.
        an::SearchAlgorithmAbiAdapter<KontrAna2> tier2;
        std::uint64_t const                      organ_anteil2 = organ_anteil_von<Effektiv2>();
        an::ComdareSegmentLatencyV2              seg2{};
        ZaehlStrategie::g_allocs       = 0;
        std::uint64_t const         n1 = tier.run_workload_segmented_v2(64, 1, 0xA8B5u, &seg2);
        std::uint64_t const         d1 = ZaehlStrategie::g_allocs;
        an::ComdareSegmentLatencyV2 seg2b{};
        ZaehlStrategie::g_allocs = 0;
        std::uint64_t const n2   = tier2.run_workload_segmented_v2(64, 1, 0xA8B5u, &seg2b);
        std::uint64_t const d2   = ZaehlStrategie::g_allocs;
        std::printf("   run_workload_segmented_v2 : Organ A (%.*s) Delta=%llu | Organ B (%.*s) Delta=%llu\n",
                    static_cast<int>(Fassade::name().size()), Fassade::name().data(),
                    static_cast<unsigned long long>(d1), static_cast<int>(Fassade2::name().size()),
                    Fassade2::name().data(), static_cast<unsigned long long>(d2));
        std::printf("   Organ-Anteile direkt gemessen: A=%llu, B=%llu -> erwartete Delta-Differenz %lld\n",
                    static_cast<unsigned long long>(organ_anteil), static_cast<unsigned long long>(organ_anteil2),
                    static_cast<long long>(organ_anteil) - static_cast<long long>(organ_anteil2));
        pruefe("run_workload_segmented_v2 lieferte je eine Probe", n1 == 1u && n2 == 1u);
        pruefe("die beiden Organe haben ueberhaupt ein UNTERSCHIEDLICHES Allokations-Profil (sonst zeigte "
               "die Differenz nichts)",
               organ_anteil != organ_anteil2);
        pruefe("DIFFERENZ-BEWEIS an der dritten Treiber-Stelle: die Delta-Differenz der beiden Kompositionen "
               "ist EXAKT die Differenz ihrer Organ-Anteile -- am ALT-Stand waere sie 0 gewesen, weil dort "
               "BEIDE Organe unsichtbar am Achsen-Default liefen",
               static_cast<long long>(d1) - static_cast<long long>(d2) ==
                   static_cast<long long>(organ_anteil) - static_cast<long long>(organ_anteil2));
    }

    std::printf("\n%s -- %d Fehler\n", (g_fail == 0) ? "ALLES GRUEN" : "ROT", g_fail);
    return (g_fail == 0) ? 0 : 1;
}
