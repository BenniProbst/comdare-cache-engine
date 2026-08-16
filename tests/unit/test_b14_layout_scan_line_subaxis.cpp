// test_b14_layout_scan_line_subaxis -- B14-NB3, SCAN-SEITE der KF-6-Auflage (D) + WRAPPER-AUDIT.
//
// SCHWESTER-WACHE zu test_p_cacheline_store_line_source: dort folgt der STORE der cacheline-Unterachse,
// hier der MESS-SCAN und der Puffer, den die drei abi_adapter-Pfade fuer ihn dimensionieren.
//
// TRAGENDE AUSSAGE (eine): der Feld-Scan der gepaddeten AoS-Repraesentation liest mit dem Stride DER
// ACHSE. Vor B14-NB3 stand in CacheLineAlignedMemoryLayout::scan_field_sum ein hartes
// `constexpr std::size_t kCacheLine = 64;`. Das war KEIN Absturz-Risiko (der Puffer war seit B14-NB2
// bereits achsen-abgeleitet, also eher zu gross als zu klein), sondern ein MESS-VALIDITAETS-LOCH: mit den
// KF-6-line_size-Permutationen {32,64,128,256} haette die Achse vier verschiedene Werte gemeldet und der
// Scan in allen vier Faellen dasselbe gelesen. Die Permutation waere in der Messung unsichtbar geblieben --
// eine Achse ohne Wirkung, und zwar lautlos.
//
// Die TU pinnt vier Ebenen:
//   (A) QUELLE       -- alle fuenf Layout-Strategien stehen am Achsen-Default 64, nackt UND gehuellt.
//   (B) NEUTRALITAET -- der CLA-Scan liefert an diesem Default byte-identisch die Werte des Alt-Stands
//                       (Referenz: der Alt-Rumpf ist unten VERBATIM nachgebaut und wird mitgerechnet).
//   (C) ANTI-ZEMENTIERUNG -- mit einer NICHT-Default-Line folgt der Scan der Achse. Genau diese Saetze
//                       BEISSEN am Alt-Stand: dort waeren alle vier Line-Groessen auf denselben Wert
//                       gefallen (Lehre "gruene Tests zementieren alte Ordnung").
//   (D) WRAPPER-AUDIT -- die Bau-Regel in abi_adapter (layout_scan_stride_bytes) verlangt von JEDEM
//                       memory_layout-Slot den benannten Unterachsen-Zugriff. Hier wird belegt, dass die
//                       Bedingung eine nicht-forwardende Huelle wirklich erkennt (Negativ-Probe) und dass
//                       alle heutigen Endinstanziierungen sie erfuellen.
//   (F) VERBRAUCHER   -- B14-NB4. Die Ebenen (A)-(E) pinnen den PRODUZENTEN. Wer nur das tut, zementiert
//                       eine geaenderte EINHEIT, ohne zu pruefen, was am Ende der Kette daraus wird --
//                       und genau daran ist B14-NB3 im Review gescheitert (der aktive CLU-Verbraucher
//                       rechnete weiter mit einem harten Nenner 64). (F) faehrt deshalb die vier
//                       Linien-Zahlen durch den ECHTEN ObserverSnapshotSystemAxis und rechnet den
//                       Alt-Nenner daneben mit.

#include <organ_axes/cacheline/cacheline_config.hpp>
#include <organ_axes/cacheline/cacheline_line_bytes.hpp>
#include <organ_axes/layout/axis_05_memory_layout_aos_strict.hpp>
#include <organ_axes/layout/axis_05_memory_layout_aosoa.hpp>
#include <organ_axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <organ_axes/layout/axis_05_memory_layout_observable.hpp>
#include <organ_axes/layout/axis_05_memory_layout_packed_bitmap.hpp>
#include <organ_axes/layout/axis_05_memory_layout_registry.hpp>
#include <organ_axes/layout/axis_05_memory_layout_soa.hpp>
#include <organ_axes/layout/axis_05_memory_layout_strategy_base.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
// B14-NB4: der ECHTE Verbraucher der Linienzahl gehoert in diese Wache. Ohne ihn zementiert sie nur die
// neue Einheit -- genau der Vorwurf, an dem B14-NB3 im Review gescheitert ist.
#include <cache_engine/measurement/system_axis.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace mp = boost::mp11;
namespace cl = ::comdare::cache_engine::cacheline;
namespace ml = ::comdare::cache_engine::layout;

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

// -- (A) QUELLE: die fuenf Strategien am Achsen-Default, nackt UND in der Registry-Huelle ----------------
// Der Registry-Eintrag der Achse ist ObservableMemoryLayout<Strategy> (topic_memory_layout_config_set.hpp:
// StaticAxisVariants_05 = mp_transform<make_observable_layout, EnabledLayouts>) -- die gehuellte Zeile ist
// also die, die real in den Kompositionen steht, nicht die nackte.
template <class S>
using Huelle = ml::ObservableMemoryLayout<S>;

static_assert(cl::line_bytes_of<ml::AoSStrictMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::CacheLineAlignedMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::SoAMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::AoSoAMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<ml::PackedBitmapMemoryLayout>() == 64);
static_assert(cl::line_bytes_of<Huelle<ml::CacheLineAlignedMemoryLayout>>() == 64);

// -- (D) WRAPPER-AUDIT, compile-time und flaechendeckend ------------------------------------------------
// Die Bedingung, die abi_adapter::detail::layout_scan_stride_bytes als static_assert traegt. Sie wird hier
// (i) gegen ALLE aktivierten Strategien und ihre Registry-Huellen gefahren und (ii) an einer bewusst
// nicht-forwardenden Huelle als WIRKSAM belegt -- ohne die Regel waere die Wache eine Behauptung.
template <class T>
using ist_achsen_bewusst = mp::mp_bool<cl::CacheLineLineBytesAware<T>>;

static_assert(mp::mp_all_of<ml::EnabledLayouts, ist_achsen_bewusst>::value,
              "WRAPPER-AUDIT: eine aktivierte Layout-Strategie beantwortet cacheline_subaxis_line_bytes() nicht.");
static_assert(mp::mp_all_of<mp::mp_transform<Huelle, ml::EnabledLayouts>, ist_achsen_bewusst>::value,
              "WRAPPER-AUDIT: eine REGISTRIERTE Huelle (ObservableMemoryLayout<S>) reicht "
              "cacheline_subaxis_line_bytes() nicht durch -- line_bytes_of<> faellt still auf 64 zurueck.");
// Und dieselbe Liste, wie sie der Permutations-Pfad wirklich baut (nicht von Hand nachgebaut):
static_assert(mp::mp_all_of<::comdare::cache_engine::memory_layout::TopicConfigSet::StaticAxisVariants_05,
                            ist_achsen_bewusst>::value,
              "WRAPPER-AUDIT: eine Variante aus TopicConfigSet::StaticAxisVariants_05 ist nicht achsen-bewusst.");

// NEGATIV-PROBE: genau die Huelle, die B14-NB2 noch stillschweigend passiert haette (kein Forwarding).
// Sie MUSS von der Bedingung abgelehnt werden, sonst waere der Audit zahnlos.
template <class Strategy>
struct NichtForwardendeHuelle {
    using topic_tag = typename Strategy::topic_tag;
    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return Strategy::cache_line_size(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    // ABSICHTLICH KEIN cacheline_subaxis_line_bytes().
};
static_assert(!cl::CacheLineLineBytesAware<NichtForwardendeHuelle<ml::CacheLineAlignedMemoryLayout>>,
              "Der Wrapper-Audit ist zahnlos: eine Huelle OHNE Forwarding gilt faelschlich als achsen-bewusst.");
// Und sie faellt genau so, wie befuerchtet, auf den Default zurueck (der stille Fehler, in Zahlen):
static_assert(cl::line_bytes_of<NichtForwardendeHuelle<ml::CacheLineAlignedMemoryLayout>>() == cl::kDefaultLineBytes);

// -- (D-2) B14-NB4: die ALTE observe_real_footprint-Form darf nicht LAUTLOS durchrutschen ---------------
// LayoutAwareChunkedStore::organ_observe_layout waehlt zwischen Real-Footprint-Pfad und observe_scan-
// Fallback per `requires`. Mit dem neuen fuenften Argument (line_bytes) faende ein Organ, das nur die
// ALTE Vier-Argument-Form traegt, ab sofort lautlos den Fallback -- und maesse eine ANDERE Groesse, ohne
// dass irgendetwas rot wuerde. Der Store traegt deshalb einen static_assert; hier wird belegt, dass
// dessen Bedingung wirklich greift (sonst waere sie eine Behauptung).
template <class Organ>
concept BietetAlteFootprintForm =
    requires(Organ& o) { o.observe_real_footprint(std::uint64_t{}, std::size_t{}, std::uint64_t{}, std::uint64_t{}); };
template <class Organ>
concept BietetNeueFootprintForm = requires(Organ& o) {
    o.observe_real_footprint(std::uint64_t{}, std::size_t{}, std::uint64_t{}, std::uint64_t{}, std::uint64_t{});
};
struct AltesFootprintOrgan { // genau der Stand, der vor B14-NB4 ueberall stand
    std::uint64_t observe_real_footprint(std::uint64_t cs, std::size_t, std::uint64_t, std::uint64_t) { return cs; }
};
static_assert(BietetAlteFootprintForm<AltesFootprintOrgan> && !BietetNeueFootprintForm<AltesFootprintOrgan>,
              "Die Negativ-Probe trifft ihren eigenen Fall nicht mehr.");
// Die Store-Bedingung LAUTET "!alt || neu". Fuer das Alt-Organ ist sie FALSCH -> der Bau bricht, statt
// still in den observe_scan-Pfad zu fallen. Das ist die Aussage.
static_assert(!(!BietetAlteFootprintForm<AltesFootprintOrgan> || BietetNeueFootprintForm<AltesFootprintOrgan>),
              "Die Bau-Regel in axis_04_node_type_layout_aware_store ist zahnlos: ein Organ mit der ALTEN "
              "Vier-Argument-Form wuerde lautlos auf den observe_scan-Fallback zurueckfallen.");
// Und die echte Registry-Huelle erfuellt sie (sonst waere die Regel unerfuellbar statt scharf).
static_assert(!BietetAlteFootprintForm<Huelle<ml::CacheLineAlignedMemoryLayout>> &&
                  BietetNeueFootprintForm<Huelle<ml::CacheLineAlignedMemoryLayout>>,
              "Die produktive Huelle traegt die Fuenf-Argument-Form nicht.");

// -- (C) Testorgane, die sich NUR in der Unterachse unterscheiden ---------------------------------------
// So (und nur so) emittiert KF-6 eine line-permutierte Layout-Variante: identischer CRTP-Kopf, identischer
// intrinsischer Deskriptor (bewusst 64 belassen), identische Repraesentation, identischer Scan-Rumpf --
// der Rumpf ist DERSELBE, den CacheLineAlignedMemoryLayout::scan_field_sum ausfuehrt
// (ml::detail::padded_aos_field_sum). Deshalb ist die Aussage unten eine ueber die ECHTE Quelle.
template <cl::CacheLineSize S>
struct ProbeCla : ml::MemoryLayoutStrategyBase<ProbeCla<S>, cl::CacheLineConfig{S}> {
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = ml::subaxes::alignment_strategy_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool             enabled      = true;
    static constexpr std::string_view algo_version = "probe";

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "probe_cla_line_variant"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "ProbeCla (line-permutiert)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PROBE_CLA"; }
    [[nodiscard]] static constexpr ml::RepresentationKind representation_kind() noexcept {
        return ml::RepresentationKind::aos_interleaved_padded;
    }
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return ml::detail::padded_aos_field_sum(buf, n, record_size, ProbeCla::cacheline_subaxis_line_bytes());
    }
};

// Die Probe traegt die Unterachse wirklich -- nackt UND in der Registry-Huelle (das ist die Aussage, die
// am Stand vor B14-NB2 gefallen waere: dort sah line_bytes_of<Huelle<...>> die Strategie nicht).
static_assert(cl::line_bytes_of<ProbeCla<cl::CacheLineSize::B32>>() == 32);
static_assert(cl::line_bytes_of<ProbeCla<cl::CacheLineSize::B128>>() == 128);
static_assert(cl::line_bytes_of<ProbeCla<cl::CacheLineSize::B256>>() == 256);
static_assert(cl::line_bytes_of<Huelle<ProbeCla<cl::CacheLineSize::B32>>>() == 32);
static_assert(cl::line_bytes_of<Huelle<ProbeCla<cl::CacheLineSize::B128>>>() == 128);
static_assert(cl::line_bytes_of<Huelle<ProbeCla<cl::CacheLineSize::B256>>>() == 256);

// -- (B) Referenz: der Alt-Rumpf aus 6a40071f, VERBATIM -------------------------------------------------
// Nicht als "Bestand", sondern als MASSSTAB: an ihm wird die Byte-Neutralitaet am Default gemessen und
// zugleich gezeigt, dass er bei anderen Line-Groessen NICHT mitgeht (das Loch, das geschlossen wurde).
[[nodiscard]] static std::uint64_t alt_rumpf_literal64(unsigned char const* buf, std::size_t n,
                                                       std::size_t record_size) noexcept {
    constexpr std::size_t kCacheLine     = 64;
    std::size_t const     aligned_stride = (record_size + kCacheLine - 1u) & ~(kCacheLine - 1u);
    std::uint64_t         s              = 0;
    for (std::size_t i = 0; i < n; ++i) {
        std::uint32_t v;
        std::memcpy(&v, buf + i * aligned_stride, sizeof(v));
        s += v;
    }
    return s;
}

// Realer Lese-Fussabdruck statt Behauptung: ab welchem Byte-Offset liest der Scan den ZWEITEN Record?
// Der Scan laeuft ueber kN Records; das Fenster, das er dabei maximal beruehrt, ist (kN-1)*256+4 < kWipe.
// Genau dieses Fenster wird vor jeder Probe genullt -- eine Nicht-Null-Summe verraet dann eindeutig, dass
// der gesetzte Marker GELESEN wurde, also auf einem Record-Anfang liegt.
template <class L>
static std::size_t gemessener_stride(std::vector<unsigned char>& buf) {
    constexpr std::size_t kN    = 64;
    constexpr std::size_t kRs   = 48;
    constexpr std::size_t kWipe = 64u * 256u + 64u;
    for (std::size_t off = 4; off <= 1024; off += 4) {
        std::memset(buf.data(), 0, kWipe);
        std::uint32_t const marker = 0x5A5Au;
        std::memcpy(buf.data() + off, &marker, sizeof(marker));
        if (L::scan_field_sum(buf.data(), kN, kRs) != 0u) return off;
    }
    return 0;
}

int main() {
    std::cout << "B14-NB3: der Layout-Mess-Scan bezieht seinen Stride aus der cacheline-Unterachse\n";

    constexpr std::size_t      kRecords    = 16384;
    constexpr std::size_t      kRecordSize = 48;
    constexpr std::size_t      kLbufBytes  = kRecords * 64; // Geometrie am Achsen-Default (abi_adapter)
    std::vector<unsigned char> buf(kLbufBytes);
    for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<unsigned char>(i * 31u + 7u);

    // -- (A) Quelle, zur Laufzeit sichtbar --
    check_eq("Achsen-Default (kDefaultLineBytes)", cl::kDefaultLineBytes, std::size_t{64});
    check_eq("CLA: Scan-Seite (cacheline_subaxis_line_bytes)",
             ml::CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes(), std::size_t{64});
    check_eq("CLA: Puffer-Seite (line_bytes_of)", cl::line_bytes_of<ml::CacheLineAlignedMemoryLayout>(),
             std::size_t{64});
    check_true("Scan-Seite und Puffer-Seite sind DIESELBE Quelle",
               ml::CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes() ==
                   cl::line_bytes_of<ml::CacheLineAlignedMemoryLayout>());
    check_true("intrinsischer Deskriptor ist NICHT die Unterachse (aos_strict: 1 vs 64)",
               ml::AoSStrictMemoryLayout::cache_line_size() != cl::line_bytes_of<ml::AoSStrictMemoryLayout>());

    // -- (B) Neutralitaet am Default: byte-identisch zum Alt-Rumpf --
    std::uint64_t const alt = alt_rumpf_literal64(buf.data(), kRecords, kRecordSize);
    check_eq("CLA-Scan (nackt) == Alt-Rumpf mit Literal 64",
             ml::CacheLineAlignedMemoryLayout::scan_field_sum(buf.data(), kRecords, kRecordSize), alt);
    check_eq("CLA-Scan (Registry-Huelle) == Alt-Rumpf",
             Huelle<ml::CacheLineAlignedMemoryLayout>::scan_field_sum(buf.data(), kRecords, kRecordSize), alt);
    {
        bool alle = true;
        for (std::size_t rs : {std::size_t{4}, std::size_t{8}, std::size_t{16}, std::size_t{48}, std::size_t{63},
                               std::size_t{64}, std::size_t{65}, std::size_t{128}}) {
            std::size_t const stride = ((rs + 63u) / 64u) * 64u;
            std::size_t const n      = (kLbufBytes - 4u) / stride;
            if (ml::CacheLineAlignedMemoryLayout::scan_field_sum(buf.data(), n, rs) !=
                alt_rumpf_literal64(buf.data(), n, rs))
                alle = false;
        }
        check_true("byte-identisch fuer 8 verschiedene record_size-Werte (nicht nur den Mess-Wert)", alle);
    }
    check_eq("CLA: gemessener Stride am Default", gemessener_stride<ml::CacheLineAlignedMemoryLayout>(buf),
             std::size_t{64});

    // -- (C) Anti-Zementierung: dieselben Daten, NUR die Unterachse permutiert --
    // round_up(48, line): B32 -> 64, B64 -> 64, B128 -> 128, B256 -> 256. B32 und B64 fallen auf der
    // SCAN-Seite bewusst zusammen (48 passt in beiden Faellen erst in zwei bzw. eine Linie); sie
    // unterscheiden sich in der Puffer-AUSRICHTUNG, s. abi_adapter-Entscheid "ALLOC-ALIGNMENT BEI line=32".
    std::size_t const s32  = gemessener_stride<ProbeCla<cl::CacheLineSize::B32>>(buf);
    std::size_t const s64  = gemessener_stride<ProbeCla<cl::CacheLineSize::B64>>(buf);
    std::size_t const s128 = gemessener_stride<ProbeCla<cl::CacheLineSize::B128>>(buf);
    std::size_t const s256 = gemessener_stride<ProbeCla<cl::CacheLineSize::B256>>(buf);
    check_eq("Probe B32  gemessener Stride (round_up(48,32))", s32, std::size_t{64});
    check_eq("Probe B64  gemessener Stride", s64, std::size_t{64});
    check_eq("Probe B128 gemessener Stride", s128, std::size_t{128});
    check_eq("Probe B256 gemessener Stride", s256, std::size_t{256});
    check_true("der Scan ist ueber die Unterachse ECHT diskriminierend (B64 != B128 != B256)",
               s64 != s128 && s128 != s256);
    // Dasselbe durch die Registry-Huelle -- der Weg, den die Kompositionen wirklich nehmen.
    check_eq("Probe B256 durch die Registry-Huelle", gemessener_stride<Huelle<ProbeCla<cl::CacheLineSize::B256>>>(buf),
             std::size_t{256});

    // Die Sonde oben nullt ihr Mess-Fenster -- fuer die folgenden Pruefsummen wird das Muster neu gelegt.
    for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<unsigned char>(i * 31u + 7u);

    // DER BISS: der Alt-Rumpf haette bei ALLEN VIER Belegungen mit Stride 64 gelesen. Genau das war das Loch.
    check_true("Alt-Rumpf (Literal 64) == neuer Rumpf NUR bei line=64",
               alt_rumpf_literal64(buf.data(), 4096, kRecordSize) ==
                   ml::detail::padded_aos_field_sum(buf.data(), 4096, kRecordSize, 64));
    check_true("Alt-Rumpf (Literal 64) != neuer Rumpf bei line=128 -- er waere der Achse NICHT gefolgt",
               alt_rumpf_literal64(buf.data(), 4096, kRecordSize) !=
                   ml::detail::padded_aos_field_sum(buf.data(), 4096, kRecordSize, 128));

    // Der gemeinsame Rumpf selbst, ueber alle vier Achsenwerte (die Zahlen muessen auseinandergehen).
    std::uint64_t const v64  = ml::detail::padded_aos_field_sum(buf.data(), 4096, kRecordSize, 64);
    std::uint64_t const v128 = ml::detail::padded_aos_field_sum(buf.data(), 4096, kRecordSize, 128);
    std::uint64_t const v256 = ml::detail::padded_aos_field_sum(buf.data(), 4096, kRecordSize, 256);
    check_true("Pruefsummen bei 64/128/256 sind paarweise verschieden", v64 != v128 && v128 != v256 && v64 != v256);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // -- (E) GESCHWISTER-BEFUND: cache_lines_touched ist eine MESSGROESSE und zaehlte in observe_scan mit
    // dem Literal 64. Dieselbe Klasse wie oben, nur ein Zaehler weiter: mit permutierter Line waere die CLU
    // in 64-B-Einheiten stehen geblieben, also blind fuer die Achse. Zahlen literal, damit die Wache beisst.
    auto clu = [&buf](auto organ, std::size_t n, std::size_t rs) {
        (void)organ.observe_scan(buf.data(), n, rs);
        return organ.statistics().cache_lines_touched;
    };
    constexpr std::size_t kCluN      = 1024;
    std::uint64_t const   alt_formel = (static_cast<std::uint64_t>(kCluN) * kRecordSize + 63u) / 64u; // = 768
    check_eq("CLU CLA am Default == Alt-Formel ceil(n*rs/64)",
             clu(Huelle<ml::CacheLineAlignedMemoryLayout>{}, kCluN, kRecordSize), alt_formel);
    check_eq("CLU Probe B32  (ceil(1024*48/32))", clu(Huelle<ProbeCla<cl::CacheLineSize::B32>>{}, kCluN, kRecordSize),
             std::uint64_t{1536});
    check_eq("CLU Probe B64  (ceil(1024*48/64))", clu(Huelle<ProbeCla<cl::CacheLineSize::B64>>{}, kCluN, kRecordSize),
             std::uint64_t{768});
    check_eq("CLU Probe B128 (ceil(1024*48/128))", clu(Huelle<ProbeCla<cl::CacheLineSize::B128>>{}, kCluN, kRecordSize),
             std::uint64_t{384});
    check_eq("CLU Probe B256 (ceil(1024*48/256))", clu(Huelle<ProbeCla<cl::CacheLineSize::B256>>{}, kCluN, kRecordSize),
             std::uint64_t{192});

    // -- (F) B14-NB4: DIE KONSUMENTEN-SEITE DERSELBEN VIER ZAHLEN ---------------------------------------
    // Die vier Saetze darueber pinnen 1536/768/384/192 als SOLL. Fuer sich genommen zementieren sie nur die
    // neue EINHEIT -- und genau daran ist B14 im Review gescheitert: der einzige aktive Verbraucher dieser
    // Zahlen (ObserverSnapshotSystemAxis, measurement/system_axis.hpp) bildete die CLU mit einem harten
    // Nenner 64 und haette aus ihnen 8/16/33/66 % gemacht, obwohl der physische Fussabdruck in allen vier
    // Faellen DERSELBE ist (1536*32 == 768*64 == 384*128 == 192*256 == 49152 B).
    // Deshalb steht der Verbraucher hier MIT im Bild: geprueft wird nicht der Zaehler, sondern die Zahl,
    // die am Ende der Kette in der Mess-Zelle landet.
    auto snap = [&buf](auto organ, std::size_t n, std::size_t rs) {
        (void)organ.observe_scan(buf.data(), n, rs);
        return organ.statistics();
    };
    struct KonsumFall {
        char const*   name;
        std::uint64_t line_bytes;
        std::uint64_t erwartete_linien;
        std::uint64_t alt_prozent; // was der Alt-Nenner (Literal 64) gemeldet haette
    };
    auto konsum = [](ml::MemoryLayoutSnapshot const& s, KonsumFall const& f) {
        // Der Weg des Produzenten in den ABI-POD -- exakt die Zuweisungen aus abi_adapter::fill_observer_v3.
        ::comdare::cache_engine::anatomy::ComdareTierObserverSnapshot pod{};
        pod.axis_stats[5][2] = s.field_bytes_read;
        pod.axis_stats[5][3] = s.cache_lines_touched;
        pod.axis_stats[5][5] = s.line_bytes;
        ::comdare::cache_engine::measurement::ObserverSnapshotSystemAxis const achse{pod};
        ::comdare::cache_engine::measurement::SystemAxisSample                 probe{
            .category = ::comdare::cache_engine::measurement::MeasurementCategory::CLU};
        achse.collect(probe);
        (void)f;
        return probe;
    };
    constexpr KonsumFall kFaelle[] = {
        {"B32", 32, 1536, 8}, {"B64", 64, 768, 16}, {"B128", 128, 384, 33}, {"B256", 256, 192, 66}};
    auto const                     s_b32    = snap(Huelle<ProbeCla<cl::CacheLineSize::B32>>{}, kCluN, kRecordSize);
    auto const                     s_b64    = snap(Huelle<ProbeCla<cl::CacheLineSize::B64>>{}, kCluN, kRecordSize);
    auto const                     s_b128   = snap(Huelle<ProbeCla<cl::CacheLineSize::B128>>{}, kCluN, kRecordSize);
    auto const                     s_b256   = snap(Huelle<ProbeCla<cl::CacheLineSize::B256>>{}, kCluN, kRecordSize);
    ml::MemoryLayoutSnapshot const kSnaps[] = {s_b32, s_b64, s_b128, s_b256};

    for (std::size_t i = 0; i < 4; ++i) {
        auto const& f = kFaelle[i];
        auto const& s = kSnaps[i];
        // (F-1) Der PRODUZENT nennt die Einheit seines Zaehlers -- ohne sie ist die Zahl nicht deutbar.
        check_eq((std::string{"Produzent "} + f.name + " meldet line_bytes").c_str(), s.line_bytes, f.line_bytes);
        check_eq((std::string{"Produzent "} + f.name + " meldet cache_lines").c_str(), s.cache_lines_touched,
                 f.erwartete_linien);
        // (F-2) Derselbe physische Fussabdruck in allen vier Faellen -- die Kontroll-Rechnung dazu.
        check_eq((std::string{"beruehrte Bytes "} + f.name + " = Linien * Einheit").c_str(),
                 s.cache_lines_touched * s.line_bytes, std::uint64_t{49152});
        // (F-3) DER BISS: der ECHTE Verbraucher liefert durchgaengig denselben Wert.
        auto const probe = konsum(s, f);
        check_true((std::string{"Verbraucher "} + f.name + " liefert einen gueltigen CLU-Wert").c_str(), probe.valid());
        check_eq((std::string{"Verbraucher "} + f.name + " CLU %").c_str(), probe.value, std::uint64_t{16});
        // (F-4) Und derselbe Snapshot durch den ALT-Nenner (Literal 64), VERBATIM nachgerechnet: vier
        // verschiedene Auslastungen fuer einen unveraenderten Scan. Das war der Landeblocker, in Zahlen.
        std::uint64_t const alt = (s.field_bytes_read * 100u) / (s.cache_lines_touched * 64u);
        check_eq((std::string{"Alt-Nenner (Literal 64) "} + f.name + " haette gemeldet").c_str(), alt, f.alt_prozent);
        if (f.line_bytes != 64)
            check_true((std::string{"Alt-Nenner "} + f.name + " weicht vom Verbraucher ab (der Biss)").c_str(),
                       alt != probe.value);
    }
    // (F-5) Fail-closed: ein Snapshot OHNE Einheit ergibt keine Prozentzahl, sondern n/a.
    {
        ml::MemoryLayoutSnapshot ohne = s_b128;
        ohne.line_bytes               = 0;
        auto const probe              = konsum(ohne, kFaelle[2]);
        check_true("Verbraucher ohne Einheit: source-unavailable statt erfundener Zahl", !probe.valid());
        check_eq("Verbraucher ohne Einheit: kein Restwert", probe.value, std::uint64_t{0});
    }

    // -- (G) B14-NB4: DIE VERGIFTUNG DER EINHEIT IST KLEBRIG ---------------------------------------------
    // cache_lines_touched AKKUMULIERT. Sind einmal zwei Linien-Groessen in dieselbe Summe geflossen, ist
    // die SUMME dauerhaft einheitenlos -- auch wenn danach wieder nur Beitraege der ersten Groesse kommen.
    // Eine Fassung, die die Einheit bei jedem Beitrag neu setzt, meldete am Ende der Folge (64, 128, 64)
    // wieder "64" und machte die Mischung in der Summe unsichtbar: der Zaehler behauptete dann mehr, als
    // gedeckt ist. Genau diese Reihenfolge wird hier gefahren.
    {
        Huelle<ProbeCla<cl::CacheLineSize::B64>> organ{};
        organ.reset();
        (void)organ.observe_scan(buf.data(), kCluN, kRecordSize); // Beitrag 1: Einheit 64
        check_eq("Vergiftung (1) erster Beitrag setzt die Einheit", organ.statistics().line_bytes, std::uint64_t{64});
        (void)organ.observe_real_footprint(0, 1, 8, 1, 128); // Beitrag 2: FREMDE Einheit 128
        check_eq("Vergiftung (2) zwei Einheiten -> Einheit vergiftet", organ.statistics().line_bytes, std::uint64_t{0});
        (void)organ.observe_scan(buf.data(), kCluN, kRecordSize); // Beitrag 3: wieder 64
        check_eq("Vergiftung (3) BLEIBT vergiftet -- die Summe ist mischig, nicht der letzte Beitrag",
                 organ.statistics().line_bytes, std::uint64_t{0});
        // Und der Verbraucher zieht daraus die einzig ehrliche Folgerung.
        check_true("Verbraucher meldet n/a auf dem vergifteten Snapshot",
                   !konsum(organ.statistics(), kFaelle[1]).valid());
        // reset() -- und NUR reset() -- hebt sie auf: die Vergiftung endet mit dem Zaehler, nicht frueher.
        organ.reset();
        (void)organ.observe_scan(buf.data(), kCluN, kRecordSize);
        check_eq("Vergiftung (4) reset() setzt sie zurueck (Zaehler und Einheit fallen gemeinsam)",
                 organ.statistics().line_bytes, std::uint64_t{64});
    }
#else
    std::cout << "  [--]  CLU-Wache uebersprungen (COMDARE_CE_ENABLE_STATISTICS aus)\n";
#endif

    std::cout << "\n==== B14-NB3 Scan-/Wrapper-Wache: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
