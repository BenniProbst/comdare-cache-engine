// test_b14_abi_adapter_line_subaxis_paths -- B14-NB3, Codex-MITTEL (C-ii) + (C-iii).
//
// BEFUND (C-ii): fuer die Line-Groessen 32/128/256 gab es KEINEN Compile- und KEINEN Laufzeittest ueber die
// drei Mess-Pfade des abi_adapter (run_workload / run_workload_segmented / run_workload_segmented_v2).
// Die B14-NB2-Ableitung des Scan-Puffers und die B14-NB3-Ableitung des Scan-Strides waren damit nur am
// Achsen-Default (B64) je gelaufen -- also genau dort, wo sie nichts aendern. Diese TU instanziiert und
// FAEHRT alle drei Pfade mit einer line-permutierten Komposition.
//
// BEFUND (C-iii): nach erfolgreicher Allokation des Layout-Scan-Puffers fuehrte jede spaetere Exception in
// das gemeinsame `catch (...) { return 0; }` -- OHNE Freigabe. Der Puffer (am Default 1 MiB, bei B256
// 4 MiB) war verloren. Kein Gedankenspiel: PoolResourceAllocator::allocate ist im Bestand als
// "[[allocation-failure-exception]] pmr allocate wirft std::bad_alloc bei OOM" deklariert und wird in
// allen drei Pfaden im Churn-Segment getrieben. Die TU erzwingt genau dieses Fenster mit einem
// Sonden-Allokator und prueft die BILANZ des Puffers, nicht die Absicht.
//
// Was die TU NICHT behauptet: sie ersetzt keine ASAN-Fahrt. Sie prueft, was ein Allokator-Ledger sehen
// kann -- Groesse, Ausrichtung und Rueckgabe jedes Blocks.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/cacheline/cacheline_config.hpp>
#include <axes/cacheline/cacheline_line_bytes.hpp>
#include <axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>
#include <axes/layout/axis_05_memory_layout_strategy_base.hpp>
#include <axes/layout/axis_05_memory_layout_subaxes_hm1_to_hm4.hpp>
#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>

namespace an  = ::comdare::cache_engine::anatomy;
namespace cl  = ::comdare::cache_engine::cacheline;
namespace ml  = ::comdare::cache_engine::layout;
namespace al  = ::comdare::cache_engine::alloc;
namespace cmp = ::comdare::cache_engine::compositions;

namespace {

// -- Sonden-Allokator: fuehrt Buch und kann auf Kommando genau EINMAL werfen -----------------------------
// Erbt die Kompositions-Achse (MimallocAllocator) und verdeckt nur allocate/deallocate -- der Adapter haelt
// `Allocator alloc;` mit statischem Typ, die Verdeckung greift also. Kein virtual, kein ABI-Eingriff.
struct SondenAllokator : ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator {
    using Basis = ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator;

    /// Buch je (bytes, alignment): +1 bei allocate, -1 bei deallocate. Am Ende MUSS jede Zeile 0 sein.
    static inline std::map<std::pair<std::size_t, std::size_t>, long> ledger{};
    /// Wurf-Ausloeser: sobald eine Allokation mit GENAU dieser Groesse gesehen wurde, wirft die
    /// NAECHSTE Allokation anderer Groesse einmalig. So liegt der Wurf garantiert HINTER dem
    /// Layout-Scan-Puffer -- also im Fenster, in dem der Puffer bisher verloren ging.
    static inline std::size_t wirf_nach_groesse = 0;
    static inline bool        marke_gesehen     = false;
    static inline long        wuerfe            = 0;

    static void zuruecksetzen() {
        ledger.clear();
        wirf_nach_groesse = 0;
        marke_gesehen     = false;
        wuerfe            = 0;
    }
    static long bilanz(std::size_t bytes, std::size_t alignment) {
        auto const it = ledger.find({bytes, alignment});
        return (it == ledger.end()) ? 0 : it->second;
    }
    static long lebende_bloecke() {
        long n = 0;
        for (auto const& [schluessel, wert] : ledger) {
            (void)schluessel;
            n += wert;
        }
        return n;
    }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        if (wirf_nach_groesse != 0) {
            if (bytes == wirf_nach_groesse) {
                marke_gesehen = true;
            } else if (marke_gesehen) {
                wirf_nach_groesse = 0;
                marke_gesehen     = false;
                ++wuerfe;
                throw std::bad_alloc{};
            }
        }
        void* p = Basis::allocate(bytes, alignment);
        if (p != nullptr) ++ledger[{bytes, alignment}];
        return p;
    }
    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p != nullptr) --ledger[{bytes, alignment}];
        Basis::deallocate(p, bytes, alignment);
    }
};

// -- Line-permutierte Layout-Variante: exakt die Form, die KF-6 emittieren wird --------------------------
// Identisch zu CacheLineAlignedMemoryLayout bis auf den NTTP der cacheline-Unterachse; der Scan-Rumpf ist
// DERSELBE (ml::detail::padded_aos_field_sum), damit die Aussage eine ueber die echte Quelle ist.
template <cl::CacheLineSize S>
struct ProbeClaLayout : ml::MemoryLayoutStrategyBase<ProbeClaLayout<S>, cl::CacheLineConfig{S}> {
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = ml::subaxes::alignment_strategy_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool             enabled      = true;
    static constexpr std::string_view algo_version = "probe";

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "memory_layout_probe_cla_line"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "ProbeClaLayout"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PROBE_CLA_LINE"; }
    [[nodiscard]] static constexpr ml::RepresentationKind representation_kind() noexcept {
        return ml::RepresentationKind::aos_interleaved_padded;
    }
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return ml::detail::padded_aos_field_sum(buf, n, record_size, ProbeClaLayout::cacheline_subaxis_line_bytes());
    }
};

/// Die Komposition unterscheidet sich von ArtComposition in GENAU zwei Slots: der Layout-Line und dem
/// Sonden-Allokator. Alles andere ist der Bestand -- jede Abweichung unten kommt also aus der Unterachse.
template <cl::CacheLineSize S>
struct ProbeLineComposition : cmp::ArtComposition {
    using memory_layout = ml::ObservableMemoryLayout<ProbeClaLayout<S>>;
    using allocator     = SondenAllokator;

    static constexpr std::string_view name = "ProbeLineComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("ProbeLineComposition",
                                        "tests/unit/test_b14_abi_adapter_line_subaxis_paths.cpp");
};

template <cl::CacheLineSize S>
using Tier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<ProbeLineComposition<S>>>;

// -- Die Geometrie, die der Adapter fuer diese Komposition rechnet (dieselben Ausdruecke) ----------------
constexpr std::size_t kRecords    = 16384;
constexpr std::size_t kRecordSize = 48;

template <cl::CacheLineSize S>
constexpr std::size_t erwartete_line() {
    return an::detail::layout_scan_align_bytes<typename ProbeLineComposition<S>::memory_layout>();
}
template <cl::CacheLineSize S>
constexpr std::size_t erwarteter_stride() {
    return an::detail::layout_scan_stride_bytes<typename ProbeLineComposition<S>::memory_layout>(kRecordSize);
}
template <cl::CacheLineSize S>
constexpr std::size_t erwartete_puffer_bytes() {
    return kRecords * erwarteter_stride<S>();
}

// COMPILE-ZEIT-TEIL von (C-ii): die Geometrie folgt der Achse ueber ALLE VIER Belegungen. round_up(48,32)
// ist 64 -- B32 teilt den Stride mit B64 und unterscheidet sich NUR in der Ausrichtung. Genau das ist der
// dokumentierte Entscheid "ALLOC-ALIGNMENT BEI line=32" in abi_adapter.hpp: gewollt, nicht Nebenwirkung.
static_assert(erwartete_line<cl::CacheLineSize::B32>() == 32);
static_assert(erwartete_line<cl::CacheLineSize::B64>() == 64);
static_assert(erwartete_line<cl::CacheLineSize::B128>() == 128);
static_assert(erwartete_line<cl::CacheLineSize::B256>() == 256);
static_assert(erwarteter_stride<cl::CacheLineSize::B32>() == 64);
static_assert(erwarteter_stride<cl::CacheLineSize::B64>() == 64);
static_assert(erwarteter_stride<cl::CacheLineSize::B128>() == 128);
static_assert(erwarteter_stride<cl::CacheLineSize::B256>() == 256);
static_assert(erwartete_puffer_bytes<cl::CacheLineSize::B64>() == 1048576);   // der Bestandswert
static_assert(erwartete_puffer_bytes<cl::CacheLineSize::B256>() == 4194304);  // 4x -- die Ableitung ist scharf
// Die Puffer-Untergrenze, die jeder Pfad als static_assert traegt, hier explizit fuer alle vier Belegungen:
static_assert(erwartete_puffer_bytes<cl::CacheLineSize::B32>() >=
              (kRecords - 1u) * erwarteter_stride<cl::CacheLineSize::B32>() + sizeof(std::uint64_t));
static_assert(erwartete_puffer_bytes<cl::CacheLineSize::B256>() >=
              (kRecords - 1u) * erwarteter_stride<cl::CacheLineSize::B256>() + sizeof(std::uint64_t));

constexpr std::uint64_t kOps     = 32;
constexpr std::uint64_t kBatches = 2;

// -- (C-ii) LAUFZEIT: jeder Pfad einmal je Line-Groesse --------------------------------------------------
template <cl::CacheLineSize S>
void pfad_run_workload() {
    SondenAllokator::zuruecksetzen();
    Tier<S>                     tier;
    std::array<std::int64_t, 8> lat{};
    std::uint64_t const         n = tier.run_workload(kOps, kBatches, 12345u, lat.data(), lat.size());
    EXPECT_EQ(n, kBatches) << "run_workload lieferte keine Samples bei line=" << erwartete_line<S>();
    EXPECT_EQ(SondenAllokator::bilanz(erwartete_puffer_bytes<S>(), erwartete_line<S>()), 0)
        << "Layout-Scan-Puffer nicht (oder falsch ausgerichtet) zurueckgegeben, line=" << erwartete_line<S>();
}

template <cl::CacheLineSize S>
void pfad_segmented() {
    SondenAllokator::zuruecksetzen();
    Tier<S>                     tier;
    an::ComdareSegmentLatencyV1 seg{};
    std::uint64_t const         n = tier.run_workload_segmented(kOps, kBatches, 12345u, &seg);
    EXPECT_EQ(n, kBatches);
    EXPECT_EQ(SondenAllokator::bilanz(erwartete_puffer_bytes<S>(), erwartete_line<S>()), 0);
}

template <cl::CacheLineSize S>
void pfad_segmented_v2() {
    SondenAllokator::zuruecksetzen();
    Tier<S>                     tier;
    an::ComdareSegmentLatencyV2 seg{};
    std::uint64_t const         n = tier.run_workload_segmented_v2(kOps, kBatches, 12345u, &seg);
    EXPECT_EQ(n, kBatches);
    EXPECT_EQ(SondenAllokator::bilanz(erwartete_puffer_bytes<S>(), erwartete_line<S>()), 0);
}

// -- (C-iii) LECK-WACHE: Wurf HINTER der Puffer-Allokation, Bilanz muss trotzdem aufgehen ----------------
// Der Ausloeser feuert bei der ERSTEN Allokation, die der Puffer-Allokation folgt. In allen drei Pfaden ist
// das die erste Churn- bzw. pf_store-Allokation -- also bevor irgendein anderer Block festgehalten wird.
// Damit misst die Wache genau den einen Block, um den es geht.
template <cl::CacheLineSize S, class Aufruf>
void leck_wache(char const* pfad, Aufruf&& aufruf) {
    SondenAllokator::zuruecksetzen();
    Tier<S> tier;
    SondenAllokator::wirf_nach_groesse = erwartete_puffer_bytes<S>();
    std::uint64_t const n              = aufruf(tier);
    SondenAllokator::wirf_nach_groesse = 0;

    ASSERT_EQ(SondenAllokator::wuerfe, 1)
        << pfad << ": der Ausloeser hat nicht gefeuert -- die Wache haette nichts geprueft (line="
        << erwartete_line<S>() << ")";
    EXPECT_EQ(n, 0u) << pfad << ": nach der Exception muss der Pfad ehrlich 0 Samples melden";
    EXPECT_EQ(SondenAllokator::bilanz(erwartete_puffer_bytes<S>(), erwartete_line<S>()), 0)
        << pfad << ": LECK -- der Layout-Scan-Puffer (" << erwartete_puffer_bytes<S>()
        << " B) wurde auf dem Exception-Pfad nicht zurueckgegeben";
}

} // namespace

// -- (C-ii) ---------------------------------------------------------------------------------------------
TEST(B14LineSubaxisPaths, RunWorkloadUeberAlleVierLineGroessen) {
    pfad_run_workload<cl::CacheLineSize::B32>();
    pfad_run_workload<cl::CacheLineSize::B64>();
    pfad_run_workload<cl::CacheLineSize::B128>();
    pfad_run_workload<cl::CacheLineSize::B256>();
}

TEST(B14LineSubaxisPaths, RunWorkloadSegmentedUeberAlleVierLineGroessen) {
    pfad_segmented<cl::CacheLineSize::B32>();
    pfad_segmented<cl::CacheLineSize::B64>();
    pfad_segmented<cl::CacheLineSize::B128>();
    pfad_segmented<cl::CacheLineSize::B256>();
}

TEST(B14LineSubaxisPaths, RunWorkloadSegmentedV2UeberAlleVierLineGroessen) {
    pfad_segmented_v2<cl::CacheLineSize::B32>();
    pfad_segmented_v2<cl::CacheLineSize::B64>();
    pfad_segmented_v2<cl::CacheLineSize::B128>();
    pfad_segmented_v2<cl::CacheLineSize::B256>();
}

// -- (C-iii) -- je Pfad einmal am Default und einmal am groessten Puffer (B256, 4 MiB) -------------------
TEST(B14LineSubaxisPaths, KeinLeckDesScanPuffersAufDemExceptionPfad) {
    auto rw = [](auto& tier) {
        std::array<std::int64_t, 8> lat{};
        return tier.run_workload(kOps, kBatches, 999u, lat.data(), lat.size());
    };
    auto sg = [](auto& tier) {
        an::ComdareSegmentLatencyV1 seg{};
        return tier.run_workload_segmented(kOps, kBatches, 999u, &seg);
    };
    auto v2 = [](auto& tier) {
        an::ComdareSegmentLatencyV2 seg{};
        return tier.run_workload_segmented_v2(kOps, kBatches, 999u, &seg);
    };

    leck_wache<cl::CacheLineSize::B64>("run_workload", rw);
    leck_wache<cl::CacheLineSize::B64>("run_workload_segmented", sg);
    leck_wache<cl::CacheLineSize::B64>("run_workload_segmented_v2", v2);

    leck_wache<cl::CacheLineSize::B256>("run_workload (B256)", rw);
    leck_wache<cl::CacheLineSize::B256>("run_workload_segmented (B256)", sg);
    leck_wache<cl::CacheLineSize::B256>("run_workload_segmented_v2 (B256)", v2);
}
