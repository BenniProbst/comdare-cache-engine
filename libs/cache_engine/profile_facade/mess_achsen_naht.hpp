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
///   -- Ohne Define ist die einzige zulaessige Aussage die VOLLMENGE: UNGESETZT == "[all]" == die
///      volle 3-Tool-Vollmenge. Eine SPEZIFISCHE Env-Combo ohne Define ist ab M-1/H-A ein Wurf
///      (Begruendung im Block direkt darunter).
///
/// ------------------------------------------------------------------------------------------------
/// M-1/H-A (06.08.2026) -- WARUM DER CT-LOSE ZWEIG EINE SPEZIFISCHE ENV-COMBO ABLEHNT
/// ------------------------------------------------------------------------------------------------
/// Der Kommentar an dieser Stelle behauptete bis hierher, die Env sei "kein Runtime-Konfigurieren des
/// Baus", weil die Wache erzwinge, "dass die Env sie nur bestaetigen, nie ueberstimmen kann". Am Objekt
/// gemessen war das nur fuer den #ifdef-Zweig wahr: BEIDE Wuerfe stehen INNERHALB von
/// COMDARE_MEASUREMENT_COMBO_CT. Der #else-Zweig hatte KEINE Wache -- und genau er ist der
/// Default-Bau, denn COMDARE_MEASUREMENT_COMBO ist per CACHE STRING "" leer
/// (profile_facade/CMakeLists.txt), das Define entsteht nur im if().
///
/// GEMESSENE FOLGE (eine und dieselbe Default-CEB-Binary, nur die Env variiert):
///     env=[all]        -> -DCOMDARE_MEASUREMENT_ON=1 -DCOMDARE_CE_ENABLE_STATISTICS=1 + 3 Deklarationen
///     env=[wallclock]  -> -DCOMDARE_MEASUREMENT_ON=1 -DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1
///     env=[macro]      -> -DCOMDARE_MEASUREMENT_ON=1 -DCOMDARE_CE_ENABLE_STATISTICS=1 + 1 Deklaration
/// -- vier verschiedene TIER-AUSSTATTUNGEN aus EINER CEB, die nie neu gebaut wurde, und mit EINEM
/// ceb_key_sha512. Das ist die STUFEN-DOKTRIN gebrochen: MESS ist DREISTUFIG, Planer (RT-Freigabe) ->
/// CEB (CT-EINBAU) -> Tier (CT-EINBAU). Hier lief die Achse Planer -> Env -> Tier-Compile-Kommando und
/// uebersprang den CT-Einbau der Stufe 2. Woertlich Owner-KERN F2: "bei einem neuen Messsystem [muss]
/// auch die CEB ... neu gebaut werden" -- und genau das geschah nicht.
///
/// DIE HEILUNG IST EIN WURF UND KEIN EINBAU: der Wurf ERZWINGT den CT-Einbau, er ersetzt ihn nicht.
/// Wer eine spezifische Mess-Achse fahren will, muss die CEB dafuer konfigurieren
/// (-DCOMDARE_MEASUREMENT_COMBO=<legend>); dann laeuft der #ifdef-Zweig oben mit seinen zwei Wachen.
///
/// WARUM [all]/UNGESETZT WEITER DURCHGEHT (und das ist keine Ausnahme, sondern dieselbe Regel):
/// [all] ist KEINE spezifische Wahl, sondern die Vollmenge -- die Aussage "es wurde nichts
/// eingeschraenkt". Sie ist der byte-stabile Default des gesamten heutigen Bestandes (Sidecar-Bestand
/// 0, jede bestehende Zeile stammt aus einem [all]-Lauf). Die Wache beisst damit exakt am ersten
/// NICHT-[all]-Batch -- also genau dort, wo die Luecke real wird, und nirgends vorher.
[[nodiscard]] inline bool combo_legend_ist_vollmenge(std::string_view legend) noexcept {
    if (legend.empty() || legend == "[all]" || legend == "all") return true;
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    return inner.empty() || inner == "all";
}

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
    if (e == nullptr || *e == '\0') return std::string{"[all]"};
    // M-1/H-A: die STUFEN-WACHE. Eine spezifische Env-Combo ohne einkompilierte Combo ist der
    // uebersprungene Stufe-2-CT-Einbau (Herleitung im Kopf dieser Funktion) -- fail-loud, nie still.
    if (!combo_legend_ist_vollmenge(e))
        throw std::runtime_error(
            "fehlerklasse=konfiguration_widerspruch: COMDARE_MEASUREMENT_COMBO ('" + std::string{e} +
            "') verlangt eine spezifische Mess-Achse, aber DIESE CEB ist nicht dafuer gebaut "
            "(COMDARE_MEASUREMENT_COMBO_CT fehlt) -- der CT-EINBAU der STUFE 2 fehlt. MESS ist "
            "dreistufig: Planer (RT-Freigabe) -> CEB (CT-Einbau) -> Tier (CT-Einbau). Abhilfe: die CEB "
            "mit -DCOMDARE_MEASUREMENT_COMBO=" +
            std::string{e} + " konfigurieren und neu bauen (Owner-KERN F2)");
    return std::string{e};
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
    namespace cm              = ::comdare::cache_engine::measurement;
    std::string_view const id = cm::kMeasurementToolingRegistry[idx].id;
    std::string            name{"-DCOMDARE_MEASUREMENT_TOOLING_"};
    for (char const c : id) name.push_back(c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c);
    name += "=1";
    return name;
}

/// mess_menge_hat_observer_gate(menge) -- traegt eine so gebaute Tier-Binary den OBSERVER (G2/G3)?
///
/// M-1/H-B (06.08.2026). Das ist DIESELBE Bedingung, aus der mess_achsen_defines() das Gate-Define
/// -DCOMDARE_CE_ENABLE_STATISTICS=1 setzt -- als benannte Funktion herausgezogen, damit es EINE Quelle
/// gibt statt zweier gleichlautender Ausdruecke. Ein zweiter, handgeschriebener Ausdruck an der
/// CSV-Seite waere exakt die Divergenz-Klasse, gegen die diese ganze Datei gebaut ist.
///
/// WOZU DIE CSV-SEITE SIE BRAUCHT: tier_observe() hat seinen KOMPLETTEN Rumpf unter
/// #ifdef COMDARE_CE_ENABLE_STATISTICS (anatomy/abi_adapter.hpp). Eine [wallclock]-Binary liefert
/// daher einen LEEREN Snapshot -- inklusive tier_fill_level == 0 und observable_axis_count == 0,
/// obwohl das Tier real gefuellt ist und obwohl diese beiden Felder ausdruecklich KEIN Mess-Zustand
/// sind (axis_operability_classification.hpp: "passive Build-/Compile-Konstante"). Wer daraus Zahlen
/// in die CSV schreibt, schreibt eine Luege, die von einer echten Messung nicht unterscheidbar ist.
[[nodiscard]] inline bool mess_menge_hat_observer_gate(MessToolingMenge const& menge) noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
    return menge[static_cast<std::size_t>(cm::MeasurementTooling::Macro)] ||
           menge[static_cast<std::size_t>(cm::MeasurementTooling::Micro)];
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

    // ------------------------------------------------------------------------------------------------
    // M-1/H-1 (06.08.2026) -- DIE DEKLARATIONS-PFLICHT FUER G1. Fail-closed, kein Byte am Bestand.
    // ------------------------------------------------------------------------------------------------
    // BEFUND, am Objekt gemessen: G1 (COMDARE_MEASUREMENT_ON) IST das wallclock-Instrument -- der
    // aeussere steady_clock je Batch in run_workload (abi_adapter.hpp). Die Zuordnung oben in diesem
    // Header sagt es woertlich: "auch [wallclock] misst ueber run_workload". Weil JEDE nicht-leere
    // Menge G1 zieht, traegt auch eine [macro]-Tier-Binary den wallclock-Messcode -- ihr Glied [3]
    // nannte ihn aber NICHT. Das ist D-1 in klein, eine Ebene tiefer:
    //     Wallclock-Messcode auf high_resolution_clock geaendert + Registry wallclock 1.0.0c -> 2.0.0c
    //     [macro]-TIER-OBJEKT   329e45a0... -> 87bbbce6...   GEAENDERT
    //     [macro]-Glied [3]     measurement_tooling=macro@1.0.0c;[...]   BYTE-GLEICH
    //     [macro]-tier_fp       ab530b58... -> ab530b58...   UNVERAENDERT
    // NENNER + GEGENPROBE (eine nackte Null ist kein Befund): dasselbe Verfahren trennt [macro] von
    // [wallclock] am Objekt (329e45a0 vs c3384d4b), und derselbe Versions-Bump BEWEGT den Fingerprint
    // sehr wohl, sobald wallclock in der Legende steht. Das Verfahren ist sehend -- der Stempel sah weg.
    // Folge: dll_is_current meldet "current" ueber eine Mess-Code-VERSIONSGRENZE hinweg. Owner-KERN F2.
    //
    // WARUM WURF UND NICHT STILLE ERGAENZUNG DER MENGE: die Menge still um wallclock zu erweitern
    // hiesse, die Bestellung des Bedieners umzuschreiben -- und die Stempel-Zeile entsteht in einem
    // ANDEREN Renderer (abi::measurement_stamp_line_from_combo_legend), der die Legende nimmt, nicht
    // die Menge. Eine hier korrigierte Menge wuerde den Stempel also gar nicht erreichen; SIE waere die
    // zweite Wahrheit. Der Wurf dagegen ist an EINER Stelle wahr und verlangt vom Bediener genau das,
    // was der Bau ohnehin tut: wallclock mitzuschreiben. "[macro]" -> "[wallclock,macro]".
    //
    // BYTE-BILANZ: [all] enthaelt wallclock -> unberuehrt. Der gesamte heutige Bestand ist [all]
    // (Sidecar-Bestand 0). Diese Wache kann kein einziges bestehendes Byte bewegen.
    if (braucht_g1 && !menge[static_cast<std::size_t>(cm::MeasurementTooling::WallClock)]) {
        std::string genannt;
        for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
            if (menge[i]) {
                if (!genannt.empty()) genannt += ",";
                genannt += std::string{cm::kMeasurementToolingRegistry[i].id};
            }
        throw std::runtime_error(
            "fehlerklasse=konfiguration_widerspruch: die Mess-Combo '[" + genannt +
            "]' nennt 'wallclock' NICHT, baut es aber unvermeidlich ein -- das Gate G1 "
            "(COMDARE_MEASUREMENT_ON) IST das wallclock-Instrument (steady_clock je Batch in "
            "run_workload) und wird von JEDEM Tooling gezogen. Ein Stempel, der es verschweigt, sieht "
            "einen Versions-Sprung der wallclock-Mess-Achse nicht (Owner-KERN F2). Abhilfe: die Combo "
            "als '[wallclock," +
            genannt + "]' schreiben");
    }
    if (braucht_g1) d.emplace_back("-DCOMDARE_MEASUREMENT_ON=1");

    // G2 OBSERVER (+ G3 FEINKORN, solange beide dasselbe Gate teilen) -- fill_observer_v3 /
    // fill_segment_timing_v3. [wallclock] allein braucht sie nicht und bekommt sie ab hier auch nicht
    // mehr: DAS ist die erste reale Wirkung, die die Mess-Achse auf das Tier-Kompilat hat.
    if (mess_menge_hat_observer_gate(menge)) d.emplace_back("-DCOMDARE_CE_ENABLE_STATISTICS=1");

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

/// live_mess_observer_ausstattung() -- traegt die Tier-Binary DIESES Laufs den Observer (G2/G3)?
/// Dieselbe Aufloesung, dieselbe Abbildung wie live_mess_achsen_defines() -- ein dritter Ableitungsweg
/// waere die Drift-Klasse aus D-1. Die CSV-Seite (cache_engine_builder_iterator) entscheidet damit
/// zwischen echten Observer-Zahlen und ehrlichem "n/a".
[[nodiscard]] inline bool live_mess_observer_ausstattung() {
    return mess_menge_hat_observer_gate(mess_tooling_menge_from_legend(resolve_live_measurement_combo_legend()));
}

// ----------------------------------------------------------------------------------------------------
// M-1/H-2 (06.08.2026) -- DIE PMC-AUSSTATTUNG GEHOERT ZUR MESS-ACHSE. Fail-closed, kein Byte am Bestand.
// ----------------------------------------------------------------------------------------------------
// BEFUND, am Objekt gemessen und AUSGEFUEHRT (Legende [micro], gleiche Maschine, gleicher Lauf):
//     COMDARE_ENABLE_PMC=OFF                    COMDARE_ENABLE_PMC=ON
//     pmc_quelle    = NullPmcSource             pmc_quelle    = LinuxPerfPmcSource
//     cache_misses_l1 = 0                       cache_misses_l1 = 1898596
//     glied3  = measurement_tooling=micro@...   glied3  = measurement_tooling=micro@...   <- identisch
//     ceb_key = a66710856b4e7071...             ceb_key = a66710856b4e7071...             <- identisch
// Dieselbe deklarierte Mess-Achse, derselbe Schluessel, andere Zahlen. Das ist Owner-KERN F6 woertlich
// verletzt ("die gleiche binary auf der selben Maschine mit den selben Messachsen liefert identische
// Ergebnisse uneingeschraenkt"). GEGENPROBE auf Vollstaendigkeit: ENABLE_PMC|pmc in
// ceb_version_stamp.hpp, anatomy_fingerprint.hpp und toolchain_stamp_glied.hpp = 0 Treffer -- PMC steht
// in KEINEM Identitaets-Glied.
//
// WARUM DIE HEILUNG EINE WACHE IST UND KEIN NEUES IDENTITAETS-GLIED: PMC in den Fingerprint oder in den
// CEB-Schluessel aufzunehmen bewegt Identitaets-BYTES (Preimage-Format bzw. der gepinnte Default-Schluessel
// 004251f4...). Das ist ein deklariertes Byte-Ereignis mit Owner-Entscheid und ausdruecklich NICHT das,
// was eine Heilungs-Scheibe nebenbei tut. Die Wache erreicht dasselbe Ziel -- keine zwei ununterscheidbaren
// Laeufe mehr -- und bewegt kein Byte: sie macht den mehrdeutigen Zustand UNERREICHBAR statt ihn zu
// benennen. micro ist die Achse, die die PMC-Instrumentierung DEKLARIERT (Registry: "feinkoernige
// PMC/Counter-Instrumentierung"), also muss micro ==> COMDARE_ENABLE_PMC gelten.
//
// WARUM NUR BEI EINKOMPILIERTER COMBO: ohne COMDARE_MEASUREMENT_COMBO_CT hat NIEMAND eine spezifische
// Mess-Achse bestellt -- H-A laesst dort ausschliesslich die Vollmenge [all] zu, und [all] ist die Aussage
// "nicht eingeschraenkt", keine PMC-Zusage. Der gesamte heutige Bestand (Sidecar 0, Default-Bau,
// COMDARE_ENABLE_PMC=OFF per CMakeLists.txt:67) faellt in diesen Zweig und bleibt unberuehrt. Die Wache
// beisst exakt am ersten CT-hart gebauten Batch -- also genau dort, wo die Mehrdeutigkeit real wird.
//
// ----------------------------------------------------------------------------------------------------
// WARUM DIE WACHE NUR IN EINE RICHTUNG GEHT -- EINE VERWORFENE VERSCHAERFUNG, BENANNT STATT VERSCHWIEGEN
// ----------------------------------------------------------------------------------------------------
// Die symmetrische Form ("PMC an, aber micro nicht genannt => Wurf") stand hier zuerst und ist beim
// Merge von development GEFALLEN -- nicht aus Bequemlichkeit, sondern weil sie am Objekt widerlegt ist.
// M-2/P-PMC-1 (development 8894d983, Owner-KERN F9 "PMC ist PFLICHT fuer die Vollstaendigkeit aller
// perf-Messwerte") emittiert -DCOMDARE_ENABLE_PMC=ON auf DERSELBEN Configure-Zeile wie das Combo-Define:
//     experiment_plan_director.hpp:874-875
//     s += "    - cmake -B build -G Ninja -DCOMDARE_V32_ENABLE=ON" + ceb_pmc_compile_define() +
//          " -DCMAKE_BUILD_TYPE=Release" + ceb_combo_compile_define(c.legend) + "\n";
// Jede CT-hart gebaute CEB bekommt PMC=ON, unabhaengig von ihrer Combo. Die symmetrische Wache haette
// damit JEDEN nicht-micro-Batch unbaubar gemacht -- also ausgerechnet die [wallclock]-Faehigkeit
// erschlagen, die M-1/D-1 ueberhaupt erst gebaut hat, und einem Owner-KERN widersprochen.
// WAS ALS BENANNTE GRENZE BLEIBT: eine [wallclock]-CEB mit PMC=ON erhebt PMC-Zahlen, die ihr Glied [3]
// nicht nennt. Das ist KEIN F6-Bruch, solange PMC fuer jeden planer-emittierten Mess-Lauf denselben Wert
// hat (M-2 macht genau das) -- zwei Laeufe derselben Achse koennen sich dann nicht unterscheiden. Es
// bleibt eine Mehrdeutigkeit fuer HAND-konfigurierte CEBs, und sie ist erst dann sauber zu schliessen,
// wenn die PMC-Wahl ein Identitaets-Glied wird. Das ist ein Byte-Ereignis mit Owner-Entscheid und
// gehoert in eine eigene Scheibe, nicht hierher.
[[nodiscard]] inline bool pmc_ist_einkompiliert() noexcept {
#ifdef COMDARE_ENABLE_PMC
    return true;
#else
    return false;
#endif
}

/// pruefe_pmc_ausstattung(legend, micro, pmc) -- die REINE Entscheidung, damit der Biss sie ohne
/// Praeprozessor-Akrobatik ueber ALLE VIER Kombinationen fahren kann (zwei davon muessen durchgehen --
/// eine Wache, die immer wirft, waere keine). Der Live-Aufrufer unten reicht nur die beiden
/// Compile-Zustaende herein; hier steht kein Makro und kein Env.
///
/// Beide Seiten sind COMPILE-Zustand DERSELBEN CEB-TU; der Vergleich stellt also Deklaration gegen
/// Deklaration, nicht Deklaration gegen Laufzeit-Glueck. Die Laufzeit-Frage ("gelingt perf_event_open
/// hier?") ist eine andere und steht bereits ehrlich in der Spalte pmc_available.
inline void pruefe_pmc_ausstattung(std::string_view legend, bool micro, bool pmc) {
    // NUR diese eine Richtung -- die Gegenrichtung ist am Objekt widerlegt (Block oben).
    if (!micro || pmc) return;
    throw std::runtime_error("fehlerklasse=mess_ausstattung_widerspruch: die einkompilierte Mess-Achse '" +
                             std::string{legend} +
                             "' nennt 'micro' (Registry: feinkoernige PMC/Counter-Instrumentierung), aber diese "
                             "CEB ist ohne COMDARE_ENABLE_PMC gebaut -- die PMC-Spalten waeren durchgehend 0, "
                             "ununterscheidbar von einer PMC-Messung mit dem Ergebnis 0 (Owner-KERN F6). "
                             "Abhilfe: -DCOMDARE_ENABLE_PMC=ON konfigurieren oder 'micro' aus der Combo nehmen");
}

/// pruefe_pmc_gegen_mess_achse() -- der LIVE-Aufruf: die einkompilierte Mess-Achse gegen die
/// einkompilierte PMC-Ausstattung DIESER CEB. Ohne einkompilierte Combo ein No-op (Begruendung oben).
inline void pruefe_pmc_gegen_mess_achse() {
    namespace cm = ::comdare::cache_engine::measurement;
    if (ct_measurement_combo_legend().empty()) return; // keine spezifische Bestellung -> keine PMC-Zusage
    std::string const      legend = resolve_live_measurement_combo_legend();
    MessToolingMenge const menge  = mess_tooling_menge_from_legend(legend);
    pruefe_pmc_ausstattung(legend, menge[static_cast<std::size_t>(cm::MeasurementTooling::Micro)],
                           pmc_ist_einkompiliert());
}

} // namespace comdare::cache_engine::profile_facade
