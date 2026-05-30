#pragma once
// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — SurfLoudsQuery + ComposedSurfLoudsFilter (echtes succinct LOUDS-Sparse-Filter-Organ).
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier) @paper P10 SuRF (Zhang et al. SIGMOD 2018)
//
// **Original-getreue Portierung** von LoudsSparse::lookupKey (louds_sparse.hpp Z.229-249) aus ext/traversal/P10-SuRF
// (Apache-2.0). is_original=false ([[pseudocode-papers-fallback]]). Identische 7-Methoden-Fassade wie S1
// (ComposedExactSurfFilter); S2 ersetzt nur Store+Query durch echtes LOUDS-Sparse+Suffix (FP>0, tunbar).
//
// **Kreuzbeleg:** S2.contains(k) >= S1.contains(k) (no-false-negative; S2 darf zusaetzlich bejahen = FP).
// **Tunbarkeit:** ST/HashLen/RealLen sind COMPILE-TIME-Parameter ([[compile-time-only]]); laengerer Suffix
// => weniger FP => hoehere bits_per_key. Default kReal/RealLen=8 (range-diskriminierend).

#include "surf_filter_organ_concept.hpp"
#include "surf_suffix_bits.hpp"
#include "louds_sparse_filter_store.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::filter::axis_filter::composable {

/// Statische Query-Logik ueber dem LOUDS-Sparse-Store.
struct SurfLoudsQuery {
    // Port LoudsSparse::lookupKey (louds_sparse.hpp Z.229-249), sparse-only (in_node_num=0).
    template <class Store>
    [[nodiscard]] static bool contains_in(Store const& s, std::uint64_t k) {
        if (s.empty()) return false;
        auto const kb = Store::to_bytes(k);
        std::uint32_t node = 0;
        std::uint32_t pos  = s.first_label_pos(node);
        unsigned level = 0;
        for (; level < 8; ++level) {
            if (!s.label_search(kb[level], pos, s.node_size(pos))) return false;
            if (!s.child_bit(pos)) return s.suffix_check_equality(s.suffix_pos(pos), kb, level + 1);
            node = s.child_node_num(pos);
            pos  = s.first_label_pos(node);
        }
        if (s.label_at(pos) == kSurfTerminator && !s.child_bit(pos))
            return s.suffix_check_equality(s.suffix_pos(pos), kb, level + 1);
        return false;
    }

    /// Konservatives no-FN Range: kleinster Key k* >= lo finden (per Trie-Descent), dessen Praefix-Unterschranke
    /// p_low <= hi. Da k* >= p_low und (bei existierendem Key in [lo,hi]) k* <= hi gilt, ist p_low<=hi notwendig
    /// erfuellt => KEINE Range-False-Negatives. Unentscheidbare Stellen werden grosszuegig (true) behandelt.
    template <class Store>
    [[nodiscard]] static bool range_may_exist_in(Store const& s, std::uint64_t lo, std::uint64_t hi) {
        if (s.empty()) return false;
        if (lo > hi) return false;
        std::array<std::uint8_t, 8> p_low{};
        if (!leftmost_ge(s, Store::to_bytes(lo), p_low)) return false;   // kein Key >= lo
        std::uint64_t const cand_low = bytes_to_uint(p_low);
        return cand_low <= hi;
    }

private:
    [[nodiscard]] static std::uint64_t bytes_to_uint(std::array<std::uint8_t, 8> const& b) noexcept {
        std::uint64_t v = 0;
        for (unsigned i = 0; i < 8; ++i) v = (v << 8) | b[i];   // b[0] = MSB (big-endian)
        return v;
    }

    // Leftmost-Descent ab einer Label-Position: haengt Labels an, bis ein Blatt erreicht ist; Rest = 0.
    template <class Store>
    static void reconstruct_leftmost(Store const& s, std::array<std::uint8_t, 8>& path, unsigned depth, std::uint32_t pos) {
        while (depth < 8) {
            path[depth] = s.label_at(pos);
            ++depth;
            if (!s.child_bit(pos)) return;                      // Blatt
            std::uint32_t const node = s.child_node_num(pos);
            pos = s.first_label_pos(node);
        }
    }

    // Kleinster Key >= lo: rekonstruiert seine fuehrenden Bytes (Rest 0) in `out`. false wenn keiner existiert.
    template <class Store>
    [[nodiscard]] static bool leftmost_ge(Store const& s, std::array<std::uint8_t, 8> const& lo,
                                          std::array<std::uint8_t, 8>& out) {
        out = {};
        struct Frame { std::uint32_t node_start; std::uint32_t node_size; std::uint32_t taken_pos; };
        std::array<Frame, 9> trail{};
        unsigned depth = 0;
        std::uint32_t node = 0;
        std::uint32_t node_start = s.first_label_pos(node);

        for (;;) {
            std::uint32_t const ns = s.node_size(node_start);
            std::uint32_t p = node_start;
            if (depth < 8 && s.label_search(lo[depth], p, ns)) {       // exakter Treffer lo[depth]
                out[depth] = lo[depth];
                if (!s.child_bit(p)) return true;                      // Blatt mit lo-Praefix => Kandidat
                trail[depth] = Frame{node_start, ns, p};
                node = s.child_node_num(p);
                node_start = s.first_label_pos(node);
                ++depth;
                continue;
            }
            // kein exaktes lo[depth]: kleinstes Label > lo[depth] in diesem Knoten
            std::uint32_t pg = node_start;
            std::uint8_t const probe = (depth < 8) ? lo[depth] : std::uint8_t{0};
            if (depth < 8 && s.label_search_greater_than(probe, pg, ns)) {
                reconstruct_leftmost(s, out, depth, pg);
                return true;
            }
            // Sackgasse: Backtrack zum Elternknoten, dort naechstgroesseres Geschwister
            for (;;) {
                if (depth == 0) return false;                          // kein Key >= lo
                --depth;
                out[depth] = 0;
                Frame const f = trail[depth];
                std::uint32_t pb = f.node_start;
                if (s.label_search_greater_than(s.label_at(f.taken_pos), pb, f.node_size)) {
                    reconstruct_leftmost(s, out, depth, pb);
                    return true;
                }
            }
        }
    }
};

/// KOMPOSITION: das echte succinct LOUDS-Sparse-Filter-Organ (S2). ST/HashLen/RealLen = Compile-Time-Tuning.
/// Default kReal/RealLen=8 (range-diskriminierend). Erfuellt SurfFilterOrgan (identisch zu S1).
template <SurfSuffixType ST = SurfSuffixType::kReal, unsigned HashLen = 0, unsigned RealLen = 8>
class ComposedSurfLoudsFilter {
public:
    using key_type = std::uint64_t;
    using store_type = LoudsSparseFilterStore<ST, HashLen, RealLen>;

    void build_from_sorted_keys(std::span<key_type const> sorted) { store_.build_from_sorted_keys(sorted); }
    [[nodiscard]] bool contains(key_type k)                      const { return SurfLoudsQuery::contains_in(store_, k); }
    [[nodiscard]] bool range_may_exist(key_type lo, key_type hi) const { return SurfLoudsQuery::range_may_exist_in(store_, lo, hi); }
    [[nodiscard]] std::size_t bit_size()     const noexcept { return store_.bit_size(); }
    [[nodiscard]] double      bits_per_key() const noexcept { return store_.bits_per_key(); }
    [[nodiscard]] std::size_t key_count()    const noexcept { return store_.key_count(); }
    void clear()                                   noexcept { store_.clear(); }
    [[nodiscard]] store_type const& store()  const noexcept { return store_; }

    // Pflicht-Properties (Habich-Fassade): is_original=false ([[pseudocode-papers-fallback]]).
    [[nodiscard]] static constexpr bool        is_original_module()  noexcept { return false; }
    [[nodiscard]] static constexpr const char* experiment_compiler() noexcept { return "self"; }

private:
    store_type store_{};
};

// Selbstbeweis: das LOUDS-Sparse-Filter-Organ erfuellt SurfFilterOrgan (mehrere Tuning-Punkte).
static_assert(SurfFilterOrgan<ComposedSurfLoudsFilter<SurfSuffixType::kReal, 0, 8>>);
static_assert(SurfFilterOrgan<ComposedSurfLoudsFilter<SurfSuffixType::kNone, 0, 0>>);

}  // namespace comdare::cache_engine::filter::axis_filter::composable
