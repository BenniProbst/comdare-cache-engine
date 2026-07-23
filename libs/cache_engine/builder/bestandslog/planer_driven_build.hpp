#pragma once
// planer_driven_build.hpp -- G3 / #46b Lagerhaltung, Scheibe I1b (Planer-getriebener provision-Bau).
//
// Producer-Consumer-Treiber des provision-Baus (opt-in, gegated wie I1). Ein ASYNC Producer slict die
// SELEKTIERTEN view-Indizes in Fenster der Groesse grain (Index-Reihenfolge ERHALTEN -- Determinismus
// A) und scannt je Fenster die Miss-Zahl (PresenceFn; INFORMIERT nur die Reservierung/ETA, filtert den
// Bau NICHT). Der Consumer (Iterator) baut Fenster fuer Fenster via provision_all(view, fenster) und
// akkumuliert den builds-Vektor -> identisch zum EINEN provision_all(alle indices); dll_is_current
// bleibt der eine Skip-Arbiter. RAII: der Producer-Thread joined im dtor (auch im Fehlerfall).
//
// Default (inaktiv) laeuft der EINE-provision_all-Ist-Pfad byte-identisch weiter -- der Aufrufer gated.
// Diese Naht ist eine FOKUS-Variante des B5-Producer-Consumer-Musters (SliceQueue): B5s BatchPlanner
// slict den GLOBALEN [0,total)-Indexraum mit Paritaet + Miss-Filter; I1b slict die SELEKTIERTE
// indices-Liste und baut VOLLE Fenster (Determinismus A) -- deshalb ein eigener, schlanker Plan-Typ.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib. pop()/push() thread-sicher.

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Praesenz je view-Index (Host-injiziert; Default absent -> "alles fehlt" -> baut alles = byte-identisch;
// real selektiv mit I2/.fingerprint-Sidecar). Nur INFORMATIV (Reservierung/ETA), filtert den Bau NICHT.
using PresenceFn = std::function<bool(std::size_t view_index)>;

// Slice-Korn (spiegelt experiment_plan_director kGnBatchSlice=4096).
inline constexpr std::size_t kBuildSliceGrain = 4096;

// Pure: slict die selektierten view-Indizes in Fenster der Groesse grain (Reihenfolge erhalten). Die
// Konkatenation der Fenster ergibt EXAKT die Eingabe zurueck -> Determinismus-Basis (aktiv == inaktiv).
[[nodiscard]] inline std::vector<std::vector<std::size_t>> slice_view_indices(std::vector<std::size_t> const& indices,
                                                                              std::size_t                     grain) {
    std::vector<std::vector<std::size_t>> out;
    if (grain == 0) grain = kBuildSliceGrain;
    for (std::size_t b = 0; b < indices.size(); b += grain) {
        std::size_t const e = std::min(indices.size(), b + grain);
        out.emplace_back(indices.begin() + static_cast<std::ptrdiff_t>(b),
                         indices.begin() + static_cast<std::ptrdiff_t>(e));
    }
    return out;
}

// Ein Bau-Fenster-Plan: die (VOLLEN) view-Indizes des Fensters + die Miss-Zahl (Reservierungs-Info).
struct BuildSlicePlan {
    std::vector<std::size_t> view_indices;
    std::size_t              missing_count = 0;

    friend bool operator==(BuildSlicePlan const&, BuildSlicePlan const&) = default;
};

// Thread-sichere FIFO-Queue der Plaene (Muster SliceQueue). Nicht kopier-/verschiebbar.
class SlicePlanQueue {
public:
    SlicePlanQueue()                                 = default;
    SlicePlanQueue(SlicePlanQueue const&)            = delete;
    SlicePlanQueue& operator=(SlicePlanQueue const&) = delete;

    void push(BuildSlicePlan p) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push_back(std::move(p));
        }
        cv_.notify_one();
    }

    // Blockiert bis ein Plan da ist ODER geschlossen + leer (dann nullopt).
    [[nodiscard]] std::optional<BuildSlicePlan> pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this] { return closed_ || !q_.empty(); });
        if (q_.empty()) return std::nullopt;
        BuildSlicePlan p = std::move(q_.front());
        q_.pop_front();
        return p;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    mutable std::mutex         mtx_;
    std::condition_variable    cv_;
    std::deque<BuildSlicePlan> q_;
    bool                       closed_ = false;
};

// Async Producer: slict indices, scannt je Fenster die Miss-Zahl (PresenceFn), schiebt die Plaene IN
// REIHENFOLGE in die Queue, schliesst am Ende. RAII: der Thread joined im dtor (auch im Fehlerfall).
class SlicePlanner {
public:
    SlicePlanner(SlicePlanQueue& q, std::vector<std::size_t> indices, std::size_t grain, PresenceFn present)
        : q_{q}, indices_{std::move(indices)}, grain_{grain == 0 ? kBuildSliceGrain : grain},
          present_{std::move(present)} {
        worker_ = std::thread([this] { run(); });
    }

    SlicePlanner(SlicePlanner const&)            = delete;
    SlicePlanner& operator=(SlicePlanner const&) = delete;

    ~SlicePlanner() { join(); }

    void join() {
        if (worker_.joinable()) worker_.join();
    }

private:
    void run() {
        for (auto& window : slice_view_indices(indices_, grain_)) {
            std::size_t missing = 0;
            if (present_) {
                for (std::size_t idx : window)
                    if (!present_(idx)) ++missing;
            } else {
                missing = window.size(); // kein Praedikat -> alles fehlt (byte-identisch: baut alles)
            }
            q_.push(BuildSlicePlan{std::move(window), missing});
        }
        q_.close();
    }

    SlicePlanQueue&          q_;
    std::vector<std::size_t> indices_;
    std::size_t              grain_;
    PresenceFn               present_;
    std::thread              worker_;
};

} // namespace comdare::cache_engine::builder::bestandslog
