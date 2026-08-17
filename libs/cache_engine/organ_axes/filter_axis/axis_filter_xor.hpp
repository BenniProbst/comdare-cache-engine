#pragma once
// V41.F.6.1.R7.5.e axis_filter XorFilter (Graf/Lemire 2020)
//
// R7.6 Paper-Reference (Task #723):
// Graf, T.M., Lemire, D. "Xor Filters: Faster and Smaller Than Bloom and
// Cuckoo Filters." ACM Journal of Experimental Algorithmics (JEA), Vol. 25,
// Article No. 1.5, March 2020.
// DOI: 10.1145/3376122
// URL (Preprint): https://arxiv.org/abs/1912.08258
// Code: https://github.com/FastFilter/fastfilter_cpp (Apache-2.0 License)
// Original-Code-File: src/xorfilter/xorfilter.h

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <organ_axes/filter_axis/axis_filter_flags.hpp>
#include <topics/filter/concepts/topic_filter_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::filter_axis {

/// XorFilter — Graf/Lemire 2020 (JEA, Apache-2.0 License).
/// XOR-Hash-Based Filter mit kleinerer Space-Bound als Bloom (~9 bits/key
/// statt ~10 bits/key bei gleicher false-positive-Rate). Immutable nach Build.
class XorFilter : public FilterStrategyBase<XorFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::error_profile_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::xor_enabled;

    // -- OFFLINE-PEELING-KONSTRUKTION (17.08.2026, Owner-Anordnung 16.08.2026) ------------------------------
    // ECHTE Xor-Filter-Konstruktion nach Graf+Lemire 2020 (JEA 25, Art. 1.5, Alg. 2/3): der Filter wird
    // OFFLINE aus dem VOLLEN Key-Satz gebaut. 3-partiter 3-Hypergraph ueber die drei disjunkten
    // Tabellen-Drittel; das Peeling loest wiederholt Slots mit Grad 1, die Zuweisung fuellt die
    // Peel-Reihenfolge RUECKWAERTS, sodass fuer JEDEN gebauten Key die Xor-Invariante gilt:
    //     fp(key) == B[h0] XOR B[h1] XOR B[h2]  (GENAU 3 Slot-Reads + 3-fach-XOR, branch-frei, Paper P.3).
    // Da die Apparat-Schnittstelle inkrementell ist (abi_adapter::tier_insert -> insert_key je Key), haelt
    // der Filter den vollen Key-Satz als sortiert-unique Bau-Eingabe (keys_/key_count_, analog dem
    // build_from_sorted_keys-Muster der composable Nachbar-Organe) und peelt nach jedem aufgenommenen Key
    // neu -- die Bau-Op ist Setup (NICHT gemessen), probe_key bleibt exakt 3 Slot-Reads. Commit-on-Success:
    // gepeelt wird in einen transienten Scratch (Praezedenz: NodeChunkedStore flatten_/organ_observe_filter-
    // Streupuffer); erst ein VOLLSTAENDIG gepeelter Satz ersetzt table_/seed_. Schlaegt das Peeling fuer
    // alle deterministischen Seed-Versuche fehl ODER ist die Kapazitaet erreicht, wird der NEUE Key ehrlich
    // NICHT aufgenommen und table_ bleibt unveraendert gueltig fuer den Altbestand (ehrliche
    // Kapazitaetsgrenze, exakt das CuckooFilter-Muster kMaxKicks/LIFO-Undo). KEIN Zufall: die Seed-Folge
    // ist deterministisch (Hausdoktrin, Zufall nur fuer Koeder). kBytes=12288 -> 3x4096 Slots, L1-resident
    // (messneutral, unveraendert). Voll-uint64-Key-Domain. Die Key-Puffer-Arrays sind BAU-Zustand (Offline-
    // Bau-Eingabe + Memento-Reproduzierbarkeit), NICHT Teil der Probe-Struktur -- filter_bit_capacity()
    // deklariert weiterhin die Slot-Tabelle (wie Bloom/Cuckoo die ihre).
    //
    // -- POSTEN 78: GEHEILT (17.08.2026; Historie ehrlich stehen gelassen) ----------------------------------
    // Bis zum 16.08.2026 trug diese Datei eine vereinfachte inkrementelle XOR-Konstruktion (insert_key
    // loeste nur den dritten Slot: B[h2] = fp XOR B[h0] XOR B[h1], OHNE Peeling): der S5-02b-Scrub hatte
    // GEMESSEN, dass sie ECHTE FALSE NEGATIVES erzeugt -- 30 von 256 gespeicherten Keys wurden als
    // "definitiv nicht enthalten" gemeldet (damals gepinnt in tests/unit/test_s5_02b_filter_perf_sanity.cpp,
    // Block (2) T14, Zeile "T14 BEFUND GEPINNT"). Der Owner-Entscheid A3 vom 05.08.2026 ("78 behalten" plus
    // Doku-Pflicht) ist am 16.08.2026 durch direkte Owner-Anordnung UEBERSCHRIEBEN worden ("Bitte direkt
    // (b) bauen, wir machen es gleich richtig"): dieser Umbau ERSETZT den vereinfachten Pfad durch das
    // echte Offline-Peeling. Die GEPINNT-Zeile der Wache ist damit kontrolliert AUFGELOEST; XorFilter
    // steht seither MIT in der neg==0-Kompositions-Wache (die no-FN-Zusage der Filter-Achse gilt wieder
    // strukturell). Die fruehere OWNER-AUFLAGE (FN-Eigenschaft bei jeder Messwert-Verwendung ausweisen)
    // entfaellt fuer Lasten INNERHALB der Kapazitaet; auszuweisen bleibt allein die ehrliche
    // Kapazitaetsgrenze: ab mehr als kKeyCapacity DISTINKTEN Keys werden weitere Keys ehrlich abgewiesen
    // (wie beim vollen CuckooFilter) und proben dann negativ -- fuer Ueberlast-Messreihen ist DAS
    // auszuweisen.
    static constexpr std::size_t kThird = 4096;        // Slots je Drittel (Zweierpotenz)
    static constexpr std::size_t kSlots = 3u * kThird; // 12 KiB Gesamt-Tabelle
    // Ehrliche Peel-Kapazitaet der FIXEN Tabelle: Graf+Lemire dimensionieren 1.23*n Slots fuer n Keys
    // (size factor c=1.23, Paper P.4); bei fixem kSlots ist n_max = kSlots/1.23 = 9990 distinkte Keys.
    static constexpr std::size_t kKeyCapacity = (kSlots * 100u) / 123u; // = 9990
    // Peel-Wiederholungen mit deterministischer Seed-Folge (Paper: neuer Seed bei Peel-Fehlschlag).
    static constexpr std::uint32_t kMaxBuildAttempts = 16;

    [[nodiscard]] static constexpr std::uint8_t fingerprint_(std::uint64_t key) noexcept {
        return static_cast<std::uint8_t>((key * 0x9E3779B97F4A7C15ull >> 40) ^ 0xA5u);
    }
    /// Deterministischer Seed je Peel-Versuch (ungerade Vielfache der splitmix64-Konstante, kein Zufall).
    [[nodiscard]] static constexpr std::uint64_t seed_for_attempt_(std::uint32_t attempt) noexcept {
        return 0x9E3779B97F4A7C15ull * (2ull * attempt + 1ull);
    }
    /// splitmix64-Finalizer ueber (key, seed) -- EIN Mix je Key und Op, daraus die drei Slot-Fenster.
    [[nodiscard]] static constexpr std::uint64_t mix_(std::uint64_t key, std::uint64_t seed) noexcept {
        std::uint64_t h = key + seed;
        h               = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ull;
        h               = (h ^ (h >> 27)) * 0x94D049BB133111EBull;
        return h ^ (h >> 31);
    }
    // Drei 12-Bit-Fenster EINES Mixes in die drei disjunkten Drittel (fastfilter_cpp-Fenstermuster).
    [[nodiscard]] static constexpr std::size_t h0_(std::uint64_t mixed) noexcept {
        return static_cast<std::size_t>(mixed & (kThird - 1u));
    }
    [[nodiscard]] static constexpr std::size_t h1_(std::uint64_t mixed) noexcept {
        return kThird + static_cast<std::size_t>((mixed >> 21) & (kThird - 1u));
    }
    [[nodiscard]] static constexpr std::size_t h2_(std::uint64_t mixed) noexcept {
        return 2u * kThird + static_cast<std::size_t>((mixed >> 42) & (kThird - 1u));
    }

    /// Build-Op (Setup, NICHT gemessen): nimmt den Key in den sortiert-unique Bau-Satz auf und peelt den
    /// Filter offline NEU (Graf+Lemire Alg. 2/3). Idempotent bei Duplikaten. Ehrlicher Reject bei voller
    /// Kapazitaet oder Peel-Fehlschlag (table_ bleibt dann unveraendert gueltig fuer den Altbestand).
    void insert_key(std::uint64_t key) noexcept {
        std::size_t lo = 0, hi = key_count_;
        while (lo < hi) {
            std::size_t const mid = lo + (hi - lo) / 2u;
            if (keys_[mid] < key)
                lo = mid + 1u;
            else
                hi = mid;
        }
        if (lo < key_count_ && keys_[lo] == key) return; // bereits vorhanden (idempotent, wie Cuckoo)
        if (key_count_ == kKeyCapacity) return;          // ehrliche Kapazitaetsgrenze (wie Cuckoo voll)
        for (std::size_t i = key_count_; i > lo; --i) keys_[i] = keys_[i - 1u]; // sortiert einfuegen
        keys_[lo] = key;
        ++key_count_;
        if (!rebuild_peeled_()) { // Peel-Fehlschlag ueber alle Seeds -> Key ehrlich zuruecknehmen
            for (std::size_t i = lo; i + 1u < key_count_; ++i) keys_[i] = keys_[i + 1u];
            --key_count_;
        }
    }

    /// Probe der REALEN Tabelle: GENAU 3 Slot-Reads + 3-fach-XOR == Fingerprint (branch-frei, Paper P.3).
    [[nodiscard]] bool probe_key(std::uint64_t key) const noexcept {
        std::uint64_t const m = mix_(key, seed_);
        std::uint8_t const  x = static_cast<std::uint8_t>(table_[h0_(m)] ^ table_[h1_(m)] ^ table_[h2_(m)]);
        return x == fingerprint_(key);
    }

    void clear() noexcept {
        table_.fill(0);
        key_count_ = 0;
        seed_      = seed_for_attempt_(0);
    }
    /// Zustands-Vergleich (Memento-Vertrag): Tabelle + Seed + Bau-Key-Satz (nur der belegte Praefix).
    [[nodiscard]] bool operator==(XorFilter const& o) const noexcept {
        if (key_count_ != o.key_count_ || seed_ != o.seed_ || table_ != o.table_) return false;
        for (std::size_t i = 0; i < key_count_; ++i)
            if (keys_[i] != o.keys_[i]) return false;
        return true;
    }

    /// Probe-Multiplizitaet je Query (3 Slot-Reads) — ehrlich deklariert fuer hash_probes_total.
    [[nodiscard]] static constexpr std::uint64_t probe_multiplicity() noexcept { return 3u; }

    /// SPACE-Seite des T16-Kern-Trade-offs: strukturelles Bit-Budget der REALEN Slot-Tabelle (kSlots 1-Byte-Slots =
    /// sizeof(table_)*8), compile-time. Static-constexpr-Descriptor analog probe_multiplicity() — kein Datenmember,
    /// kein neuer Achsenwert, kein POD-Feld (golden-320/Gate-1/1272 neutral).
    [[nodiscard]] static constexpr std::size_t filter_bit_capacity() noexcept { return kSlots * 8u; }
    /// bits/key bei key_count Keys — spiegelt composable/ bits_per_key() (key_count==0 → 0.0).
    [[nodiscard]] static constexpr double bits_per_key(std::size_t key_count) noexcept {
        return key_count ? static_cast<double>(filter_bit_capacity()) / static_cast<double>(key_count) : 0.0;
    }

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "filter_xor"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::filter_axis::XorFilter",
                                  "organ_axes/filter_axis/axis_filter_xor.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "XorFilter (Graf+Lemire 2020, ~9 bits/key, smaller than Bloom)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "XOR"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    /// BUMP 17.08.2026 (1.0.0.c -> 1.1.0.c): Offline-Peeling-Konstruktion (Graf+Lemire Alg. 2/3) ersetzt
    /// die vereinfachte inkrementelle XOR-Konstruktion; Slot-Fenster seeded (splitmix64), no-FN bis
    /// kKeyCapacity distinkte Keys (Owner-Anordnung 16.08.2026, POSTEN 78 geheilt).
    static constexpr std::string_view algo_version = "1.1.0.c";

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEINE persistente XOR-Tabelle, daher wird `buf` als Pseudo-Fingerprint-
    // Tabelle (1 Byte/Slot) der Länge `n` interpretiert. Der Aufwand ist REAL und strategie-abhängig
    // (Xor-Filter nach Graf+Lemire 2020 §3: Lookup = fp == B[h0(x)] XOR B[h1(x)] XOR B[h2(x)] — GENAU 3
    // unabhängige Tabellenzugriffe + 3-fach-XOR, KEIN früher Abbruch), KEINE erfundene Zahl.
    // Charakteristisch vs Bloom/Cuckoo: immer exakt 3 (branch-freie) Slot-Reads → andere, gleichmäßige
    // Latenz-Signatur ohne datenabhängigen early-exit. Punkt-Query only, immutable nach Build.
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n < 3) return 0; // Xor-Filter braucht 3 disjunkte Tabellen-Drittel
        std::uint64_t     hits  = 0;
        std::size_t const third = n / 3u;         // garantiert >= 1 (n >= 3), Rest fällt in Drittel 3
        std::size_t const rest  = n - 2u * third; // Drittel 3 = Rest (>= third >= 1)
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];
            std::uint8_t const  fp  = static_cast<std::uint8_t>(key * 0x9Du ^ 0xA5u); // 1-Byte-Fingerprint
            // 3 unabhängige Hash-Positionen (Graf+Lemire: drei disjunkte Tabellen-Drittel)
            std::size_t const h0 = (key * 2654435761u) % third;
            std::size_t const h1 = third + ((key * 40503u) % third);
            std::size_t const h2 = 2u * third + ((key * 0x85EBCA6Bu) % rest);
            // GENAU 3 Slot-Reads + 3-fach-XOR (branch-frei, kein early-exit — Xor-charakteristisch)
            std::uint8_t const x = static_cast<std::uint8_t>(buf[h0] ^ buf[h1] ^ buf[h2]);
            hits += (x == fp) ? (1u + (key & 7u)) : 0u; // order-sensitiv
        }
        return hits;
    }

private:
    // -- Offline-Peeling (Graf+Lemire 2020 Alg. 2/3), transienter Bau-Scratch -------------------------------
    // Der Scratch ist TRANSIENT (lebt nur waehrend insert_key, heap-alloziert nothrow) -- KEIN Organ-
    // Zustand (Praezedenz: NodeChunkedStore flatten_/organ_observe_filter-Streupuffer am Default-
    // Allokator). Bei Alloc-Fehlschlag wird der Insert ehrlich abgewiesen (kein Terminate, kein stiller
    // Datenverlust; table_ bleibt gueltig fuer den Altbestand).
    struct PeelScratch {
        std::array<std::uint16_t, kSlots>       count;      // Grad je Slot (n <= kKeyCapacity < 65536)
        std::array<std::uint64_t, kSlots>       key_xor;    // XOR der inzidenten Keys je Slot
        std::array<std::uint16_t, kSlots>       queue;      // Slots mit Grad 1 (jeder Slot <= 1x enqueued)
        std::array<std::uint64_t, kKeyCapacity> order_key;  // Peel-Reihenfolge: geloester Key
        std::array<std::uint16_t, kKeyCapacity> order_slot; // Peel-Reihenfolge: geloester Slot
        std::array<std::uint8_t, kSlots>        tab;        // Bau-Ziel (Commit erst bei Voll-Erfolg)
    };

    /// Peelt den vollen Key-Satz mit deterministischer Seed-Folge; Commit von table_/seed_ NUR bei
    /// Voll-Erfolg eines Versuchs. false == kein Versuch hat alle Keys gepeelt (oder OOM am Scratch).
    [[nodiscard]] bool rebuild_peeled_() noexcept {
        PeelScratch* const sc = new (std::nothrow) PeelScratch;
        if (sc == nullptr) return false; // OOM -> ehrlicher Reject des Inserts
        bool ok = false;
        for (std::uint32_t attempt = 0; attempt < kMaxBuildAttempts && !ok; ++attempt) {
            std::uint64_t const seed = seed_for_attempt_(attempt);
            ok                       = try_peel_(seed, *sc);
            if (ok) {
                table_ = sc->tab;
                seed_  = seed;
            }
        }
        delete sc;
        return ok;
    }

    /// EIN Peel-Versuch (Paper Alg. 2: Grad-1-Peeling; Alg. 3: Rueckwaerts-Zuweisung). true == ALLE
    /// key_count_ Keys gepeelt und sc.tab traegt die fertige Tabelle fuer diesen Seed.
    [[nodiscard]] bool try_peel_(std::uint64_t seed, PeelScratch& sc) const noexcept {
        sc.count.fill(0);
        sc.key_xor.fill(0);
        for (std::size_t i = 0; i < key_count_; ++i) {
            std::uint64_t const k = keys_[i];
            std::uint64_t const m = mix_(k, seed);
            // drei Slots in DISJUNKTEN Dritteln -> nie identisch (3-partiter Hypergraph).
            std::size_t const e[3] = {h0_(m), h1_(m), h2_(m)};
            for (std::size_t const s : e) {
                ++sc.count[s];
                sc.key_xor[s] ^= k;
            }
        }
        // Grad faellt monoton -> jeder Slot passiert "==1" hoechstens einmal -> queue[kSlots] reicht.
        std::size_t qt = 0;
        for (std::size_t s = 0; s < kSlots; ++s)
            if (sc.count[s] == 1u) sc.queue[qt++] = static_cast<std::uint16_t>(s);
        std::size_t peeled = 0;
        for (std::size_t qh = 0; qh < qt; ++qh) {
            std::size_t const s = sc.queue[qh];
            if (sc.count[s] != 1u) continue;       // inzwischen Grad 0 (stale Queue-Eintrag)
            std::uint64_t const k = sc.key_xor[s]; // Grad 1 -> key_xor traegt exakt DEN einen Key
            sc.order_key[peeled]  = k;
            sc.order_slot[peeled] = static_cast<std::uint16_t>(s);
            ++peeled;
            std::uint64_t const m    = mix_(k, seed);
            std::size_t const   e[3] = {h0_(m), h1_(m), h2_(m)};
            for (std::size_t const t : e) { // Kante k aus allen drei Slots entfernen
                --sc.count[t];
                sc.key_xor[t] ^= k;
                if (sc.count[t] == 1u && qt < kSlots) sc.queue[qt++] = static_cast<std::uint16_t>(t);
            }
        }
        if (peeled != key_count_) return false; // 2-Kern uebrig -> Fehlschlag, Aufrufer versucht neuen Seed
        sc.tab.fill(0);
        for (std::size_t i = peeled; i-- > 0;) { // RUECKWAERTS fuellen (Alg. 3)
            std::uint64_t const m = mix_(sc.order_key[i], seed);
            std::size_t const   s = sc.order_slot[i]; // je Peel-Schritt eindeutig, sc.tab[s] ist noch 0
            sc.tab[s] = static_cast<std::uint8_t>(fingerprint_(sc.order_key[i]) ^ sc.tab[h0_(m)] ^ sc.tab[h1_(m)] ^
                                                  sc.tab[h2_(m)]);
        }
        return true;
    }

    // REALE persistente XOR-Fingerprint-Tabelle (Probe-Struktur, P5 #124). 3 disjunkte Drittel, 1 Byte/Slot.
    std::array<std::uint8_t, kSlots> table_{};
    // Sortiert-unique Bau-Eingabe (voller Key-Satz fuer den Offline-Bau + Memento-Reproduzierbarkeit).
    std::array<std::uint64_t, kKeyCapacity> keys_{};
    std::size_t                             key_count_ = 0;
    // Seed des zuletzt ERFOLGREICH gepeelten Baus (deterministische Folge, kein Zufall).
    std::uint64_t seed_ = seed_for_attempt_(0);
};

} // namespace comdare::cache_engine::filter_axis

namespace comdare::cache_engine::filter_axis {
static_assert(concepts::FilterStrategy<XorFilter>);
static_assert(concepts::CacheEnginePermutationStrategy<XorFilter>);
} // namespace comdare::cache_engine::filter_axis
