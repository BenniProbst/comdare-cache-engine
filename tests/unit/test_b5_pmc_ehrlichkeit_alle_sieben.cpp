// test_b5_pmc_ehrlichkeit_alle_sieben.cpp -- B-5 BISS (08.08.2026).
//
// DREI ZU BEWEISENDE AUSSAGEN:
//   B-5/A  Die errno-KLASSEN sind getrennt. "Diesen Zaehler gibt es auf dieser CPU nicht" (ENOENT) und
//          "es gibt ihn, aber wir duerfen nicht" (EACCES) sind verschiedene Zustaende mit verschiedenen
//          Log-Etiketten -- und dennoch derselben Zelle, weil in BEIDEN Faellen nicht gemessen wurde.
//   B-5/B  KEINER der SIEBEN PMC-Zaehler rendert je eine Zahl, wenn seine Quelle nicht geliefert hat.
//          Das ist der eigentliche Koeder: er beisst, sobald irgendein Zaehler wieder ungated wird.
//   B-5/C  Eine ECHTE Null bleibt eine Zahl. Die Ehrlichkeit darf keinen realen Nullbefund verdecken --
//          sonst waere sie nur die umgekehrte Luege.
//
// WARUM ES DIESE WACHEN BRAUCHT -- AM OBJEKT GEMESSEN, NICHT VERMUTET.
// prod1 (AMD Ryzen 9 9950X3D, Zen 5, family 26 / model 68, Kernel 6.17.0-35-generic,
// perf_event_paranoid=1, uid 1001), eigene perf_event_open-Sonde:
//     PERF_TYPE_HW_CACHE / LL / READ / MISS   pid=0  cpu=-1  -> errno=2  (ENOENT)
//     amd_l3 type=17 config=0x104             pid=-1 cpu=0   -> errno=13 (EACCES)
//     amd_l3 type=17 config=0x104 per-task    pid=0  cpu=-1  -> errno=22 (EINVAL)
// Der Bestand kannte nur `bool open()` und ebnete alle drei zu "Quelle nicht da" ein. Daraus ist eine
// falsche Begruendung entstanden (ce 5c102e05: die amd_l3-PMU sei "nicht geladen" -- sie ist geladen,
// `/sys/bus/event_source/devices/amd_l3/type` liefert 17; gesperrt ist sie, nicht abwesend).
//
// UND DIE ZWEITE, GROESSERE LUECKE: l1 und dtlb gingen als EINZIGE der sieben ueber `zelle` statt
// `pmc_zelle`. Auf prod1 fiel das nie auf, weil beide dort oeffnen -- auf WINDOWS ist die 0 dagegen
// heute schon gelogen (windows_pcm_pmc_source.hpp sagt selbst "L1 bleibt ehrlich 0", und dtlb_misses
// wird dort nie gesetzt). Genau diese Sorte Luecke faengt Fall B jetzt strukturell ab.
//
// DIESE TU IST HARDWARE-UNABHAENGIG -- ABSICHTLICH, und das ist eine Lehre aus M-1: sie prueft die
// REINE Renderer-/Klassifikations-Logik mit von Hand gesetzten POD-Feldern, oeffnet KEINEN Counter und
// liest KEINE PMU. Ihr Ergebnis ist identisch, ob diese Binary mit COMDARE_ENABLE_PMC=ON oder =OFF
// gebaut wird. Deshalb traegt sie KEIN 'pmc'-Label -- sonst schloesse test:unit (-LE pmc) sie aus,
// waehrend die .pmc-Jobs sie nie bauen: 'Not Run' in jedem Job, dieselbe Fehlerklasse wie M-CE-25.
//
// ASCII-only. Keine ABI-Flaeche, kein Preimage-Format, keine golden-Beruehrung.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <cache_engine/measurement/pmc_counter_outcome.hpp>

#include <cerrno>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace cem = ::comdare::cache_engine::measurement;
namespace ceb = ::comdare::cache_engine::builder::experiment;

namespace {

int fehler = 0;

void pruefe(bool bedingung, char const* was) {
    if (!bedingung) {
        std::printf("  FEHLGESCHLAGEN: %s\n", was);
        ++fehler;
    }
}

/// Die sieben PMC-Spalten, wie sie im Header stehen -- Namen woertlich aus lazy_csv_header().
/// Sie sind der Gegenstand von B-5: keine von ihnen darf je eine Zahl tragen, die nichts gemessen hat.
constexpr char const* kPmcSpalten[] = {"pmc_cache_misses_l1", "pmc_cache_misses_l2",         "pmc_cache_misses_l3",
                                       "pmc_dtlb_misses",     "pmc_coherence_invalidations", "pmc_energy_micro_joules",
                                       "pmc_branch_misses"};
constexpr std::size_t kPmcSpaltenZahl = sizeof(kPmcSpalten) / sizeof(kPmcSpalten[0]);

/// Zerlegt eine CSV-Zeile an ';'. Der Header dieser CSV nutzt dasselbe Trennzeichen.
std::vector<std::string> spalten(std::string const& zeile) {
    std::vector<std::string> out;
    std::string              akt;
    for (char const c : zeile) {
        if (c == ';') {
            out.push_back(akt);
            akt.clear();
        } else if (c != '\n') {
            akt.push_back(c);
        }
    }
    out.push_back(akt);
    return out;
}

/// Der Wert EINER benannten Spalte aus einer gerenderten Zeile -- ueber den ECHTEN Header aufgeloest,
/// nicht ueber einen von Hand gezaehlten Index. Ein Index waere beim naechsten END-Append still falsch
/// geworden; genau die Blindheit, die dieses Paket bekaempft.
std::string zell_wert(std::vector<std::string> const& kopf, std::vector<std::string> const& zeile,
                      char const* spalten_name) {
    for (std::size_t i = 0; i < kopf.size(); ++i) {
        if (kopf[i] == spalten_name) { return i < zeile.size() ? zeile[i] : std::string{"<fehlt>"}; }
    }
    return "<spalte-unbekannt>";
}

} // namespace

int main() {
    std::printf("== B-5: PMC-Ehrlichkeit, alle sieben Zaehler ==\n");

    // ------------------------------------------------------------------------------------------
    // FALL A -- die errno-Klassen sind getrennt, und sie sind es an den am Objekt GEMESSENEN Werten.
    // ------------------------------------------------------------------------------------------
    std::printf("\n[A] errno-Klassifikation\n");
    pruefe(cem::classify_pmc_open_errno(ENOENT) == cem::PmcCounterOutcome::EventNichtVorhanden,
           "ENOENT (generischer LL-Zaehler auf Zen 5) muss EventNichtVorhanden sein");
    pruefe(cem::classify_pmc_open_errno(EACCES) == cem::PmcCounterOutcome::ZugriffVerweigert,
           "EACCES (amd_l3 unter paranoid=1) muss ZugriffVerweigert sein");
    pruefe(cem::classify_pmc_open_errno(EINVAL) == cem::PmcCounterOutcome::OeffnungUngueltig,
           "EINVAL (Uncore per-Task geoeffnet) muss OeffnungUngueltig sein");
    // Die Kern-Unterscheidung des ganzen Pakets: im LOG verschieden, in der ZELLE gleich.
    pruefe(cem::pmc_outcome_label(cem::PmcCounterOutcome::EventNichtVorhanden) !=
               cem::pmc_outcome_label(cem::PmcCounterOutcome::ZugriffVerweigert),
           "'gibt es nicht' und 'darf nicht' muessen im Log unterscheidbar sein");
    pruefe(cem::pmc_outcome_cell(cem::PmcCounterOutcome::EventNichtVorhanden) == std::string_view{"n/a"},
           "ein fehlender Zaehler rendert n/a");
    pruefe(cem::pmc_outcome_cell(cem::PmcCounterOutcome::ZugriffVerweigert) == std::string_view{"n/a"},
           "ein gesperrter Zaehler rendert ebenfalls n/a -- gemessen wurde in beiden Faellen nichts");
    // Ein unbekanntes errno darf NIE als "alles gut" durchrutschen. Der Wert ist bewusst keiner, der
    // in irgendeiner Doku steht -- ein Koeder, den man nicht versehentlich mitpflegt.
    pruefe(cem::classify_pmc_open_errno(31337) == cem::PmcCounterOutcome::UnbekannterFehler,
           "ein unbekanntes errno wird SICHTBAR, nicht auf eine Nachbar-Klasse geraten");
    pruefe(!cem::pmc_outcome_hat_wert(cem::classify_pmc_open_errno(31337)),
           "ein unbekanntes errno traegt NIE einen Wert");
    // Genau EINE Klasse traegt einen Wert -- ueber alle Klassen durchgezaehlt, nicht stichprobenartig.
    std::size_t mit_wert = 0;
    for (std::size_t i = 0; i < cem::kPmcCounterOutcomeCount; ++i) {
        if (cem::pmc_outcome_hat_wert(static_cast<cem::PmcCounterOutcome>(i))) ++mit_wert;
    }
    pruefe(mit_wert == 1, "von allen Outcome-Klassen darf GENAU EINE einen Wert tragen");

    // ------------------------------------------------------------------------------------------
    // FALL B -- DER KOEDER. Eine real gemessene Zeile, in der KEINE Quelle geliefert hat, aber alle
    // sieben Zaehler-Felder eine Zahl tragen. So sieht der Defekt aus, den B-5 heilt: die Zahlen sind
    // Muell aus dem POD, und ohne die Flags haette die CSV sie als Messwerte ausgegeben.
    // Wird auch nur EIN Zaehler wieder ungated (zurueck auf `zelle` statt `pmc_zelle`), taucht seine
    // Zahl hier auf und dieser Fall wird ROT.
    // ------------------------------------------------------------------------------------------
    std::printf("\n[B] Koeder: available=1, ALLE Quellen-Flags false, alle Werte != 0\n");
    ceb::LazyMeasuredRow koeder{};
    koeder.pmc.available               = true; // die Zeile IST eine echte Messung -- nur die PMC-Quellen fehlen
    koeder.pmc.cache_misses_l1         = 111111;
    koeder.pmc.cache_misses_l2         = 222222;
    koeder.pmc.cache_misses_l3         = 333333;
    koeder.pmc.dtlb_misses             = 444444;
    koeder.pmc.coherence_invalidations = 555555;
    koeder.pmc.energy_micro_joules     = 666666;
    koeder.pmc.branch_misses           = 777777;
    // ALLE sieben Quellen-Flags bleiben auf ihrem Default false = "diese Quelle hat nichts geliefert".

    std::vector<std::string> const kopf     = spalten(ceb::lazy_csv_header());
    std::vector<std::string> const zeile    = spalten(ceb::format_csv_row(koeder));
    std::string_view const         erwartet = cem::sample_status_token(cem::SampleStatus::SourceUnavailable);

    for (std::size_t i = 0; i < kPmcSpaltenZahl; ++i) {
        std::string const wert = zell_wert(kopf, zeile, kPmcSpalten[i]);
        std::printf("    %-30s -> '%s'\n", kPmcSpalten[i], wert.c_str());
        pruefe(wert == erwartet, kPmcSpalten[i]);
        // Die haerteste Form derselben Aussage: die Zelle darf die Zahl NICHT enthalten, auch nicht
        // als Teilstring. Ein Renderer, der "0" oder "111111" durchreicht, faellt hier auf.
        pruefe(wert.find_first_of("0123456789") == std::string::npos,
               "eine nicht gelieferte PMC-Zelle darf KEINE Ziffer enthalten");
    }

    // ------------------------------------------------------------------------------------------
    // FALL C -- die Gegenrichtung. Eine ECHTE Null ist eine Zahl und bleibt eine. Ohne diesen Fall
    // waere Fall B auch dann gruen, wenn jemand die Zellen pauschal auf "n/a" verdrahtet -- und das
    // waere nur die umgekehrte Luege: ein realer Nullbefund duerfte dann nie mehr sichtbar werden.
    // ------------------------------------------------------------------------------------------
    std::printf("\n[C] Gegenrichtung: Quelle hat geliefert, der Wert ist echt 0\n");
    ceb::LazyMeasuredRow echt_null{};
    echt_null.pmc.available                        = true;
    echt_null.pmc.cache_misses_l1                  = 0;
    echt_null.pmc.cache_misses_l1_source_available = true;
    echt_null.pmc.cache_misses_l3                  = 0;
    echt_null.pmc.cache_misses_l3_source_available = true;
    echt_null.pmc.dtlb_misses                      = 0;
    echt_null.pmc.dtlb_misses_source_available     = true;
    echt_null.pmc.branch_misses                    = 4242;
    echt_null.pmc.branch_misses_source_available   = true;

    std::vector<std::string> const zeile_c = spalten(ceb::format_csv_row(echt_null));
    for (char const* spalte : {"pmc_cache_misses_l1", "pmc_cache_misses_l3", "pmc_dtlb_misses"}) {
        std::string const wert = zell_wert(kopf, zeile_c, spalte);
        std::printf("    %-30s -> '%s'\n", spalte, wert.c_str());
        pruefe(wert == "0", "eine ECHTE Null bleibt eine Null -- kein Token verdeckt sie");
    }
    pruefe(zell_wert(kopf, zeile_c, "pmc_branch_misses") == "4242",
           "ein gelieferter Wert bleibt unveraendert eine Zahl");
    // Und die ungelieferten Felder DERSELBEN Zeile tragen weiter den Token -- die Ehrlichkeit ist
    // feinkoernig JE ZAEHLER, nicht zeilen-weit. Genau das war der M-3a-Befund an system_axis.hpp.
    pruefe(zell_wert(kopf, zeile_c, "pmc_cache_misses_l2") == erwartet,
           "in derselben Zeile traegt eine nicht gelieferte Quelle weiter n/a");

    // ------------------------------------------------------------------------------------------
    // FALL D -- Drift-Wache auf die Spaltenzahl selbst. Kommt ein achter PMC-Zaehler dazu, ohne dass
    // diese Liste mitwaechst, prueft Fall B ihn nie. Die Wache stuende dann still daneben -- exakt die
    // Bauform "eine Wache, die bei leerem Gegenstand gruen meldet", die dieses Paket bekaempft.
    // ------------------------------------------------------------------------------------------
    std::printf("\n[D] Drift-Wache auf die Spaltenliste\n");
    std::size_t gefunden = 0;
    for (auto const& spalte : kopf) {
        if (spalte.rfind("pmc_", 0) == 0 && spalte != "pmc_available") ++gefunden;
    }
    std::printf("    pmc_*-Wertspalten im ECHTEN Header: %zu, in dieser Wache gefuehrt: %zu\n", gefunden,
                kPmcSpaltenZahl);
    pruefe(gefunden == kPmcSpaltenZahl,
           "die Wache fuehrt JEDE pmc_*-Wertspalte des echten Headers -- sonst prueft sie an einer vorbei");

    std::printf("\n%s (%d Fehler)\n", fehler == 0 ? "ALLE FAELLE BESTANDEN" : "FEHLGESCHLAGEN", fehler);
    return fehler == 0 ? 0 : 1;
}
