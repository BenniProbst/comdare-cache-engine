#pragma once
// V41.F.6.1.R7.1.b axis_05 CacheLineAlignedMemoryLayout Default-Wrapper (Goldstandard-Update)

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include <axes/cacheline/cacheline_line_bytes.hpp> // B14-NB3: Anti-Divergenz-Pin Scan-Seite <-> Puffer-Seite
#include <axes/layout/axis_05_memory_layout_flags.hpp>
#include <topics/memory_layout/concepts/topic_memory_layout_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::layout {

namespace detail {

/// B14-NB3: der Feld-Scan der GEPADDETEN AoS-Repraesentation (RepresentationKind::aos_interleaved_padded),
/// Stride = round_up(record_size, line).
///
/// WARUM ALS EIGENE FUNKTION UND NICHT INLINE IM RUMPF: `line` ist ein PARAMETER, kein Default. Die Funktion
/// kennt keine Line-Groesse und erfindet auch keine -- der Aufrufer MUSS sie aus der cacheline-Unterachse
/// beziehen (L::cacheline_subaxis_line_bytes() bzw. line_bytes_of<L>()). Damit ist genau der Rumpf, den
/// CacheLineAlignedMemoryLayout::scan_field_sum ausfuehrt, auch fuer eine line-permutierte Variante
/// ausfuehrbar -- also PRUEFBAR, ohne die Achse zu faelschen. Vor B14-NB3 stand die Formel mit dem Literal 64
/// nur im Rumpf: sie war weder ableitbar noch bei 32/128/256 testbar (die Wache haette nichts gebissen).
/// Voraussetzung (vom Aufrufer compile-hart gepinnt, s. scan_field_sum): line > 0.
[[nodiscard]] inline std::uint64_t padded_aos_field_sum(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size, std::size_t line) noexcept {
    std::size_t const aligned_stride = ((record_size + line - 1u) / line) * line; // round_up auf die Achsen-Line
    std::uint64_t     s              = 0;
    for (std::size_t i = 0; i < n; ++i) {
        std::uint32_t v;
        std::memcpy(&v, buf + i * aligned_stride, sizeof(v)); // CLA: cache-line-gepaddeter Stride
        s += v;
    }
    return s;
}

} // namespace detail

/// CacheLineAlignedMemoryLayout — Default: 64-byte aligned AoS layout.
/// Standard fuer ART/HOT/Masstree/START. Vermeidet False-Sharing,
/// optimal fuer concurrent Schreiber.
class CacheLineAlignedMemoryLayout : public MemoryLayoutStrategyBase<CacheLineAlignedMemoryLayout> {
public:
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = subaxes::alignment_strategy_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::cache_line_aligned_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "memory_layout_cache_line_aligned"; }

    // REALE Repraesentation (P-MD1-ERDUNG #167): jeder Record ist im Store auf eine VOLLE 64-B-Cache-Line
    // gepaddet ([key|value|48 B pad], Stride 64). Der Key-only-Scan beruehrt damit PRO Record eine eigene
    // Linie, von der nur 8 Key-Bytes nuetzlich sind → die NIEDRIGSTE CLU der 5 Layouts (bewusstes
    // False-Sharing-Vermeidungs-Padding kostet Cache-Line-Effizienz). CLU aus dem ECHTEN 64-B-Store-Stride.
    [[nodiscard]] static constexpr RepresentationKind representation_kind() noexcept {
        return RepresentationKind::aos_interleaved_padded;
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CacheLineAlignedMemoryLayout (64-byte AoS, standard cache architectures)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CACHE_LINE_ALIGNED"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (macht die Layout-Achse F15-operativ).
    // LAYOUT-FIX (X-Abschn.4, 2026-06-04): cache_line_aligned modelliert auf Cache-Line-Grenzen GEPADDETE
    // Records -> effektiver Stride = round_up(record_size, Line-Groesse). Bei record_size < Line (z.B. dem
    // Mess-Build record_size=48) entsteht Padding (16 B) -> groesserer Stride als aos_strict (kompakt-gepackt,
    // Stride = record_size) -> MEHR Cache-Lines/Record -> messbar hoehere, vom Layout VERSCHIEDENE Latenz.
    // (Fruehere Fassung war byte-identisch zu aos_strict -- der cache_line_size()-Wert ging nicht in den Scan
    // ein; das war ein Duplikat-Bug.) Bei record_size == Vielfachem der Line faellt aligned_stride==record_size
    // -> dann (korrekt) wieder identisch zu aos_strict; der Mess-Build waehlt daher kRecordSize=48
    // (abi_adapter.hpp), damit die Layout-Achse aos_strict vs cache_line_aligned real differenziert.
    // Echter Cache-Effekt, kein synthetischer Wert.
    //
    // B14-NB3 (2026-08-06) -- SCAN-SEITE DER KF-6-AUFLAGE (D), die zweite Haelfte zu B14-NB2:
    // hier stand `constexpr std::size_t kCacheLine = 64;`. Die Line-Groesse ist aber Eigentum der
    // cacheline-Unterachse (Owner-KERN 04.08., generalisierte Schnitt-Regel: ALLE Achsen-Eigenschaften NUR
    // ueber die Achsen). Das Literal war KEIN Absturz-Risiko, sondern ein MESS-VALIDITAETS-LOCH: mit den
    // KF-6-line_size-Permutationen {32,64,128,256} haette die Achse vier verschiedene Werte gemeldet, dieser
    // Scan aber in ALLEN VIER Faellen mit Stride 64 gelesen -- die Permutation waere in der Messung
    // unsichtbar geblieben (am Objekt reproduziert: Sonde nb3-repro-A, vier Achsen-Belegungen, EIN Stride).
    // Die Line kommt deshalb jetzt aus dem benannten Unterachsen-Zugriff der CRTP-Basis. BEWUSST NICHT ueber
    // line_bytes_of<>: dessen Fallback-Stufe 3 wuerde ein fehlendes Achsen-Glied STILL auf 64 zurueckfallen
    // lassen; der direkte Aufruf bricht in dem Fall den BUILD. Die Gleichheit beider Wege ist unten am
    // Datei-Ende gepinnt (kein Auseinanderlaufen von Puffer-Seite und Scan-Seite).
    // BYTE-NEUTRAL: die Basis traegt heute den Default-NTTP CacheLineConfig{} == B64, also unveraendert 64.
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        constexpr std::size_t kCacheLine = cacheline_subaxis_line_bytes(); // die ACHSE, kein Literal
        static_assert(kCacheLine > 0u, "cacheline-Unterachse liefert Line-Groesse 0 -- round_up unmoeglich");
        return detail::padded_aos_field_sum(buf, n, record_size, kCacheLine);
    }
};

} // namespace comdare::cache_engine::layout

namespace comdare::cache_engine::layout {
static_assert(concepts::MemoryLayoutStrategy<CacheLineAlignedMemoryLayout>);
static_assert(concepts::CacheEnginePermutationStrategy<CacheLineAlignedMemoryLayout>);

// B14-NB3: ANTI-DIVERGENZ-PIN zwischen Scan-Seite und Puffer-Seite. Der Scan oben nimmt die Line aus dem
// benannten Unterachsen-Zugriff, abi_adapter.hpp dimensioniert den Scan-Puffer ueber die Konsum-Bruecke
// line_bytes_of<>. Laufen die beiden je auseinander (z.B. weil eine Huelle den Zugriff nicht forwardet und
// die Bruecke still auf kDefaultLineBytes faellt), dann UNTERdimensioniert der Puffer den Scan -> genau der
// OOB, den die Ableitung verhindern soll. Diese Zeile bricht in dem Fall den BUILD statt die Messung.
static_assert(::comdare::cache_engine::cacheline::line_bytes_of<CacheLineAlignedMemoryLayout>() ==
                  CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes(),
              "Scan-Seite (cacheline_subaxis_line_bytes) und Puffer-Seite (line_bytes_of) liefern verschiedene "
              "Line-Groessen -- der Layout-Scan-Puffer waere unterdimensioniert.");
// Byte-Neutralitaet am Achsen-Default festgenagelt (Stand vor B14-NB3: hartes Literal 64).
static_assert(CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes() ==
              ::comdare::cache_engine::cacheline::kDefaultLineBytes);
} // namespace comdare::cache_engine::layout
