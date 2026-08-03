#pragma once
// planer_driven_build.hpp -- G3 / #46b Lagerhaltung, Scheibe I1b (Planer-getriebener provision-Bau).
//
// Producer-Consumer-Treiber des provision-Baus (opt-in, gegated wie I1). Ein ASYNC Producer slict die
// SELEKTIERTEN view-Indizes in Fenster der Groesse grain (Index-Reihenfolge ERHALTEN) und erkennt je
// Fenster JEDES fehlende Binary EINZELN (PresenceFn + detect_missing_in_window, LEDGER:3397). Der
// Consumer (Iterator) baut je Fenster NUR die Fehlenden (filter_window_for_build) und akkumuliert den
// builds-Vektor; dll_is_current bleibt als ZWEITE Verteidigungslinie der lokale Skip-Arbiter. RAII:
// der Producer-Thread joined im dtor (auch im Fehlerfall).
//
// Lager-TP1 (B) / G-A2: bis zu diesem Paket war die PresenceFn nur INFORMATIV (Reservierungs-Info,
// volle Fenster wurden gebaut, "Determinismus A"). Das SOLL (LEDGER:3397: "jedes fehlende Binary
// EINZELN erkannt, NICHT als Gesamt-Batch") macht sie zum BAU-FILTER: bekannte Bestands-Treffer
// werden VOR dem Bau-Versuch uebersprungen und als Skips ausgewiesen. OHNE Praedikat (Default) ist
// missing == Fenster -> der Bau bleibt byte-identisch zum Ist-Pfad (die Byte-Wachen bleiben gruen).
//
// Diese Naht ist eine FOKUS-Variante des B5-Producer-Consumer-Musters (SliceQueue): B5s BatchPlanner
// slict den GLOBALEN [0,total)-Indexraum mit Paritaet; I1b slict die SELEKTIERTE indices-Liste --
// beide erkennen Misses ueber DIESELBE Funktion (detect_missing_in_window, batch_planner.hpp).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib. pop()/push() thread-sicher.

#include "batch_planner.hpp" // detect_missing_in_window (die EINE per-Binary-Miss-Erkennung, B5)

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Praesenz je view-Index (Host-injiziert bzw. seit Lager-TP1(B) vom Iterator aus lager_contains +
// Fingerprint-Provider gebunden; Default absent -> "alles fehlt" -> baut alles = byte-identisch).
// Seit G-A2 ist sie der BAU-FILTER: present==true heisst "liegt als Bestand im Lager, nicht bauen";
// dll_is_current bleibt die zweite, lokale Verteidigungslinie fuer alles, was gebaut wird.
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

// TP1FK1-B1 (Codex-Befund CX-W2): das Reservierungs-Fenster als INTERVALL [begin, begin+count), das die
// (moeglicherweise luecken-behaftete) Index-MENGE des Slices VOLL ENTHAELT. Die Reservierung traegt auf
// dem Draht nur dieses Paar (slice_begin/slice_count, bestandslog_document.hpp); scope_covers_slice
// zaehlt darin die Selektions-Eintraege des uebernehmenden Laufs.
//
// begin = min(view_indices), count = die SPANNE max-min+1 -- NICHT die Kardinalitaet. Fuer die
// zusammenhaengenden Fenster, die dieses System schneidet (slice_view_indices ueber eine dichte
// Selektion), ist das EXAKT [front, front+size) und damit byte-identisch zur bisherigen Form. Bei einer
// LUECKENHAFTEN Menge trennen sich die beiden:
//   {0,2} als (front, size) = (0,2) beschreibt das Intervall {0,1} -- eine fremde Selektion {0,1}
//         bestand die Deckungspruefung und gab den Claim frei, obwohl Index 2 von NIEMANDEM gebaut
//         wird. Das war der VERLUSTBEHAFTETE Schreibweg (Arbeit bleibt liegen).
//   {0,2} als Spanne = (0,3) UMFASST die reale Menge: freigegeben wird erst, wenn die uebernehmende
//         Selektion das ganze Intervall {0,1,2} -- und damit jeden realen Index -- baut.
// Die Deckungs-Frage faellt so KONSERVATIV aus: nie faelschlich released, hoechstens einmal zu selten
// uebernommen (ein Claim aus luecken-behafteter Herkunft laeuft dann in seine pro-forma-Frist).
//
// GRENZE, ehrlich benannt: eine MENGEN-genaue Reservierung (die auch die Gegenrichtung -- der Lauf mit
// der identischen gappy Menge reapt sein eigenes Fenster -- aufloesen wuerde) braucht ein weiteres
// Draht-Feld und damit einen syntax_version-Bump des Bestandslog-Dokuments. Das ist ein Owner-Entscheid
// und hier bewusst NICHT genommen; die Spannen-Form kommt ohne jede Draht-Aenderung aus.
struct SliceWindowBounds {
    std::uint64_t begin = 0; // min(view_indices)
    std::uint64_t count = 0; // SPANNE max-min+1; == view_indices.size() genau bei Lueckenfreiheit
};

[[nodiscard]] inline SliceWindowBounds slice_window_bounds(std::vector<std::size_t> const& view_indices) noexcept {
    if (view_indices.empty()) return SliceWindowBounds{0, 0}; // leeres Fenster: nichts zu beanspruchen
    auto const [mn, mx] = std::minmax_element(view_indices.begin(), view_indices.end());
    auto const lo       = static_cast<std::uint64_t>(*mn);
    auto const hi       = static_cast<std::uint64_t>(*mx);
    return SliceWindowBounds{lo, hi - lo + 1}; // hi >= lo -> Spanne >= 1, kein Unterlauf
}

// Ein Bau-Fenster-Plan: die (VOLLEN) view-Indizes des Fensters + die Miss-Zahl. Das Fenster bleibt
// VOLL im Plan (die Reservierung beansprucht das FENSTER, nicht nur seine Luecken); welche Indizes
// wirklich gebaut werden, entscheidet der Consumer ueber filter_window_for_build (G-A2).
struct BuildSlicePlan {
    std::vector<std::size_t> view_indices;
    std::size_t              missing_count = 0;

    friend bool operator==(BuildSlicePlan const&, BuildSlicePlan const&) = default;
};

// Ergebnis des BAU-FILTERS eines Fensters (G-A2): die zu bauenden (fehlenden) Indizes in
// Fenster-Reihenfolge + die Zahl der als Bestand uebersprungenen. Invariante:
// zu_bauen.size() + bestand_uebersprungen == Fenstergroesse.
struct BuildFilterErgebnis {
    std::vector<std::size_t> zu_bauen;
    std::size_t              bestand_uebersprungen = 0;

    friend bool operator==(BuildFilterErgebnis const&, BuildFilterErgebnis const&) = default;
};

// DER BAU-FILTER (G-A2, LEDGER:3397): jedes fehlende Binary des Fensters EINZELN erkannt -- ueber
// detect_missing_in_window (B5), positions-basiert auf die Fenster-Liste gehoben. OHNE Praedikat
// fehlt alles (byte-identisch: volles Fenster wird gebaut). Die Erkennungs-Funktion ist damit
// dieselbe, die der globale BatchPlanner nutzt -- EINE Miss-Wahrheit, zwei Slicing-Formen.
[[nodiscard]] inline BuildFilterErgebnis filter_window_for_build(std::vector<std::size_t> const& window,
                                                                 PresenceFn const&               present) {
    BuildFilterErgebnis erg;
    if (!present) {
        erg.zu_bauen = window; // kein Praedikat -> alles fehlt -> volles Fenster (Ist-Verhalten)
        return erg;
    }
    auto const fehlende_positionen =
        detect_missing_in_window(0, static_cast<std::uint64_t>(window.size()),
                                 [&](std::uint64_t pos) { return present(window[static_cast<std::size_t>(pos)]); });
    erg.zu_bauen.reserve(fehlende_positionen.size());
    for (auto const pos : fehlende_positionen) erg.zu_bauen.push_back(window[static_cast<std::size_t>(pos)]);
    erg.bestand_uebersprungen = window.size() - erg.zu_bauen.size();
    return erg;
}

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
            // DIESELBE Miss-Erkennung wie der Consumer-Bau-Filter (EINE Wahrheit): missing_count ist
            // exakt die Zahl der Indizes, die filter_window_for_build spaeter zum Bau freigibt.
            std::size_t const missing = filter_window_for_build(window, present_).zu_bauen.size();
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
