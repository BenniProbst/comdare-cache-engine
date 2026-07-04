#pragma once
// Per-Achsen-Vervollstaendigung Phase B (2026-06-04) — ObservablePathCompression<Strategy>: ObservableAxis-Huelle
// um eine path_compression-Strategie (axis_02, T3). Schwester-Muster zu ObservableNodeType / ObservableMemoryLayout
// (statische Treibe-Op durchgereicht + Instanz-Driver mit Zaehlern), erweitert um ein ECHTES Path-Compression-Organ:
// die Huelle haelt ein materialisiertes ByteWiseKeyPrefix (das gepackte Byte-Prefix einer ART-Trie-Inner-Node,
// V41 #43 s4 / Leis ICDE 2013, axis_02_path_compression_byte_wise.hpp:43-99) und treibt es im compress()-Driver —
// statt nur ueber den rohen Record-Puffer zu simulieren.
//
// @topic nodes @achse 02 @saeule 2 (Per-Achsen-Observer) @task Phase-B-T3
//
// **Achsen-Semantik (treu, nicht erfunden):** Die path_compression-Achse modelliert, WIE Pfad-Information beim
// Trie-Abstieg komprimiert wird: ByteWise = 8-Bit-Schritte (gemeinsame Byte-Prefixe via common_prefix_len + cut),
// Patricia = 1-Bit-Schritte (Single-Bit-Split, key_split_bit, Morrison 1968 / HOT / Wormhole), None = keine
// Kompression (raw path). Die Counter machen die Kompressions-Aktivitaet observable; der reine Latenz-Unterschied
// der Descent-Granularitaet bleibt der Wall-Clock-Messung vorbehalten (Pfad B, Hybrid-Messmodell, Doku 24 §8).
//
// **Treibe-Op compress(key, depth) (der eigentliche „Driver"):**
//   1. Materialisiert das ECHTE Byte-Prefix-Organ aus den Low-7-Bytes des Schluessels (ByteWiseKeyPrefix::from_bytes)
//      und misst die gemeinsame Byte-Prefix-Laenge mit dem um `depth` Bytes geshifteten Schluessel
//      (common_prefix_len). Das ist die kanonische, verhaltens-tragende Byte-Skip-Op der ART-Path-Compression.
//   2. cut(): entfernt die gemeinsamen Prefix-Bytes (Trie-Abstieg) → zaehlt cuts_performed + bytes_saved_total.
//   3. Wenn die Strategie zusaetzlich Patricia ist (static key_split_bit vorhanden), wird der Single-Bit-Split an
//      `depth` ausgewertet → der prefix_len_total erhaelt zusaetzlich die 1-Bit-Descent-Charakteristik (Patricia
//      unterscheidet sich von ByteWise genau durch 1-Bit- statt 8-Bit-Schritte).
//
// EHRLICHKEIT der Zaehler (Pflicht-Deklaration, [[no-success-marks-without-literal-output]]): jeder Zaehler folgt
// der ECHTEN Op am materialisierten ByteWiseKeyPrefix bzw. (bei Patricia) am static key_split_bit. KEIN Zaehler
// wird mit einem erfundenen Konstantwert befuellt. Bei PathCompressionNone (raw path, common_prefix_len liefert
// reale, aber kleine Laengen) folgt der Zaehler ebenfalls der echten Op — None ist kein Sonderfall, nur die
// Strategie mit der niedrigsten Kompressions-Aktivitaet.
//
// Gating exakt nach Praezedenz: snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS. Bei OFF:
// compress() = nackter ByteWiseKeyPrefix-Durchlauf ohne Tracking (0 Footprint), ObservableAxis<...> = false →
// observe_all() faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt). Die static path_descend_scan +
// compression_ratio() werden als Drop-in fuer den seg19-Timer (abi_adapter do_seg19:490) durchgereicht.

#include "axis_02_path_compression_byte_wise.hpp" // ByteWiseKeyPrefix (das echte Prefix-Organ)
#include "axis_02_path_compression_real_trie.hpp" // §4.3: MATERIALISIERTER Patricia-Trie (additiv, none = leer/No-Op)
#include "concepts/axis_02_path_compression_concept.hpp"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::path_compression {

/// ABI-taugliches Path-Compression-Snapshot (standard_layout + trivially_copyable → in den generischen Cross-ABI-
/// Observer-POD axis_stats[3] mappbar). NUR uint64-Felder.
struct PathCompressionStatistics {
    std::uint64_t compress_calls = 0; ///< Anzahl compress()-Driver-Aufrufe
    std::uint64_t prefix_len_total =
        0; ///< kumulierte gemeinsame Prefix-Laenge (Bytes bei ByteWise, +Bits bei Patricia)
    std::uint64_t bytes_saved_total = 0; ///< durch cut() eingesparte Prefix-Bytes (komprimierte Pfad-Information)
    std::uint64_t cuts_performed    = 0; ///< Anzahl cut()-Operationen (Trie-Abstiege mit nicht-leerem Prefix)
    std::uint64_t last_checksum     = 0; ///< letztes packed_-Wort des Prefix-Organs (Korrektheits-/Strategie-Anker)

    [[nodiscard]] bool operator==(PathCompressionStatistics const&) const noexcept = default;
};

/// ObservableAxis-Huelle: path_compression-Strategie + ECHTES ByteWiseKeyPrefix-Organ + Per-Achsen-Mess-Mechanik
/// (gegated). KEIN Aggregat (private member + Methoden) → direkt als Anatomie-/abi_adapter-Member haltbar. Reicht
/// die static API durch, damit die Huelle ueberall als path_compression-Slot funktioniert (seg19-Timer ruft
/// PathCompression::path_descend_scan bei Patricia; composition_registry ruft C::path_compression::name()).
template <class Strategy>
    requires concepts::PathCompressionStrategy<Strategy>
class ObservablePathCompression {
public:
    using strategy_type = Strategy;
    using topic_tag     = typename Strategy::topic_tag; // NodesComponent → PathCompressionStrategy erfuellt

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): Strategie-Inspektion durchgereicht. NUR die static API wird weitergereicht
    // (name/family_name/flag_suffix) — exakt wie ObservableNodeType/ObservableMemoryLayout. compression_ratio()
    // ist eine NICHT-static Methode der Strategie und wird von KEINEM Slot-Aufrufer gebraucht (composition_registry
    // / axis_path_serialization rufen nur C::path_compression::name()); zudem ist die CRTP-StrategyBase protected-
    // konstruiert → ein gehaltenes Strategie-Objekt ist nicht als Member instanziierbar (C2248). Daher wird die
    // Strategie NICHT als Instanz gehalten; der compress()-Driver nutzt das freie ByteWiseKeyPrefix-Organ + die
    // static Strategy::key_split_bit (Patricia) — beides ohne Strategie-Instanz.
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept
        requires requires { Strategy::family_name(); }
    {
        return Strategy::family_name();
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept
        requires requires { Strategy::flag_suffix(); }
    {
        return Strategy::flag_suffix();
    }
    /// Transparenter Pass-Through der AxisBase-Eigenschaft get_compiler() (Default "original") — die rohe
    /// Strategie traegt sie, die Huelle reicht sie durch (test_v41_compositions ruft C::path_compression::get_compiler()).
    /// SFINAE-sicher: existiert nur, wenn die Strategie das Primitiv traegt (kein Bruch sonst).
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); }
    {
        return Strategy::get_compiler();
    }

    /// STATIC Pass-Through (Drop-in): Patricia-Strategien tragen path_descend_scan (seg19-Timer abi_adapter:490).
    /// SFINAE-detektiert (nur Patricia hat sie) → die Huelle reicht sie unveraendert durch. Trackt NICHT (static).
    template <class S = Strategy>
        requires requires(unsigned char const* b, std::size_t n) { S::path_descend_scan(b, n, n); }
    [[nodiscard]] static std::uint64_t path_descend_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        return S::path_descend_scan(buf, n, record_size);
    }

    /// STATIC Pass-Through (Drop-in): Patricia-Bit-Split-Primitiv. SFINAE-detektiert.
    template <class S = Strategy>
        requires requires(std::uint64_t k, unsigned d) { S::key_split_bit(k, d); }
    [[nodiscard]] static std::uint8_t key_split_bit(std::uint64_t key, unsigned depth) noexcept {
        return S::key_split_bit(key, depth);
    }

    // ── §4.3 (User 2026-06-04) — MATERIALISIERTER Patricia-Trie + ECHTER Descent (additiv, none = leer/No-Op) ──────
    // Die Huelle haelt die ECHTE Trie-Struktur-Instanz (real_trie_), compile-zeit-selektiert je Strategie
    // (axis_02_path_compression_real_trie.hpp): Patricia (traegt static key_split_bit) → PatriciaTrie (echter
    // Single-Bit-Split-Descent); none (+ ByteWise, das sein eigenes Byte-Prefix-Organ traegt) → EmptyPatriciaTrie
    // (leer, 0 Footprint). insert_key/descend/clear_trie existieren NUR, wenn die Strategie real ist (EmptyPatriciaTrie
    // traegt KEIN insert_key → der abi_adapter-Build-Hook greift nicht → `none` bleibt EXAKT No-Op + messneutral,
    // Leitplanke 1). EXAKT analog axis_14_value_handle_real_slot (§4.3) real_slot_/store_value/deref_value.

    using real_trie_type = real_trie_t<Strategy>;

    /// Build (Setup, NICHT gemessen): den Schluessel inkrementell in den materialisierten Patricia-Trie einordnen
    /// (crit-bit-Trie-Insert, Set-Semantik). Existiert NUR fuer Patricia (EmptyPatriciaTrie ohne insert_key →
    /// `none`-Build-Hook greift nicht → M3-Pin bleibt EXAKT No-Op).
    void insert_key(std::uint64_t key) noexcept
        requires requires(real_trie_type t) { t.insert_key(key); }
    {
        real_trie_.insert_key(key);
    }

    /// ECHTER bit-weiser Descent gegen den materialisierten Trie: ab Wurzel je innerem Knoten das crit_bit lesen →
    /// left/right → bis Blatt, am Blatt voller Schluesselvergleich. Liefert true gdw. der Key real im Trie steht.
    /// Existiert NUR fuer Patricia (none traegt KEINEN Trie → kein Descent-Organ noetig).
    [[nodiscard]] bool descend(std::uint64_t key) const noexcept
        requires requires(real_trie_type const ct) {
            { ct.descend(key) } -> std::convertible_to<bool>;
        }
    {
        return real_trie_.descend(key);
    }

    /// Leert den materialisierten Trie (Memento/tier_clear-Symmetrie). NUR fuer Patricia (EmptyPatriciaTrie::clear
    /// ist no-op, traegt aber clear() → der requires greift; `none` bleibt 0-Footprint).
    void clear_trie() noexcept
        requires requires(real_trie_type t) { t.clear(); }
    {
        real_trie_.clear();
    }

    /// Lesezugriff auf den materialisierten Trie (Test-Verifikation + Diagnose: node_count/key_count/last_descent_depth).
    [[nodiscard]] real_trie_type const& real_trie() const noexcept { return real_trie_; }
    [[nodiscard]] real_trie_type&       real_trie() noexcept { return real_trie_; }

    /// Bit-exakter Vergleich des MATERIALISIERTEN Trie (Memento-Verifikation, §4.3, analog ObservableValueHandle).
    /// Vergleicht NUR die persistente Trie-Struktur (real_trie_), NICHT die diagnostischen Stats — der Memento-
    /// Vertrag betrifft das Trie-Backing.
    [[nodiscard]] bool operator==(ObservablePathCompression const& o) const noexcept {
        return real_trie_ == o.real_trie_;
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): treibt das ECHTE ByteWiseKeyPrefix-Organ. Der
    /// Observer-Treiber (abi_adapter fill_observer_v3, Pfad B) ruft dies je registriertem Schluessel. `depth` =
    /// aktuelle Trie-Tiefe (Byte-Position), modelliert die Descent-Position. Liefert die gemessene gemeinsame
    /// Prefix-Laenge (auch fuer einen optionalen Korrektheits-sink).
    std::uint64_t compress(std::uint64_t key, unsigned depth) noexcept {
        // (1) ECHTES Byte-Prefix-Organ materialisieren: Low-7-Bytes des Schluessels als gepacktes Prefix (Laenge 7).
        ByteWiseKeyPrefix prefix = ByteWiseKeyPrefix::from_bytes(key, ByteWiseKeyPrefix::kCapacity);
        // (2) gemeinsame Byte-Prefix-Laenge mit dem um `depth` Bytes geshifteten Schluessel (kanonische Byte-Skip-Op).
        unsigned const      shift_bytes = (depth & 7u); // 0..7 Byte-Positionen (Low-7-Bytes-Organ)
        std::uint64_t const shifted     = key >> (shift_bytes * 8u);
        unsigned const      common      = prefix.common_prefix_len(shifted);
        std::uint64_t       bit_steps   = 0;
        // (3) Patricia-Charakteristik: Single-Bit-Split an `depth` (1-Bit-Descent statt 8-Bit) — nur wenn die
        //     Strategie das Primitiv traegt. Zaehlt die 1-Bit-Schritte bis zum ersten 0-Bit (variable Trie-Tiefe).
        if constexpr (requires { Strategy::key_split_bit(key, depth); }) {
            for (unsigned d = depth; d < 64u; ++d) {
                std::uint8_t const bit = Strategy::key_split_bit(key, d);
                ++bit_steps;
                if (bit == 0u) break;
            }
        }
        // (4) cut(): die gemeinsamen Prefix-Bytes entfernen (Trie-Abstieg) — nur wenn common > 0 (echter Abstieg).
        std::uint64_t saved = 0;
        if (common > 0u) {
            unsigned const cut_len = (common <= prefix.length()) ? common : prefix.length();
            if (cut_len > 0u) {
                prefix.cut(cut_len);
                saved = cut_len;
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.compress_calls;
        stats_.prefix_len_total += static_cast<std::uint64_t>(common) + bit_steps;
        stats_.bytes_saved_total += saved;
        if (saved > 0u) ++stats_.cuts_performed;
        stats_.last_checksum = prefix.packed_;
#endif
        return static_cast<std::uint64_t>(common) + bit_steps;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = PathCompressionStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif

private:
    // ── §4.3 — MATERIALISIERTER Patricia-Trie-Instanz (IMMER vorhanden, auch ohne Statistics-Define) ─────────────
    // Traegt den echten Single-Bit-Split-Trie (Patricia) bzw. EmptyPatriciaTrie (none/ByteWise, leer, 0 Footprint).
    // std::vector-basiert (Patricia) bzw. leer → copy-constructible + copy-assignable + operator== → Observable-
    // PathCompression kopierbar/vergleichbar → fuer den symmetrischen Memento (saved_pc_ in tier_save_all/
    // tier_rollback_all) snapshot-faehig (R1, Leitplanke 3). Default-konstruiert = leer (none-aequivalente Baseline).
    real_trie_type real_trie_{};
};

} // namespace comdare::cache_engine::path_compression
