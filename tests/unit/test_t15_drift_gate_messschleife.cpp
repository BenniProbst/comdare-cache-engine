// test_t15_drift_gate_messschleife.cpp -- T-15 (2026-08-09): DAS DRIFT-GATE IN DER MESS-SCHLEIFE.
//
// SELBSTCHECK: dieser Test behauptet NICHTS ueber die Drift-MECHANIK (die haelt test_chaos_drift_gate
// seit #197). Er behauptet genau eine Sache: DIE ZELLE DER PRODUKTIVEN MESS-SCHLEIFE LAEUFT DURCH DAS
// GATE. Wer hier eine Median- oder Schwellen-Rechnung nachprueft, ist in der falschen Datei.
//
// DER BEFUND, DER IHN AUSGELOEST HAT (Register S5-06 / Ledger-Nachtrag 09.08.2026, am Objekt
// nachgeprueft: exakt 3 Nicht-Build-Dateien im ganzen Repo nannten run_with_drift_gate ueberhaupt --
// der Header selbst, sein Test und dessen CMakeLists; 0 von 1824 .hpp/.cpp ausserhalb davon):
//
//     "Das Drift-Gate laeuft ueber keinen einzigen realen Messwert. Die 5-%-Regel ueber 3
//      Wiederholungen ist implementiert und getestet -- aber nicht in die Mess-Schleife geklammert."
//
// DIE ROTE AUSGABE VOR DEM BAU, woertlich (dieselbe Datei, Abschnitt 1, gegen den damaligen
// Zell-Rumpf "PermResult const pr = ... run_observable_perm(...)"):
//
//     -- (1) Drift 100 % > 5 % -> Rerun --
//          Mess-Aufrufe = 1, Versuche = 1, Reruns = 0, Gate aktiv = nein
//       [ERR] das Gate war ueberhaupt aktiv
//       [ERR] Reruns bei 100 % Drift = 0  (erwartet 1)
//
// KEIN ZUFALL (Owner-KERN "Wir arbeiten NIE mit Zufall -- ausser fuer Koeder"): die Mess-Werte dieses
// Tests sind ein FESTES Skript aus ns-Zahlen, keine echte Zeitnahme. Eine Wall-Clock-Messung waere
// hier nicht reproduzierbar und die Gegenrichtung ("unter der Schwelle -> KEIN Rerun") gar nicht
// pruefbar -- ein Gate, das immer feuert, macht jede Messung unbezahlbar und waere genauso falsch wie
// eines, das nie feuert.
//
// Build: Standalone int main() (kein gtest), reiner Host-Pfad -- kein Tier, keine DLL, keine Achse.

#include <harness/drift_gated_cell.hpp>                              // die Klammer (T-15)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // LazyRunConfig/-Row + CSV-Schema
#include <serialization/xml_config_parser/xml_config_parser.hpp>     // <drift_gate .../> aus dem Profil-XML

#include "comdare_test_tmp.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;

namespace {

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void eq(std::string const& was, A const& ist, B const& soll) {
    bool const ok = (ist == static_cast<A>(soll));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << " = " << ist;
    if (!ok) std::cout << "  (erwartet " << soll << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// == Die Ersatz-Zelle: EIN Skript aus ns-Zahlen statt einer echten Zeitnahme ========================
// Sie steht fuer PermResult::total_ns -- die Groesse, die perm_runner je Zelle als steady_clock-Paar
// (run_observable_perm) bzw. als Summe der per-Op-Samples (run_workload_perm) bildet. Jeder Aufruf
// liefert den naechsten Skript-Wert und zaehlt sich selbst mit; laeuft das Skript aus, wiederholt sich
// der letzte Wert (so bleibt jede Gruppen-Laenge definiert).
struct SkriptZelle {
    std::vector<std::int64_t> werte;
    std::size_t               aufrufe = 0;

    [[nodiscard]] std::int64_t operator()() {
        std::int64_t const ns = werte.empty() ? 0 : werte[(aufrufe < werte.size()) ? aufrufe : werte.size() - 1];
        ++aufrufe;
        return ns;
    }
};

// Die Nutzlast der Zelle im Test: eine nackte ns-Zahl. Im produktiven Pfad ist es PermResult -- die
// Klammer ist ueber die Nutzlast templatisiert und sieht von ihr nur, was sample_of() herauszieht.
[[nodiscard]] std::int64_t probe_von(std::int64_t const& ns) { return ns; }

[[nodiscard]] std::vector<std::string> spalten(std::string const& zeile) {
    std::vector<std::string> out;
    std::string              cur;
    for (char const c : zeile) {
        if (c == ';') {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\n' && c != '\r') {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

[[nodiscard]] std::size_t spalten_index(std::vector<std::string> const& kopf, std::string_view name) {
    for (std::size_t i = 0; i < kopf.size(); ++i)
        if (kopf[i] == name) return i;
    return kopf.size();
}

[[nodiscard]] std::string zelle_von(std::vector<std::string> const& kopf, std::vector<std::string> const& zellen,
                                    std::string_view name) {
    std::size_t const i = spalten_index(kopf, name);
    if (i >= kopf.size() || i >= zellen.size()) return {};
    return zellen[i];
}

[[nodiscard]] std::string trimme(std::string const& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

} // namespace

int main() {
    std::cout << "== T-15: das Drift-Gate in der Mess-Schleife ==\n";

    ex::DriftGateConfig cfg; // die Produktions-Defaults
    cfg.reps       = 3;
    cfg.threshold  = 0.05;
    cfg.max_reruns = 2; // im Test klein gehalten; Abschnitt (6) prueft den echten Default

    // == (1) DER KERN: eine Zelle mit > 5 % gestreuten Wiederholungen MUSS einen Rerun ausloesen ====
    // Skript: {100, 100, 200} -> Median 100, Spannweite 100 -> Drift 100 % > 5 % -> instabil.
    // Danach {100, 100, 100} -> Drift 0 % -> stabil. Erwartung: 2 Versuche, 1 Rerun, am Ende stabil,
    // 6 Mess-Aufrufe (2 Gruppen a 3 Wiederholungen).
    {
        std::cout << "-- (1) Drift 100 % > 5 % -> Rerun --\n";
        SkriptZelle        zelle{{100, 100, 200, 100, 100, 100}, 0};
        std::ostringstream warn;

        auto const e = ex::run_cell_with_drift_gate(cfg, zelle, probe_von, &warn, "t15_kern");

        std::cout << "     Mess-Aufrufe = " << zelle.aufrufe << ", Versuche = " << e.attempts
                  << ", Reruns = " << e.reruns << ", Gate aktiv = " << (e.gate_aktiv ? "ja" : "nein") << "\n";
        tr("das Gate war ueberhaupt aktiv", e.gate_aktiv);
        eq("Reruns bei 100 % Drift", e.reruns, std::size_t{1});
        eq("Versuche (1 Gruppe + 1 Rerun)", e.attempts, std::size_t{2});
        tr("die angenommene Gruppe ist stabil", e.stable);
        tr("die angenommene Gruppe hat eine Drift-AUSSAGE (D4)", e.verdict.bestimmbar);
        eq("Mess-Aufrufe gesamt (2 Gruppen a 3)", zelle.aufrufe, std::size_t{6});
        eq("gezaehlte Einzelmessungen", e.messungen, std::size_t{6});
        eq("behaltene Probe = zuletzt gemessene der angenommenen Gruppe", e.payload, std::int64_t{100});
        tr("jedes Rerun-Ereignis steht als WARNUNG im Log",
           warn.str().find("Wiederholungs-Drift") != std::string::npos);
    }

    // == (2) DIE GEGENRICHTUNG: unter der Schwelle darf KEIN Rerun stattfinden ======================
    // Skript: {100, 101, 102} -> Median 101, Spannweite 2 -> Drift 1,98 % < 5 % -> stabil beim ersten
    // Versuch. Die Folgewerte sind absichtlich absurd hoch: wuerde hier ein Rerun laufen, faellt es
    // ueber die behaltene Probe sofort auf. Ein Gate, das hier feuert, verdreifacht die Kampagne ohne
    // jede Erkenntnis.
    {
        std::cout << "-- (2) Drift 1,98 % < 5 % -> KEIN Rerun --\n";
        SkriptZelle        zelle{{100, 101, 102, 999999, 999999, 999999}, 0};
        std::ostringstream warn;

        auto const e = ex::run_cell_with_drift_gate(cfg, zelle, probe_von, &warn, "t15_gegenrichtung");

        std::cout << "     Mess-Aufrufe = " << zelle.aufrufe << ", Versuche = " << e.attempts
                  << ", Reruns = " << e.reruns << "\n";
        eq("Reruns unter der Schwelle", e.reruns, std::size_t{0});
        eq("Versuche (nur die erste Gruppe)", e.attempts, std::size_t{1});
        tr("stabil", e.stable);
        eq("Mess-Aufrufe gesamt (genau eine Gruppe a 3)", zelle.aufrufe, std::size_t{3});
        eq("behaltene Probe (letzte der ersten Gruppe)", e.payload, std::int64_t{102});
        tr("kein Warn-Log", warn.str().empty());
    }

    // == (3) DIE SCHWELLE SELBST: 5 % ist die Grenze, und sie ist STRIKT ============================
    // {100,100,105}: Median 100, Spannweite 5 -> Drift genau 5 %. Die Regel lautet "> 5 %", also
    // stabil. {100,100,106} -> 6 % -> Rerun. Ohne dieses Paar waere nicht belegt, dass ueberhaupt die
    // konfigurierte Schwelle wirkt und nicht irgendeine andere Zahl.
    {
        std::cout << "-- (3) die Schwelle: genau 5 % haelt, 6 % nicht --\n";
        SkriptZelle zelle_genau{{100, 100, 105, 100, 100, 100}, 0};
        auto const  genau = ex::run_cell_with_drift_gate(cfg, zelle_genau, probe_von, nullptr, "t15_genau5");
        eq("genau 5 % -> Reruns", genau.reruns, std::size_t{0});
        tr("genau 5 % -> stabil", genau.stable);

        SkriptZelle zelle_drueber{{100, 100, 106, 100, 100, 100}, 0};
        auto const  drueber = ex::run_cell_with_drift_gate(cfg, zelle_drueber, probe_von, nullptr, "t15_ueber5");
        eq("6 % -> Reruns", drueber.reruns, std::size_t{1});
        tr("6 % -> nach dem Rerun stabil", drueber.stable);
    }

    // == (4) BUDGET ERSCHOEPFT: der Deckel haelt, und die Zelle sagt es =============================
    // Durchgehend instabil. Die Drift-Folge ist bewusst 100 %, 20 %, 50 % -- die BESTE Gruppe ist die
    // MITTLERE, nicht die letzte. Nur so unterscheidet der Test "die stabilste Gruppe wird behalten"
    // von "die zuletzt gemessene wird behalten": die letzte Probe der besten Gruppe ist 120, die
    // zuletzt gemessene ueberhaupt ist 150. Mit gleichfoermig fallender Drift waeren beide gleich und
    // eine kaputte Gruppen-Zuordnung bliebe unsichtbar.
    // max_reruns=2 -> 3 Gruppen -> 9 Messungen.
    {
        std::cout << "-- (4) Rerun-Budget erschoepft (advisory, kein Abbruch) --\n";
        SkriptZelle        zelle{{100, 100, 200, 100, 100, 120, 100, 100, 150, 100, 100, 100}, 0};
        std::ostringstream warn;

        auto const e = ex::run_cell_with_drift_gate(cfg, zelle, probe_von, &warn, "t15_erschoepft");

        std::cout << "     Mess-Aufrufe = " << zelle.aufrufe << ", Versuche = " << e.attempts
                  << ", Reruns = " << e.reruns << ", erschoepft = " << (e.exhausted ? "ja" : "nein") << "\n";
        eq("Versuche (1 + max_reruns)", e.attempts, std::size_t{3});
        eq("Reruns == max_reruns", e.reruns, std::size_t{2});
        tr("erschoepft", e.exhausted);
        tr("NICHT als stabil ausgegeben", !e.stable);
        eq("Mess-Aufrufe gesamt (3 Gruppen a 3)", zelle.aufrufe, std::size_t{9});
        eq("behaltene Probe = letzte der STABILSTEN Gruppe", e.payload, std::int64_t{120});
        tr("finale Warnung im Log", warn.str().find("weiterhin instabil") != std::string::npos);
    }

    // == (5) GATE AUS (reps < 2): GENAU EIN Aufruf -- das Verhalten vor dieser Scheibe ==============
    // D4-Regel: eine einzelne Probe hat keine Wiederholungs-Streuung. Die Zelle behauptet dann auch
    // keine Stabilitaet -- bestimmbar bleibt false ("keine Aussage"), nicht "stabil".
    {
        std::cout << "-- (5) Gate aus (reps < 2) -> genau eine Messung, keine Aussage --\n";
        ex::DriftGateConfig aus;
        aus.reps = 1;
        SkriptZelle zelle{{4242, 999999, 999999}, 0};

        auto const e = ex::run_cell_with_drift_gate(aus, zelle, probe_von, nullptr, "t15_aus");

        eq("Mess-Aufrufe", zelle.aufrufe, std::size_t{1});
        eq("Reruns", e.reruns, std::size_t{0});
        tr("gate_aktiv == false", !e.gate_aktiv);
        tr("KEINE Drift-Aussage (D4: bestimmbar == false)", !e.verdict.bestimmbar);
        tr("und auch NICHT als stabil ausgegeben", !e.stable);
        eq("behaltene Probe = die eine Messung", e.payload, std::int64_t{4242});
        tr("aktiv() sagt dasselbe", !aus.aktiv());
    }

    // == (6) DIE PRODUKTIONS-DEFAULTS NACH DEM KON26-04-UMZUG ======================================
    // GOAL-v8 VI.5: "Drift-Gate 5 % ueber 3 Wiederholungen [...] beim Scheitern bis zu 5
    // Wiederholungen". KON26-04 (20.08.2026, owner-entschieden) hat die ZWEI Lesarten der 5
    // getrennt: die Owner-5 gehoert zum BINARY-RETRY (T-15b, MessRetryKonfig -- bis zu 5 Versuche
    // des gesamten Pruefdock-Durchlaufs bei hartem Scheitern, KON37-06 "je 5 Mal"); der
    // Drift-Rerun (Gruppen-Rerun bei STREUUNG) faellt auf den Mechanismus-Default 3 des Detektors
    // (#197) zurueck. Der fruehere Pin "max_reruns == 5" hier war der Stand ce 4cd1ab91 -- die
    // FALSCHE der zwei im Commit dokumentierten Lesarten; das Register S5-06 ist datiert ueberholt.
    {
        std::cout << "-- (6) Produktions-Defaults == Owner-Zahlen (C-07-Kanon) --\n";
        ex::DriftGateConfig const d;
        eq("Default reps", d.reps, std::uint32_t{3});
        tr("Default threshold == 5 %", d.threshold > 0.0499 && d.threshold < 0.0501);
        eq("Default max_reruns (Mechanismus-3; die 5 ist umgezogen)", d.max_reruns, std::uint32_t{3});
        tr("das Gate ist per Default AN", d.aktiv());

        ex::LazyRunConfig const lauf;
        eq("LazyRunConfig traegt denselben Default", lauf.drift_gate.reps, std::uint32_t{3});
        eq("LazyRunConfig max_reruns", lauf.drift_gate.max_reruns, std::uint32_t{3});
        // Die Owner-5 lebt jetzt HIER -- und speist Mess- UND Bau-Budget aus einer Quelle.
        ex::MessRetryKonfig const rk;
        eq("MessRetryKonfig::max_versuche (Owner-Zahl 5, umgezogen)", rk.max_versuche, std::uint32_t{5});
        eq("LazyRunConfig::mess_retry traegt dieselbe 5", lauf.mess_retry.max_versuche, std::uint32_t{5});
        tr("PAAR-Pflicht per Default (kein --debug-Kaltlauf)", !lauf.mess_kaltlauf_debug);
    }

    // == (7) DIE PROVENIENZ STEHT IN DER AUSGABE ===================================================
    // Ohne die vier Spalten waere eine Zelle, die ihr Rerun-Budget erschoepft hat, von einer beim
    // ersten Versuch stabilen nicht zu unterscheiden. Kontaminierte Messdaten sind die einzige
    // unheilbare Klasse dieser Arbeit -- ihre Sichtbarkeit haengt an genau diesen Zahlen.
    {
        std::cout << "-- (7) die vier Drift-Spalten im CSV-Schema --\n";
        std::vector<std::string> const kopf = spalten(ex::lazy_csv_header());
        for (char const* n : {"drift_reps", "drift_reruns", "drift_relative", "drift_status"})
            tr(std::string{"Spalte "} + n + " existiert", spalten_index(kopf, n) < kopf.size());

        ex::LazyMeasuredRow row;
        row.binary_id                    = "t15_probe";
        row.drift_reps                   = 3;
        row.drift_reruns                 = 2;
        row.drift_relative               = 0.2;
        row.drift_bestimmbar             = true;
        row.drift_stabil                 = false;
        std::vector<std::string> const z = spalten(ex::format_csv_row(row));
        eq("Zellenzahl == Spaltenzahl", z.size(), kopf.size());
        eq("drift_reps", zelle_von(kopf, z, "drift_reps"), std::string{"3"});
        eq("drift_reruns", zelle_von(kopf, z, "drift_reruns"), std::string{"2"});
        eq("drift_status (erschoepft)", zelle_von(kopf, z, "drift_status"), std::string{"instabil"});

        // Gate aus -> KEINE Aussage, und ausdruecklich keine 0, die wie "stabil gemessen" aussaehe.
        ex::LazyMeasuredRow aus;
        aus.binary_id                     = "t15_probe_aus";
        std::vector<std::string> const za = spalten(ex::format_csv_row(aus));
        eq("Gate aus -> drift_reps", zelle_von(kopf, za, "drift_reps"), std::string{"0"});
        eq("Gate aus -> drift_status n/a", zelle_von(kopf, za, "drift_status"), std::string{"n/a"});
        eq("Gate aus -> drift_relative n/a", zelle_von(kopf, za, "drift_relative"), std::string{"n/a"});

        // Bestimmbar == false bei aktivem Gate -> auch dort kein erfundener Anteil (D4).
        ex::LazyMeasuredRow unbest;
        unbest.binary_id                  = "t15_probe_unbest";
        unbest.drift_reps                 = 3;
        unbest.drift_bestimmbar           = false;
        std::vector<std::string> const zu = spalten(ex::format_csv_row(unbest));
        eq("unbestimmbar -> drift_relative n/a", zelle_von(kopf, zu, "drift_relative"), std::string{"n/a"});
        eq("unbestimmbar -> drift_status", zelle_von(kopf, zu, "drift_status"), std::string{"unbestimmbar"});
    }

    // == (8) DIE VERDRAHTUNGS-WACHE ================================================================
    // EHRLICHE EINORDNUNG: das ist eine QUELLEN-Wache, keine Verhaltens-Wache. Die produktive
    // Mess-Schleife (run_lazy_static_then_dynamic) braucht einen Orchestrator und echte .so-Dateien;
    // sie laesst sich in einem Unit-Test nicht fahren, und die Drift waere dort eine echte Wall-Clock-
    // Groesse, also nicht deterministisch stellbar. Was hier geprueft wird, ist deshalb strukturell --
    // aber es faengt GENAU den Defekt, der T-15 ausgeloest hat: ein Mechanismus ohne Aufrufer.
    //
    // Geprueft wird: JEDER Mess-Aufruf des Iterators (run_observable_perm / run_workload_perm) liegt
    // zwischen [T-15-KLAMMER] und [T-15-KLAMMER-ENDE], und diese Klammer ist das Argument von
    // run_cell_with_drift_gate. Wer den Aufruf herausloest, macht diesen Abschnitt rot.
    {
        std::cout << "-- (8) Verdrahtungs-Wache auf der Iterator-Quelle --\n";
        std::ifstream quelle{COMDARE_T15_ITERATOR_QUELLE};
        tr("Iterator-Quelle lesbar", quelle.good());

        std::string zeile;
        std::size_t nr          = 0;
        std::size_t klammer_auf = 0, klammer_zu = 0, gate_aufruf = 0;
        std::size_t mess_aufrufe = 0, mess_aufrufe_in_klammer = 0;
        bool        in_klammer = false;
        while (std::getline(quelle, zeile)) {
            ++nr;
            std::string const t         = trimme(zeile);
            bool const        kommentar = (t.rfind("//", 0) == 0) || (t.rfind("*", 0) == 0);
            if (t.find("// [T-15-KLAMMER]") != std::string::npos) {
                in_klammer = true;
                ++klammer_auf;
                continue;
            }
            if (t.find("// [T-15-KLAMMER-ENDE]") != std::string::npos) {
                in_klammer = false;
                ++klammer_zu;
                continue;
            }
            if (kommentar) continue; // Kommentare nennen die Namen erlaeuternd, das ist kein Aufruf
            if (t.find("run_cell_with_drift_gate(") != std::string::npos) ++gate_aufruf;
            if (t.find("run_observable_perm(") != std::string::npos ||
                t.find("run_workload_perm(") != std::string::npos) {
                ++mess_aufrufe;
                if (in_klammer) ++mess_aufrufe_in_klammer;
            }
        }
        std::cout << "     Zeilen = " << nr << ", Mess-Aufrufe = " << mess_aufrufe << " (davon in der Klammer "
                  << mess_aufrufe_in_klammer << "), run_cell_with_drift_gate-Aufrufe = " << gate_aufruf << "\n";
        eq("genau eine oeffnende Klammer-Marke", klammer_auf, std::size_t{1});
        eq("genau eine schliessende Klammer-Marke", klammer_zu, std::size_t{1});
        tr("die Klammer wird ueberhaupt gerufen", gate_aufruf >= 1);
        tr("es gibt ueberhaupt Mess-Aufrufe", mess_aufrufe >= 1);
        eq("KEIN Mess-Aufruf ausserhalb der Klammer", mess_aufrufe - mess_aufrufe_in_klammer, std::size_t{0});

        // Das LETZTE Kettenglied: der Fassaden-Rand, der die XML-Zahlen in die LazyRunConfig legt.
        // Ohne diese drei Zuweisungen traegt der produktive Lauf stumm die Code-Defaults, und das
        // Profil-XML koennte die Schwelle nicht stellen -- genau die Hartkodierung, die S5-06 ruegt.
        // Auch das ist eine Quellen-Pruefung: make_cfg ist ein Lambda in run_profile und braucht zum
        // Fahren einen vollstaendigen Profil-Lauf.
        std::ifstream rand{COMDARE_T15_FASSADE_QUELLE};
        tr("Fassaden-Quelle lesbar", rand.good());
        std::size_t zuweisungen = 0;
        while (std::getline(rand, zeile)) {
            std::string const t = trimme(zeile);
            if (t.rfind("//", 0) == 0) continue;
            if (t.find("cfg.drift_gate.reps") != std::string::npos ||
                t.find("cfg.drift_gate.threshold") != std::string::npos ||
                t.find("cfg.drift_gate.max_reruns") != std::string::npos)
                ++zuweisungen;
        }
        eq("alle drei XML-Zahlen werden in die LazyRunConfig gelegt", zuweisungen, std::size_t{3});
    }

    // == (9) DIE ZAHLEN KOMMEN AUS DEM PROFIL-XML, NICHT AUS DEM CODE ===============================
    // Register-Befund S5-06: "reps/threshold/max_reruns aus dem Profil-XML, nicht hartkodiert". Die
    // Schwelle ist #156-kalibrierungs-gegatet -- wer sie nach einem mehrtaegigen Kalibrierlauf
    // nachziehen will, darf dafuer nicht neu uebersetzen muessen.
    //
    // Geprueft wird die GANZE Kette: XML -> ThesisProfile -> die drei Zahlen. Und ebenso das Gegenteil:
    // fehlt das Element, bleibt declared==false und der Owner-Default der cache_engine-Schicht stehen
    // (eine Kopie der Zahlen in der common-Schicht waere die zweite Wahrheit).
    {
        std::cout << "-- (9) <drift_gate/> aus dem Profil-XML --\n";
        namespace fs = std::filesystem;
        namespace cx = comdare::builder::xml;

        auto const      dir = comdare::test::user_tmp_dir() / "t15_drift_gate";
        std::error_code ec;
        fs::create_directories(dir, ec);

        auto schreibe = [&](std::string const& name, std::string const& inhalt) {
            std::ofstream f{dir / name, std::ios::trunc};
            f << inhalt;
            f.close();
            return dir / name;
        };
        // reps=4 / 37 Promille / max_reruns=7 sind bewusst KEINE der Default-Zahlen: nur so ist
        // belegt, dass wirklich das XML gelesen wird und nicht der Code-Default durchschlaegt.
        auto const mit = schreibe("mit_drift_gate.profile.xml",
                                  "<comdare_thesis_profile id=\"t15\" schema_version=\"1\">\n"
                                  "  <repetitions count=\"3\"/>\n"
                                  "  <drift_gate reps=\"4\" threshold_permille=\"37\" max_reruns=\"7\"/>\n"
                                  "</comdare_thesis_profile>\n");
        auto const ohne =
            schreibe("ohne_drift_gate.profile.xml", "<comdare_thesis_profile id=\"t15\" schema_version=\"1\">\n"
                                                    "  <repetitions count=\"3\"/>\n"
                                                    "</comdare_thesis_profile>\n");
        // Negativ: eine Schwelle von 0 liesse JEDE Zelle als instabil gelten und verbrennte das
        // Rerun-Budget bei jeder einzelnen -- eine Fehlkonfiguration, die erst nach Tagen Messzeit
        // auffiele. Das Profil muss deshalb unlesbar sein, nicht still normalisiert.
        auto const kaputt = schreibe("kaputtes_drift_gate.profile.xml",
                                     "<comdare_thesis_profile id=\"t15\" schema_version=\"1\">\n"
                                     "  <drift_gate reps=\"3\" threshold_permille=\"0\" max_reruns=\"5\"/>\n"
                                     "</comdare_thesis_profile>\n");

        cx::XmlConfigParser const p;
        auto const                tp_mit = p.parse_thesis_profile(mit);
        tr("Profil MIT <drift_gate> ist lesbar", tp_mit.has_value());
        if (tp_mit.has_value()) {
            tr("declared", tp_mit->drift_gate_declared);
            eq("reps aus dem XML", tp_mit->drift_gate_reps, 4);
            eq("threshold_permille aus dem XML", tp_mit->drift_gate_threshold_permille, 37);
            eq("max_reruns aus dem XML", tp_mit->drift_gate_max_reruns, 7);
            // Dieselbe Umrechnung wie im letzten Kettenglied (profile_run_entry make_cfg).
            ex::DriftGateConfig g;
            g.reps       = static_cast<std::uint32_t>(tp_mit->drift_gate_reps);
            g.threshold  = static_cast<double>(tp_mit->drift_gate_threshold_permille) / 1000.0;
            g.max_reruns = static_cast<std::uint32_t>(tp_mit->drift_gate_max_reruns);
            tr("37 Promille == 3,7 %", g.threshold > 0.0369 && g.threshold < 0.0371);
            // Und die Zahlen wirken auch: 4 Wiederholungen je Gruppe, Schwelle 3,7 %.
            // {100,100,100,104} -> Median 100, Spannweite 4 -> 4 % > 3,7 % -> Rerun.
            SkriptZelle zelle{{100, 100, 100, 104, 100, 100, 100, 100}, 0};
            auto const  e = ex::run_cell_with_drift_gate(g, zelle, probe_von, nullptr, "t15_xml");
            eq("Gruppen-Groesse 4 aus dem XML wirkt", zelle.aufrufe, std::size_t{8});
            eq("4 % ueber der XML-Schwelle 3,7 % -> Rerun", e.reruns, std::size_t{1});
        }

        auto const tp_ohne = p.parse_thesis_profile(ohne);
        tr("Profil OHNE <drift_gate> ist lesbar", tp_ohne.has_value());
        if (tp_ohne.has_value()) tr("declared == false -> Code-Default bleibt", !tp_ohne->drift_gate_declared);

        tr("Schwelle 0 macht das Profil UNLESBAR (nicht still normalisiert)",
           !p.parse_thesis_profile(kaputt).has_value());
    }

    std::cout << (g_fail == 0 ? "== T-15 GRUEN ==\n" : "== T-15 ROT ==\n");
    return g_fail == 0 ? 0 : 1;
}
