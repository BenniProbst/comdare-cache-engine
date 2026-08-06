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
// WAS DIESE TU BEWEIST (literal, nicht behauptet):
//   (1) KONFORMITAET DER ORAKEL   -- beide Stub-Strategien sind vollwertige Achsen-Varianten an
//                                    AllocatorStrategyBase, kein Fremdkoerper neben der Achse.
//   (2) F1 NEGATIV + POSITIV      -- pmr::vector ueber as_pmr_resource() wirft bad_alloc statt in
//                                    Nullspeicher zu bauen; Zero-Size unveraendert; reale Strategie laeuft.
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

// Die Stores der Wache: Node4 (Kapazitaet 4 -> mehrere Chunks bei kleiner Slot-Zahl) x CacheLineAligned.
using ErschoepfterStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, ErschoepfteStubStrategie>;
using BudgetStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, BudgetStubStrategie>;

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

        // Zero-Size bleibt unangetastet (die Zero-Size-Wachen der Organe bleiben gueltig).
        bool geworfen_0 = false;
        try {
            void* p = resource.allocate(0, alignof(std::max_align_t));
            std::printf("  [INFO] do_allocate(0) lieferte %s\n", p == nullptr ? "nullptr" : "einen Zeiger");
            resource.deallocate(p, 0, alignof(std::max_align_t));
        } catch (std::bad_alloc const&) { geworfen_0 = true; }
        check("bytes == 0 wirft NICHT (Zero-Size-Verhalten unveraendert)", !geworfen_0);

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
