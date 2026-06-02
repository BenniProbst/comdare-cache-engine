#pragma once
// KF-10 (2026-06-02) — Validierungs-Wiederholungen: jede Experiment-Einstellung wird N-mal (Default 3)
// SEPARAT ausgeführt und SEPARAT dokumentiert; NIEMALS interpoliert/gemittelt.
//
// User-Direktive 2026-06-02: „Experimente für Validität zusätzlich konfigurierbar default 3 Mal wiederholt …
// vollständig und separat dokumentiert und nie interpoliert." Jede Wiederholung trägt einen repetition_index
// und ihr ROHES Mess-Ergebnis; die Auswertung (Diagramm-Overlay, KF-14) legt die N Kurven ÜBEREINANDER statt
// sie zu mitteln. Builder-Seite (das WIE des Messens). C++23, header-only.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Ein roher Mess-Datensatz EINER Wiederholung (nie aggregiert).
struct RepetitionRecord {
    std::string   setting_id;             // die Experiment-Einstellung (Binary × dyn. Belegung)
    std::uint32_t repetition_index = 0;   // 0 .. repetitions-1 (separate Identität)
    double        micros_per_op    = 0.0; // rohes Ergebnis DIESER Wiederholung
    std::uint64_t op_count         = 0;
};

class RepetitionPlan {
public:
    /// Default 3 Wiederholungen (konfigurierbar); 0 → auf 1 normalisiert (mind. ein Lauf).
    explicit RepetitionPlan(std::uint32_t repetitions = 3) noexcept
        : reps_{repetitions == 0 ? 1u : repetitions} {}

    [[nodiscard]] std::uint32_t repetitions() const noexcept { return reps_; }

    /// Führt `measure(repetition_index)` GENAU reps_-mal aus und sammelt JEDE Wiederholung SEPARAT
    /// (kein Mittelwert, keine Interpolation). measure liefert {micros_per_op, op_count} der Wiederholung.
    template <class MeasureFn>
    [[nodiscard]] std::vector<RepetitionRecord> run(std::string const& setting_id, MeasureFn&& measure) const {
        std::vector<RepetitionRecord> out;
        out.reserve(reps_);
        for (std::uint32_t r = 0; r < reps_; ++r) {
            auto const m = measure(r);  // {micros_per_op, op_count}
            out.push_back(RepetitionRecord{setting_id, r, m.first, m.second});
        }
        return out;
    }

    /// CSV mit repetition_index-Spalte: EINE Zeile je Wiederholung (separat, NIE aggregiert).
    [[nodiscard]] static std::string to_csv(std::vector<RepetitionRecord> const& recs) {
        std::string s = "setting_id,repetition_index,micros_per_op,op_count\n";
        for (auto const& r : recs) {
            s += r.setting_id;
            s += ',';
            s += std::to_string(r.repetition_index);
            s += ',';
            s += format_double(r.micros_per_op);
            s += ',';
            s += std::to_string(r.op_count);
            s += '\n';
        }
        return s;
    }

private:
    [[nodiscard]] static std::string format_double(double v) {
        // feste, lokal-unabhängige Darstellung (kein std::ostringstream-Locale-Risiko im Hot-Doc-Pfad)
        char buf[64];
        int const n = std::snprintf(buf, sizeof(buf), "%.6f", v);
        return (n > 0) ? std::string(buf, static_cast<std::size_t>(n)) : std::string{"0.000000"};
    }

    std::uint32_t reps_;
};

}  // namespace comdare::cache_engine::builder::experiment
