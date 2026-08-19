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
// Plan-Soll dagegen (LEDGER §62-B, KOMPILATIONS-STATUS-KOPPLUNG, Owner 21.07.): "welche Pruef-Tools in
// die CEB EINKOMPILIERT sind [...] bestimmt, was auch das Tier-Binary beinhalten MUSS". Owner-KERN F2
// (LEDGER Nachtrag 06.08.2026 abend-4, Abschnitt 1): "eine Mess-Achsen-Aenderung baut CEB UND alle
// Binaries neu" -- bei einem neuen Messsystem muessen also auch die CEB und ALLE Binaries fuer die
// Mess-Achsen-Einstellung neu gebaut werden.
//
// ANKER-REGEL (10.08.2026 an genau dieser Stelle erhoben, 11.08.2026 nachgeschaerft):
// MARKE STATT ZEILENNUMMER.
// Hier stand ein Verweis auf Ledger-Zeile 3319. Der Satz ist unveraendert, die Zahl war tot: der
// Ledger wuchs von 16.785 auf 19.310 Zeilen, Zeile 3319 traegt heute den work_mode/--debug-Entscheid.
// Der Ledger hatte den Bruch selbst bemerkt -- in KON2-15, das die Verschiebung der im Code zitierten
// Zeilennummer 3319 ausdruecklich vermerkt -- und auf Zeile 9077 korrigiert. Auch diese Zahl war
// binnen Tagen tot; dort steht heute die §19.C/§19.D-Dock-Topologie.
//
// VIER ZAHLEN FUER EINEN UNVERAENDERTEN SATZ -- die vierte starb, waehrend dieser Absatz entstand.
// Die Fassung vom 10.08.2026 schrieb hier "am Objekt wohnt der Satz heute bei Zeile 10409". Das war
// am 10.08. richtig und ist am 11.08. falsch: der Ledger wuchs ueber Nacht von 19.310 auf 19.589
// Zeilen, Zeile 10409 traegt jetzt den §52-B13-Rest, der Satz wohnt bei 10688. Die Kette lautet also
// 3319 -> 9077 -> 10409 -> 10688; drei dieser vier Zahlen schrieb jemand, der GERADE EINE TOTE ZAHL
// REPARIERTE. Eine Zeilennummer laesst sich nicht pflegen, nur ersetzen.
//
// DARAUS DIE REGEL: Code verweist auf die MARKE (§62-B, KON2-15, "Nachtrag 05.08.2026 mittag-9"), nie
// auf eine Zeile -- auch nicht auf eine, die heute stimmt. Eine Gegenwarts-Zeilennummer ist keine
// Ausnahme von der Regel; sie ist der naechste Fall der Regel. Die Zahlen oben stehen als HISTORIE
// (was wann tot war), nicht als Nachschlage-Anker -- das ist der Unterschied, der sie zulaessig macht.
// Wache: tests/unit/test_anker_marke_statt_ledgerzeile.cpp.
// SELBSTCHECK: dieser Absatz nennt die toten Zahlen bewusst OHNE die Doppelpunkt-Form, sonst zaehlte
// die Wache ihre eigene Begruendung als Verstoss. Eine Ausnahme-Liste waere die schlechtere Loesung
// gewesen: sie waere die naechste Sache, die still verrottet.
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
// Die schaltbare Flaeche zerfaellt in DREI Schichten -- seit B2 (15.08.2026) sind ALLE drei trennbar:
//   G1 BASIS-ZEIT    IMeasurableWorkload::run_workload (abi_adapter.hpp:589-1123) -- der aeussere
//                    steady_clock je Batch. Liegt unter #if COMDARE_MEASUREMENT_ON.
//   G2 OBSERVER      fill_observer_v3 / axis_stats (abi_adapter.hpp:1426-1714) -- die per-Achsen-
//                    Zaehler. Liegt unter #ifdef COMDARE_CE_ENABLE_STATISTICS.
//   G3 FEINKORN      fill_segment_timing_v3 (abi_adapter.hpp) -- die 18 per-Achsen-Segment-Timer je
//                    Batch (samt spaetem T17-Read und dessen Reset). Lag bis B2 im G2-Gate; liegt
//                    seither unter #if COMDARE_CE_ENABLE_SEGMENT_TIMING (Ableitung/Vererbung:
//                    cache_engine/abi/mess_gate_segment_timing.hpp).
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
//               -> zusaetzlich CE_ENABLE_SEGMENT_TIMING=1 (B2; bis dahin wohnte G3 im G2-Gate).
//
// ------------------------------------------------------------------------------------------------
// DIE GRENZE IST GEFALLEN -- B2 (15.08.2026): macro UND micro SIND GETRENNT SCHALTBAR
// ------------------------------------------------------------------------------------------------
// Das hier bis B2 angekuendigte Folgepaket ist gebaut. G3 traegt sein eigenes Gate:
//     G2 (macro) : COMDARE_CE_ENABLE_STATISTICS      -- unveraendert (#ifdef, alle Bestands-Stellen)
//     G3 (micro) : COMDARE_CE_ENABLE_SEGMENT_TIMING  -- wertbasiert; die EINE Ableitung wohnt in
//                  cache_engine/abi/mess_gate_segment_timing.hpp (VERERBUNG: ist das Makro nicht
//                  gesetzt, folgt G3 dem Alt-Gate G2 -- kein Bestandsbau bewegt seinen Gate-Zustand)
// Geschaltet sind die drei Segment-Lauf-Flaechen in anatomy/abi_adapter.hpp (fill_segment_timing_v3,
// fill_observer_pathb_driven_v3, reset_pathb_driven_organs_); die A8-S4-Praeprozessor-Wache kennt
// das dritte Gate; das neunte Preimage-Glied traegt es als Feld <st> (mess_gates_glied.hpp).
// Diese Naht ENTSCHEIDET das G3-Gate ab B2 IMMER MIT, sobald sie G2 emittiert: =1 wenn micro in der
// Menge ist, =0 sonst (mess_achsen_defines unten). Ein blosses Weglassen waere die Vererbung und
// damit KEINE Entscheidung -- der Stufe-2-CT-Einbau (KON37-01) entscheidet aus der EIGENEN Menge.
//   wallclock : G1                  macro : G1+G2            micro : G1+G2+G3
//
// [HISTORIE, bis B2 wahr -- bleibt als Beleg, warum die Trennung nicht frueher behauptet wurde]
// G2 und G3 teilten sich EIN Gate; es gab im anatomy/-Baum kein drittes Makro. Ein solches Gate zu
// ERFINDEN, ohne die Schalt-Flaechen zu bauen, hiesse Semantik zu behaupten, die der Code nicht
// traegt -- der D-1-Fehler in die andere Richtung. Bis B2 galt deshalb: macro und micro erzeugten
// denselben GATE-Zustand und unterschieden sich ausschliesslich im Deklarations-Define
// (COMDARE_MEASUREMENT_TOOLING_<ID>=1, das die Wahl im Kompilat sichtbar und injektiv machte).
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
//   (d) STUFEN-DOKTRIN (LEDGER Nachtrag 05.08.2026 mittag-9/-10, Owner-Abnahme mittag-11; Tabellen-
//       form im Nachtrag 06.08.2026 abend-4 Abschnitt 1): MESS ist DREISTUFIG DEHNBAR
//       Planer (Stufe 1 = RT-Freigabe) -> CEB (CT-Einbau: Mess-Design + Pruefdock-Konfiguration)
//       -> [Hybrid (CT)] -> Tier (CT-Einbau: Observer/Ausstattung). SYSTEM ist ZWEISTUFIG DEHNBAR
//       (CEB = RT-Freigabe -> [Hybrid CT] -> Tier CT), ORGAN ZWEISTUFIG und hybrid-unberuehrt.
//       GESETZ: Stufe 1 ist IMMER die RT-Freigabe in der Traeger-Binary, jede Folgestufe ist
//       CT-Einbau entlang Planer -> CEB -> Tier.
//       Die Wahl der PMC-Quelle ist danach Stufe-2-Ware (CEB-Einbau + Pruefdock-Konfiguration) und
//       endet dort; sie hat auf Stufe 3 kein Objekt, an dem sie wirken koennte.
//       ANKER-REGEL (10.08.2026, s. Kopf dieser Datei): hier standen die Ledger-Zeilen 4082-4095. Tot --
//       Zeilen tragen heute den Landungsstand des Tages (super/ce-SHAs). Der Nachtrag ist eine
//       datierte MARKE und waechst nicht mit dem Dokument. NICHT zu verwechseln mit §61-STUFEN: das
//       ist die MODI-Leiter (measure liegt in release), eine andere Doktrin mit demselben Wort.
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
// (kAnatomyFingerprintGliedCount = 8 -- der Stand von M-1; heute sind es ZEHN, s. den
// UEBERHOLT-Vermerk direkt darunter). Was sich aendert, ist der WERT der Compile-Kommandos -- nicht
// die Form des Preimage.
//
// ------------------------------------------------------------------------------------------------
// [UEBERHOLT DURCH R-3, 07.08.2026] -- DER ABSATZ DARUEBER GILT FUER M-1 UND NICHT MEHR ALS LAGE.
// ------------------------------------------------------------------------------------------------
// Er ist HISTORIE und bleibt stehen, weil er richtig BEGRUENDET, warum M-1 keinen Bump brauchte:
// M-1 bewegte kein Preimage-Byte, es aenderte nur die WERTE der Compile-Kommandos. R-3 dagegen MUSS
// ein Byte in die Identitaet bringen -- der GATE-Zustand der Uebersetzungseinheit muss den Digest
// diskriminieren, und Glied [3] kann das nicht leisten: es ist ein HOST-Literal (die Behauptung des
// Bauwerkzeugs), nicht die Wahrheit der TU. Am Objekt gemessen (tests/unit/r3_mess_gate_stamp_module,
// EIN Quelltext, zwei .so, byte-identische Stempel-Literale):
//     Objekt GATES AN   sha256 820533fb...    sha512_line 6739cae7...
//     Objekt GATES AUS  sha256 e92658ce...    sha512_line 6739cae7...   <- IDENTISCH
// R-3 haengt deshalb ein NEUNTES Glied an (abi/mess_gates_glied.hpp) und bumpt fingerprint_format
// 3 -> 4. Die M-1-Kopplung (eine Legenden-Aufloesung speist Stempel UND Defines) bleibt, was sie war:
// eine VERHALTENS-Kopplung. Die IDENTITAETS-Kopplung ist erst das neue Glied.
// DIE HOST-SEITE DIESES GLIEDS WOHNT IN DIESER DATEI: mess_gates_glied_for_legend() unten.
//
// header-only, ASCII-only, keine Bau-Abhaengigkeit ausser der Mess-Tooling-Registry.

#include <cache_engine/abi/mess_gates_glied.hpp>      // R-3: die EINE Grammatik des Mess-Gates-Preimage-Glieds
#include <mess_axes/measurement_tooling_registry.hpp> // kMeasurementToolingRegistry (Single-Source)

#include <algorithm> // R-3: std::find ueber den Define-Vektor (die Spiegelung liest, statt nachzubauen)
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

/// mess_menge_mit_wallclock_erbe(bestellung) -- DIE B3-ENTSCHEIDUNG (KON37-01/KON34-04, 19.08.2026):
/// die CEB entscheidet die wallclock-Deklaration SELBST, das Tier erbt sie.
///
/// KON37-01 (Owner verbatim): "Die CEB ruft nur die Messfuehler in der Tier-Binary und Hybrid auf,
/// die sie selbst gebaut hat bzw. baut hoehere Traeger-Stufen nur nach ihren eigenen
/// Messeigenschaften." G1 (COMDARE_MEASUREMENT_ON) IST das wallclock-Instrument (M-1/H-1-Befund,
/// Block in mess_achsen_defines) und wird von JEDER nicht-leeren Menge gezogen -- jede messende CEB
/// TRAEGT also wallclock und BAUT es in jede Tier-Binary ein. Diese Funktion macht diese Eigenschaft
/// zur expliziten Entscheidung: die effektive Menge nennt wallclock, sobald irgendein Tooling
/// gewaehlt ist. Eine leere Bestellung bleibt leer (nichts gewaehlt -> nichts geerbt).
///
/// DAS IST DIE G3-MECHANIK EINE SCHICHT TIEFER: wie die Naht das G3-Gate immer MIT-entscheidet,
/// sobald sie G2 emittiert (B2), entscheidet sie die w-Deklaration immer mit, sobald G1 gezogen
/// wird. Ein blosses Weglassen waere keine Entscheidung, sondern der alte D-1-Zustand in klein:
/// wallclock-Messcode im Kompilat, aber keine Deklaration, die ihn nennt.
[[nodiscard]] inline MessToolingMenge mess_menge_mit_wallclock_erbe(MessToolingMenge bestellung) noexcept {
    namespace cm    = ::comdare::cache_engine::measurement;
    bool nicht_leer = false;
    for (bool const b : bestellung) nicht_leer = nicht_leer || b;
    if (nicht_leer) bestellung[static_cast<std::size_t>(cm::MeasurementTooling::WallClock)] = true;
    return bestellung;
}

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
/// B3 (19.08.2026, KON37-01/KON34-04): die Rueckgabe ist ab B3 die GEERBTE Legende -- jede
/// Rueckgabe laeuft durch mess_combo_legende_mit_wallclock_erbe (Herleitung dort). BYTE-BILANZ:
/// fuer jede bis B3 baubare Aufloesung ist das ein No-op, denn der M-1/H-1-Pflicht-Wurf wies
/// w-lose Bestellungen ab und die Vollmenge traegt wallclock ohnehin; die WACHEN unten vergleichen
/// weiter die ROHEN Bestell-Kanaele (Env gegen CT-Define), das Erbe liegt HINTER ihnen. Erst eine
/// w-lose spezifische Bestellung -- bis B3 unbaubar, Bestand 0 -- bekommt hier erstmals eine
/// geerbte Legende: Stempel-Seite UND Bau-Seite sehen dieselbe, weil BEIDE nur diese eine
/// Aufloesung lesen.
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

/// mess_combo_legende_mit_wallclock_erbe(legend) -- die LEGENDEN-Form der B3-Entscheidung
/// (mess_menge_mit_wallclock_erbe oben): nennt eine spezifische Legende wallclock nicht, wird es
/// VORN ergaenzt -- exakt die Abhilfe-Form, die der bis B3 hier lebende M-1/H-1-Pflicht-Wurf dem
/// Bediener diktierte ("die Combo als '[wallclock,...]' schreiben"). Nennt sie es, kommt der
/// Eingabe-String BYTE-IDENTISCH zurueck (keine Umsortierung, keine Kanonisierung).
///
/// WARUM ES DIESE FORM BRAUCHT: das Preimage-Glied [3] entsteht in einem ANDEREN Renderer
/// (abi::measurement_stamp_line_from_combo_legend), der die LEGENDE nimmt, nicht die Menge. Eine
/// nur auf Mengen-Ebene geerbte Deklaration erreichte den Stempel nie -- der Stempel verschwiege
/// wallclock, und ein Versions-Sprung der wallclock-Mess-Achse bewegte den Fingerprint nicht
/// (Owner-KERN F2, der M-1/H-1-Befund). Deshalb laeuft die EINE Aufloesung
/// (resolve_live_measurement_combo_legend) durch DIESE Funktion: Stempel-Seite und Bau-Seite sehen
/// dieselbe geerbte Wahrheit, keine zweite.
///
/// BEWUSST NICHT MEINE FAELLE (Semantik der Konsumenten, hier nur durchgereicht):
///   -- leere Legende: Stempel-Seite rendert leer, Bau-Seite wirft (mess_tooling_menge_from_legend);
///      beide Orte bleiben, wo sie sind.
///   -- Vollmengen-Formen ([all]/all/[]/leer-innen): wallclock ist Teil der Vollmenge, nichts zu
///      erben; der String bleibt byte-stabil (der gesamte Bestand ist [all]).
///   -- unbekannte ids: bleiben stehen; der Bau wirft an seiner bestehenden Stelle (fail-loud),
///      der Stempel-Renderer behaelt sein @0.0.0-Sentinel-Verhalten. Diese Funktion beantwortet
///      NUR die wallclock-Frage und verschiebt keinen Wurf-Ort.
/// Die Zerlegung (Klammern strippen, ',', leere Tokens ueberspringen) ist dieselbe wie im Parser
/// und im Renderer; die Kommutativitaets-Wache im M-1-Biss haelt Mengen- und Legenden-Form aneinander.
[[nodiscard]] inline std::string mess_combo_legende_mit_wallclock_erbe(std::string legend) {
    namespace cm = ::comdare::cache_engine::measurement;
    if (combo_legend_ist_vollmenge(legend)) return legend;
    std::string_view inner          = legend;
    bool const       hat_klammern   = inner.size() >= 2 && inner.front() == '[' && inner.back() == ']';
    if (hat_klammern) inner = inner.substr(1, inner.size() - 2);
    std::string_view const wallclock_id =
        cm::kMeasurementToolingRegistry[static_cast<std::size_t>(cm::MeasurementTooling::WallClock)].id;
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start && inner.substr(start, end - start) == wallclock_id) return legend; // w genannt: unbewegt
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    std::string geerbt;
    geerbt.reserve(legend.size() + wallclock_id.size() + 3);
    if (hat_klammern) geerbt += '[';
    geerbt += wallclock_id;
    geerbt += ',';
    geerbt += inner;
    if (hat_klammern) geerbt += ']';
    return geerbt;
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
    return mess_combo_legende_mit_wallclock_erbe(ct_legend); // B3: das Tier erbt die CEB-Entscheidung
#else
    char const* const e = std::getenv("COMDARE_MEASUREMENT_COMBO");
    // UNGESETZT == [all]: der Renderer bildet "[all]" auf measurement_stamp_line_full_set() ab, die
    // Vollmengen-Provenienz bleibt damit byte-identisch zum frueheren full_set()-Direktaufruf.
    // (B3: die Vollmenge traegt wallclock -- das Erbe unten ist hier beweisbar ein No-op; die
    // Rueckgaben laufen trotzdem ausnahmslos durch die eine Erbe-Stelle, damit kein Pfad an ihr
    // vorbei existiert.)
    if (e == nullptr || *e == '\0') return mess_combo_legende_mit_wallclock_erbe(std::string{"[all]"});
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
    return mess_combo_legende_mit_wallclock_erbe(std::string{e}); // B3: nur Vollmengen-Formen erreichbar, No-op
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

/// mess_menge_hat_feinkorn_gate(menge) -- traegt eine so gebaute Tier-Binary das FEINKORN (G3)?
///
/// B2 (15.08.2026). Dieselbe Rolle wie mess_menge_hat_observer_gate eine Schicht hoeher: die EINE
/// benannte Bedingung, aus der mess_achsen_defines() den Wert des G3-Gates
/// (-DCOMDARE_CE_ENABLE_SEGMENT_TIMING={1|0}) setzt und aus der die CSV-Seite ihr ehrliches "n/a"
/// fuer die seg_*-Spalten ableitet (live_mess_feinkorn_ausstattung unten). G3 ist der TIER-Beitrag
/// von micro (die Segment-Timer); die HOST-Kollektor-Frage PMC bleibt davon getrennt (Block unten).
///
/// WOZU DIE CSV-SEITE SIE BRAUCHT: fill_segment_timing_v3 hat seinen Rumpf ab B2 unter dem G3-Gate.
/// Eine [wallclock,macro]-Binary liefert seg_ns durchgehend 0 bei batches_measured == 0 -- Zahlen,
/// die von einer echten Messung mit Ergebnis 0 nicht unterscheidbar waeren. Dieselbe Luegen-Klasse,
/// die M-1/H-B fuer die Observer-Zellen geschlossen hat, eine Schicht hoeher.
[[nodiscard]] inline bool mess_menge_hat_feinkorn_gate(MessToolingMenge const& menge) noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
    return menge[static_cast<std::size_t>(cm::MeasurementTooling::Micro)];
}

/// mess_achsen_defines(menge) -- DIE ABBILDUNG: einkompilierte Mess-Achse -> Tier-Compile-Defines.
///
/// Das ist die Funktion, die D-1 heilt. Sie ist REIN (Argument rein, Vektor raus, kein Env, kein
/// Makro) -- deshalb ist sie testbar, und deshalb kann der BISS zeigen, dass zwei verschiedene
/// Mess-Achsen-Konfigurationen verschiedene Tier-Defines erzeugen.
///
/// REIHENFOLGE ist bindend (deterministisches Compile-Kommando): erst die GATES in der Reihenfolge
/// ihrer Schichten (G1 -> G2 -> G3), dann die DEKLARATIONEN in Registry-Reihenfolge.
///
/// BYTE-BILANZ [HISTORIE M-1, bis B2]: fuer die Vollmenge standen die beiden Gates in derselben
/// Reihenfolge und mit demselben Wortlaut da wie in der abgeloesten Literal-Liste; neu waren nur die
/// drei leserlosen Deklarations-Defines.
/// BYTE-BILANZ B2 (15.08.2026), DEKLARIERTES GOLDEN-EREIGNIS: [all] emittiert ab B2 zusaetzlich
/// -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1 (zwischen G2 und den Deklarationen). Der GATE-ZUSTAND und
/// damit das VERHALTEN jedes [all]-Kompilats sind unveraendert (G2+G3 an, wie immer); das
/// Bau-Kommando und das neunte Preimage-Glied (Feld <st>) bewegen sich aber fuer JEDE TU -- jeder
/// Fingerprint wandert, jedes Sidecar wird stale, die Flotte baut neu. Das ist die bestellte
/// Wirkung der Gate-Trennung (KON34/B2), kein Nebeneffekt; das golden-Fenster kommt separat.
/// BYTE-BILANZ B3 (19.08.2026): die Eingangs-Bestellung laeuft durch mess_menge_mit_wallclock_erbe.
/// Fuer jede bis B3 baubare Bestellung (wallclock genannt oder Vollmenge) ist das Erbe ein No-op --
/// derselbe Vektor Byte fuer Byte. w-lose Bestellungen waren bis B3 der M-1/H-1-Wurf (Bestand 0);
/// sie werden erstmals baubar und additiv. Kein bestehendes Compile-Kommando bewegt sich.
[[nodiscard]] inline std::vector<std::string> mess_achsen_defines(MessToolingMenge const& bestellung) {
    namespace cm = ::comdare::cache_engine::measurement;
    // B3 (KON37-01/KON34-04): die CEB entscheidet die wallclock-Deklaration selbst, das Tier erbt --
    // die EINE Entscheidung wohnt in mess_menge_mit_wallclock_erbe (Herleitung dort und im Block unten).
    MessToolingMenge const   menge = mess_menge_mit_wallclock_erbe(bestellung);
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
    // B3 (19.08.2026, KON37-01/KON34-04) -- DIE SCHALTER-HOHEIT LIEGT BEI DER CEB. Der M-1/H-1-
    // Pflicht-Wurf ist GEFALLEN; die wallclock-Deklaration ist am Funktionseingang GEERBT.
    // ------------------------------------------------------------------------------------------------
    // BEFUND M-1/H-1 (06.08.2026), am Objekt gemessen -- er bleibt der GRUND, nur die Antwort hat
    // sich gedreht: G1 (COMDARE_MEASUREMENT_ON) IST das wallclock-Instrument -- der aeussere
    // steady_clock je Batch in run_workload (abi_adapter.hpp). Die Zuordnung oben in diesem Header
    // sagt es woertlich: "auch [wallclock] misst ueber run_workload". Weil JEDE nicht-leere Menge G1
    // zieht, traegt auch eine [macro]-Tier-Binary den wallclock-Messcode -- ihr Glied [3] nannte ihn
    // aber NICHT. Das ist D-1 in klein, eine Ebene tiefer:
    //     Wallclock-Messcode auf high_resolution_clock geaendert + Registry wallclock 1.0.0c -> 2.0.0c
    //     [macro]-TIER-OBJEKT   329e45a0... -> 87bbbce6...   GEAENDERT
    //     [macro]-Glied [3]     measurement_tooling=macro@1.0.0.c;[...]   BYTE-GLEICH
    //     [macro]-tier_fp       ab530b58... -> ab530b58...   UNVERAENDERT
    // NENNER + GEGENPROBE (eine nackte Null ist kein Befund): dasselbe Verfahren trennt [macro] von
    // [wallclock] am Objekt (329e45a0 vs c3384d4b), und derselbe Versions-Bump BEWEGT den Fingerprint
    // sehr wohl, sobald wallclock in der Legende steht. Das Verfahren ist sehend -- der Stempel sah weg.
    // Folge: dll_is_current meldet "current" ueber eine Mess-Code-VERSIONSGRENZE hinweg. Owner-KERN F2.
    //
    // DIE B3-ANTWORT AUF DEN BEFUND (KON37-01, Owner verbatim: "Die CEB ruft nur die Messfuehler in
    // der Tier-Binary und Hybrid auf, die sie selbst gebaut hat bzw. baut hoehere Traeger-Stufen nur
    // nach ihren eigenen Messeigenschaften"; KON34-04: der Wallclock-Umzug Tier -> CEB hebt den
    // Pflicht-Wurf auf): nicht mehr der BEDIENER muss wallclock nennen -- die CEB ENTSCHEIDET die
    // Deklaration selbst, weil sie das Instrument ohnehin in jede Tier-Binary einbaut, und das Tier
    // ERBT sie. Mengen-Form am Funktionseingang (mess_menge_mit_wallclock_erbe), Legenden-Form in der
    // EINEN Aufloesung (resolve_live_measurement_combo_legend -> mess_combo_legende_mit_wallclock_erbe),
    // damit das Glied [3] dieselbe geerbte Wahrheit stempelt, die dieser Vektor baut.
    //
    // [HISTORIE M-1/H-1, bis B3 -- WARUM DAMALS WURF UND NICHT STILLE ERGAENZUNG DER MENGE: die
    // Stempel-Zeile entsteht in einem ANDEREN Renderer (abi::measurement_stamp_line_from_combo_legend),
    // der die Legende nimmt, nicht die Menge; eine NUR hier korrigierte Menge haette den Stempel nie
    // erreicht und waere die zweite Wahrheit gewesen. Der Wurf verlangte deshalb vom Bediener, was der
    // Bau ohnehin tat: "[macro]" -> "[wallclock,macro]". GENAU DIESES Argument ist durch die
    // Legenden-Form der Vererbung gegenstandslos: die Ergaenzung passiert jetzt VOR dem Renderer, in
    // der Aufloesung, die Stempel- UND Bau-Seite gemeinsam lesen -- sie erzeugt byte-genau die
    // Abhilfe-Form, die der Wurf diktierte, nur ohne den Umweg ueber den Bediener.]
    //
    // BYTE-BILANZ: [all] enthaelt wallclock -> unberuehrt; jede bis B3 baubare Bestellung nannte
    // wallclock selbst -> Erbe ist No-op. Der gesamte heutige Bestand ist [all] (Sidecar-Bestand 0).
    // Die Vererbung kann kein einziges bestehendes Byte bewegen; w-lose Bestellungen sind additiv neu.
    if (braucht_g1) d.emplace_back("-DCOMDARE_MEASUREMENT_ON=1");

    // G2 OBSERVER -- fill_observer_v3 / axis_stats. [wallclock] allein braucht sie nicht und bekommt
    // sie seit M-1 auch nicht mehr: die erste reale Wirkung der Mess-Achse auf das Tier-Kompilat.
    if (mess_menge_hat_observer_gate(menge)) d.emplace_back("-DCOMDARE_CE_ENABLE_STATISTICS=1");

    // G3 FEINKORN (B2, 15.08.2026) -- fill_segment_timing_v3 samt spaetem T17-Read und Reset. Die
    // Naht ENTSCHEIDET das Gate immer mit, sobald die Observer-Schicht existiert: =1 wenn micro in
    // der EIGENEN Menge dieser CEB ist (KON37-01), =0 sonst. Ein blosses WEGLASSEN waere keine
    // Entscheidung -- im Kompilat griffe dann die Vererbung des Ableitungs-Headers (G3 folgt G2),
    // und eine [wallclock,macro]-Binary bekaeme die Segment-Timer, die ihr Stempel verschweigt.
    // Unterhalb der Observer-Schicht ([wallclock]) gibt es nichts zu entscheiden: kein G3-Define,
    // die Vererbung laeuft dort auf dasselbe AUS, das auch G2 hat.
    if (mess_menge_hat_observer_gate(menge))
        d.emplace_back(mess_menge_hat_feinkorn_gate(menge) ? "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1"
                                                           : "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=0");

    // DEKLARATION -- ein Define je EINKOMPILIERTEM Tooling, Registry-Reihenfolge. Traegt die
    // Injektivitaet der Bestellung unabhaengig vom Gate-Teil (seit B2 unterscheidet auch der).
    for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
        if (menge[i]) d.push_back(mess_tooling_deklarations_define(i));

    return d;
}

/// mess_achsen_defines_for_legend(legend) -- die Bequemform fuer Tests und Werkzeuge: Legende rein,
/// Define-Vektor raus. Genau die Verkettung, die live_mess_achsen_defines() intern faehrt.
[[nodiscard]] inline std::vector<std::string> mess_achsen_defines_for_legend(std::string_view legend) {
    return mess_achsen_defines(mess_tooling_menge_from_legend(legend));
}

/// mess_gates_glied_for_legend(legend) -- DIE HOST-VORHERSAGE DES NEUNTEN PREIMAGE-GLIEDS (R-3).
///
/// Sie liefert exakt den Wert, den abi::kMessGatesTuGlied in einer Tier-Uebersetzungseinheit annimmt,
/// die mit den von mess_achsen_defines(mess_tooling_menge_from_legend(legend)) emittierten Defines
/// uebersetzt wird. Der Laufzeit-Zwilling des Fingerprints (lazy_adhoc_fingerprint_for) braucht diese
/// Vorhersage, weil er das Glied nicht aus der TU lesen KANN -- er rechnet ja auf der Host-Seite.
///
/// SIE IST EINE SPIEGELUNG UND KEINE ZWEITE ABBILDUNG, und zwar mechanisch: sie liest den
/// Define-VEKTOR, den mess_achsen_defines() erzeugt, statt dessen Bedingungen nachzubauen. Ein
/// zweiter, handgeschriebener Ausdruck ("micro oder macro -> s1") waere exakt die Divergenz-Klasse
/// aus D-1: die Bau-Seite haengte eine Ausstattung an, die Vorhersage behauptete eine andere, und
/// nichts braeche. Die GRAMMATIK wiederum wohnt nur EINMAL, in abi::mess_gates_glied_komponieren --
/// dieselbe Funktion, an die der Praeprozessor-Weg per static_assert gebunden ist.
///
/// WARUM x1 FEST STEHT: COMDARE_EXPERIMENT_MODE_ON ist NICHT Teil der Mess-Achsen-Abbildung. Es wird
/// von perm_mess_defines() (profile_run_facade.cpp) fuer JEDE Perm-Uebersetzung gesetzt -- die
/// Experiment-Kompilat-Markierung gilt unabhaengig von der Tooling-Wahl. Diese Funktion sagt deshalb
/// den PERM-BAU-Pfad vorher, und dort ist das Gate immer an. Wer eine Tier-Binary AUSSERHALB des
/// Perm-Pfads baut (Test-Modul, Release-Bau), bekommt ein anderes Glied -- gewollt: es ist ein
/// anderes Kompilat. Die Konsequenz ist in anatomy_fingerprint.hpp (Format 4) benannt.
[[nodiscard]] inline std::string mess_gates_glied_for_legend(std::string_view legend) {
    namespace cm                       = ::comdare::cache_engine::measurement;
    std::vector<std::string> const def = mess_achsen_defines_for_legend(legend);
    auto const hat = [&def](std::string const& d) { return std::find(def.begin(), def.end(), d) != def.end(); };
    // B2: das <st>-Feld spiegelt die ABLEITUNG der TU (mess_gate_segment_timing.hpp), mechanisch aus
    // dem Define-Vektor: ein explizites =1 gewinnt; ohne jede G3-Entscheidung erbt G3 den G2-Zustand.
    // (Aus DIESER Emission ist der Erb-Fall nur ohne G2 erreichbar -- die Naht entscheidet G3 immer
    // mit, wenn sie G2 emittiert. Die Spiegelung bildet trotzdem die volle Regel ab, nicht die
    // Emissions-Gewohnheit: ein zweiter, engerer Ausdruck waere die Drift-Klasse aus D-1.)
    bool const g2 = hat("-DCOMDARE_CE_ENABLE_STATISTICS=1");
    bool const g3_entschieden =
        hat("-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1") || hat("-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=0");
    bool const st = hat("-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1") || (!g3_entschieden && g2);
    // .str() ist der Laufzeit-Ausgang der EINEN Bildung: sie rechnet in einem heap-freien Puffer
    // (abi::MessGatesGliedText), damit derselbe Aufruf auch in einer constant expression laufen kann.
    return ::comdare::cache_engine::abi::mess_gates_glied_komponieren(
               hat("-DCOMDARE_MEASUREMENT_ON=1"), g2, st, /*experiment_mode_on=*/true,
               hat(mess_tooling_deklarations_define(static_cast<std::size_t>(cm::MeasurementTooling::WallClock))),
               hat(mess_tooling_deklarations_define(static_cast<std::size_t>(cm::MeasurementTooling::Macro))),
               hat(mess_tooling_deklarations_define(static_cast<std::size_t>(cm::MeasurementTooling::Micro))),
               // A-12/B-5e: die Hybrid-Ebene. Der PERM-BAU-Pfad baut TIER-Binaries -- er setzt die
               // Hybrid-Gates NICHT, also sagt die Vorhersage sie AUS vorher. Das ist keine Annahme,
               // sondern dieselbe Mechanik wie oben: was nicht im Define-Vektor steht, ist aus. Sobald
               // der Hybrid-Bau eigene Defines emittiert (B-8/A-12), gehoert HIER sein hat(...)-Paar
               // hin -- eine zweite Ableitung woanders waere die Drift-Klasse aus D-1.
               hat("-DCOMDARE_HYBRID_TOOLING_MACRO=1"), hat("-DCOMDARE_HYBRID_TOOLING_MICRO=1"))
        .str();
}

/// live_mess_gates_glied() -- der LIVE-Wert: das Mess-Gates-Glied der Tier-Binaries, die DIESE CEB
/// baut. Dieselbe EINE Aufloesung wie live_mess_achsen_defines() und wie die Stempel-Seite -- ein
/// dritter Ableitungsweg waere die Drift-Klasse aus D-1.
[[nodiscard]] inline std::string live_mess_gates_glied() {
    return mess_gates_glied_for_legend(resolve_live_measurement_combo_legend());
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

/// live_mess_feinkorn_ausstattung() -- traegt die Tier-Binary DIESES Laufs das FEINKORN (G3)?
/// B2 (15.08.2026): dieselbe Aufloesung, dieselbe Abbildung wie live_mess_observer_ausstattung --
/// eine Schicht hoeher. Die CSV-Seite (cache_engine_builder_iterator, seg_*-Spalten) entscheidet
/// damit zwischen echten Segment-Timern und ehrlichem "n/a": eine [wallclock,macro]-Binary liefert
/// seg_ns strukturell 0 (fill_segment_timing_v3 ist ohne G3 nicht einkompiliert) -- diese Nullen als
/// Messwerte zu schreiben waere dieselbe Luege, die M-1/H-B fuer die Observer-Zellen geschlossen hat.
[[nodiscard]] inline bool live_mess_feinkorn_ausstattung() {
    return mess_menge_hat_feinkorn_gate(mess_tooling_menge_from_legend(resolve_live_measurement_combo_legend()));
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
// 004251f4..., seit dem R-3-Format-Bump db7bac00...). Das ist ein deklariertes Byte-Ereignis mit
// Owner-Entscheid und ausdruecklich NICHT das,
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
