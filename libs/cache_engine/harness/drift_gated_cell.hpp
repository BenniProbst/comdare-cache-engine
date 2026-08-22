#pragma once
// drift_gated_cell.hpp -- T-15 (2026-08-09): DIE KLAMMER. Die Zeitnahme JE ZELLE laeuft durch das
// Drift-Gate.
//
// SELBSTCHECK: diese Datei enthaelt KEINE Drift-Mathematik und KEINE Auswahl-Regel fuer die beste
// Gruppe. Beides steht in builder/commands/drift_detector.hpp (assess_drift / run_with_drift_gate,
// #197) und wird hier ausschliesslich GERUFEN. Wer hier eine zweite Median-, Schwellen- oder
// Bestwahl-Rechnung findet, hat eine Methoden-Drift gefunden -- die Datei darf keine haben.
//
// == WARUM ES DIESE DATEI GIBT ======================================================================
// Der Drift-Detektor war seit dem 27.06.2026 gebaut, getestet und CI-verifiziert -- und hatte am
// 09.08.2026 NULL produktive Aufrufer. Nachgemessen am Objekt: exakt drei Nicht-Build-Dateien im
// ganzen Repo nannten run_with_drift_gate ueberhaupt (der Header selbst, sein Test, dessen
// CMakeLists); 0 von 1824 .hpp/.cpp ausserhalb davon. Der Ledger-Nachtrag vom 09.08. sagt es
// woertlich:
//
//     "Das Drift-Gate laeuft ueber keinen einzigen realen Messwert. Die 5-%-Regel ueber 3
//      Wiederholungen ist implementiert und getestet -- aber nicht in die Mess-Schleife geklammert."
//
// Ein Gate, das nichts durchlaesst, weil es nicht laeuft, ist kein Gate. Diese Datei ist das fehlende
// Glied zwischen dem Mechanismus (drift_detector.hpp) und der Zelle (harness/perm_runner.hpp ueber
// builder/experiment_tree/cache_engine_builder_iterator.hpp, Lambda measure_under_setting).
//
// == DIE REGEL, DIE HIER VOLLZOGEN WIRD (GOAL-v8-DOSSIER VI.5, 08.08.2026, GELTENDES ZIEL) =========
//     "Drift-Gate 5 % ueber 3 Wiederholungen; bei Ueberschreitung den ganzen Lauf neu starten -- ein
//      fremder Verbraucher ist aktiv, den man abwarten muss. Beim Scheitern bis zu 5 Wiederholungen."
//
// WAS DAVON HIER GEBAUT IST: die Wiederholungs-Gruppe je Zelle, die Schwelle, das Rerun-Budget --
// alle drei aus dem Profil-XML, nicht hartkodiert (DriftGateConfig).
//
// C-07-BEGRIFFS-KANON (KON26-04, 2026-08-20 -- der VI.5-Satz enthaelt DREI verschieden grosse
// Mechanismen, die vorher unter einem Wort liefen; die frueher hier deklarierte Undefiniertheit von
// "ganzer Lauf" ist damit GESCHLOSSEN, nicht mehr offen):
//   (1) "den ganzen Lauf neu starten" (T-15a) = die KAMPAGNE -- Granularitaet KON19-06 (= ALLES,
//       die mehrtaegige Kampagne, nicht Zelle/Batch/Tag). Lebt NICHT hier (Wiederaufsetzpunkt-
//       Posten); diese Datei baut weiterhin nur die GRUPPE.
//   (2) "Beim Scheitern bis zu 5 Wiederholungen" (T-15b) = bis zu 5 Versuche des GESAMTEN
//       Pruefdock-Durchlaufs EINER Tier-Binary bei hartem Fehlschlag -- GEBAUT in
//       harness/mess_retry_klammer.hpp (Bau- und Mess-Budget je 5, KON37-06/KON28-02).
//   (3) das Rerun-Budget DIESER Datei = Gruppen-Rerun bei STREUUNG (Drift ueber der Schwelle) --
//       eine dritte Groesse mit eigener Bedingung und eigenem Default (3, s. max_reruns unten).
// "T-15 ist kein CI-Gate" (C-07 Punkt 3) ist ebenfalls geschlossen: die ctest-Registrierung von
// test_t15_drift_gate_messschleife + test_t15b_retry_warmup_paar IST das Gate -- jede CI-Testzelle
// faehrt ctest; ein eigener YAML-Job waere eine zweite Wahrheit.
//
// == WELCHE EINZELMESSUNG DIE ZELLE BEHAELT (bewusste, begrenzte Festlegung) ========================
// Das Gate liefert die Proben-Folge der ANGENOMMENEN Gruppe. Die Zeile der Auswerte-Ausgabe traegt aber
// EINE Messung (volle Observer-Matrix, PMC-Zaehler, per-Op-Latenzen) -- nicht drei. Behalten wird die
// ZULETZT gemessene Probe der angenommenen Gruppe.
//   * Warum nicht der Median der Gruppe: das waere eine AUSWAHL nach einer Statistik, die niemand
//     festgelegt hat -- und der Mess-Kanon dieses Projekts verlangt ausdruecklich "separat, nie
//     interpoliert" (KF-10). Die zuletzt gemessene Probe ist eine reale, unveraenderte Einzelmessung.
//   * Warum "zuletzt": damit reps == 1 (Gate aus) exakt in das bisherige Verhalten uebergeht -- eine
//     einzige Messung, und genau die reist in die Zeile. Der Uebergang ist damit stetig, nicht sprunghaft.
// Das Drift-Urteil reist NEBEN der Probe (DriftGatedCellResult) -- wer die Zeile auswertet, sieht, wie
// stark die Gruppe streute und wie oft nachgemessen wurde. Ohne diese Mitgabe waere eine Zelle, die ihr
// Rerun-Budget erschoepft hat, von einer sauberen nicht zu unterscheiden.

#include <builder/commands/drift_detector.hpp> // #197: assess_drift / run_with_drift_gate -- die EINE Mechanik

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// A11/K-19: derselbe Namespace wie perm_runner.hpp -- die Datei liegt in harness/, gehoert logisch zum
// builder-Experiment-Kern.
namespace comdare::cache_engine::builder::experiment {

namespace drift_cmd = ::comdare::cache_engine::builder::commands;

// == Die Konfiguration der Zell-Klammer -- PRODUKTIV aus dem Profil-XML (<drift_gate .../>) ========
//
// SELBSTCHECK: DIESE Defaults sind die produktiven. Die Signatur-Defaults von run_with_drift_gate
// (reps=3, threshold=0.05, max_reruns=3) sind MECHANISMUS-Defaults fuer Aufrufer ohne Konfiguration;
// der Mess-Pfad uebergibt IMMER explizit die Werte von hier. Es gibt damit keine zweite Wahrheit,
// sondern eine Quelle (Profil-XML) und einen Rueckfall (diese Struktur).
struct DriftGateConfig {
    // Wiederholungen je Gruppe. GOAL-v8 VI.5: "5 % ueber 3 Wiederholungen".
    //
    // NICHT ZU VERWECHSELN mit der KF-10-Wiederholungs-ACHSE (LazyRunConfig::n_repeats /
    // <repetitions count>): die ist eine dynamische Baum-Dimension und erzeugt N SEPARATE Zeilen mit
    // eigenem repetition_index ("separat, NIE aggregiert" -- die Berichts-Wiederholung aus Termin 3).
    // DIESE Zahl hier ist gruppen-INTERN: N Proben derselben Zelle, aus denen EIN Drift-Urteil
    // entsteht. Zwei verschiedene Achsen, die im Korpus zufaellig beide den Default 3 tragen.
    //
    // reps < 2 == GATE AUS. Das ist keine Abkuerzung, sondern die D4-Regel vom 08.08.2026: eine
    // einzelne Probe hat keine Wiederholungs-Streuung, die Drift ist grundsaetzlich unbestimmbar, und
    // ein Rerun aendert daran nichts. Ausgeschaltet heisst hier ehrlich "keine Drift-Aussage", nicht
    // "stabil".
    //
    // WAS DIESE ZAHL KOSTET, ausdruecklich, weil sie es tut: reps == 3 misst jede Zelle DREIMAL statt
    // einmal. Zusammen mit der KF-10-Achse (<repetitions count="3"/>, drei eigene Zeilen) sind das
    // NEUN Messungen je (Binary x Einstellung) statt drei -- die MESS-Phase verdreifacht sich, die
    // Bau-Phase nicht. Reruns kommen obendrauf, gedeckelt durch max_reruns.
    //
    // OFFEN UND NICHT HIER ENTSCHIEDEN: ob die Owner-Regel "5 % ueber 3 Wiederholungen" genau diese
    // gruppen-interne Drei meint (so ist es hier gebaut, so verlangt es der Mechanismus aus #197 und
    // so liest es das Register S5-06) -- oder ob sie die DREI KF-10-Wiederholungen meint, ueber die
    // dann OHNE Mehrkosten geurteilt wuerde. Die zweite Lesart braeuchte ein anderes Bauteil: die drei
    // Rep-Zeilen sind eigene Baum-Blaetter und liegen zum Mess-Zeitpunkt nie zusammen vor. Wer die
    // Mehrkosten fuer die Kampagne nicht will, stellt reps im Profil-XML -- der Wert ist genau dafuer
    // dort und nicht hier.
    std::uint32_t reps = 3;

    // Median-relative Schwelle. GOAL-v8 VI.5: 5 %. Ihre KALIBRIERUNG gegen reale PMC-Laeufe ist
    // #156-gegatet (mehrtaegiger Voll-Lauf) -- deshalb steht sie im XML und nicht im Code.
    double threshold = 0.05;

    // Rerun-Budget je Zelle.
    //
    // DEFAULT-RUECKBAU 5 -> 3 (KON26-04, OWNER-ENTSCHEIDEN, 2026-08-20 vollzogen -- LAUT, nicht
    // still): ce 4cd1ab91 (09.08.) legte die Owner-Zahl 5 aus GOAL-v8 VI.5 ("Beim Scheitern bis zu
    // 5 Wiederholungen") auf DIESES Feld und dokumentierte selbst die zweite Lesart -- den
    // Wiederholungs-Versuch einer als "failed" klassifizierten Messung. KON26-04 entschied: die
    // ZWEITE Lesart ist die richtige. Der Satz meint bis zu 5 Versuche des GESAMTEN
    // Pruefdock-Durchlaufs EINER Tier-Binary bei hartem Scheitern (Owner verbatim KON37-06: "ein
    // build oder eine Messung duerfen je 5 Mal scheitern bis wir aufgeben"). Die 5 ist deshalb
    // UMGEZOGEN: sie lebt jetzt in MessRetryKonfig::max_versuche (harness/mess_retry_klammer.hpp,
    // Bau- UND Mess-Budget je 5) -- und dieses Feld faellt auf den Mechanismus-Default 3 des
    // Detektors (#197, run_with_drift_gate-Signatur) zurueck. Das Register S5-06 (09.08.), das
    // "max_reruns=3 gegen die Owner-Zahl 5" als Defekt fuehrte, ist damit DATIERT UEBERHOLT: die
    // Owner-5 begrenzt den Binary-Retry, nicht den Drift-Rerun (Korrektur-Entwurf in der
    // Strang-Ergebnisdatei; das Register liegt im super-Repo).
    //
    // DIE DREI GROESSEN, die vorher unter einem Wort liefen (C-07-Kanon, s. auch
    // mess_retry_klammer.hpp): T-15a "ganzen Lauf neu starten" = die KAMPAGNE (nicht hier, nicht
    // gebaut -- Wiederaufsetzpunkt-Posten); T-15b "bis zu 5" = Binary-Retry (mess_retry_klammer);
    // DIESES Feld = Gruppen-Rerun bei STREUUNG (andere Bedingung/Einheit: Drift, nicht Fehlschlag).
    std::uint32_t max_reruns = 3;

    [[nodiscard]] constexpr bool aktiv() const noexcept { return reps >= 2u; }
};

// == Das Ergebnis EINER Zelle: die behaltene Einzelmessung + das Drift-Urteil daneben ===============
template <class Payload>
struct DriftGatedCellResult {
    Payload                 payload{};          // die BEHALTENE Einzelmessung (s. Kopf: die zuletzt gemessene)
    drift_cmd::DriftVerdict verdict{};          // Urteil der ANGENOMMENEN Gruppe (bestimmbar==false => keine Aussage)
    std::size_t             attempts   = 0;     // gemessene Gruppen insgesamt (1 .. max_reruns+1)
    std::size_t             reruns     = 0;     // ausgeloeste Reruns (attempts-1)
    std::size_t             messungen  = 0;     // Einzelmessungen insgesamt (attempts * reps bzw. 1 bei Gate aus)
    bool                    stable     = false; // angenommene Gruppe bestimmbar UND unter der Schwelle
    bool                    exhausted  = false; // Budget erschoepft und nie stabil geworden
    bool                    gate_aktiv = false; // reps >= 2 -- sonst liegt ueberhaupt keine Drift-Aussage vor
};

// == Die Klammer ===================================================================================
/// Misst EINE Zelle unter dem Drift-Gate.
///
/// `measure_cell()` liefert die volle Einzelmessung (im Mess-Pfad: PermResult). `sample_of(payload)`
/// zieht daraus die ns-Zahl, ueber die die Drift beurteilt wird (im Mess-Pfad: PermResult::total_ns --
/// die Zeitnahme je Zelle, die perm_runner als steady_clock-Paar bzw. als Summe der per-Op-Samples
/// bildet). `warn` nimmt die Rerun-Warnungen (nullptr = stumm), `label` benennt die Zelle im Log.
///
/// GATE AUS (cfg.reps < 2): GENAU EIN Aufruf von measure_cell() -- das Verhalten vor dieser Scheibe,
/// unveraendert. verdict.bestimmbar bleibt false: es liegt keine Drift-Aussage vor, und die Zelle
/// behauptet auch keine.
template <class MeasureCell, class SampleOf>
[[nodiscard]] auto run_cell_with_drift_gate(DriftGateConfig const& cfg, MeasureCell&& measure_cell,
                                            SampleOf&& sample_of, std::ostream* warn = nullptr,
                                            std::string const& label = "zelle")
    -> DriftGatedCellResult<std::decay_t<std::invoke_result_t<MeasureCell&>>> {
    using Payload = std::decay_t<std::invoke_result_t<MeasureCell&>>;
    DriftGatedCellResult<Payload> out;

    if (!cfg.aktiv()) {
        // Genau ein Aufruf -- der Zell-Rumpf, wie er vor T-15 aussah. Kein erfundenes "stabil".
        out.payload    = measure_cell();
        out.attempts   = 1;
        out.messungen  = 1;
        out.gate_aktiv = false;
        return out;
    }
    out.gate_aktiv = true;

    // Je gemessener Gruppe: ihre ns-Folge + die ZULETZT gemessene Einzelmessung dieser Gruppe. Mehr
    // wird nicht aufgehoben -- die Zeile traegt eine Messung, nicht reps Stueck.
    std::vector<std::vector<std::int64_t>> gruppen_ns;
    std::vector<Payload>                   gruppen_letzte;
    std::vector<std::int64_t>              laufende;
    Payload                                letzte{};
    std::size_t const                      reps = static_cast<std::size_t>(cfg.reps);

    auto gruppe_abschliessen = [&]() {
        if (laufende.empty()) return;
        gruppen_ns.push_back(laufende);
        gruppen_letzte.push_back(letzte);
        laufende.clear();
    };

    // Das Gate ruft measure_one() genau reps-mal je Versuch. Erreicht die laufende Folge reps, ist die
    // vorige Gruppe fertig -- der naechste Aufruf gehoert schon zum Rerun.
    auto measure_one = [&]() -> std::int64_t {
        if (laufende.size() == reps) gruppe_abschliessen();
        letzte                = measure_cell();
        std::int64_t const ns = static_cast<std::int64_t>(sample_of(letzte));
        laufende.push_back(ns);
        ++out.messungen;
        return ns;
    };

    drift_cmd::DriftGateResult const r = drift_cmd::run_with_drift_gate(
        measure_one, reps, cfg.threshold, static_cast<std::size_t>(cfg.max_reruns), warn, label);
    gruppe_abschliessen(); // die letzte Gruppe schliesst erst nach der Rueckkehr

    out.verdict   = r.verdict;
    out.attempts  = r.attempts;
    out.reruns    = r.reruns;
    out.stable    = r.stable;
    out.exhausted = r.exhausted;

    // WELCHE Gruppe das Gate genommen hat, wird NICHT nachgerechnet, sondern WIEDERERKANNT: r.samples
    // IST die ns-Folge der angenommenen Gruppe. Die Auswahl-Regel (Bestimmbarkeit schlaegt Zahlenwert,
    // erst darunter die kleinere Drift) bleibt allein im Detektor. Bei zwei identischen Folgen gewinnt
    // die erste -- dieselbe Reihenfolge wie das strikte "<" der Bestwahl dort, und beide Gruppen
    // tragen ohnehin dieselben Zahlen.
    std::size_t gewaehlt = gruppen_ns.size();
    for (std::size_t i = 0; i < gruppen_ns.size(); ++i) {
        if (gruppen_ns[i] == r.samples) {
            gewaehlt = i;
            break;
        }
    }
    if (gewaehlt < gruppen_letzte.size()) {
        out.payload = gruppen_letzte[gewaehlt];
    } else if (!gruppen_letzte.empty()) {
        // Kein Treffer. Das kann nur eintreten, wenn Detektor und Klammer ueber die Gruppen-Grenze
        // auseinanderlaufen -- also bei einem Defekt, nicht bei einer Messlage. Er wird beziffert
        // statt verschluckt, und die Zelle behaelt die zuletzt gemessene Probe (irgendeine Zahl zu
        // erfinden waere schlimmer als eine reale Probe unter einer lauten Meldung).
        out.payload = gruppen_letzte.back();
        if (warn != nullptr)
            (*warn) << "[chaos:drift] WARN " << label
                    << ": die vom Gate angenommene Proben-Folge (n=" << r.samples.size() << ") ist in keiner der "
                    << gruppen_ns.size()
                    << " gemessenen Gruppen wiederzufinden -- die Zelle behaelt die zuletzt gemessene Probe. "
                    << "Das ist ein Defekt der Klammer, keine Mess-Lage.\n";
    }
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
