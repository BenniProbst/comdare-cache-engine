#pragma once
// slice_queue.hpp -- G3 / #46b Lagerhaltung, Scheibe B5 (Ledger §62-B-NACHTRAG-2, B22).
//
// Producer-Consumer-Queue der Batch-Slices (mutex+cv+deque, Muster AsyncPushPump). Der async
// Batch-Planer (Producer, batch_planner.hpp) fuellt sie ueber ALLE Fenster; der Compile-Pool
// (Consumer) konsumiert ab der ERSTEN fertigen Slice, waehrend der Planer noch laeuft (Start-vor-
// Fertig, B22). close() signalisiert das Ende der Planung -> pop() liefert danach nullopt, sobald
// gedraint.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib (thread-sync), keine Deps.

#include "bestandslog/bestandslog_document.hpp" // BatchTyp

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Eine Batch-Slice = ein Fenster [begin, begin+count) im globalen Binary-Index-Raum (Korn
// kGnBatchSlice) + die darin tatsaechlich FEHLENDEN globalen Indizes (per-Binary-Miss-Erkennung).
struct BatchSlice {
    std::uint64_t              begin = 0;
    std::uint64_t              count = 0;
    BatchTyp                   typ   = BatchTyp::tier;
    std::vector<std::uint64_t> missing; // die zu bauenden globalen Binary-Indizes dieses Fensters

    friend bool operator==(BatchSlice const&, BatchSlice const&) = default;
};

// Thread-sichere Slice-Queue. Nicht kopier-/verschiebbar (haelt Mutex + CV).
class SliceQueue {
public:
    SliceQueue()                             = default;
    SliceQueue(SliceQueue const&)            = delete;
    SliceQueue& operator=(SliceQueue const&) = delete;

    // Producer: eine Slice einreihen und einen wartenden Consumer wecken.
    void push(BatchSlice s) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.push_back(std::move(s));
        }
        cv_.notify_one();
    }

    // Consumer: blockiert bis eine Slice verfuegbar ist ODER die Queue geschlossen UND leer ist
    // (dann nullopt = Planung fertig + gedraint).
    [[nodiscard]] std::optional<BatchSlice> pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return std::nullopt; // closed_ && leer -> fertig
        BatchSlice s = std::move(queue_.front());
        queue_.pop_front();
        return s;
    }

    // Nicht-blockierendes pop (Test/Diagnose): nullopt wenn gerade leer.
    [[nodiscard]] std::optional<BatchSlice> try_pop() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (queue_.empty()) return std::nullopt;
        BatchSlice s = std::move(queue_.front());
        queue_.pop_front();
        return s;
    }

    // Ende der Planung: kein weiteres push; wartende Consumer werden geweckt.
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.size();
    }
    [[nodiscard]] bool closed() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return closed_;
    }

private:
    mutable std::mutex      mtx_;
    std::condition_variable cv_;
    std::deque<BatchSlice>  queue_;
    bool                    closed_ = false;
};

} // namespace comdare::cache_engine::builder::bestandslog
