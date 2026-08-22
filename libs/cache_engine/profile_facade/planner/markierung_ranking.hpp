#pragma once
// markierung_ranking -- M14-RANKING-GRAMMATIK + Markierungs-/Ranking-AUSGABE des Planers
// (P-H/#89, KON110-05 R-1 Forscher-Workflow, KON112-08 (f); Bau 2026-08-21).
//
// OWNER-R-1 (17.08., PV-4): "FORSCHER-WORKFLOW: eigene Achsen-Algorithmen + Gesamt-Funktions-
// Kompositionen hinter dem Gattung+Genus-Interface MARKIEREN und ausmessen -> Ranking gegen
// alle Konkurrenten ueber alle verlangten (default ALLE) Parameter -> der PLANER gibt am
// Experiment-Ende aus, wo die markierten Achsen je gewaehltem Parameter im Ranking gegen den
// Lager-Stand stehen -> iteratives Tweaken in neuen Runden."
//
// SCOPE-SCHNITT (K4-Karte 2d): HIER lebt die GRAMMATIK (Typen + deterministisches Ranking +
// Zeilen-Ausgabe); der REPORT-BAU (Daten-Fuellung aus realen Messwerten/Lager-Bestand) ist
// nach-Trigger-Arbeit (M14-Report-Bau) und konsumiert diese Typen unveraendert. Die XML-Seite
// (markier-/ranking-Elemente im Experiment-Dialekt) ist XSD-Arbeit des s13-Schema-Zugs
// (Kollisions-Auflage #89: KEINE XSD-Aenderung in diesem Strang; Bedarfsliste liegt in der
// Strang-Ergebnisdatei).
//
// ZEILEN-GRAMMATIK der Ausgabe (deterministisch, ASCII, eine Aussage je Zeile):
//   RANKING parameter=<p> richtung=<groesser_ist_besser|kleiner_ist_besser> plaetze=<n>
//   PLATZ <i>/<n> parameter=<p> binary=<binary_id> wert=<wert> markiert=<ja|nein>
//   MARKIERUNG parameter=<p> binary=<binary_id> platz=<i>/<n>
// Die MARKIERUNG-Zeilen sind die Planer-SCHLUSS-Ausgabe (je markiertem Kandidaten je Parameter
// eine Zeile: wo steht er im Ranking). Determinismus: stabile Sortierung, Tie-Break lexikalisch
// ueber binary_id, Wert-Rendering via std::to_string (feste 6 Nachkommastellen).
//
// C++23, header-only, ASCII-only, KEINE Registry-/Mess-Includes (Planer-leichtgewichtig).

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::thesis_lazy::ranking {

// -- Markierung (R-1): eine markierte ACHSEN-Auspraegung ("achse=wert"-Token der binary_id-
//    Grammatik) oder eine markierte GESAMT-Komposition (voller binary_id, z.B. sota::-Raum). --
struct MarkierteAchse {
    std::string achse; // kCompositionAxisNames-Slot (z.B. "concurrency")
    std::string wert;  // Registry-Wrapper-name() (z.B. "olc_optimistic")
};

struct MarkierungsSatz {
    std::vector<MarkierteAchse> achsen;        // markierte Achsen-Algorithmen
    std::vector<std::string>    kompositionen; // markierte Gesamt-Kompositionen (voller binary_id)
};

/// ist_markiert -- traegt der Kandidat eine markierte Achsen-Auspraegung (Token-Treffer
/// "achse=wert" in der binary_id-Grammatik) oder IST er eine markierte Gesamt-Komposition?
[[nodiscard]] inline bool ist_markiert(MarkierungsSatz const& satz, std::string_view binary_id) {
    for (auto const& k : satz.kompositionen)
        if (binary_id == k) return true;
    for (auto const& a : satz.achsen) {
        std::string const token = a.achse + "=" + a.wert;
        if (binary_id.find(token) != std::string_view::npos) return true;
    }
    return false;
}

// -- Ranking-Grammatik: je Parameter eine gerichtete, deterministische Platz-Liste. --
enum class RankingRichtung {
    GroesserIstBesser, // z.B. THROUGHPUT
    KleinerIstBesser   // z.B. LATENCY_P99
};

[[nodiscard]] constexpr std::string_view richtung_name(RankingRichtung r) noexcept {
    switch (r) {
        case RankingRichtung::GroesserIstBesser: return "groesser_ist_besser";
        case RankingRichtung::KleinerIstBesser: return "kleiner_ist_besser";
    }
    return "unbekannt"; // nicht erreichbar (-Wswitch deckt Enum-Erweiterungen)
}

struct RankingKandidat {
    std::string binary_id;  // Tier-/SOTA-/Pruefling-Identitaet (Lager-Stand + Markierte gemeinsam)
    double      wert = 0.0; // Messwert des Parameters (Fuellung = nach-Trigger M14-Report-Bau)
};

struct ParameterRanking {
    std::string                  parameter; // Mess-Parameter-Name (default ALLE, R-1)
    RankingRichtung              richtung = RankingRichtung::GroesserIstBesser;
    std::vector<RankingKandidat> plaetze; // absteigend nach Guete sortiert (Platz 1 zuerst)
};

/// rank_parameter -- die EINE deterministische Platz-Bildung: sortiert nach Guete gemaess
/// Richtung; Ties brechen LEXIKALISCH ueber binary_id (stabil, resumierbar, byte-gleich je Lauf).
[[nodiscard]] inline ParameterRanking rank_parameter(std::string parameter, RankingRichtung richtung,
                                                     std::vector<RankingKandidat> kandidaten) {
    std::sort(kandidaten.begin(), kandidaten.end(), [richtung](RankingKandidat const& a, RankingKandidat const& b) {
        if (a.wert != b.wert) return richtung == RankingRichtung::GroesserIstBesser ? a.wert > b.wert : a.wert < b.wert;
        return a.binary_id < b.binary_id; // Tie-Break: deterministisch lexikalisch
    });
    return ParameterRanking{std::move(parameter), richtung, std::move(kandidaten)};
}

struct MarkierungsRankingBericht {
    std::vector<ParameterRanking> parameter; // je verlangtem Parameter (default ALLE) ein Ranking
};

/// render_markierungs_ranking -- die Planer-Ausgabe am Experiment-Ende (Zeilen-Grammatik im
/// Kopf): je Parameter der RANKING-Kopf + PLATZ-Zeilen; danach je markiertem Kandidaten die
/// MARKIERUNG-Schlusszeile (wo die markierten Achsen im Ranking gegen den Lager-Stand stehen).
[[nodiscard]] inline std::string render_markierungs_ranking(MarkierungsRankingBericht const& bericht,
                                                            MarkierungsSatz const&           markierung) {
    std::string out;
    for (auto const& pr : bericht.parameter) {
        out += "RANKING parameter=" + pr.parameter + " richtung=";
        out += richtung_name(pr.richtung);
        out += " plaetze=" + std::to_string(pr.plaetze.size()) + "\n";
        std::size_t const n = pr.plaetze.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto const& k = pr.plaetze[i];
            out += "PLATZ " + std::to_string(i + 1) + "/" + std::to_string(n) + " parameter=" + pr.parameter +
                   " binary=" + k.binary_id + " wert=" + std::to_string(k.wert) +
                   " markiert=" + (ist_markiert(markierung, k.binary_id) ? "ja" : "nein") + "\n";
        }
    }
    for (auto const& pr : bericht.parameter) {
        std::size_t const n = pr.plaetze.size();
        for (std::size_t i = 0; i < n; ++i)
            if (ist_markiert(markierung, pr.plaetze[i].binary_id))
                out += "MARKIERUNG parameter=" + pr.parameter + " binary=" + pr.plaetze[i].binary_id +
                       " platz=" + std::to_string(i + 1) + "/" + std::to_string(n) + "\n";
    }
    return out;
}

} // namespace comdare::cache_engine::thesis_lazy::ranking
