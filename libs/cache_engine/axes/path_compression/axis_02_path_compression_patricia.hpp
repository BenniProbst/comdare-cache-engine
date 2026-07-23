#pragma once
// V41.F.6.1.R7.1.c axis_02 PatriciaPathCompression (Single-Bit-Split, HOT/Wormhole)

#include "axis_02_path_compression_strategy_base.hpp"
#include "axis_02_path_compression_subaxes_pc1_to_pc3.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include <axes/path_compression/axis_02_path_compression_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <anatomy/organ_location.hpp> // INC-B/R-B: COMDARE_DEFINE_ORGAN_LOCATION (per-Organ-Codegen-Lokation)
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::path_compression {

/// PatriciaPathCompression — Single-Bit-Split (Patricia-Trie).
/// Verwendet von HOT (Binna PVLDB 2018) + Wormhole (Wu EuroSys 2019, 10.1145/3302424.3303955).
/// Vorteil: Trie-Hoehe drastisch reduziert (nur signifikante Bits).
class PatriciaPathCompression : public PathCompressionStrategyBase<PatriciaPathCompression> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::skip_strategy_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::patricia_enabled;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "path_compression_patricia"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PatriciaPathCompression (single-bit-split, HOT/Wormhole)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PATRICIA"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // INC-B/R-B (2026-07-14): Per-Organ-Codegen-Lokation. Dieses CE-Organ ist zugleich der golden-verdrahtete
    // prt-art-Merge-Baustein (PrtArtPathCompressionSlot, compositions/prt_art_merge_reference.hpp:42-47 ->
    // PrueflingVariants = mp_list<PatriciaPathCompression>); die prt-art-Registry emittiert seinen Header hierher.
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::path_compression::PatriciaPathCompression",
                                  "axes/path_compression/axis_02_path_compression_patricia.hpp");

    [[nodiscard]] double compression_ratio() const noexcept { return 0.3; } // typisch ~3x kompakter

    // T3-Treibe-Op (2026-06-04): ByteWise traegt sein echtes Prefix-Organ (ByteWiseKeyPrefix::common_prefix_len);
    // Patricia hatte bis hier KEINE operative, treibbare Op und waere im Per-Achsen-Timer als n/a gelandet.
    // Die folgenden zwei Ops machen die Patricia-Strategie F15-operativ (analog axis_05 scan_field_sum /
    // axis_10 serialize_scan / axis_04 node_find_scan), OHNE einen erfundenen konstanten Wert.
    //
    // key_split_bit(key, depth): das verhaltens-tragende KERN-Primitiv eines Patricia-Trie (Single-Bit-Split,
    // Morrison 1968 / HOT Binna PVLDB 2018 / Wormhole Wu EuroSys 2019): extrahiert das EINE signifikante Bit an
    // Tiefe `depth`, radix-trie-konventionell MSB-first (depth 0 = hoechstwertiges Bit). Reine, branch-freie
    // Bit-Extraktion — die Operation, durch die sich Patricia (1-Bit-Descent) von ByteWise (8-Bit-Descent)
    // unterscheidet.
    [[nodiscard]] static constexpr std::uint8_t key_split_bit(std::uint64_t key, unsigned depth) noexcept {
        return static_cast<std::uint8_t>((key >> (63U - (depth & 63U))) & 1U);
    }

    // path_descend_scan(buf, n, record_size): goldstandard-foermige `_scan`-Treibe-Op (statische
    // _scan(buf,n,record_size)-Signatur wie scan_field_sum/serialize_scan/node_find_scan). Pro Record wird der
    // 64-Bit-Schluessel gelesen und ein Patricia-Descent SIMULIERT: bitweiser Single-Bit-Split via key_split_bit
    // ueber bis zu 64 Ebenen, mit Fruehabbruch sobald ein 0-Bit gefunden wird (modelliert die variable, vom
    // Schluessel ABHAENGIGE Trie-Tiefe von Patricia — KEINE konstante Anzahl Schritte). Die Laufzeit ist damit
    // real und strategie-charakteristisch (1-Bit-Schritte statt ByteWise-8-Bit-Schritte) und schluessel-daten-
    // abhaengig.
    //
    // EHRLICHKEIT (Pflicht-Deklaration, [[no-success-marks-without-literal-output]]): dies ist eine SYNTHETISCHE
    // Mindest-Op. Patricia haelt hier KEINEN echten materialisierten Trie wie ByteWiseKeyPrefix; der Descent wird
    // ueber den rohen Record-Puffer simuliert. Der Aufwand ist NICHT erfunden/konstant, sondern ergibt sich aus
    // dem Single-Bit-Split-Verhalten ueber die tatsaechlichen Schluesselbits — er erzeugt eine reale, von der
    // Strategie und den Daten abhaengige Laufzeit.
    [[nodiscard]] static std::uint64_t path_descend_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint64_t key = 0;
            std::memcpy(&key, buf + i * record_size, sizeof(key)); // 64-Bit-Schluessel je Record
            for (unsigned depth = 0; depth < 64U; ++depth) {       // Patricia: Single-Bit-Descent
                std::uint8_t const bit = key_split_bit(key, depth);
                sum += bit;
                if (bit == 0U) break; // variable, schluessel-abhaengige Tiefe
            }
        }
        return sum;
    }
};

} // namespace comdare::cache_engine::path_compression

namespace comdare::cache_engine::path_compression {
static_assert(concepts::PathCompressionStrategy<PatriciaPathCompression>);
static_assert(concepts::CacheEnginePermutationStrategy<PatriciaPathCompression>);
} // namespace comdare::cache_engine::path_compression
