#pragma once
// profile_facade/mess_achsen_naht.hpp -- M-1/D-1 (06.08.2026): die STUFE-2->STUFE-3-NAHT der Mess-Achse.
// Praezedenz und Spiegel: toolchain_stamp_naht.hpp (NB/CX-4) und system_cell_values_naht.hpp (W10-C4).
//
// ------------------------------------------------------------------------------------------------
// WAS HIER GEHEILT WIRD (Befund D-1, am Objekt gemessen)
// ------------------------------------------------------------------------------------------------
// Bis hierher setzte perm_mess_defines() die drei Mess-Defines als LITERALE in einer
// Initialisierungsliste -- -DCOMDARE_MEASUREMENT_ON=1, -DCOMDARE_CE_ENABLE_STATISTICS=1,
// -DCOMDARE_EXPERIMENT_MODE_ON=1 -- unabhaengig von jeder Tooling-Wahl. Gleichzeitig traegt das
// Preimage-Glied [3] (abi/anatomy_fingerprint.hpp) die "Mess-Tooling-Zeile" als "die CT-Mess-
// AUSSTATTUNG dieser Tier-Binary". Ergebnis: das Glied [3] DEKLARIERTE eine Ausstattung, die der Bau
// ignorierte. Eine [wallclock]-Binary und eine [micro]-Binary bekamen byte-gleiche Compile-Kommandos
// und damit identische Mess-Ausstattung -- ihr Stempel behauptete Verschiedenes. Der Stempel log.
//
// Plan-Soll dagegen (LEDGER:3319): "welche Pruef-Tools in die CEB EINKOMPILIERT sind [...] bestimmt,
// was auch das Tier-Binary beinhalten MUSS". Owner-KERN F2: "bei einem neuen Messsystem [muessen] auch
// die CEB und ALLE Binaries fuer die Mess-Achsen-Einstellung neu gebaut werden".
//
// ------------------------------------------------------------------------------------------------
// DIE HARTE BEDINGUNG DIESER NAHT -- EINE AUFLOESUNG, ZWEI VERBRAUCHER
// ------------------------------------------------------------------------------------------------
// Die Mess-Combo-Legende wird ab hier GENAU EINMAL aufgeloest (resolve_live_measurement_combo_legend)
// und von BEIDEN Seiten der Naht konsumiert:
//   (a) STEMPEL-SEITE: lazy_adhoc_source_gen.hpp measurement_stamp_from_env() rendert daraus die
//       Mess-Zeile, die als Glied [3] in das Tier-Preimage und in die Tier-ABI reist.
//   (b) BAU-SEITE: perm_mess_defines() (profile_run_facade.cpp) leitet daraus den Define-Vektor ab,
//       den perm_compile_flags() an ALLE vier make_gpp_compile_fn-Baustellen reicht.
// Zwei getrennte Aufloesungen waeren exakt die Divergenz-Klasse, gegen die schon die Toolchain-Naht
// gebaut ist (NB/CX-4, W-6/W-13): der Bau haengt eine Ausstattung an, waehrend das Glied eine andere
// behauptet -- und nichts bricht. Deshalb ist die Aufloesung EINE Funktion und kein Argument.
//
// ------------------------------------------------------------------------------------------------
// DIE SEMANTIK DER DREI TOOLINGS -- AM OBJEKT ERHOBEN, NICHT ERFUNDEN
// ------------------------------------------------------------------------------------------------
// Vollstaendiger Makro-Zensus ueber libs/cache_engine/anatomy/ (Stand b241a272): es existieren dort
// GENAU ZWEI Mess-Gate-Makros --
//     COMDARE_CE_ENABLE_STATISTICS   39 Vorkommen (abi_adapter 10, view_policies_observable 11,
//                                    inner_container_observable 11, growth_policy_observable 4,
//                                    observer_aggregate 3)
//     COMDARE_MEASUREMENT_ON         36 Vorkommen (abi_adapter 30, observable_tier 2, idriveable_tier 1,
//                                    rollbackable_tier 1, scannable_tier 1, resource_controllable_tier 1)
// (COMDARE_EXPERIMENT_MODE_ON, 4 Vorkommen, ist KEIN Mess-Instrument: es markiert Experiment-Kompilate
// und verlangt seinerseits MEASUREMENT_ON, abi_adapter.hpp:9-10 #error.)
//
// Die schaltbare Flaeche zerfaellt damit in DREI Schichten, aber nur ZWEI davon sind heute trennbar:
//   G1 BASIS-ZEIT    IMeasurableWorkload::run_workload (abi_adapter.hpp:589-1123) -- der aeussere
//                    steady_clock je Batch. Liegt unter #if COMDARE_MEASUREMENT_ON.
//   G2 OBSERVER      fill_observer_v3 / axis_stats (abi_adapter.hpp:1426-1714) -- die per-Achsen-
//                    Zaehler. Liegt unter #ifdef COMDARE_CE_ENABLE_STATISTICS.
//   G3 FEINKORN      fill_segment_timing_v3 (abi_adapter.hpp:1784-2057) -- die 18 per-Achsen-Segment-
//                    Timer je Batch. Liegt EBENFALLS unter #ifdef COMDARE_CE_ENABLE_STATISTICS.
//
// DIE TRENNLINIE WAR BIS M-1 NICHT BAUBAR -- AM OBJEKT GEMESSEN, NICHT VERMUTET.
// Der erste Bau-Versuch der [wallclock]-Konfiguration (MEASUREMENT_ON=1 OHNE CE_ENABLE_STATISTICS)
// brach hart: zwei Aufrufe von mig_organ_.reset() -- ein STATISTICS-Member -- standen ungeschuetzt in
// MEASUREMENT_ON-Bloecken (abi_adapter.hpp tier_rollback_all und tier_migrate_step). g++ 15.3:
// "ObservableMigration<NoMigration> has no member named reset", 2 Fehler.
// NENNER + GEGENPROBE (eine nackte Null ist kein Befund): DIESELBE Uebersetzungszeile MIT dem Gate
// lieferte 0 Fehler -- das Verfahren findet also, und es fand genau die zwei Stellen.
// M-1 hat beide mit dem requires-Guard versehen, den dieselbe Datei fuer pt_organ_ schon fuehrt
// (reset_pathb_driven_organs_). Nachher: [wallclock] 0 Fehler, [all] unveraendert 0 Fehler.
// Gegengeprueft ueber 7 Tier-Modul-/Adapter-TUs (genus_adhoc_buildvariant, auto_emitted_perm_module,
// genus_module_set, genus_buildvariant_avx512, test_v41_anatomy_module_abi, test_234_va/vb): im
// anatomy/-Code 0 Fehler in BEIDEN Konfigurationen. (test_234_va meldet 2 Fehler in seinem EIGENEN
// Testkoerper -- er ruft pool_node_count(), ein STATISTICS-Member; das ist Test-Code, nicht Tier-Code.)
// DAS IST DIE EIGENTLICHE WURZEL VON D-1: die Mess-Achse konnte gar keine waehlbare Tier-Ausstattung
// haben, weil die einzige Trennlinie, die der Code anbot, nicht uebersetzte. Ein Define-Vektor, der
// sie gezogen haette, haette den Bau gebrochen -- und deshalb zog ihn niemand.
// GRENZE DIESES BELEGS, ehrlich benannt: 7 TUs sind kein Zensus ueber die golden-320. Vor dem ersten
// produktiven [wallclock]-Batch gehoert die Sonde ueber den vollen Kompositions-Satz gefahren.
//
// Daraus folgt die Zuordnung, die diese Naht faehrt -- und nur sie ist am Objekt belegt:
//   wallclock : braucht G1. Die Registry nennt es "die Basis-Zeitmessung" -> MEASUREMENT_ON.
//   macro     : braucht G1+G2. Die Registry nennt es "Ende-zu-Ende-Durchsatz/Latenz ueber Observer"
//               -> zusaetzlich CE_ENABLE_STATISTICS.
//   micro     : braucht G1+G2+G3. Die Registry nennt es "feinkoernige Counter-Instrumentierung"
//               -> ebenfalls CE_ENABLE_STATISTICS, WEIL G3 heute im selben Gate wohnt wie G2.
//
// ------------------------------------------------------------------------------------------------
// EHRLICHE GRENZE DIESER SCHEIBE -- macro UND micro SIND HEUTE NICHT TRENNBAR
// ------------------------------------------------------------------------------------------------
// G2 und G3 teilen sich EIN Gate. Es gibt im gesamten anatomy/-Baum kein drittes Makro, ueber das man
// die Segment-Timer ohne die Observer-Zaehler (oder umgekehrt) entfernen koennte. Ein solches Gate
// hier zu ERFINDEN hiesse, Semantik zu behaupten, die der Code nicht traegt -- also genau der Fehler,
// den D-1 heilt, nur in die andere Richtung.
// DIESE NAHT TRAEGT DIE TRENNUNG DESHALB, SOBALD ES SIE GIBT, UND BEHAUPTET SIE NICHT VORHER:
//   -- der GATE-Teil (MEASUREMENT_ON / CE_ENABLE_STATISTICS) unterscheidet heute real
//      {wallclock} von {macro, micro};
//   -- der DEKLARATIONS-Teil (COMDARE_MEASUREMENT_TOOLING_<ID>=1, ein Define je EINKOMPILIERTEM
//      Tooling) macht die Wahl im Kompilat vollstaendig sichtbar und injektiv, auch wo noch kein
//      Gate daran haengt.
// Das Herausloesen von G3 aus dem STATISTICS-Gate in ein eigenes Makro ist ein EIGENES FOLGEPAKET
// (es beruehrt abi_adapter.hpp im Hot-Path und die A8-S4-Praeprozessor-Wache,
// tests/unit/test_a8s4_release_pfad_neutralitaet.cpp TEIL 1, die genau zwei Gate-Makros kennt).
// Solange es nicht gebaut ist, gilt: macro und micro erzeugen denselben GATE-Zustand und
// unterscheiden sich ausschliesslich im Deklarations-Define. Das steht hier, damit es niemand fuer
// eine gebaute Trennung haelt.
//
// ------------------------------------------------------------------------------------------------
// WARUM DIE DEKLARATIONS-DEFINES KEIN ZWEITER LUEGEN-KANAL SIND
// ------------------------------------------------------------------------------------------------
// Ein Define ohne Leser klingt zunaechst nach derselben Krankheit. Es ist die Umkehrung: D-1 war
// "der STEMPEL deklariert, der BAU ignoriert". Hier deklariert der BAU exakt das, was der Stempel
// deklariert, aus DERSELBEN Quelle. Der Nutzen ist dreifach:
//   (1) INJEKTIVITAET (Owner-KERN F6): jede unterscheidbare Mess-Achsen-Wahl erzeugt einen
//       unterscheidbaren Define-Vektor -- auch {macro} vs {micro}, wo der Gate-Teil noch gleich ist.
//       Ohne diese Defines fielen macro und micro auf byte-gleiche Compile-Kommandos zusammen.
//   (2) Das Folgepaket (G3-Gate) kann sein #if an einen Namen haengen, der bereits im Kompilat
//       ankommt -- ohne diese Naht ein zweites Mal anzufassen.
//   (3) Sie sind im -E/rsp-Trace lesbar: was eine Tier-Binary an Mess-Ausstattung MITBEKOMMEN hat,
//       steht ab jetzt woertlich im Bau-Kommando und nicht nur im Stempel.
//
// ------------------------------------------------------------------------------------------------
// PMC-ENTSCHEIDUNG (Auftrag Punkt 3): COMDARE_ENABLE_PMC WIRD NICHT TIER-SEITIG.
// ------------------------------------------------------------------------------------------------
// Begruendung am Objekt, in vier Schritten:
//   (a) NENNER + GEGENPROBE: die Suche perf_event|rdtsc|__rdpmc|PERF_COUNT|ENABLE_PMC findet in
//       libs/cache_engine/anatomy/ NULL Treffer. Dasselbe Verfahren findet ausserhalb sieben Dateien
//       (builder/linux_perf_pmc_source.hpp, builder/windows_pcm_pmc_source.hpp,
//       builder/pmc_source_factory.hpp, builder/experiment_tree/cache_engine_builder_iterator.hpp,
//       builder/commands/execute_engine_command.hpp, harness/perm_runner.hpp,
//       planner/experiment_plan_director.hpp) -- das Verfahren findet also, nur nicht im Tier.
//   (b) Das Flag EXISTIERT bereits, als Wurzel-CMake-Option (CMakeLists.txt:67) mit
//       add_compile_definitions(COMDARE_ENABLE_PMC) und genau drei Konsumenten, alle HOST-seitig.
//       perf_event_open misst den messenden PROZESS, nicht den Code der .so -- die PMC-Quelle sitzt
//       auf der Host-Seite der Modulgrenze und braucht vom Tier-Kompilat nichts.
//   (c) Ein -DCOMDARE_ENABLE_PMC im Tier-Compile-Kommando waere damit ein Flag ohne Wirkung, das
//       eine Ausstattung behauptet, die die Binary nicht traegt -- die exakte Spiegelung von D-1.
//   (d) STUFEN-DOKTRIN (LEDGER:4082-4095): MESS ist DREISTUFIG Planer -> CEB -> Tier. Die Wahl der
//       PMC-Quelle ist Stufe-2-Ware (CEB-Einbau + Pruefdock-Konfiguration) und endet dort; sie hat
//       auf Stufe 3 kein Objekt, an dem sie wirken koennte.
// FOLGE FUER DIE REGISTRY-ZEILE: measurement_axis_registry.xml ordnet PmcSystemAxis dem micro-Tooling
// zu. Das bleibt richtig -- aber es ist eine HOST-Kollektor-Wahl. Der TIER-Beitrag von micro ist G3
// (die Segment-Timer), nicht PMC. Wer das anders entscheidet, aendert eine Owner-Frage, nicht diese
// Datei.
//
// ------------------------------------------------------------------------------------------------
// PREIMAGE (Auftrag Punkt 5): DIESE NAHT AENDERT GLIED [3] NICHT.
// ------------------------------------------------------------------------------------------------
// Glied [3] ist die vom Renderer abi::measurement_stamp_line_from_combo_legend erzeugte Zeile. Der
// Renderer ist unangetastet und bekommt denselben Legenden-String wie zuvor
// (resolve_live_measurement_combo_legend gibt die bisherige Aufloesung 1:1 zurueck). Es gibt keinen
// Format-Bump 3->4: das Preimage traegt die Mess-Zeile bereits als eigenes Glied von acht
// (kAnatomyFingerprintGliedCount = 8). Was sich aendert, ist der WERT der Compile-Kommandos -- nicht
// die Form des Preimage.
//
// header-only, ASCII-only, keine Bau-Abhaengigkeit ausser der Mess-Tooling-Registry.

#include <cache_engine/measurement/measurement_tooling_registry.hpp> // kMeasurementToolingRegistry (Single-Source)

#include <array>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::profile_facade {

/// Die EINKOMPILIERTE Mess-Tooling-MENGE einer CEB: ein Flag je Registry-Eintrag, Index == Tooling-Wert
/// (die Registry garantiert Index==Enum per static_assert). Bewusst KEIN std::variant und kein
/// dynamischer Container -- die Menge ist eine statische Achsen-Belegung, kein Laufzeit-Zustand.
using MessToolingMenge = std::array<bool, ::comdare::cache_engine::measurement::kMeasurementToolingCount>;

/// ct_measurement_combo_legend() -- die EINE Lesung des einkompilierten Mess-Combo-Makros.
///
/// Bis M-1 stand diese Lesung als nacktes #ifdef mitten in measurement_stamp_from_env(). Sie steht
/// jetzt hier, damit die BAU-Seite dieselbe Lesung benutzt und nicht ihre eigene aufmacht.
///
/// cppcheck-ADJAZENZ-FALLE (Fallen-Kanon 05.08., lint:static 14673): das Makro darf NIE zwischen zwei
/// String-Literalen stehen (unknownMacro/Configuration-required). Deshalb EINMAL in ein benanntes
/// std::string-Objekt und danach nur noch ueber den Namen arbeiten.
///
/// RUECKGABE: der einkompilierte Legenden-String, oder LEER wenn kein Define gesetzt ist. Ein leerer
/// Rueckgabewert heisst "die CEB hat KEINE spezifische Mess-Achse einkompiliert" -- NICHT "[all]";
/// die Vollmengen-Entscheidung faellt eine Ebene hoeher (resolve_live_measurement_combo_legend).
[[nodiscard]] inline std::string ct_measurement_combo_legend() {
#ifdef COMDARE_MEASUREMENT_COMBO_CT
    std::string const ct_legend{COMDARE_MEASUREMENT_COMBO_CT};
    return ct_legend;
#else
    return {};
#endif
}

/// resolve_live_measurement_combo_legend() -- DIE EINE AUFLOESUNG der Mess-Combo dieses Laufs.
///
/// Wortlaut und Verhalten sind aus measurement_stamp_from_env() (lazy_adhoc_source_gen.hpp) hierher
/// GEZOGEN, nicht neu geschrieben: dieselben zwei Zweige, dieselben zwei Wuerfe, dieselbe
/// [all]-Ausnahme. Der einzige Unterschied ist, WAS zurueckkommt -- die LEGENDE statt der gerenderten
/// Stempel-Zeile. Der Renderer bleibt beim Aufrufer, und weil er derselbe ist und denselben String
/// bekommt, ist die Mess-Zeile der Tier-Fingerprints byte-identisch zum Vor-M-1-Stand.
///
/// SEMANTIK (K7b-2 / Section 64-D1-B, unveraendert):
///   -- Ist eine Combo EINKOMPILIERT, ist SIE die Wahrheit (Stufe-2-CT-EINBAU der Stufen-Doktrin).
///   -- WIDERSPRUCHS-WACHE (F-B2): eine fehlende oder abweichende Laufzeit-Env gegen ein
///      einkompiliertes Kompilat ist ein Konfigurationswiderspruch -- fail-loud, nie still.
///      AUSNAHME [all]/leer: der einkompilierte Wert ist dann selbst die Vollmenge und damit KEINE
///      spezifische Wahl; die leere Env ist dort die SYNCHRONE Form.
///   -- Ohne Define entscheidet die Env; UNGESETZT == "[all]" == die volle 3-Tool-Vollmenge.
///
/// WARUM DIE ENV IM CT-LOSEN ZWEIG KEIN "RUNTIME-KONFIGURIEREN" DES BAUS IST: sie ist der einzige
/// Kanal, ueber den die STUFE-1-Freigabe des Planers eine CEB erreicht, die selbst ohne spezifische
/// Mess-Achse gebaut wurde. Sobald eine Achse einkompiliert IST, gewinnt sie -- und die Wache oben
/// erzwingt, dass die Env sie nur bestaetigen, nie ueberstimmen kann. Der Bau folgt also immer der
/// einkompilierten Achse, wo es eine gibt.
[[nodiscard]] inline std::string resolve_live_measurement_combo_legend() {
    std::string const ct_legend = ct_measurement_combo_legend();
#ifdef COMDARE_MEASUREMENT_COMBO_CT
    char const* const e         = std::getenv("COMDARE_MEASUREMENT_COMBO");
    bool const        env_fehlt = (e == nullptr || *e == '\0');
    // [all]/leer als einkompilierter Wert ist die Vollmenge, also KEINE spezifische Wahl (s. AUSNAHME oben).
    bool const ct_ist_vollmenge = ct_legend.empty() || ct_legend == "[all]";
    if (env_fehlt && !ct_ist_vollmenge)
        throw std::runtime_error("fehlerklasse=konfiguration_widerspruch: COMDARE_MEASUREMENT_COMBO fehlt/leer, "
                                 "aber die Mess-Combo ist einkompiliert ('" +
                                 ct_legend +
                                 "') -- beide Kanaele muessen synchron sein (die Env speist +mtool und die "
                                 "Bestandslog-Zelle, das Compile-Define den Stempel)");
    if (!env_fehlt && std::string_view{e} != std::string_view{ct_legend})
        throw std::runtime_error("fehlerklasse=konfiguration_widerspruch: COMDARE_MEASUREMENT_COMBO ('" +
                                 std::string{e} + "') != einkompilierte Combo ('" + ct_legend + "')");
    return ct_legend;
#else
    char const* const e = std::getenv("COMDARE_MEASUREMENT_COMBO");
    // UNGESETZT == [all]: der Renderer bildet "[all]" auf measurement_stamp_line_full_set() ab, die
    // Vollmengen-Provenienz bleibt damit byte-identisch zum frueheren full_set()-Direktaufruf.
    return (e != nullptr && *e != '\0') ? std::string{e} : std::string{"[all]"};
#endif
}

/// mess_tooling_menge_from_legend(legend) -- die Legende [a,b,c] auf die Registry-MENGE abbilden.
///
/// Dieselbe Zerlegung wie abi::measurement_stamp_line_from_combo_legend (Klammern strippen, an ','
/// trennen, leere Tokens ueberspringen), damit Stempel und Menge dieselbe Legende gleich lesen.
/// Ein Zweit-Parser mit eigener Auslegung waere genau die Drift, gegen die diese Datei gebaut ist.
///
/// ZWEI UNTERSCHIEDLICHE LEER-FAELLE, bewusst getrennt (der Renderer trennt sie ebenso):
///   -- legend LEER ("keine Legende gereicht"): fuer den STEMPEL ist das die leere Zeile. Fuer den BAU
///      ist es KEIN baubarer Zustand -- eine Tier-Binary ohne jedes Mess-Tooling haette weder G1 noch
///      G2/G3, und COMDARE_EXPERIMENT_MODE_ON verlangt seinerseits MEASUREMENT_ON (abi_adapter.hpp:9).
///      Also fail-loud statt stiller Vollmenge. Aus der Emission ist dieser Zustand nicht erreichbar:
///      CMake setzt das Define nur fuer eine NICHT-leere Cache-Variable, und der CT-lose Zweig oben
///      liefert "[all]". Der Wurf deckt ausschliesslich ein von Hand gesetztes leeres Define ab.
///   -- INNERES leer oder "all": die VOLLE Vollmenge (Section 64-D1-B).
///
/// UNBEKANNTE id -> Wurf (Fehlerklassen-Doktrin): ein Tippfehler in der Legende darf nicht still zu
/// einer kleineren Ausstattung fuehren, als der Bediener bestellt hat.
[[nodiscard]] inline MessToolingMenge mess_tooling_menge_from_legend(std::string_view legend) {
    namespace cm = ::comdare::cache_engine::measurement;
    MessToolingMenge menge{};
    if (legend.empty())
        throw std::runtime_error("fehlerklasse=konfiguration_widerspruch: leere Mess-Combo-Legende -- eine "
                                 "Tier-Binary ohne jedes Mess-Tooling ist kein baubarer Zustand (die "
                                 "Vollmenge heisst '[all]', nicht '')");
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    if (inner.empty() || inner == "all") {
        menge.fill(true);
        return menge;
    }
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start) {
            std::string_view const tok = inner.substr(start, end - start);
            bool                   ok  = false;
            for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
                if (cm::kMeasurementToolingRegistry[i].id == tok) {
                    menge[i] = true;
                    ok       = true;
                    break;
                }
            if (!ok)
                throw std::runtime_error("fehlerklasse=konfiguration_widerspruch: unbekanntes Mess-Tooling '" +
                                         std::string{tok} + "' in der Combo-Legende '" + std::string{legend} +
                                         "' -- gueltig sind ausschliesslich die ids aus kMeasurementToolingRegistry");
        }
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    bool leer = true;
    for (bool const b : menge) leer = leer && !b;
    if (leer)
        throw std::runtime_error("fehlerklasse=konfiguration_widerspruch: die Mess-Combo-Legende '" +
                                 std::string{legend} + "' waehlt kein einziges Tooling aus");
    return menge;
}

/// mess_tooling_deklarations_define(i) -- der Deklarations-Define-NAME eines Registry-Eintrags.
/// Aus der id gebildet (Single-Source): "wallclock" -> COMDARE_MEASUREMENT_TOOLING_WALLCLOCK. Kein
/// Literal je Tooling -- ein viertes Tooling in der Registry bekommt seinen Namen automatisch.
[[nodiscard]] inline std::string mess_tooling_deklarations_define(std::size_t idx) {
    namespace cm         = ::comdare::cache_engine::measurement;
    std::string_view const id = cm::kMeasurementToolingRegistry[idx].id;
    std::string            name{"-DCOMDARE_MEASUREMENT_TOOLING_"};
    for (char const c : id) name.push_back(c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c);
    name += "=1";
    return name;
}

/// mess_achsen_defines(menge) -- DIE ABBILDUNG: einkompilierte Mess-Achse -> Tier-Compile-Defines.
///
/// Das ist die Funktion, die D-1 heilt. Sie ist REIN (Argument rein, Vektor raus, kein Env, kein
/// Makro) -- deshalb ist sie testbar, und deshalb kann der BISS zeigen, dass zwei verschiedene
/// Mess-Achsen-Konfigurationen verschiedene Tier-Defines erzeugen.
///
/// REIHENFOLGE ist bindend (deterministisches Compile-Kommando): erst die GATES in der Reihenfolge
/// ihrer Schichten (G1 -> G2/G3), dann die DEKLARATIONEN in Registry-Reihenfolge.
///
/// BYTE-BILANZ: fuer die Vollmenge (der gesamte golden-/CI-Bestand, [all]) stehen die beiden Gates in
/// derselben Reihenfolge und mit demselben Wortlaut da wie in der abgeloesten Literal-Liste --
/// -DCOMDARE_MEASUREMENT_ON=1 dann -DCOMDARE_CE_ENABLE_STATISTICS=1. Neu sind ausschliesslich die drei
/// Deklarations-Defines; sie haben im Tier heute keinen Leser und aendern damit kein Byte des
/// erzeugten Kompilats, nur das Bau-Kommando.
[[nodiscard]] inline std::vector<std::string> mess_achsen_defines(MessToolingMenge const& menge) {
    namespace cm = ::comdare::cache_engine::measurement;
    std::vector<std::string> d;

    // G1 BASIS-ZEIT -- IMeasurableWorkload::run_workload (abi_adapter.hpp:589-1123) haengt an diesem
    // Gate. JEDES Tooling braucht es: auch [wallclock] misst ueber run_workload. Die Menge ist per
    // mess_tooling_menge_from_legend garantiert nicht-leer, also ist dieses Gate immer gesetzt --
    // ausgeschrieben statt stillschweigend, damit die Bedingung sichtbar bleibt, falls je ein Tooling
    // dazukommt, das ohne G1 auskommt.
    bool const braucht_g1 = [&] {
        for (bool const b : menge)
            if (b) return true;
        return false;
    }();
    if (braucht_g1) d.emplace_back("-DCOMDARE_MEASUREMENT_ON=1");

    // G2 OBSERVER (+ G3 FEINKORN, solange beide dasselbe Gate teilen) -- fill_observer_v3 /
    // fill_segment_timing_v3. [wallclock] allein braucht sie nicht und bekommt sie ab hier auch nicht
    // mehr: DAS ist die erste reale Wirkung, die die Mess-Achse auf das Tier-Kompilat hat.
    bool const braucht_g2 = menge[static_cast<std::size_t>(cm::MeasurementTooling::Macro)] ||
                            menge[static_cast<std::size_t>(cm::MeasurementTooling::Micro)];
    if (braucht_g2) d.emplace_back("-DCOMDARE_CE_ENABLE_STATISTICS=1");

    // DEKLARATION -- ein Define je EINKOMPILIERTEM Tooling, Registry-Reihenfolge. Traegt die
    // Injektivitaet auch dort, wo der Gate-Teil (noch) nicht unterscheidet (macro vs micro).
    for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
        if (menge[i]) d.push_back(mess_tooling_deklarations_define(i));

    return d;
}

/// mess_achsen_defines_for_legend(legend) -- die Bequemform fuer Tests und Werkzeuge: Legende rein,
/// Define-Vektor raus. Genau die Verkettung, die live_mess_achsen_defines() intern faehrt.
[[nodiscard]] inline std::vector<std::string> mess_achsen_defines_for_legend(std::string_view legend) {
    return mess_achsen_defines(mess_tooling_menge_from_legend(legend));
}

/// live_mess_achsen_defines() -- der LIVE-Wert fuer perm_mess_defines(): die Defines der Mess-Achse,
/// die in DIESE CEB einkompiliert ist. Argumentlos und rein im selben Sinn wie die Toolchain-Naht:
/// derselbe Aufruf liefert im Bau-Kanal genau das, was resolve_live_measurement_combo_legend() auf der
/// Stempel-Seite als Glied [3] rendern laesst. Beide koennen nicht auseinanderlaufen, weil sie
/// dieselbe Aufloesung benutzen -- nicht, weil jemand sie synchron haelt.
[[nodiscard]] inline std::vector<std::string> live_mess_achsen_defines() {
    return mess_achsen_defines_for_legend(resolve_live_measurement_combo_legend());
}

} // namespace comdare::cache_engine::profile_facade
