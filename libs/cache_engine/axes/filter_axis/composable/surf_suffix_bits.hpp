#pragma once
// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — Suffix-Organ: der FP/bits_per_key-Tuning-Kern.
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier) @paper P10 SuRF (Zhang et al. SIGMOD 2018)
//
// **Original-getreue Portierung** der Suffix-Semantik aus ext/traversal/P10-SuRF/SuRF/include/{suffix,hash}.hpp
// (Apache-2.0). is_original=false ([[pseudocode-papers-fallback]]). Reine portable uint-Arithmetik (leveldb-Hash).
//
// **Tuning-Kern (Saeule-2):** suffix_len = HashLen+RealLen ist ein COMPILE-TIME-Template-Parameter
// ([[compile-time-only]], kein Runtime-Switch). Laengerer Suffix => weniger False-Positives, hoehere
// bits_per_key. Das EINZIGE FP-Tor ist check_equality() und es ist EINSEITIG (gibt fuer einen gespeicherten
// Key NIE false) => strukturelles no-FN.
//
// **Bit-Packing (selbst-konsistent):** append_bits/read_bits sind MSB-first und zueinander invers
// (read(store(x))==x). Das genuegt fuer no-FN (stored==querying fuer echten Key); es wird NICHT die exakte
// Cross-Word-Bitlage von suffix.hpp Z.198-200 reproduziert (dort ein subtiler, hier vermiedener Shift-Sonderfall).
//
// A8-S5 Familie 02b (2026-08-04) -- SCHNITT-REGEL (Dossier 20260803-a8_f2 Abschn. 3.4): der Suffix-Bit-Puffer
// lief bis hierher ueber den Default-Allokator. Seit dem Schnitt haengt er an der Strategie-Instanz des
// besitzenden Organs (LoudsSparseFilterStore), die im Konstruktor hereingereicht wird. Die Suffix-SEMANTIK
// ist unberuehrt: construct_real/construct_hash/check_equality/compare sind reine uint-Arithmetik ohne
// Speicherbezug (allesamt static bzw. nur lesend), die Zeilen-Anker in suffix.hpp/hash.hpp sitzen unveraendert
// an ihren Rumpf-Zeilen. Das EINSEITIGE no-FN-Tor (check_equality) bleibt Bit fuer Bit dasselbe.

#include "surf_axis_allocator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::filter_axis::composable {

enum class SurfSuffixType : std::uint8_t { kNone = 0, kHash = 1, kReal = 2, kMixed = 3 };

inline constexpr int kSurfHashShift       = 7;    // config.hpp Z.26
inline constexpr int kSurfCouldBePositive = 2018; // config.hpp Z.28

// leveldb-Hash (hash.hpp Z.17-46), reine uint32-Arithmetik (voll portabel).
[[nodiscard]] inline std::uint32_t surf_leveldb_hash(std::uint8_t const* data, std::size_t n,
                                                     std::uint32_t seed) noexcept {
    constexpr std::uint32_t m     = 0xc6a4a793U;
    constexpr std::uint32_t r     = 24U;
    std::uint8_t const*     limit = data + n;
    std::uint32_t           h     = seed ^ static_cast<std::uint32_t>(n * m);
    while (data + 4 <= limit) {
        std::uint32_t w = static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) |
                          (static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24);
        data += 4;
        h += w;
        h *= m;
        h ^= (h >> 16);
    }
    switch (limit - data) {
        case 3: h += static_cast<std::uint32_t>(data[2]) << 16; [[fallthrough]];
        case 2: h += static_cast<std::uint32_t>(data[1]) << 8; [[fallthrough]];
        case 1:
            h += static_cast<std::uint32_t>(data[0]);
            h *= m;
            h ^= (h >> r);
            break;
        default: break;
    }
    return h;
}

/// Suffix-Bit-Container + Konstruktion + einseitiges no-FN-Gleichheits-Tor.
/// Keys sind 8-Byte-Big-Endian (std::array<uint8_t,8>); `level` = Byte-Tiefe im Trie.
template <SurfSuffixType ST, unsigned HashLen, unsigned RealLen>
class SurfSuffixBits {
public:
    static constexpr unsigned kSuffixLen = HashLen + RealLen;
    static_assert(kSuffixLen <= 64, "suffix_len <= 64");
    static_assert(ST != SurfSuffixType::kNone || kSuffixLen == 0, "kNone => 0 Suffix-Bits");

    using key_bytes_t = std::array<std::uint8_t, 8>;

    /// A8-S5-02b Form-B-Ausweis: der Bit-Puffer laeuft ueber die Allokator-Achse.
    using allocator_type = filter_surf_allocator_t;

    /// EINZIGER Weg, einen Suffix-Container anzulegen: mit der Strategie-Instanz des besitzenden Organs
    /// (der Adapter ist nicht default-konstruierbar -> kein stilles Zurueckfallen auf einen Default-Allokator).
    explicit SurfSuffixBits(allocator_type& strat) : bits_(strat.template as_std_allocator<std::uint64_t>()) {}
    /// ALLOKATOR-ERWEITERTE Kopie: rebindet auf die Strategie-Instanz des ZIEL-Organs; die gewoehnliche
    /// Kopie ist geloescht, weil sie den Adapter der QUELLE mitschleppte (dangling, sobald die Quelle stirbt).
    SurfSuffixBits(SurfSuffixBits const& o, allocator_type& strat)
        : bits_(o.bits_, strat.template as_std_allocator<std::uint64_t>()), num_bits_(o.num_bits_) {}
    SurfSuffixBits(SurfSuffixBits const&) = delete;
    /// Kopier-ZUWEISUNG bleibt zulaessig und sicher (POCCA am Adapter false -> das Ziel behaelt seinen eigenen).
    SurfSuffixBits& operator=(SurfSuffixBits const&) = default;
    ~SurfSuffixBits()                                = default;

    // constructRealSuffix (suffix.hpp Z.43-65): RealLen Bits ab Byte `level`; 0 wenn Key zu kurz.
    [[nodiscard]] static std::uint64_t construct_real(key_bytes_t const& kb, unsigned level) noexcept {
        if (level > 8U || (8U - level) * 8U < RealLen) return 0;
        std::uint64_t  suffix             = 0;
        unsigned const num_complete_bytes = RealLen / 8U;
        if (num_complete_bytes > 0) {
            suffix = kb[level];
            for (unsigned i = 1; i < num_complete_bytes; ++i) {
                suffix <<= 8;
                suffix += kb[level + i];
            }
        }
        unsigned const offset = RealLen % 8U;
        if (offset > 0) {
            suffix <<= offset;
            std::uint64_t remaining = kb[level + num_complete_bytes];
            remaining >>= (8U - offset);
            suffix += remaining;
        }
        return suffix;
    }

    // constructHashSuffix (suffix.hpp Z.36-41): HashLen Bits aus dem Hash des GANZEN Keys.
    [[nodiscard]] static std::uint64_t construct_hash(key_bytes_t const& kb) noexcept {
        if (HashLen == 0) return 0;
        std::uint64_t suffix = surf_leveldb_hash(kb.data(), 8, 0xbc9f1d34U);
        suffix <<= (64U - HashLen - kSurfHashShift);
        suffix >>= (64U - HashLen);
        return suffix;
    }

    [[nodiscard]] static std::uint64_t construct_suffix(key_bytes_t const& kb, unsigned level) noexcept {
        if constexpr (ST == SurfSuffixType::kHash)
            return construct_hash(kb);
        else if constexpr (ST == SurfSuffixType::kReal)
            return construct_real(kb, level);
        else if constexpr (ST == SurfSuffixType::kMixed)
            return (construct_hash(kb) << RealLen) | construct_real(kb, level);
        else
            return 0; // kNone
    }

    // BUILD: einen Suffix in Speicher-Reihenfolge anhaengen (== Leaf-Reihenfolge des Trie).
    void append(key_bytes_t const& kb, unsigned level) {
        if constexpr (ST == SurfSuffixType::kNone) {
            (void)kb;
            (void)level;
            return;
        } else {
            append_bits(construct_suffix(kb, level), kSuffixLen);
        }
    }
    // BUILD: einen bereits konstruierten Suffix-Word anhaengen (Store legt level-major flach).
    void append_word(std::uint64_t w) {
        if constexpr (ST == SurfSuffixType::kNone) {
            (void)w;
            return;
        } else {
            append_bits(w, kSuffixLen);
        }
    }

    [[nodiscard]] std::uint64_t read(std::size_t idx) const noexcept {
        if constexpr (ST == SurfSuffixType::kNone) {
            (void)idx;
            return 0;
        } else {
            if (static_cast<std::uint64_t>(idx) * kSuffixLen >= num_bits_) return 0;
            return read_bits(static_cast<std::uint64_t>(idx) * kSuffixLen, kSuffixLen);
        }
    }

    // checkEquality (suffix.hpp Z.208-227) — EINSEITIGES no-FN-Tor: fuer einen gespeicherten Key NIE false.
    [[nodiscard]] bool check_equality(std::size_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        if constexpr (ST == SurfSuffixType::kNone) {
            (void)idx;
            (void)kb;
            (void)level;
            return true;
        } else {
            if (static_cast<std::uint64_t>(idx) * kSuffixLen >= num_bits_) return false;
            std::uint64_t const stored = read(idx);
            if constexpr (ST == SurfSuffixType::kReal) {
                // POSTEN 77 / OWNER-ENTSCHEID A2 (Ledger-Nachtrag 05.08.2026 abend-10, Owner-Antwort A2
                // VERBATIM: "Freigabe wie empfohlen."; Befund gepinnt im S5-02b-Scrub, siehe
                // tests/unit/test_s5_02b_filter_perf_sanity.cpp Block (4) sowie
                // tests/unit/test_v41_axis_filter_surf_louds_organ.cpp, dort konkret als S2R16 =
                // ComposedSurfLoudsFilter<kReal, HashLen=0, RealLen=16> gefuehrt): fuer KURZE Keys (Key
                // traegt weniger als RealLen Bit ab `level`) liefert construct_real() oben 0 zurueck, und die
                // naechste Zeile behandelt stored==0 als "Key zu kurz fuer Suffix-Info" -> check_equality()
                // liefert dann unbedingt true. Auf solchen kurzen Keys KOLLABIERT der Suffix-Typ kReal --
                // gleich welcher RealLen (8, 16, ...) -- faktisch auf das Verhalten von kNone (das ebenfalls
                // immer true liefert, s.o.). Die S5-02b-Papertreue-Probe hat ALT==NEU bit-identisch
                // nachgewiesen (272.961 Antworten) -- KEIN Regress, reiner Bestandsbefund. PIN (keine
                // Verhaltensaenderung ohne neue Owner-Entscheidung), aber MESS-INTERPRETATIONS-HINWEIS
                // PFLICHT: bits_per_key-/FP-Raten-Messungen mit hoeherem RealLen (z.B. 16) auf kurzen Keys
                // unterschaetzen die tatsaechliche Diskriminierungs-Wirkung eines laengeren Suffixes, weil ein
                // Teil der Keys unbemerkt auf diesen Null-Suffix-Pfad faellt.
                if (stored == 0) return true;                                // Key zu kurz fuer Suffix-Info
                if (level > 8U || (8U - level) * 8U < RealLen) return false; // Query kuerzer als gespeicherter Key
            }
            return stored == construct_suffix(kb, level);
        }
    }

    // compare (suffix.hpp Z.249-267) — fuer Range-no-FN: kNone/kHash -> kCouldBePositive (immer „koennte"),
    // kReal/kMixed -> echter Vergleich. Wir geben die SuRF-Semantik zurueck (Range-Filter bleibt konservativ).
    [[nodiscard]] int compare(std::size_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        if constexpr (ST == SurfSuffixType::kNone || ST == SurfSuffixType::kHash) {
            (void)idx;
            (void)kb;
            (void)level;
            return kSurfCouldBePositive;
        } else {
            if (static_cast<std::uint64_t>(idx) * kSuffixLen >= num_bits_) return kSurfCouldBePositive;
            std::uint64_t       stored   = read(idx);
            std::uint64_t const querying = construct_real(kb, level);
            if constexpr (ST == SurfSuffixType::kMixed) stored &= ((RealLen >= 64) ? ~0ULL : ((1ULL << RealLen) - 1));
            if (stored == 0 && querying == 0) return kSurfCouldBePositive;
            if (stored == 0 || stored < querying) return -1;
            if (stored == querying) return kSurfCouldBePositive;
            return 1;
        }
    }

    [[nodiscard]] std::size_t bit_size() const noexcept { return num_bits_; }
    [[nodiscard]] std::size_t count() const noexcept { return (kSuffixLen == 0) ? 0 : (num_bits_ / kSuffixLen); }
    void                      clear() noexcept {
        bits_.clear();
        num_bits_ = 0;
    }

private:
    void append_bits(std::uint64_t value, unsigned len) {
        for (unsigned i = 0; i < len; ++i) {
            std::uint64_t const bit = (value >> (len - 1 - i)) & 1ULL;
            std::uint64_t const p   = num_bits_ + i;
            if (p / 64 >= bits_.size()) bits_.push_back(0);
            if (bit) bits_[p / 64] |= (0x8000000000000000ULL >> (p % 64)); // MSB-first
        }
        num_bits_ += len;
    }
    [[nodiscard]] std::uint64_t read_bits(std::uint64_t base, unsigned len) const noexcept {
        std::uint64_t result = 0;
        for (unsigned i = 0; i < len; ++i) {
            std::uint64_t const p   = base + i;
            std::uint64_t const bit = (bits_[p / 64] >> (63 - (p % 64))) & 1ULL;
            result                  = (result << 1) | bit;
        }
        return result;
    }

    // A8-S5-02b: der Bit-Puffer haengt an der Achse (surf_word_vec_t == std::vector<uint64,Achsen-Adapter>).
    surf_vec_t<std::uint64_t> bits_;
    std::uint64_t             num_bits_ = 0;
};

} // namespace comdare::cache_engine::filter_axis::composable
