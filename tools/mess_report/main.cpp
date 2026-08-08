// SPDX-License-Identifier: Apache-2.0
// tools/mess_report/main.cpp -- comdare-mess-report: der KONSUMENT des A9-S3-xlsx-Writers (A9-S4).
//
// A9-Doc Abschnitt 4.3: "Haupt-Konsument = neues CLI tools/mess_report/ (comdare_mess_report):
// offizielle Mess-CSV(s) -> xlsx-Baum. Subcommand-Stil wie Planer-CLI (clig.dev): report
// render|plan|version; plan = dry-run und druckt den Ziel-Baum literal
// (Kein-Haken-ohne-Ausgabe-tauglich)."
//
// CLI (clig.dev, Muster aus apps/experiment_planner/main.cpp: FLACHER Subkommando-Dispatcher, KEIN
// Gruppen-Verb -- Lead-Entscheid A9-S4-Plan Punkt 1: "comdare-mess-report report render" traegt das
// Wort dreimal in einer Zeile, also flach):
//   render (--csv=<pfad> | --realm-root=<dir>) --out=<dir> [--format=xlsx|csv] [--dry-run]
//   plan   (--csv=<pfad> | --realm-root=<dir>) --out=<dir> [--format=xlsx|csv]
//   version
//   help [<subcommand>]
//
// render/plan-BEZIEHUNG: plan ist render mit erzwungenem dry_run=true UND einem Epochen-
// Zeitstempel-Platzhalter (Determinismus, LED-61) statt der echten Wall-Clock-Zeit. render OHNE
// --dry-run schreibt tatsaechlich; render --dry-run verhaelt sich wie plan, druckt aber mit der
// ECHTEN Zeit (Vorschau des Laufs, den man gleich ausfuehren wuerde, nicht des kanonischen
// Zeitstempel-Platzhalters).
//
// --fassung3 (Fassung 3, checkpoint_measure ce cc028e1d -- NICHT gebaut): ein ausdrueckliches Flag,
// das gegen eine Quelle ohne die 8 Pflichtspalten LAUT abbricht. KEIN stiller Fassung-1/2-
// Rueckfall, KEIN Platzhalter (Lead-Entscheid A9-S4-Plan Punkt 2).
//
// Ausgaben (clig.dev): Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr.

#include "mess_report_render.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace mr  = ::comdare::cache_engine::tools::mess_report;
namespace lab = ::comdare::cache_engine::builder::lager_ablage;

// ---------------------------------------------------------------------------------------------------------------
// Flag-Parsing: dieselbe '--flag=wert'-Praefix-Form + '-'-Positional-Reserve wie
// apps/experiment_planner/main.cpp (status-Subkommando). Flags und Positional in beliebiger
// Reihenfolge.
struct RenderFlags {
    std::string csv;
    std::string realm_root;
    std::string out;
    std::string format   = "xlsx";
    bool        dry_run  = false;
    bool        fassung3 = false;
    bool        hilfe    = false;
    std::string unbekanntes_flag;
};

[[nodiscard]] bool praefix(std::string_view a, std::string_view p) noexcept { return a.rfind(p, 0) == 0; }

[[nodiscard]] RenderFlags parse_render_flags(std::vector<std::string> const& args, std::size_t ab_index) {
    RenderFlags f;
    for (std::size_t i = ab_index; i < args.size(); ++i) {
        std::string const& a = args[i];
        if (a == "--help" || a == "-h") {
            f.hilfe = true;
        } else if (praefix(a, "--csv=")) {
            f.csv = a.substr(std::string_view{"--csv="}.size());
        } else if (praefix(a, "--realm-root=")) {
            f.realm_root = a.substr(std::string_view{"--realm-root="}.size());
        } else if (praefix(a, "--out=")) {
            f.out = a.substr(std::string_view{"--out="}.size());
        } else if (praefix(a, "--format=")) {
            f.format = a.substr(std::string_view{"--format="}.size());
        } else if (a == "--dry-run") {
            f.dry_run = true;
        } else if (a == "--fassung3") {
            f.fassung3 = true;
        } else {
            f.unbekanntes_flag = a;
            return f;
        }
    }
    return f;
}

void help_for(std::string const& topic) {
    if (topic == "render") {
        std::cout << "comdare-mess-report render (--csv=<pfad> | --realm-root=<dir>) --out=<dir>\n"
                     "                            [--format=xlsx|csv] [--dry-run] [--fassung3]\n"
                     "  Liest offizielle Mess-CSV(s) und schreibt einen xlsx- (Default) oder csv-Baum nach --out.\n"
                     "  Quelle GENAU EINE: --csv=<pfad> (eine Datei) XOR --realm-root=<dir> (rekursiver Scan nach\n"
                     "  *.result.csv; .stale-Bestaende werden weder gelesen noch angefasst).\n"
                     "  --dry-run: liest und gruppiert wie ein echter Lauf, schreibt aber NICHTS (Vorschau mit der\n"
                     "  echten Wall-Clock-Zeit dieses Aufrufs -- fuer eine deterministische Vorschau: 'plan').\n"
                     "  --fassung3: verlangt die 8 checkpoint_measure-Pflichtspalten (prozess/thread/mess_ebene/\n"
                     "  ziel/aufrufer/checkpoint/zeitpunkt/messwert, ce cc028e1d). Fehlen sie -> LAUTER Abbruch,\n"
                     "  KEIN stiller Fassung-1/2-Rueckfall (checkpoint_measure ist noch nicht gebaut).\n"
                     "  Exit: 0 ok; 1 Usage; 2 Quelle/Gruppierung/Dateiname-Fehler; 3 Fassung3 ohne Pflichtspalten.\n";
        return;
    }
    if (topic == "plan") {
        std::cout << "comdare-mess-report plan (--csv=<pfad> | --realm-root=<dir>) --out=<dir> [--format=xlsx|csv]\n"
                     "  Dry-run: druckt den Ziel-Baum literal (Dateiname, Sheets je binary_id, konstante\n"
                     "  Meta-Achsen, uebersprungene/gekappte Zeilen) -- schreibt NICHTS. Deterministisch: der\n"
                     "  Zeitstempel ist auf den Epochen-Platzhalter 19700101-000000 gepinnt (LED-61: zwei Laeufe\n"
                     "  ueber dieselben Quellen liefern byte-gleichen Text).\n"
                     "  Exit: 0 ok; 1 Usage; 2 Quelle/Gruppierung/Dateiname-Fehler.\n";
        return;
    }
    if (topic == "version") {
        std::cout << "comdare-mess-report version\n"
                     "  Druckt den Werkzeug-Selbst-Stempel (Name + statische Version).\n";
        return;
    }
    std::cout << "comdare-mess-report -- der Konsument des A9-S3-xlsx-Writers (A9-S4)\n\n"
                 "Usage:\n"
                 "  comdare-mess-report <subcommand> [argumente]\n\n"
                 "Subcommands:\n"
                 "  render (--csv=<pfad> | --realm-root=<dir>) --out=<dir> [--format=xlsx|csv] [--dry-run]\n"
                 "                           Mess-CSV(s) -> xlsx/csv-Baum (schreibt tatsaechlich)\n"
                 "  plan   (--csv=<pfad> | --realm-root=<dir>) --out=<dir> [--format=xlsx|csv]\n"
                 "                           deterministische Vorschau, schreibt NICHTS\n"
                 "  version                  Werkzeug-Selbst-Stempel\n"
                 "  help [<subcommand>]      diese Uebersicht bzw. Detail-Hilfe (auch: <subcommand> --help)\n\n"
                 "Ausgaben: Daten/Emissionen -> stdout; Diagnose/Fehler -> stderr (clig.dev).\n"
                 "Exit-Codes: 0 Erfolg | 1 Usage | 2 Quelle/Gruppierung/Dateiname-Fehler | 3 Fassung3 ohne\n"
                 "Pflichtspalten.\n";
}

[[nodiscard]] int fehler_usage(std::string const& was) noexcept {
    std::cerr << "comdare-mess-report: " << was << " -- 'comdare-mess-report help' zeigt die Uebersicht.\n";
    return 1;
}

/// Loest die Quellen-Liste auf: GENAU EINE von --csv/--realm-root. Bei --realm-root wird sofort
/// gescannt (fuer die stale_uebersprungen-Zahl, die sowohl render als auch plan berichten).
struct QuellenAufloesung {
    std::vector<std::filesystem::path> pfade;
    std::size_t                        stale_uebersprungen = 0;
    bool                               ok                  = false;
    std::string                        fehler;
};

[[nodiscard]] QuellenAufloesung loese_quellen(RenderFlags const& f) {
    QuellenAufloesung out;
    bool const        hat_csv  = !f.csv.empty();
    bool const        hat_root = !f.realm_root.empty();
    if (hat_csv == hat_root) { // beide oder keine
        out.fehler = "genau EINE Quelle angeben: --csv=<pfad> XOR --realm-root=<dir>";
        return out;
    }
    if (hat_csv) {
        out.pfade.push_back(std::filesystem::path{f.csv});
        out.ok = true;
        return out;
    }
    auto const scan = mr::scanne_realm_root(std::filesystem::path{f.realm_root});
    if (!scan.wurzel_vorhanden) {
        out.fehler = "--realm-root='" + f.realm_root + "' existiert nicht";
        return out;
    }
    out.pfade               = scan.gefundene_csvs;
    out.stale_uebersprungen = scan.uebersprungen_stale;
    if (out.pfade.empty()) {
        out.fehler = "--realm-root='" + f.realm_root + "' enthaelt keine *.result.csv (nach .stale-Ausschluss)";
        return out;
    }
    out.ok = true;
    return out;
}

/// Fassung-3-Bissprobe: prueft die Kopfzeile der ERSTEN Quelle gegen die 8 Pflichtspalten. Ein
/// gemischter Quellensatz (manche Dateien tragen die Spalten, manche nicht) waere ein Folgeposten --
/// heute (checkpoint_measure ungebaut) gibt es real KEINE Quelle, die sie traegt, die erste Datei
/// reicht fuer den ehrlichen Abbruch.
[[nodiscard]] std::vector<std::string> fehlende_fassung3_spalten(std::vector<std::string> const& header) {
    ::comdare::cache_engine::measurement::csv::HeaderIndex const idx(header);
    std::vector<std::string>                                     fehlend;
    for (std::string_view s : mr::kFassung3PflichtSpalten) {
        std::size_t dummy = 0;
        if (!idx.find(s, dummy)) fehlend.emplace_back(s);
    }
    return fehlend;
}

[[nodiscard]] int run_render_or_plan(RenderFlags const& f, bool ist_plan) noexcept {
    try {
        if (f.hilfe) {
            help_for(ist_plan ? "plan" : "render");
            return 0;
        }
        if (!f.unbekanntes_flag.empty()) return fehler_usage("unbekanntes Flag '" + f.unbekanntes_flag + "'");
        if (f.out.empty()) return fehler_usage("--out=<dir> ist Pflicht");
        if (f.format != "xlsx" && f.format != "csv")
            return fehler_usage("--format muss 'xlsx' oder 'csv' sein, nicht '" + f.format + "'");

        QuellenAufloesung const q = loese_quellen(f);
        if (!q.ok) return fehler_usage(q.fehler);

        if (f.fassung3) {
            auto const erste   = mr::lies_wide_csv(q.pfade.front());
            auto const fehlend = fehlende_fassung3_spalten(erste.header);
            if (!fehlend.empty()) {
                std::cerr << "comdare-mess-report: --fassung3 verlangt die checkpoint_measure-Pflichtspalten "
                             "(SOLL-Design ce cc028e1d), aber '"
                          << q.pfade.front().string() << "' traegt sie nicht. Es fehlen: ";
                for (std::size_t i = 0; i < fehlend.size(); ++i) std::cerr << (i ? ", " : "") << fehlend[i];
                std::cerr << ". checkpoint_measure ist noch nicht gebaut -- KEIN stiller Fassung-1/2-Rueckfall.\n";
                return 3;
            }
            // Fassung-3-Schreibpfad selbst folgt, sobald checkpoint_measure real Daten liefert
            // (Sequenz-Regel A9-Doc Abschnitt 7); bis dahin ist das obige der vollstaendige Vertrag.
        }

        lab::ErgebnisFormat const fmt = (f.format == "csv") ? lab::ErgebnisFormat::csv : lab::ErgebnisFormat::xlsx;

        if (ist_plan || f.dry_run) {
            mr::DatumZeit const  dz = ist_plan ? mr::kPlanZeitstempelPlatzhalter : mr::jetzt_utc();
            mr::RenderPlan const plan =
                mr::berechne_plan(q.pfade, std::filesystem::path{f.out}, dz, fmt, q.stale_uebersprungen);
            mr::drucke_plan(plan, std::cout);
            return 0;
        }

        mr::fuehre_render_aus(q.pfade, std::filesystem::path{f.out}, mr::jetzt_utc(), fmt);
        std::cout << "comdare-mess-report: geschrieben nach " << f.out << "\n";
        return 0;
    } catch (mr::MessReportFehler const& e) {
        std::cerr << "[Fehler: " << (ist_plan ? "plan" : "render") << "] " << e.what() << "\n";
        return 2;
    } catch (lab::ErgebnisSchreibFehler const& e) {
        std::cerr << "[Fehler: " << (ist_plan ? "plan" : "render") << "] " << e.what() << "\n";
        return 2;
    } catch (std::exception const& e) {
        std::cerr << "[Fehler: " << (ist_plan ? "plan" : "render") << "] " << e.what() << "\n";
        return 2;
    } catch (...) {
        std::cerr << "[Fehler: " << (ist_plan ? "plan" : "render") << "] unbekannte Ausnahme\n";
        return 2;
    }
}

[[nodiscard]] int run_version_guarded() noexcept {
    try {
        std::cout << "comdare-mess-report 1.0.0\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "[Fehler: version] " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[Fehler: version] unbekannte Ausnahme\n";
        return 1;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
    std::string const a1 = args.size() > 1 ? args[1] : std::string{};

    if (a1.empty()) return fehler_usage("kein Subkommando angegeben");
    if (a1 == "--help" || a1 == "-h") {
        help_for({});
        return 0;
    }
    if (a1 == "help") {
        help_for(args.size() > 2 ? args[2] : std::string{});
        return 0;
    }
    if (a1 == "version" || a1 == "--version") return run_version_guarded();
    if (a1 == "render") return run_render_or_plan(parse_render_flags(args, 2), /*ist_plan=*/false);
    if (a1 == "plan") return run_render_or_plan(parse_render_flags(args, 2), /*ist_plan=*/true);

    return fehler_usage("unbekanntes Subkommando '" + a1 + "' -- erwartet: render | plan | version | help");
}
