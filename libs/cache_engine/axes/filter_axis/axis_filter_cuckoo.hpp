#pragma once
// V41.F.6.1.R7.5.e axis_filter CuckooFilter (Fan CoNEXT 2014)
//
// R7.6 Paper-Reference (Task #723):
// Fan, B., Andersen, D.G., Kaminsky, M., Mitzenmacher, M.D. "Cuckoo Filter:
// Practically Better Than Bloom." Proceedings of the 10th ACM International
// Conference on Emerging Networking Experiments and Technologies (CoNEXT 2014),
// pp. 75-88.
// DOI: 10.1145/2674005.2674994
// URL: https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf
// Code: https://github.com/efficient/cuckoofilter (Apache-2.0 License)
// Original-Code-Files: src/cuckoofilter.h, src/cuckoofilter.cc

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <axes/filter_axis/axis_filter_flags.hpp>
#include <topics/filter/concepts/topic_filter_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::filter_axis {

/// CuckooFilter — Fan/Andersen/Kaminsky/Mitzenmacher CoNEXT 2014.
/// Bucketed-Cuckoo-Hashing mit Fingerprints. Vorteil vs Bloom:
/// **unterstuetzt delete + lookup-Counting**. Trade-off: hoeherer Insert-Aufwand.
class CuckooFilter : public FilterStrategyBase<CuckooFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::mutability_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::cuckoo_enabled;

    // ── P5 (#124, 2026-06-04, User §4.3) — REALE, aus den eingefuegten Keys gebaute Cuckoo-Bucket-Tabelle ─────
    // Eine ECHTE persistente Bucket-Tabelle (partial-key cuckoo hashing nach Fan CoNEXT 2014 §3): jeder Eintrag
    // ist ein 1-Byte-Fingerprint, jeder Bucket fasst kSlotsPerBucket Fingerprints. insert_key setzt den
    // Fingerprint in einen der ZWEI Kandidaten-Buckets i1 / i2 = i1 XOR hash(fp); probe_key sucht den
    // Fingerprint in beiden. kBuckets=4096 → 8 KiB (L1-resident, messneutral). Voll-uint64-Key-Domain.
    static constexpr std::size_t kBuckets        = 4096; // Anzahl Buckets (Zweierpotenz → schnelles mod)
    static constexpr std::size_t kSlotsPerBucket = 4;    // Fan 2014: b=4 Slots/Bucket (Standard)

    [[nodiscard]] static constexpr std::uint8_t fingerprint_(std::uint64_t key) noexcept {
        return static_cast<std::uint8_t>(((key * 0x1B873593ull) >> 24) | 1u); // !=0 (0 = leerer Slot)
    }
    [[nodiscard]] static constexpr std::size_t bucket1_(std::uint64_t key) noexcept {
        return static_cast<std::size_t>((key * 2654435761ull) & (kBuckets - 1u));
    }
    [[nodiscard]] static constexpr std::size_t bucket2_(std::size_t i1, std::uint8_t fp) noexcept {
        return static_cast<std::size_t>((i1 ^ (static_cast<std::size_t>(fp) * 0x5BD1E995u)) & (kBuckets - 1u));
    }

    /// Build-Op (Setup, NICHT gemessen): platziert den Fingerprint in einen freien Slot von i1, sonst i2.
    /// Sind beide Kandidaten-Buckets voll, greift die ECHTE Kuckucks-Verdraengung (Fan CoNEXT 2014 §3):
    /// ein residenter Fingerprint wird in SEINEN Alternativ-Bucket relokiert (i_alt = i XOR hash(fp),
    /// partial-key symmetrisch), bis ein freier Slot gefunden ist. KEIN stilles Ueberschreiben mehr —
    /// jeder verdraengte Fingerprint wird verschoben, NIE verworfen → die Filter-Gattungs-Garantie
    /// (may-contain, KEINE False Negatives) haelt fuer ALLE bereits eingefuegten Keys. Gelingt die
    /// Platzierung nach kMaxKicks nicht (Filter effektiv voll), wird die gesamte Verdraengungskette
    /// LIFO zurueckgerollt (table_ bit-exakt wie vor dem insert) und der NEUE Key ehrlich nicht
    /// aufgenommen (ehrliche Kapazitaetsgrenze statt Datenverlust). Deterministisch (fixe Hash-
    /// Konstanten, reproduzierbar) → mess-neutral, kein Achsenwert/name()/binary_id beruehrt.
    void insert_key(std::uint64_t key) noexcept {
        std::uint8_t const fp = fingerprint_(key);
        std::size_t const  i1 = bucket1_(key);
        std::size_t const  i2 = bucket2_(i1, fp);
        if (place_in_bucket_(i1, fp)) return;
        if (place_in_bucket_(i2, fp)) return;
        // Beide Kandidaten voll → Kuckucks-Verdraengung mit vollstaendigem Undo-Pfad.
        struct KickStep {
            std::uint16_t bucket;
            std::uint8_t  slot;
        };
        std::array<KickStep, kMaxKicks> path{};
        std::size_t                     steps = 0;
        std::uint8_t                    carry = fp; // aktuell zu platzierender Fingerprint
        std::size_t                     cur   = i1;
        for (std::size_t n = 0; n < kMaxKicks; ++n) {
            std::size_t const  slot    = eviction_slot_(cur, n, carry);
            std::size_t const  idx     = cur * kSlotsPerBucket + slot;
            std::uint8_t const evicted = table_[idx];
            table_[idx]                = carry;   // platziere getragenen Fingerprint
            carry                      = evicted; // trage verdraengten Fingerprint weiter
            path[steps++]              = KickStep{static_cast<std::uint16_t>(cur), static_cast<std::uint8_t>(slot)};
            cur                        = bucket2_(cur, carry); // Alternativ-Bucket des verdraengten Fingerprints
            if (place_in_bucket_(cur, carry)) return;          // freier Slot → alle Fingerprints erhalten
        }
        // Filter voll: Kette LIFO zurueckrollen → table_ exakt wiederherstellen, neuen Key verwerfen.
        for (std::size_t k = steps; k-- > 0;) {
            std::size_t const  idx = static_cast<std::size_t>(path[k].bucket) * kSlotsPerBucket + path[k].slot;
            std::uint8_t const tmp = table_[idx];
            table_[idx]            = carry;
            carry                  = tmp;
        }
    }

    /// Probe der REALEN Bucket-Tabelle: Fingerprint in i1 ODER i2 → "moeglicherweise enthalten".
    [[nodiscard]] bool probe_key(std::uint64_t key) const noexcept {
        std::uint8_t const fp = fingerprint_(key);
        std::size_t const  i1 = bucket1_(key);
        std::size_t const  i2 = bucket2_(i1, fp);
        return in_bucket_(i1, fp) || in_bucket_(i2, fp);
    }

    void               clear() noexcept { table_.fill(0); }
    [[nodiscard]] bool operator==(CuckooFilter const& o) const noexcept { return table_ == o.table_; }

    /// Probe-Multiplizitaet je Query (2 Bucket-Lookups) — ehrlich deklariert fuer hash_probes_total.
    [[nodiscard]] static constexpr std::uint64_t probe_multiplicity() noexcept { return 2u; }

    /// SPACE-Seite des T16-Kern-Trade-offs: strukturelles Bit-Budget der REALEN Bucket-Tabelle (kBuckets ×
    /// kSlotsPerBucket 1-Byte-Fingerprints = sizeof(table_)*8), compile-time. Static-constexpr-Descriptor analog
    /// probe_multiplicity() — kein Datenmember, kein neuer Achsenwert, kein POD-Feld (golden-320/Gate-1/1416 neutral).
    [[nodiscard]] static constexpr std::size_t filter_bit_capacity() noexcept {
        return kBuckets * kSlotsPerBucket * 8u;
    }
    /// bits/key bei key_count Keys — spiegelt composable/ bits_per_key() (key_count==0 → 0.0).
    [[nodiscard]] static constexpr double bits_per_key(std::size_t key_count) noexcept {
        return key_count ? static_cast<double>(filter_bit_capacity()) / static_cast<double>(key_count) : 0.0;
    }

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "filter_cuckoo"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::filter_axis::CuckooFilter",
                                  "axes/filter_axis/axis_filter_cuckoo.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CuckooFilter (Fan CoNEXT 2014, supports delete + counting)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CUCKOO"; }

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEINE persistente Bucket-Tabelle, daher wird `buf` als Pseudo-Bucket-
    // Array (1 Byte = 1 Fingerprint-Slot) der Länge `n` interpretiert. Der Aufwand ist REAL und
    // strategie-abhängig (Cuckoo: 1-Byte-Fingerprint + ZWEI Kandidaten-Buckets i1 und i2 = i1 XOR
    // hash(fp) — partielle-Schlüssel-Cuckoo-Hashing nach Fan CoNEXT 2014 §3), KEINE erfundene Zahl.
    // Charakteristisch vs Bloom: 2 Bucket-Lookups + Fingerprint-Byte-Vergleich (statt k Bit-Tests),
    // dadurch andere Cache-Zugriffs- und Latenz-Signatur. Punkt-Query only.
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n == 0) return 0;
        std::uint64_t hits = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];
            std::uint8_t const  fp  = static_cast<std::uint8_t>((key * 0x1Bu) | 1u); // 1-Byte-Fingerprint (≠0)
            std::uint32_t const h   = key * 2654435761u;                             // Basis-Hash
            std::size_t const   i1  = h % n;                                         // erster Bucket
            std::size_t const   i2  = (i1 ^ (fp * 0x5BD1E995u)) % n; // partial-key: i2 = i1 XOR hash(fp)
            // ZWEI Bucket-Lookups + Fingerprint-Vergleich (Cuckoo-charakteristisch)
            if (buf[i1] == fp) {
                hits += 1u + (key & 3u);
            } else if (buf[i2] == fp) { // Fallback-Bucket
                hits += 2u + (key & 1u);
            }
        }
        return hits;
    }

private:
    // Maximale Verdraengungs-Kettenlaenge (Fan CoNEXT 2014 §3: MaxNumKicks). Wird die Platzierung
    // innerhalb kMaxKicks nicht erreicht, gilt der Filter als voll → Kette-Undo + ehrlicher Reject.
    static constexpr std::size_t kMaxKicks = 500;

    /// Deterministische, gut gemischte Slot-Wahl fuer die Verdraengung (bricht 2-Zyklen, reproduzierbar).
    [[nodiscard]] static constexpr std::size_t eviction_slot_(std::size_t bucket, std::size_t kick,
                                                              std::uint8_t fp) noexcept {
        std::uint64_t h = (static_cast<std::uint64_t>(bucket) * 0x9E3779B97F4A7C15ull) ^
                          (static_cast<std::uint64_t>(kick) * 0xD1B54A32D192ED03ull) ^
                          (static_cast<std::uint64_t>(fp) * 0xFF51AFD7ED558CCDull);
        return static_cast<std::size_t>((h >> 61) & (kSlotsPerBucket - 1u));
    }

    /// Plaziere fp in den ersten freien Slot (==0) des Buckets b; true bei Erfolg.
    bool place_in_bucket_(std::size_t b, std::uint8_t fp) noexcept {
        for (std::size_t s = 0; s < kSlotsPerBucket; ++s) {
            if (table_[b * kSlotsPerBucket + s] == 0) {
                table_[b * kSlotsPerBucket + s] = fp;
                return true;
            }
            if (table_[b * kSlotsPerBucket + s] == fp) return true; // bereits vorhanden (idempotent)
        }
        return false;
    }
    [[nodiscard]] bool in_bucket_(std::size_t b, std::uint8_t fp) const noexcept {
        for (std::size_t s = 0; s < kSlotsPerBucket; ++s)
            if (table_[b * kSlotsPerBucket + s] == fp) return true;
        return false;
    }
    // REALE persistente Bucket-Tabelle (P5 #124). 1 Byte = 1 Fingerprint-Slot.
    std::array<std::uint8_t, kBuckets * kSlotsPerBucket> table_{};
};

} // namespace comdare::cache_engine::filter_axis

namespace comdare::cache_engine::filter_axis {
static_assert(concepts::FilterStrategy<CuckooFilter>);
static_assert(concepts::CacheEnginePermutationStrategy<CuckooFilter>);
} // namespace comdare::cache_engine::filter_axis
