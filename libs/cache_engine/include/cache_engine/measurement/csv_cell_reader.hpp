#pragma once
// measurement/csv_cell_reader.hpp -- Single-Source der RFC-4180-nahen CSV-Zell-/Header-Primitiven.
//
// HERKUNFT (Extraktion 2026-08-08, A9-S4-Vorlauf): `builder/curve_fit/curve_fit.hpp` und
// `heuristik/measurement_curve_loader.hpp` trugen die Zell-Split- und die streng-numerische
// Zellen-Parse-Funktion BYTE-IDENTISCH dupliziert (beide Dateien dokumentierten das seit ihrer
// Entstehung ausdruecklich als "identische Doktrin", ohne die Duplikation aufzuloesen). Ein drittes,
// neues CLI (`tools/mess_report/`) haette eine dritte Kopie derselben Logik gebraucht -- das ist
// exakt der Punkt, an dem eine kuenftige Korrektur an einer Stelle die anderen zwei stumm
// zurueckliesse. Owner-KERN "Sauberkeit und Gruendlichkeit stehen im Zentrum": der saubere Weg ist
// EXTRAHIEREN statt eine dritte Kopie zu schreiben.
//
// BEHAVIOR-PRESERVING: dies ist eine reine Code-Verschiebung. `curve_fit.hpp` und
// `measurement_curve_loader.hpp` binden die Funktionen seither ueber `using`-Deklarationen in ihren
// jeweiligen `detail`/`loader_detail`-Namensraeumen ein (Aufrufstellen bleiben woertlich
// unveraendert). `measurement_curve_loader.hpp` traegt einen `AXIS_ALGO_VERSION`-Marker
// (axis_version_lock); der Marker verfolgt SEMANTISCHE Aenderungen (welche Zeilen zu Kurvenpunkten
// werden), keine Implementierungs-Verschiebung -- diese Extraktion aendert daran nichts und bumpt
// den Marker deshalb NICHT (vor/nach-Testlauf als Beleg, siehe Commit-Nachricht).
//
// Kein Python, kein externes CSV-Framework -- Hausdoktrin.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace comdare::cache_engine::measurement::csv {

/// Zell-Split mit minimalem RFC-4180-Quoting (Repo-Writer csv_quote quotet unconditional) und
/// CRLF-Strip -- Review wf_c99a2132: Windows-CSVs und gequotete Zellen duerfen weder Spalten
/// verschieben noch das Header-Matching brechen.
[[nodiscard]] inline std::vector<std::string> split_csv_line(std::string_view line, char delimiter) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    std::vector<std::string> cells;
    std::string              cell;
    bool                     in_quotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char const c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') { // escaped quote
                cell.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == delimiter && !in_quotes) {
            cells.push_back(std::move(cell));
            cell.clear();
        } else {
            cell.push_back(c);
        }
    }
    cells.push_back(std::move(cell));
    return cells;
}

/// Streng-numerischer Zellen-Parser (Ganzzahl): die GANZE Zelle muss konsumiert werden (Review:
/// 'n/a' darf NIE still zum Phantom-Punkt (0,0) werden).
[[nodiscard]] inline bool parse_u64_cell(std::string const& cell, std::uint64_t& out) {
    if (cell.empty()) return false;
    char*                    end = nullptr;
    unsigned long long const v   = std::strtoull(cell.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') return false;
    out = static_cast<std::uint64_t>(v);
    return true;
}

/// Streng-numerischer Zellen-Parser (Gleitkomma): die GANZE Zelle muss konsumiert werden, NaN/Inf
/// gelten NICHT als gueltig (kein stiller Phantom-Punkt).
[[nodiscard]] inline bool parse_double_cell(std::string const& cell, double& out) {
    if (cell.empty()) return false;
    char*        end = nullptr;
    double const v   = std::strtod(cell.c_str(), &end);
    if (end == nullptr || *end != '\0' || !std::isfinite(v)) return false;
    out = v;
    return true;
}

/// Prueft, ob `cell` einer der bekannten NA-Tokens ist (z.B. "n/a", "failed", "gesperrt",
/// "nicht_gebaut" -- die konkrete Liste liegt beim Aufrufer, axis_version_lock-Marker dort).
[[nodiscard]] inline bool is_na(std::string const& cell, std::vector<std::string> const& na_tokens) {
    for (std::string const& t : na_tokens)
        if (cell == t) return true;
    return false;
}

/// Header-Index einer Spalte (linearer Scan -- fuer schmale, fest bekannte Spaltenmengen wie
/// axis/variant/workload/x/y). -1 wenn der Name leer ist oder nicht vorkommt.
[[nodiscard]] inline std::ptrdiff_t col_index(std::vector<std::string> const& header, std::string const& name) {
    if (name.empty()) return -1;
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(header.size()); ++i)
        if (header[static_cast<std::size_t>(i)] == name) return i;
    return -1;
}

/// Zell-Wert an idx oder Default-Fallback "-" (idx<0 = Spalte fehlt bzw. leer konfiguriert; idx
/// aeusserhalb der Zeile = unvollstaendige Zeile). Leere Zelle wird ebenfalls zu "-" (Gruppen-Key
/// braucht einen druckbaren, stabilen Platzhalter statt einer leeren Zeichenkette).
[[nodiscard]] inline std::string cell_or_dash(std::vector<std::string> const& cells, std::ptrdiff_t idx) {
    if (idx < 0 || idx >= static_cast<std::ptrdiff_t>(cells.size())) return "-";
    std::string const& v = cells[static_cast<std::size_t>(idx)];
    return v.empty() ? std::string{"-"} : v;
}

/// Header-Index einer BREITEN Spaltenmenge (O(1) je Lookup statt linearer Scan -- Muster aus
/// `best_binary_selector.cpp::parse_measurement_csv`, hier als wiederverwendbarer Typ statt einer
/// vierten Ad-hoc-Kopie). Gedacht fuer CSVs mit vielen Spalten (Bestands-WIDE-CSV ~180 Spalten),
/// bei denen ein linearer Scan je Spalte teuer waere.
class HeaderIndex {
public:
    explicit HeaderIndex(std::vector<std::string> const& header) {
        for (std::size_t i = 0; i < header.size(); ++i) index_.emplace(header[i], i);
    }

    /// Sucht `name`; bei Fund wird `out_idx` gesetzt und true zurueckgegeben. Bei fehlender Spalte
    /// bleibt `out_idx` unveraendert (Aufrufer entscheidet: Pflicht-Spalte -> hart abbrechen,
    /// optionale Spalte -> degradieren).
    [[nodiscard]] bool find(std::string_view name, std::size_t& out_idx) const {
        auto const it = index_.find(std::string{name});
        if (it == index_.end()) return false;
        out_idx = it->second;
        return true;
    }

    [[nodiscard]] bool contains(std::string_view name) const { return index_.contains(std::string{name}); }

    [[nodiscard]] std::size_t size() const noexcept { return index_.size(); }

private:
    std::unordered_map<std::string, std::size_t> index_;
};

} // namespace comdare::cache_engine::measurement::csv
