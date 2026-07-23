#pragma once
// batch_planner.hpp -- G3 / #46b Lagerhaltung, Scheibe B5 (Ledger §62-B-NACHTRAG-2, B13/B15/B22).
//
// Der ASYNC Batch-Planer: EIN Producer-Thread konsolidiert ZU BEGINN ueber ALLE Binaries, erkennt
// jede fehlende Binary EINZELN (per-Binary-Miss gegen den Lager-Index), teilt die Fenster per
// PARITAET dieser Maschine zu (Gleichverteilung, B15/B16) und schiebt die betroffenen Slices in die
// Queue. Der Compile-Pool konsumiert ab der ersten Slice, waehrend der Planer weiterlaeuft (B22).
//
// ENTKOPPLUNG von B3: die Praesenz-Pruefung laeuft ueber ein PresencePredicate (index -> bool). Die
// Convenience-Bindung make_presence_predicate baut es aus einer Key-Menge (die vorhandenen
// key_sha512-Hex-Strings aus dem Bestandslog) + einer index->key-Abbildung -> keine Abhaengigkeit
// vom konkreten B3-Sha512Key-Typ.
//
// KORN: kGnBatchSlice=4096 spiegelt experiment_plan_director.hpp:439 (die Slice-Grenzen uebernimmt
// das Bestandslog 1:1 vom Planner-Takt). B13: Batch-Typ-Sequenz-Wache (planer_block->ceb->tier).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + slice_queue.hpp. Der
// Planer-Thread ist I/O-/Koordinations-Ebene (kein Hot-Path) -> std::function-Praedikat zulaessig.

#include "bestandslog_document.hpp" // BatchTyp
#include "slice_queue.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Korn-Konstante (spiegelt experiment_plan_director.hpp:439 kGnBatchSlice=4096).
inline constexpr std::uint64_t kGnBatchSlice = 4096;

// Praedikat: ist die Binary mit globalem Index i schon im Lager?
using PresencePredicate = std::function<bool(std::uint64_t index)>;

// Baut ein Praesenz-Praedikat aus einer Menge vorhandener key_sha512 + einer index->key-Abbildung
// (Membership gegen den Lager-Index; entkoppelt von B3s Sha512Key ueber die Hex-Strings).
[[nodiscard]] inline PresencePredicate make_presence_predicate(std::unordered_set<std::string>           present_keys,
                                                               std::function<std::string(std::uint64_t)> key_of) {
    return [present = std::move(present_keys), key_of = std::move(key_of)](std::uint64_t i) {
        return present.count(key_of(i)) != 0;
    };
}

// Paritaets-Zuteilung (Gleichverteilung): gehoert Fenster window_index dieser Maschine (rank von
// n_machines)? n_machines==0 -> alle (Ein-Maschinen-Fall).
[[nodiscard]] inline bool window_belongs_to(std::uint64_t window_index, unsigned rank, unsigned n_machines) noexcept {
    if (n_machines == 0) return true;
    return (window_index % n_machines) == rank;
}

// Einzeln-Miss-Erkennung in einem Fenster [begin, begin+count): die fehlenden globalen Indizes.
[[nodiscard]] inline std::vector<std::uint64_t> detect_missing_in_window(std::uint64_t begin, std::uint64_t count,
                                                                         PresencePredicate const& is_present) {
    std::vector<std::uint64_t> missing;
    for (std::uint64_t i = begin; i < begin + count; ++i)
        if (!is_present(i)) missing.push_back(i);
    return missing;
}

// B13 Batch-Typ-Sequenz-Wache: gueltige Bau-Ordnung planer_block(0) -> ceb(1) -> tier(2). Die
// Phasen-Raenge einer Sequenz muessen NICHT-fallend sein (kein Rueckfall in eine fruehere Phase).
[[nodiscard]] inline int type_phase_rank(BatchTyp t) noexcept {
    switch (t) {
        case BatchTyp::planer_block: return 0;
        case BatchTyp::ceb: return 1;
        case BatchTyp::tier: return 2;
    }
    return 0;
}
[[nodiscard]] inline bool is_valid_type_sequence(std::span<BatchTyp const> seq) noexcept {
    int prev = -1;
    for (BatchTyp t : seq) {
        int const r = type_phase_rank(t);
        if (r < prev) return false;
        prev = r;
    }
    return true;
}

// ---------------------------------------------------------------------------
// BatchPlanner -- EIN Producer-Thread. Laeuft ueber alle Fenster, filtert per Paritaet die eigenen,
// erkennt darin die fehlenden Binaries und schiebt nicht-leere Slices in die Queue; schliesst die
// Queue am Ende. Der Consumer konsumiert ab der ersten Slice (Start-vor-Fertig). Der dtor joined.
// ---------------------------------------------------------------------------
class BatchPlanner {
public:
    BatchPlanner(SliceQueue& q, std::uint64_t total_count, BatchTyp typ, unsigned rank, unsigned n_machines,
                 PresencePredicate is_present, std::uint64_t slice_grain = kGnBatchSlice)
        : q_{q}, total_{total_count}, typ_{typ}, rank_{rank}, n_machines_{n_machines},
          is_present_{std::move(is_present)}, grain_{slice_grain == 0 ? kGnBatchSlice : slice_grain} {
        worker_ = std::thread([this] { run(); });
    }

    BatchPlanner(BatchPlanner const&)            = delete;
    BatchPlanner& operator=(BatchPlanner const&) = delete;

    ~BatchPlanner() { join(); }

    void join() {
        if (worker_.joinable()) worker_.join();
    }

private:
    void run() {
        std::uint64_t const num_windows = (grain_ == 0) ? 0 : (total_ + grain_ - 1) / grain_;
        for (std::uint64_t w = 0; w < num_windows; ++w) {
            if (!window_belongs_to(w, rank_, n_machines_)) continue;
            std::uint64_t const begin   = w * grain_;
            std::uint64_t const count   = std::min(grain_, total_ - begin);
            auto                missing = detect_missing_in_window(begin, count, is_present_);
            if (!missing.empty()) q_.push(BatchSlice{begin, count, typ_, std::move(missing)});
        }
        q_.close();
    }

    SliceQueue&       q_;
    std::uint64_t     total_;
    BatchTyp          typ_;
    unsigned          rank_;
    unsigned          n_machines_;
    PresencePredicate is_present_;
    std::uint64_t     grain_;
    std::thread       worker_;
};

} // namespace comdare::cache_engine::builder::bestandslog
