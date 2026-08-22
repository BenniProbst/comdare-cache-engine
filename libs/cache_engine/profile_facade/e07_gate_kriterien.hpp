#pragma once
// e07_gate_kriterien.hpp -- E-07-Gate: die Kriterien K1-K5 + N1-N3 als ausfuehrbarer Pruefer
// (W2 R-12, 2026-08-21; Position B10 der Vor-Trigger-Checkliste, trigger-blockierend).
//
// QUELLE (normativ): super docs/sessions/20260803-DOSSIER-e07-gate-definition-b5-zweistufig.md
// Abschn. 1 -- "GO-Kriterium E-07: K1-K5 UND N1-N3 saemtlich mit literalen Zeilen erfuellt, je
// Familie mindestens eine gueltige Differenz-Probe" -- plus Ungueltigkeits-Regel: "Eine
// Differenz-Probe, deren erwartetes Delta 0 ist, ist UNGUELTIG und zaehlt nicht als bestanden."
// Die Definition existierte bisher NUR als Dossier (Befund r3 C4: die E07_-Gate-Literale hatten
// 0 Treffer im gesamten Code); dieser Header macht sie am Objekt pruefbar. Er ist der KRITERIEN-
// Teil des Gates -- der B5.b-Beweislauf (Mini-Profil, XML-Differenz-Laeufe) EMITTIERT die
// E07_*-Zeilen spaeter in ein Protokoll, DIESER Pruefer faellt das Verdikt daraus.
//
// FAIL-CLOSED-DOKTRIN (jede Abweichung = NO-GO, nie stilles GO):
//   * fehlendes Pflicht-Literal            -> Befund FEHLT           -> NO-GO
//   * unbekannter E07_-Schluessel          -> Befund UNBEKANNT       -> NO-GO (ein Protokoll,
//     das mehr behauptet als die Definition kennt, ist kein Beleg -- Kriterien duerfen nur im
//     Pruefer selbst geschaerft werden, nie per Protokoll-Erfindung)
//   * widerspruechliches Doppel-Literal    -> Befund KONFLIKT        -> NO-GO
//   * unlesbarer Wert (leer/nicht-dezimal) -> Befund UNLESBAR        -> NO-GO
//   * Wertebereichs-Verstoss bei 0/1-Flags -> Befund WERTEBEREICH    -> NO-GO
//   * Differenz-Probe mit erwartetem Delta 0 (Anti-Schein-Gruen)     -> Befund UNGUELTIG -> NO-GO
//   * leeres Protokoll                     -> 0/kLiteraleGesamt      -> NO-GO
// Identische Wiederholungen desselben Literals sind erlaubt (E07_BINARY_ID_SET_CHANGED deckt
// laut Dossier K3b UND K3c -- zwei gleichlautende Zeilen sind ein Literal, kein Konflikt).
// Nicht-E07_-Zeilen sind Traeger-Log und werden ignoriert (das Protokoll darf im Lauf-Log stehen).
//
// NENNER-DOKTRIN: das Verdikt nennt IMMER beide Nenner literal in der Ausgabe
// (E07_GATE_KRITERIEN_ERFUELLT=<k>/10 und E07_GATE_PFLICHT_LITERALE=<p>/23) -- auch bei leerem
// Protokoll. 10 = K1,K2,K3a,K3b,K3c,K4,K5,N1,N2,N3 (die Dossier-Tabelle zaehlt K3 als drei
// Familien-Proben, "je Familie mindestens eine gueltige Differenz-Probe" => einzeln gefuehrt);
// 23 = die im Dossier benannten Pflicht-Literale + E07_WINDOW_COUNT (Parametrisierung von K4:
// "Cursor-Maximum == count-1" ist nur mit count IM Protokoll geschlossen pruefbar -- eine
// SCHAERFUNG im Sinne von B5.a "Stufe 1 darf Kriterien nur schaerfen/parametrisieren").
//
// SELBSTCHECK (R-12, 2026-08-21)
//   ZUSICHERT: das Verdikt ist eine reine Funktion des Protokoll-Texts; GO genau dann, wenn
//              alle 23 Pflicht-Literale vorhanden, alle 10 Kriterien erfuellt und 0 Befunde
//              offen sind; jede der Fail-Closed-Klassen oben erzwingt NO-GO.
//   ZUSICHERT NICHT: dass die E07_*-Zahlen der WIRKLICHKEIT entsprechen -- das beweist erst
//              der B5.b-Beweislauf, der sie arithmetisch aus der XML-Differenz ableitet und
//              woertlich emittiert. Der Pruefer haelt die Form und die Relationen, nicht die
//              Herkunft.
//
// Header-only, C++23, nur stdlib, ASCII-only. Bewusst umbrella-frei -> direkt testbar.

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::e07 {

// ------------------------------------------------------------------------------------------------
// Die EINE Liste der Pflicht-Literale (Reihenfolge == Dossier-Tabelle K1..K5, dann N1..N3).
// ------------------------------------------------------------------------------------------------
inline constexpr std::array<std::string_view, 23> kPflichtLiterale{
    // K1 -- Kardinalitaets-Identitaet
    "E07_TREE_COUNT", "E07_PLAN_PERMS", "E07_BUILT_COUNT",
    // K2 -- Pfad-Roundtrip
    "E07_ID_ROUNDTRIP_MISMATCH",
    // K3a -- Organ-Differenz-Probe
    "E07_ORGAN_DELTA_EXPECTED", "E07_ORGAN_DELTA_ACTUAL", "E07_REMOVED_VALUE_HITS",
    // K3b -- System-Config-Differenz-Probe (E07_BINARY_ID_SET_CHANGED deckt auch K3c)
    "E07_SYS_CELLS_BEFORE", "E07_SYS_CELLS_AFTER", "E07_BINARY_ID_SET_CHANGED",
    // K3c -- Mess-Achsen-Differenz-Probe
    "E07_MEASURE_PASSES_BEFORE", "E07_MEASURE_PASSES_AFTER",
    // K4 -- Fenster-Treue (E07_WINDOW_COUNT = Parametrisierung, s. Kopf)
    "E07_WINDOW_COUNT", "E07_WINDOW_FIRST_ID_MATCH", "E07_WINDOW_LAST_ID_MATCH", "E07_PROGRESS_CURSOR_MAX",
    "E07_PROGRESS_DONE_COUNT",
    // K5 -- Determinismus
    "E07_PLAN_BYTE_EQUAL", "E07_RERUN_ID_SET_EQUAL",
    // N1..N3 -- Negativ-Proben
    "E07_BYPASS_FINDINGS", "E07_STALE_ID_MEASURED", "E07_WINDOW_SUBSET", "E07_WINDOW_NEW_IDS"};

/// Die 0/1-Flags unter den Pflicht-Literalen: jeder andere Wert ist ein WERTEBEREICH-Befund.
/// (Zaehler wie E07_ID_ROUNDTRIP_MISMATCH duerfen jede Zahl TRAGEN -- das Kriterium fordert 0,
/// aber "3 Mismatches" ist eine gueltige Protokoll-AUSSAGE; "BYTE_EQUAL=7" ist keine.)
inline constexpr std::array<std::string_view, 6> kBoolLiterale{
    "E07_WINDOW_FIRST_ID_MATCH", "E07_WINDOW_LAST_ID_MATCH", "E07_PLAN_BYTE_EQUAL",
    "E07_RERUN_ID_SET_EQUAL",    "E07_WINDOW_SUBSET",        "E07_BINARY_ID_SET_CHANGED"};

/// Die 10 Kriterien-Namen (Nenner der Verdikt-Zeile), Reihenfolge == Auswertung.
inline constexpr std::array<std::string_view, 10> kKriterienNamen{"K1", "K2", "K3a", "K3b", "K3c",
                                                                  "K4", "K5", "N1",  "N2",  "N3"};

struct E07GateErgebnis {
    static constexpr int kKriterienGesamt = static_cast<int>(kKriterienNamen.size());  // 10
    static constexpr int kLiteraleGesamt  = static_cast<int>(kPflichtLiterale.size()); // 23

    bool go = false; // Gesamtverdikt; Default NO-GO (fail-closed)
    std::array<bool, static_cast<std::size_t>(kKriterienGesamt)> kriterium_erfuellt{};   // je kKriterienNamen[i]
    int                                                          kriterien_erfuellt = 0; // Zaehler zum Nenner 10
    int                                                          literale_vorhanden = 0; // Zaehler zum Nenner 23
    std::vector<std::string>                                     befunde;                // leer <=> kein Verstoss

    /// Die literale Verdikt-Ausgabe -- Nenner IMMER enthalten, Befunde woertlich, letzte Zeile
    /// E07_GATE=GO|NO-GO. Befund-Zeilen beginnen mit "E07_GATE_BEFUND: " (Doppelpunkt, kein
    /// KEY=VALUE) -- wird ein Verdikt faelschlich als Mess-Protokoll wiedereingespeist, laufen
    /// seine Zeilen als unbekannte E07_-Schluessel in NO-GO (fail-closed statt Echo-Gruen).
    [[nodiscard]] std::string protokoll() const {
        std::string p;
        p += "E07_GATE_KRITERIEN_ERFUELLT=" + std::to_string(kriterien_erfuellt) + "/" +
             std::to_string(kKriterienGesamt) + "\n";
        p += "E07_GATE_PFLICHT_LITERALE=" + std::to_string(literale_vorhanden) + "/" + std::to_string(kLiteraleGesamt) +
             "\n";
        p += "E07_GATE_BEFUNDE=" + std::to_string(befunde.size()) + "\n";
        for (auto const& b : befunde) p += "E07_GATE_BEFUND: " + b + "\n";
        p += go ? "E07_GATE=GO\n" : "E07_GATE=NO-GO\n";
        return p;
    }
};

namespace detail {

[[nodiscard]] constexpr std::string_view trim_ascii(std::string_view s) noexcept {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
    return s;
}

/// Dezimal-uint64 streng: nur [0-9]+, kein Vorzeichen, kein Praefix, Ueberlauf -> nullopt.
[[nodiscard]] constexpr std::optional<std::uint64_t> parse_u64(std::string_view s) noexcept {
    if (s.empty()) return std::nullopt;
    std::uint64_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        std::uint64_t const d = static_cast<std::uint64_t>(c - '0');
        if (v > (UINT64_MAX - d) / 10u) return std::nullopt; // Ueberlauf = unlesbar
        v = v * 10u + d;
    }
    return v;
}

} // namespace detail

/// Wertet ein Beweis-Protokoll (Zeilen-Text; darf in ein groesseres Lauf-Log eingebettet sein)
/// gegen die E-07-Kriterien aus. Reine Funktion, fail-closed (s. Kopf-Doktrin).
[[nodiscard]] inline E07GateErgebnis e07_gate_auswerten(std::string_view protokoll_text) {
    E07GateErgebnis ergebnis;

    // ---- Phase 1: E07_-Zeilen einsammeln (KEY=VALUE), Fail-Closed-Klassen als Befunde. ----
    std::map<std::string, std::uint64_t, std::less<>> werte;
    std::string_view                                  rest = protokoll_text;
    while (!rest.empty()) {
        std::size_t const      nl    = rest.find('\n');
        std::string_view const zeile = detail::trim_ascii(nl == std::string_view::npos ? rest : rest.substr(0, nl));
        rest                         = (nl == std::string_view::npos) ? std::string_view{} : rest.substr(nl + 1);
        if (!zeile.starts_with("E07_")) continue; // Traeger-Log
        std::size_t const eq = zeile.find('=');
        if (eq == std::string_view::npos) {
            ergebnis.befunde.push_back("UNLESBAR (kein '='): " + std::string(zeile));
            continue;
        }
        std::string_view const key = detail::trim_ascii(zeile.substr(0, eq));
        std::string_view const val = detail::trim_ascii(zeile.substr(eq + 1));
        if (std::find(kPflichtLiterale.begin(), kPflichtLiterale.end(), key) == kPflichtLiterale.end()) {
            ergebnis.befunde.push_back("UNBEKANNT: " + std::string(key));
            continue;
        }
        auto const zahl = detail::parse_u64(val);
        if (!zahl.has_value()) {
            ergebnis.befunde.push_back("UNLESBAR (kein dezimal-uint64): " + std::string(zeile));
            continue;
        }
        if (std::find(kBoolLiterale.begin(), kBoolLiterale.end(), key) != kBoolLiterale.end() && *zahl > 1u) {
            ergebnis.befunde.push_back("WERTEBEREICH (0/1-Flag): " + std::string(zeile));
            continue;
        }
        auto const [it, neu] = werte.emplace(std::string(key), *zahl);
        if (!neu && it->second != *zahl) {
            ergebnis.befunde.push_back("KONFLIKT: " + std::string(key) + "=" + std::to_string(it->second) +
                                       " vs =" + std::to_string(*zahl));
        }
    }

    // ---- Phase 2: Vollzaehligkeit (fail-closed: jedes fehlende Literal ist ein Befund). ----
    auto const wert = [&](std::string_view key) -> std::optional<std::uint64_t> {
        auto const it = werte.find(key);
        if (it == werte.end()) return std::nullopt;
        return it->second;
    };
    for (auto const key : kPflichtLiterale) {
        if (werte.contains(key)) {
            ++ergebnis.literale_vorhanden;
        } else {
            ergebnis.befunde.push_back("FEHLT: " + std::string(key));
        }
    }

    // ---- Phase 3: die 10 Kriterien (jedes nur mit vollstaendigen Eingaben erfuellbar). ----
    auto const setze = [&](std::string_view name, bool erfuellt) {
        auto const it                    = std::find(kKriterienNamen.begin(), kKriterienNamen.end(), name);
        auto const idx                   = static_cast<std::size_t>(it - kKriterienNamen.begin());
        ergebnis.kriterium_erfuellt[idx] = erfuellt;
        if (erfuellt) ++ergebnis.kriterien_erfuellt;
    };
    auto const tree  = wert("E07_TREE_COUNT");
    auto const plan  = wert("E07_PLAN_PERMS");
    auto const built = wert("E07_BUILT_COUNT");
    // K1: Baum == Plan == gebaut, und nicht-leer (ein 0-Fenster bewiese nichts -> Schein-Gruen).
    setze("K1", tree && plan && built && *tree == *plan && *plan == *built && *tree >= 1u);

    auto const mism = wert("E07_ID_ROUNDTRIP_MISMATCH");
    setze("K2", mism && *mism == 0u);

    // K3a: erwartet == gemessen, 0 Treffer des entfernten Werts, und ECHTES Schrumpf-Delta
    // gegen den K1-Basis-Count (Ungueltigkeits-Regel: erwartetes Delta 0 => UNGUELTIG).
    auto const oexp = wert("E07_ORGAN_DELTA_EXPECTED");
    auto const oact = wert("E07_ORGAN_DELTA_ACTUAL");
    auto const ohit = wert("E07_REMOVED_VALUE_HITS");
    bool       k3a  = oexp && oact && ohit && tree && *oexp == *oact && *ohit == 0u && *oexp < *tree;
    if (oexp && tree && *oexp >= *tree) {
        ergebnis.befunde.push_back("UNGUELTIG (K3a): erwartetes Delta 0 oder Wachstum -- E07_ORGAN_DELTA_EXPECTED=" +
                                   std::to_string(*oexp) + " gegen E07_TREE_COUNT=" + std::to_string(*tree));
        k3a = false;
    }
    setze("K3a", k3a);

    auto const sb   = wert("E07_SYS_CELLS_BEFORE");
    auto const sa   = wert("E07_SYS_CELLS_AFTER");
    auto const idch = wert("E07_BINARY_ID_SET_CHANGED");
    bool       k3b  = sb && sa && idch && *sa < *sb && *idch == 0u;
    if (sb && sa && *sa >= *sb) {
        ergebnis.befunde.push_back("UNGUELTIG (K3b): kein Schrumpf-Delta -- E07_SYS_CELLS_BEFORE=" +
                                   std::to_string(*sb) + " E07_SYS_CELLS_AFTER=" + std::to_string(*sa));
        k3b = false;
    }
    setze("K3b", k3b);

    auto const mb  = wert("E07_MEASURE_PASSES_BEFORE");
    auto const ma  = wert("E07_MEASURE_PASSES_AFTER");
    bool       k3c = mb && ma && idch && *ma < *mb && *idch == 0u;
    if (mb && ma && *ma >= *mb) {
        ergebnis.befunde.push_back("UNGUELTIG (K3c): kein Schrumpf-Delta -- E07_MEASURE_PASSES_BEFORE=" +
                                   std::to_string(*mb) + " E07_MEASURE_PASSES_AFTER=" + std::to_string(*ma));
        k3c = false;
    }
    setze("K3c", k3c);

    // K4: Fenster nicht leer, erste/letzte id treu, Cursor-Maximum == count-1, genau EIN done.
    auto const wcount = wert("E07_WINDOW_COUNT");
    auto const wfirst = wert("E07_WINDOW_FIRST_ID_MATCH");
    auto const wlast  = wert("E07_WINDOW_LAST_ID_MATCH");
    auto const cmax   = wert("E07_PROGRESS_CURSOR_MAX");
    auto const done   = wert("E07_PROGRESS_DONE_COUNT");
    setze("K4", wcount && wfirst && wlast && cmax && done && *wcount >= 1u && *wfirst == 1u && *wlast == 1u &&
                    *cmax == *wcount - 1u && *done == 1u);

    auto const pbe = wert("E07_PLAN_BYTE_EQUAL");
    auto const rid = wert("E07_RERUN_ID_SET_EQUAL");
    setze("K5", pbe && rid && *pbe == 1u && *rid == 1u);

    auto const byp = wert("E07_BYPASS_FINDINGS");
    setze("N1", byp && *byp == 0u);
    auto const stale = wert("E07_STALE_ID_MEASURED");
    setze("N2", stale && *stale == 0u);
    auto const subset = wert("E07_WINDOW_SUBSET");
    auto const newids = wert("E07_WINDOW_NEW_IDS");
    setze("N3", subset && newids && *subset == 1u && *newids == 0u);

    // ---- Verdikt: GO nur bei Vollzaehligkeit + 10/10 + 0 Befunden (fail-closed). ----
    ergebnis.go = ergebnis.befunde.empty() && ergebnis.literale_vorhanden == E07GateErgebnis::kLiteraleGesamt &&
                  ergebnis.kriterien_erfuellt == E07GateErgebnis::kKriterienGesamt;
    return ergebnis;
}

} // namespace comdare::cache_engine::e07
