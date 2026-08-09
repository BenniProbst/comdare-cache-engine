#pragma once
// tools/mess_report/realm_scan.hpp -- A9-S4: der --realm-root-Verzeichnis-Scan.
//
// Findet Mess-Ergebnis-CSVs unter einer Wurzel: alle Dateien, deren Name auf ".result.csv" endet
// (per_binary/*.result.csv-Baum, EIN Datensatz je gefundener Datei). Eine etwaige Aggregat-Datei
// ("measurements.csv" im Erstbeleg-Archiv, dieselben Zeilen wie die Summe aller per_binary-Dateien)
// wird vom Verzeichnis-Scan bewusst NICHT mitgenommen -- sonst zaehlte jede Zeile doppelt, einmal
// aus der Aggregat-Datei und einmal aus ihrer per-Binary-Quelle. Wer die Aggregat-Sicht statt der
// aufgesplitteten Quellen rendern will, benennt sie explizit ueber --csv=<pfad>.
//
// ".stale"-BESTAENDE WERDEN WEDER GELESEN NOCH ANGEFASST (A9-Doc Abschnitt 8: "Messdaten nie
// loeschen: ... invalidiert nie; .stale-Bestaende werden weder gelesen noch angefasst"). Der Scan
// prueft einen ENDUNGS-Vergleich (".stale" als Suffix, dokumentiert in
// builder/bestandslog/lager_pfad_grammatik.hpp:75 "Suffixe, die spaetere Schichten anhaengen") --
// bewusst ANDERS als profile_facade/planner/planner_status_types.hpp::kResultStaleName (dort ein
// EXAKTER Dateiname "result.csv.stale" fuer einen fixen Stamm; hier tragen die Dateien
// unterschiedliche Staemme wie "<binary_id>.result.csv", also passt nur der Suffix-Vergleich).
//
// BOUNDED SCAN (dieselbe Doktrin wie planner_status_reader.hpp: Tiefen-Kappe + Eintrags-Kappe): ein
// vom Anwender benannter Baum, den das Kommando nicht kontrolliert, darf es nicht unbegrenzt
// festhalten (versehentlich verschachtelt / zyklischer Symlink). Greift die Kappe dennoch, sagt der
// Bericht es LITERAL (scan_gekappt=true) statt still eine zu kleine Bilanz zu melden.

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace comdare::cache_engine::tools::mess_report {

inline constexpr std::string_view kStaleSuffix      = ".stale";
inline constexpr std::string_view kResultCsvSuffix  = ".result.csv";
inline constexpr std::size_t      kMaxScanTiefe     = 8;
inline constexpr std::size_t      kMaxScanEintraege = 200000;

[[nodiscard]] inline bool endet_mit(std::string_view s, std::string_view suffix) noexcept {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

struct RealmScanErgebnis {
    std::vector<std::filesystem::path> gefundene_csvs; // sortiert (Determinismus, LED-61 diff==0)
    std::size_t                        besuchte_eintraege  = 0;
    std::size_t                        uebersprungen_stale = 0;
    bool                               wurzel_vorhanden    = false;
    bool                               scan_gekappt        = false;
};

/// Scanned `root` rekursiv (bounded) nach Mess-Ergebnis-CSVs. `.stale`-Dateien werden GEZAEHLT, aber
/// NICHT in `gefundene_csvs` aufgenommen -- der Aufrufer darf sie damit strukturell nicht erreichen.
[[nodiscard]] inline RealmScanErgebnis scanne_realm_root(std::filesystem::path const& root) {
    RealmScanErgebnis erg;
    if (root.empty()) return erg;
    std::error_code ec;
    erg.wurzel_vorhanden = std::filesystem::exists(root, ec) && !ec;
    if (!erg.wurzel_vorhanden) return erg; // ehrlich leer, kein Wurf -- Aufrufer meldet fehlende Wurzel

    std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied,
                                                     ec);
    if (ec) return erg;
    std::filesystem::recursive_directory_iterator const ende;

    while (it != ende) {
        if (erg.besuchte_eintraege >= kMaxScanEintraege) {
            erg.scan_gekappt = true;
            break;
        }
        ++erg.besuchte_eintraege;

        if (static_cast<std::size_t>(it.depth()) + 1 >= kMaxScanTiefe) {
            // Tiefen-Kappe erreicht: dieser Eintrag wird noch besucht, aber NICHT mehr abgestiegen.
            std::error_code disable_ec;
            it.disable_recursion_pending();
            (void)disable_ec;
            erg.scan_gekappt = true;
        }

        std::error_code reg_ec;
        bool const      ist_datei = it->is_regular_file(reg_ec);
        if (!reg_ec && ist_datei) {
            std::string const name = it->path().filename().string();
            if (endet_mit(name, kStaleSuffix)) {
                ++erg.uebersprungen_stale;
            } else if (endet_mit(name, kResultCsvSuffix)) {
                erg.gefundene_csvs.push_back(it->path());
            }
        }

        std::error_code inc_ec;
        it.increment(inc_ec);
        if (inc_ec) break;
    }

    std::sort(erg.gefundene_csvs.begin(), erg.gefundene_csvs.end());
    return erg;
}

} // namespace comdare::cache_engine::tools::mess_report
