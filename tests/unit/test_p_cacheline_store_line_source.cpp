// test_p_cacheline_store_line_source -- P-CACHELINE-LITERAL (2026-08-04), Vor-Anker-Pflichtposten der
// generalisierten Schnitt-Regel (Owner-KERN 04.08.: ALLE Achsen-Eigenschaften NUR ueber die Achsen).
//
// TRAGENDE AUSSAGE (eine, nicht drei): die Cache-Line-Groesse des LayoutAwareChunkedStore ist KEIN
// Eigenwert des Stores mehr, sondern der Wert der cacheline-Unterachse des Layout-Organs. Der Store darf
// nicht mehr still von der Achse auseinanderlaufen -- weder heute (Default B64 == 64) noch nach der
// KF-6-Durchbindung (Posten 62), die die per-Organ-Config als NTTP emittiert.
//
// Der Guard pinnt deshalb DREI Ebenen:
//   (A) QUELLE: axes/cacheline/cacheline_line_bytes.hpp loest die Line-Groesse eines Organs auf; der
//       Achsen-Default ist 64. Abgrenzung zum INTRINSISCHEN Deskriptor cache_line_size() (aos_strict = 1,
//       packed_bitmap = 8) -- wer den verwechselt, baut den Duplikat-Bug aus
//       axis_05_memory_layout_cache_line_aligned.hpp:54 nach.
//   (B) NEUTRALITAET: am Achsen-Default ist die Geometrie ALLER 5 Repraesentationen byte-identisch zum
//       Stand vor der Substitution (Zahlen literal, aus der Vorher-Sonde uebernommen).
//   (C) ANTI-ZEMENTIERUNG (Lehre "gruene Tests zementieren alte Ordnung"): mit einer NICHT-Default-Config
//       folgt der Store der Achse -- Record-Stride, Chunk-Bytes, Chunk-Alignment, FF2-Knotenbreite UND die
//       CLU-Linienzaehlung. Genau diese Saetze BEISSEN am Alt-Stand (dort 64 hartkodiert): dieselben Daten
//       ergaeben dieselbe Linienzahl, obwohl die Achse permutiert -- der mess-verfaelschende Zustand.
//
// Die drei Testlayout-Familien unterscheiden sich UNTEREINANDER NUR in der Unterachse (gleicher CRTP-Kopf,
// gleicher intrinsischer Deskriptor 64, gleiche Repraesentation) -- jede Divergenz unten kommt also aus der
// cacheline-Unterachse und aus nichts sonst.

#include <axes/cacheline/cacheline_config.hpp>
#include <axes/cacheline/cacheline_line_bytes.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_strategy_base.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aos_strict.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aosoa.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_packed_bitmap.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_soa.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_strategy_base.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace cl = comdare::cache_engine::cacheline;
namespace nd = comdare::cache_engine::node;
namespace ml = comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace al = comdare::cache_engine::allocator::axis_06_allocator;

static int g_fail = 0;
template <typename A, typename B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

// -- (A) QUELLE: der Achsen-Default und die Aufloesungs-Ordnung -------------------------------------------
static_assert(cl::kDefaultLineBytes == 64);
static_assert(static_cast<std::size_t>(cl::CacheLineConfig{}.line_size) == cl::kDefaultLineBytes);
// Alle 5 heutigen Layout-Strategien tragen die Unterachse und stehen am Default.
static_assert(cl::line_bytes_of<ml::AoSStrictMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::CacheLineAlignedMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::SoAMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::AoSoAMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::PackedBitmapMemoryLayout>() == 64);
// ABGRENZUNG: der intrinsische Deskriptor ist NICHT die Unterachse (sonst waere der Store bei aos_strict
// auf 1-Byte-"Linien" und bei packed_bitmap auf 8 gelaufen -- der Duplikat-Bug).
static_assert(ml::AoSStrictMemoryLayout::cache_line_size() == 1);
static_assert(ml::PackedBitmapMemoryLayout::cache_line_size() == 8);
static_assert(cl::line_bytes_of<ml::AoSStrictMemoryLayout>() != ml::AoSStrictMemoryLayout::cache_line_size());
static_assert(cl::line_bytes_of<ml::PackedBitmapMemoryLayout>() != ml::PackedBitmapMemoryLayout::cache_line_size());

// -- (B) NEUTRALITAET: die 5 Repraesentationen am Default, Zahlen aus der Vorher-Sonde --------------------
template <class L>
using Store4 = nd::LayoutAwareChunkedStore<nd::Node4NodeType, L, al::MimallocAllocator>;

static_assert(Store4<ml::AoSStrictMemoryLayout>::cacheline_line_bytes() == 64);
static_assert(Store4<ml::AoSStrictMemoryLayout>::chunk_align() == 64);
static_assert(Store4<ml::AoSStrictMemoryLayout>::record_phys_bytes() == 16);
static_assert(Store4<ml::AoSStrictMemoryLayout>::chunk_bytes() == 64);
static_assert(Store4<ml::CacheLineAlignedMemoryLayout>::record_phys_bytes() == 64);
static_assert(Store4<ml::CacheLineAlignedMemoryLayout>::chunk_bytes() == 256);
static_assert(Store4<ml::SoAMemoryLayout>::record_phys_bytes() == 16);
static_assert(Store4<ml::SoAMemoryLayout>::chunk_bytes() == 64);
static_assert(Store4<ml::AoSoAMemoryLayout>::record_phys_bytes() == 16);
static_assert(Store4<ml::AoSoAMemoryLayout>::chunk_bytes() == 64);
static_assert(Store4<ml::PackedBitmapMemoryLayout>::record_phys_bytes() == 16);
static_assert(Store4<ml::PackedBitmapMemoryLayout>::chunk_bytes() == 64);
static_assert(Store4<ml::AoSStrictMemoryLayout>::node_capacity() == 4);
static_assert(Store4<ml::CacheLineAlignedMemoryLayout>::node_capacity() == 4);

// -- (C) ANTI-ZEMENTIERUNG: Testorgane, die sich NUR in der Unterachse unterscheiden ----------------------
// So (und nur so) wuerde der KF-6-Codegen eine line_size-permutierte Layout-Variante emittieren: identischer
// CRTP-Kopf, identischer intrinsischer Deskriptor (bewusst 64 belassen!), identische Repraesentation.
template <cl::CacheLineSize S>
struct ProbeSoaLayout : ml::MemoryLayoutStrategyBase<ProbeSoaLayout<S>, cl::CacheLineConfig{S}> {
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    [[nodiscard]] static constexpr std::size_t            cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view       name() noexcept { return "probe_soa_line_variant"; }
    [[nodiscard]] static constexpr ml::RepresentationKind representation_kind() noexcept {
        return ml::RepresentationKind::soa_split_columns;
    }
};
template <cl::CacheLineSize S>
struct ProbePaddedLayout : ml::MemoryLayoutStrategyBase<ProbePaddedLayout<S>, cl::CacheLineConfig{S}> {
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    [[nodiscard]] static constexpr std::size_t            cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view       name() noexcept { return "probe_padded_line_variant"; }
    [[nodiscard]] static constexpr ml::RepresentationKind representation_kind() noexcept {
        return ml::RepresentationKind::aos_interleaved_padded;
    }
};
// Knoten mit 32 Slots: erst damit spannt der Key-Scan mehr als eine Linie auf -> CLU wird diskriminierend.
struct ProbeNode32 : nd::NodeTypeStrategyBase<ProbeNode32> {
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 32; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "probe_node32"; }
};
// FF2-Kreuzprobe: Knoten mit deklarierter Breite 1 Cache-Line -> Chunk == 1 * Line-Groesse der Achse.
struct ProbeNodeW1
    : nd::NodeTypeStrategyBase<ProbeNodeW1, cl::CacheLineConfig{}, cl::NodeWidthConfig{cl::NodeWidthInLines::W1}> {
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 4; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "probe_node_w1"; }
};

template <cl::CacheLineSize S>
using SoaStore32 = nd::LayoutAwareChunkedStore<ProbeNode32, ProbeSoaLayout<S>, al::MimallocAllocator>;
template <cl::CacheLineSize S>
using PaddedStore32 = nd::LayoutAwareChunkedStore<ProbeNode32, ProbePaddedLayout<S>, al::MimallocAllocator>;
template <cl::CacheLineSize S>
using PaddedStoreW1 = nd::LayoutAwareChunkedStore<ProbeNodeW1, ProbePaddedLayout<S>, al::MimallocAllocator>;

// C-1: die Line-Groesse des Stores IST die der Achse (nicht 64, nicht cache_line_size()).
static_assert(SoaStore32<cl::CacheLineSize::B32>::cacheline_line_bytes() == 32);
static_assert(SoaStore32<cl::CacheLineSize::B64>::cacheline_line_bytes() == 64);
static_assert(SoaStore32<cl::CacheLineSize::B128>::cacheline_line_bytes() == 128);
static_assert(SoaStore32<cl::CacheLineSize::B256>::cacheline_line_bytes() == 256);
// C-2: das Chunk-Alignment folgt derselben Quelle (Alt-Stand: immer 64).
static_assert(SoaStore32<cl::CacheLineSize::B128>::chunk_align() == 128);
static_assert(SoaStore32<cl::CacheLineSize::B32>::chunk_align() == 32);
// C-3: das AoS-Padding (round_up(16, line)) folgt der Achse (Alt-Stand: immer 64).
static_assert(PaddedStore32<cl::CacheLineSize::B64>::record_phys_bytes() == 64);
static_assert(PaddedStore32<cl::CacheLineSize::B128>::record_phys_bytes() == 128);
static_assert(PaddedStore32<cl::CacheLineSize::B256>::record_phys_bytes() == 256);
static_assert(PaddedStore32<cl::CacheLineSize::B32>::record_phys_bytes() == 32);
static_assert(PaddedStore32<cl::CacheLineSize::B128>::chunk_bytes() == 32u * 128u);
// C-4: die FF2-Knotenbreite ist W * Line-Groesse DER ACHSE (Alt-Stand: W * 64).
static_assert(PaddedStoreW1<cl::CacheLineSize::B64>::chunk_bytes() == 64);
static_assert(PaddedStoreW1<cl::CacheLineSize::B128>::chunk_bytes() == 128);
static_assert(PaddedStoreW1<cl::CacheLineSize::B256>::chunk_bytes() == 256);
static_assert(PaddedStoreW1<cl::CacheLineSize::B32>::chunk_bytes() == 32);
// Kapazitaet bleibt dabei 1 Record je Knoten (Record-Stride waechst mit der Linie) -- Aussage: die Breite
// ist ECHT in Linien gemessen, nicht in Bytes umgedeutet.
static_assert(PaddedStoreW1<cl::CacheLineSize::B64>::node_capacity() == 1);
static_assert(PaddedStoreW1<cl::CacheLineSize::B128>::node_capacity() == 1);

// -- CLU-Sonde: der Observer-Pfad, ueber den die Linienzahl in die Messung geht ---------------------------
struct FootprintProbe {
    std::uint64_t checksum  = 0;
    std::uint64_t key_bytes = 0;
    std::uint64_t lines     = 0;
    std::size_t   count     = 0;
    std::uint64_t observe_real_footprint(std::uint64_t cs, std::size_t n, std::uint64_t kb, std::uint64_t ln) {
        checksum  = cs;
        count     = n;
        key_bytes = kb;
        lines     = ln;
        return ln;
    }
};

template <class Store>
static Store fill(std::size_t n) {
    Store s;
    for (std::size_t i = 0; i < n; ++i) s.append_slot(0xD00D0000ull + i, 11ull * i + 3ull);
    return s;
}
template <class Store>
static bool roundtrip_ok(Store const& s, std::size_t n) {
    if (s.slot_count() != n) return false;
    for (std::size_t i = 0; i < n; ++i)
        if (s.key_at(i) != 0xD00D0000ull + i || s.value_at(i) != 11ull * i + 3ull) return false;
    return true;
}
template <class Store>
static std::uint64_t clu_lines(Store const& s) {
    FootprintProbe p;
    return s.organ_observe_layout(p);
}

int main() {
    std::cout << "P-CACHELINE-LITERAL: der Store bezieht die Cache-Line-Groesse aus der cacheline-Achse\n";

    // -- (A) Quelle, zur Laufzeit sichtbar --
    check_eq("Achsen-Default (kDefaultLineBytes)", cl::kDefaultLineBytes, std::size_t{64});
    check_eq("Store(CLA) line_bytes == Achse(CLA)", Store4<ml::CacheLineAlignedMemoryLayout>::cacheline_line_bytes(),
             ml::CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes());
    check_true("Store-Line-Groesse ist NICHT der intrinsische Deskriptor (aos_strict = 1)",
               Store4<ml::AoSStrictMemoryLayout>::cacheline_line_bytes() !=
                   ml::AoSStrictMemoryLayout::cache_line_size());

    // -- (B) Neutralitaet am Default: die Geometrie der 5 Repraesentationen --
    check_eq("aos_strict   record_phys_bytes", Store4<ml::AoSStrictMemoryLayout>::record_phys_bytes(), std::size_t{16});
    check_eq("cla_padded   record_phys_bytes", Store4<ml::CacheLineAlignedMemoryLayout>::record_phys_bytes(),
             std::size_t{64});
    check_eq("soa          record_phys_bytes", Store4<ml::SoAMemoryLayout>::record_phys_bytes(), std::size_t{16});
    check_eq("aosoa        record_phys_bytes", Store4<ml::AoSoAMemoryLayout>::record_phys_bytes(), std::size_t{16});
    check_eq("packed_bitmap record_phys_bytes", Store4<ml::PackedBitmapMemoryLayout>::record_phys_bytes(),
             std::size_t{16});
    check_eq("cla_padded   chunk_bytes", Store4<ml::CacheLineAlignedMemoryLayout>::chunk_bytes(), std::size_t{256});
    check_eq("cla_padded   chunk_align", Store4<ml::CacheLineAlignedMemoryLayout>::chunk_align(), std::size_t{64});
    {
        auto const s = fill<Store4<ml::CacheLineAlignedMemoryLayout>>(24);
        check_true("Round-Trip verlustfrei (Default, cla_padded)", roundtrip_ok(s, 24));
        check_eq("Default: chunk_capacity_bytes", s.chunk_capacity_bytes(), std::size_t{256});
        check_true("Default: Chunk-Adresse am Chunk-Alignment ausgerichtet",
                   reinterpret_cast<std::uintptr_t>(s.backing_begin()) %
                           Store4<ml::CacheLineAlignedMemoryLayout>::chunk_align() ==
                       0u);
    }

    // -- (C) Anti-Zementierung: dieselben Daten, NUR die Unterachse permutiert --
    constexpr std::size_t kN   = 32;
    auto const            b32  = fill<SoaStore32<cl::CacheLineSize::B32>>(kN);
    auto const            b64  = fill<SoaStore32<cl::CacheLineSize::B64>>(kN);
    auto const            b128 = fill<SoaStore32<cl::CacheLineSize::B128>>(kN);
    auto const            b256 = fill<SoaStore32<cl::CacheLineSize::B256>>(kN);
    check_true("Round-Trip verlustfrei (B32)", roundtrip_ok(b32, kN));
    check_true("Round-Trip verlustfrei (B64)", roundtrip_ok(b64, kN));
    check_true("Round-Trip verlustfrei (B128)", roundtrip_ok(b128, kN));
    check_true("Round-Trip verlustfrei (B256)", roundtrip_ok(b256, kN));
    // 32 Keys a 8 B = 256 B Key-Spalte -> beruehrte Linien = 256 / line_size. Am Alt-Stand waeren ALLE VIER
    // Werte 4 gewesen (hartkodierte 64) -- genau das ist die mess-verfaelschende Blindheit.
    check_eq("CLU-Linien B32  (256 B Key-Spalte / 32)", clu_lines(b32), std::uint64_t{8});
    check_eq("CLU-Linien B64  (256 B Key-Spalte / 64)", clu_lines(b64), std::uint64_t{4});
    check_eq("CLU-Linien B128 (256 B Key-Spalte / 128)", clu_lines(b128), std::uint64_t{2});
    check_eq("CLU-Linien B256 (256 B Key-Spalte / 256)", clu_lines(b256), std::uint64_t{1});
    check_true("CLU ist ueber die Unterachse ECHT diskriminierend (4 distinkte Werte)",
               clu_lines(b32) != clu_lines(b64) && clu_lines(b64) != clu_lines(b128) &&
                   clu_lines(b128) != clu_lines(b256));

    auto const p64  = fill<PaddedStore32<cl::CacheLineSize::B64>>(8);
    auto const p128 = fill<PaddedStore32<cl::CacheLineSize::B128>>(8);
    check_eq("AoS-Padding B64:  chunk_capacity_bytes", p64.chunk_capacity_bytes(), std::size_t{32u * 64u});
    check_eq("AoS-Padding B128: chunk_capacity_bytes", p128.chunk_capacity_bytes(), std::size_t{32u * 128u});
    check_true("Round-Trip verlustfrei (AoS-Padding B128)", roundtrip_ok(p128, 8));
    check_true("B128-Chunk-Adresse am 128-B-Alignment der Achse ausgerichtet",
               reinterpret_cast<std::uintptr_t>(p128.backing_begin()) %
                       PaddedStore32<cl::CacheLineSize::B128>::chunk_align() ==
                   0u);

    auto const w1_64  = fill<PaddedStoreW1<cl::CacheLineSize::B64>>(4);
    auto const w1_128 = fill<PaddedStoreW1<cl::CacheLineSize::B128>>(4);
    check_eq("FF2 W1 x B64:  chunk_capacity_bytes", w1_64.chunk_capacity_bytes(), std::size_t{64});
    check_eq("FF2 W1 x B128: chunk_capacity_bytes", w1_128.chunk_capacity_bytes(), std::size_t{128});
    check_true("Round-Trip verlustfrei (FF2 W1 x B128)", roundtrip_ok(w1_128, 4));

    std::cout << "\n==== P-CACHELINE-LITERAL Wache: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
