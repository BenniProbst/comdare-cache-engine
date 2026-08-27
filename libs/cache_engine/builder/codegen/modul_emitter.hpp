#pragma once
// STEMPEL TEIL 2 (B-7/RN-78 EMITTER-HAELFTE, WEICHE A) -- D-08 / H-23 TEIL B, Antworten L4 (=D1) + L3 (=C1).
// Owner 26.08.2026 13:00:03Z: "bitte auch den vergessenen Stempel Teil 2 suchen und mit implementieren vor
// dem Trigger" + RF-2 (~19:0xZ) "Volles GO" auf die Lesart B-7/RN-78-Emitter-Haelfte. Board #147.
//
// ZWEI-SCHICHTEN-MODELL (H-23 Teil B, Grundsatz): Schicht (a) = die PFLICHT (Loader-Riegel status 13,
// version_lines_symbol_missing, anatomy_module_loader.cpp) ist GEBAUT (A-11/golden-102, 19.08.2026) und
// wird hier NICHT umgebaut. Schicht (b) = die EMISSION (wer erzeugt gestempelten Modul-Quelltext) wird hier
// als EINE Strategy-Familie unter dem #24/B4-Ring verallgemeinert -- niemals als zweite Strecke neben
// emit_adhoc_modules. Begriffsdreiteilung (H-23 L5): stempel_namensfeld (kNM) / MODUL_EMITTER (dieses
// Werkzeug) / reroute_mess_emitter (HY-B/#123, hier unberuehrt).
//
// WEICHE A, WOERTLICH (H-23 L1 = A1): "der Stempel-Call bleibt eine separate Quelltext-Zeile, die das
// MODUL-EMITTER-WERKZEUG anhaengt ... die DEFINE-Makro-Aritaet aller 13 Makros/9 Dateien bleibt stabil".
// Folge fuer diese Datei: KEIN Makro wird angefasst. Der Stempel (COMDARE_ANATOMY_VERSION_STAMP bzw. _M)
// wird ANGEHAENGT, NACH dem DEFINE-Makro, im A-11-Muster der Hand-Fixtures (hybrid_tier_module.cpp:31-33,
// genus_module_set.cpp:26-27). CT-Haerte liefern Werkzeug (emittiert IMMER, sobald Stempel-Zeilen
// vorliegen) und Loader (status 13) GEMEINSAM.
//
// DIE EINE RENDERING-KERNFUNKTION: render_modul_source(Strategie, idx, macro_args, organ, system,
// measurement). Fuer die SearchAlgorithm-Gattung DELEGIERT sie an render_adhoc_module_source
// (adhoc_emitter.hpp) -- die Bytes der bestehenden SA-Emission (Katalog-/lazy-Pfad, 320er-Round-Trip-
// Wachen) bleiben UNANGETASTET. Die Container-Gattungen und der Hybrid fahren DIESELBE Form mit ihren
// Strategy-DATEN (Makro-Name, ABI-Include, Aritaet, Datei-Praefix; H-23 L3 woertlich: "je Gattung Makro-
// Name, Umbrella-Include und Achsen-/Argument-Bildung als STRATEGY-DATEN derselben Rendering-Funktion").
// Die Stempel-Anhaengung ist EINMAL geschrieben (append_anatomy_version_stamp) und byte-gleich zur
// SA-Weiche: 2-arg-Kurzform ohne Mess-Zeile, 3-arg _M-Vollform mit Mess-Zeile, Argument-Folge MESS,
// SYSTEM, ORGAN (S-6a, KON21-03).
//
// IDENTITAETS- UND GOLDEN-NEUTRALITAET (FINAL-stempel-teil2 K9): kFP/kNM rechnen aus den LITERALEN der
// Makro-Expansion, nicht aus emittierten Datei-Bytes; Glied [7] deckt die organ_axes/topics-Mengen, nicht
// die Emitter. Dieses Werkzeug bewegt weder ein Glied noch ein Format noch eine TABU-Datei; der
// V-03R-Budget-Posten bleibt unverbraucht.
//
// EINGANG = CEB-BAU-ENTSCHEID (H-23 L4): das D1-Verdict genus_build_verdict aus builder/experiment_tree/
// genus_build_admission.hpp. Dessen Kopf deklariert seit E-24 C5: "der RUNTIME-Verbraucher dieses Gates
// ist die gattungs-aware Modul-Quellen-Emission ... existiert am a-Teil noch nicht ... bereit fuer die
// Emitter-Naht des b-Teils". DIESE Datei ist dieser Verbraucher: modul_emission_zulassung() reicht das
// Verdict durch, und die Strategie-Aritaeten sind gegen kGenusBuildSlotCounts gepinnt (Cross-Pins unten).
// AUSGANG = kompilierbare, GESTEMPELTE Quelldatei (+ CMake-Anbindung: cmake/modul_emitter.cmake baut die
// Erzeugnisse als SHARED-Module; die Vertragspaare je Gattung laufen ueber den ECHTEN dlopen-Weg).
//
// F-17 (Owner 26.08.2026 13:00Z; FINAL-f17 = Option C "dynamisch je Planer"): die Dock-Zahl eines Hybrids
// ist ein Planer-Datum (HybridTierConfig::max_docks, XML-Pflichtangabe bei enabled), NIE eine Emitter-
// Konstante. Die Hybrid-Strategie dieser Datei kennt keine Dock-Zahl (makro_aritaet = 1 = das ZielGenus);
// die L2-Schwester (hybrid_modul_emitter.hpp) liest sie aus der Konfiguration und schreibt sie NICHT in
// den Quelltext. Die CEB-Pruefdock-Doktrin (CEB = EIN Pruefdock; MaxDocks = 1 im Hybrid-Makro, F8) bleibt
// unberuehrt.
//
// BEDARFSLAGE JE GATTUNG (ehrlich, am Objekt @ ce d3b5a393): SearchAlgorithm = produktiver Bedarfstraeger
// (lazy_adhoc_source_gen, pilot_source_map, ceb_generator, apps/adhoc_emitter, sota_catalog).
// Set/Sequence/View/Adapter = Engines mit for_each_composition_type vorhanden (E-24 C2), aber 0 produktive
// Emitter-Aufrufer -- die CEB baut heute SearchAlgorithm-Binaries; die vier Container-Genera existieren
// als Hand-Fixtures (perm_set_d9/perm_sequence_d10/perm_view_d11/perm_adapter_d12). Hybrid = zwei
// Hand-Fixtures (hybrid_reroute_searchalgorithm/_set). Diese Datei liefert je Gattung Vertrag, Strategie,
// Argument-Bildung, Organ-Zeile und Emitter und beweist sie ueber die C2-Vertragspaare (apps/modul_emitter
// + test_stempel2_vertragspaare, echter dlopen-Weg, stempellos = status 13). Der produktive Anschluss der
// Container-Emitter an die CEB-Bau-Naht ist der W4/W7-Posten nach Kampagnen-Scope (H-23 L3), kein
// Gegenstand dieses Zugs -- kein Blindbau von Totcode, aber auch keine Kuerzung des Vertrags.
//
// STEMPELBARKEIT AM OBJEKT (27.08.2026, ehrlich nachgemessen): die 18 SearchAlgorithm-Achsen tragen
// name() + algo_version an ihren Registry-Wrappern (der R5.G-Pilot stempelt sie). Die Set-Gattung besteht
// aus 13 dieser Achsen -> mit REALEN Registry-Typen stempelbar. Die genus-EIGENEN Achsen der drei anderen
// Container-Gattungen -- axis_growth (Sequence), axis_extent/layout/accessor (View), inner_container
// (Adapter) -- tragen am Objekt KEIN name()/algo_version (anatomy/DoublingGrowth, DynamicExtent,
// LayoutRight, DefaultAccessor, DequeInner<> [dort ist 'name' ein Daten-Member] und die topics/-Registries
// axis_growth_registry.hpp / view_registries.hpp; topics/ ist TABU-Menge). Eine reale Sequence-/View-/
// Adapter-Komposition ist damit heute NICHT stempelbar und wird von emit_modules EHRLICH stempellos
// emittiert (= nicht ladbar seit A-11). Die C2-Vertragspaare beweisen die gestempelte Form dieser drei
// Gattungen ueber stampbare HUELLEN-Typen der Fixture-Seite (apps/modul_emitter/stempel2_fixture_slots.hpp);
// die produktive Stempelbarkeit der genus-eigenen Achsen (name()/algo_version an ihren Varianten) ist ein
// eigener, TABU-beruehrender Zug (W4/W7) und hier NICHT vorgetaeuscht.
//
// @doku super docs/plaene/20260822-DESIGN-h23-d08-91-86-konsolidiert.md TEIL B (L1-L7)
//       ~/backups-workflow/20260826-absicherung-owner-tranche/FINAL-stempel-teil2.md Abschnitt 4
//       ~/backups-workflow/20260826-absicherung-owner-tranche/FINAL-f17-pruefdock.md (F-17 = C)

#include "adhoc_emitter.hpp" // render_adhoc_module_source (SA-Strategie), adhoc_macro_args, strip_all_elaborated
#include "type_name.hpp"

#include "../experiment_tree/genus_build_admission.hpp" // GenusBuildVerdict / genus_build_verdict / Slot-Zahlen (D1)

#include <anatomy/anatomy_base.hpp>                        // AnatomyGenus / genus_name
#include <cache_engine/abi/anatomy_version_stamp.hpp>      // organ_stamp_line<Comp>, system_stamp_line, OrganMetaMetas
#include <cache_engine/measurement/axis_version_stamp.hpp> // AxisVersionEntry + build_axis_version_stamp_line

#include <array>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::codegen {

// -----------------------------------------------------------------------------------------------------
// (1) DIE STRATEGY-DATEN JE GATTUNG -- was eine Gattung fuer die EINE Rendering-Funktion beisteuert.
// -----------------------------------------------------------------------------------------------------

/// ModulEmitterStrategie -- die DATEN einer Gattung/eines Genus fuer render_modul_source. Kein Code je
/// Gattung: Makro-Name, ABI-Include, Aritaet und Datei-Praefix sind Werte, die Rendering-Form ist EINE.
struct ModulEmitterStrategie {
    anatomy::AnatomyGenus genus;           ///< Genus des Erzeugnisses (Hybrid: FunctionInterfaceReroute)
    std::string_view      gattung_etikett; ///< Kopfzeilen-Etikett ("Set", "Hybrid", ...)
    std::string_view      abi_include;     ///< der Header, der das DEFINE-Makro traegt (ohne <>)
    std::string_view      define_makro;    ///< das DEFINE-Makro der Gattung (13/9-Karte, unangetastet)
    std::size_t           makro_aritaet;   ///< Argumente des DEFINE-Makros (Container: Slot-Zahl; Hybrid: 1)
    std::string_view      datei_praefix;   ///< Loader-Pattern comdare_anatomy_perm_* (load_all-Filter)
};

/// SearchAlgorithm: die bestehende ADHOC-Strecke. Umbrella-Include + 18 FQ-Achsen-Typen; render_modul_source
/// DELEGIERT fuer dieses Genus an render_adhoc_module_source (byte-gleich zur bestehenden Emission).
inline constexpr ModulEmitterStrategie kSearchAlgorithmModulStrategie{anatomy::AnatomyGenus::SearchAlgorithm,
                                                                      "SearchAlgorithm",
                                                                      "builder/codegen/all_axes_umbrella.hpp",
                                                                      "COMDARE_DEFINE_ANATOMY_MODULE_ADHOC",
                                                                      18,
                                                                      "comdare_anatomy_perm_auto_"};
/// Set (Container-Gattung, K-only): 13 Slots, Reihenfolge = SetComposition<T0..T12>.
inline constexpr ModulEmitterStrategie kSetModulStrategie{
    anatomy::AnatomyGenus::Set,  "Set", "cache_engine/abi/set_module_abi_v1.hpp",
    "COMDARE_DEFINE_SET_MODULE", 13,    "comdare_anatomy_perm_auto_set_"};
/// Sequence (Container-Gattung, V-indexed): 8 geteilte + growth_policy = 9 Slots (SequenceComposition).
inline constexpr ModulEmitterStrategie kSequenceModulStrategie{
    anatomy::AnatomyGenus::Sequence,  "Sequence", "cache_engine/abi/sequence_module_abi_v1.hpp",
    "COMDARE_DEFINE_SEQUENCE_MODULE", 9,          "comdare_anatomy_perm_auto_sequence_"};
/// View (Container-Gattung, non-owning): 2 geteilte + extent/layout/accessor = 5 Slots (ViewComposition).
inline constexpr ModulEmitterStrategie kViewModulStrategie{
    anatomy::AnatomyGenus::View,  "View", "cache_engine/abi/view_module_abi_v1.hpp",
    "COMDARE_DEFINE_VIEW_MODULE", 5,      "comdare_anatomy_perm_auto_view_"};
/// Adapter (Container-Gattung, Wrapper): 10 geteilte/delegierte + inner_container = 11 Slots.
inline constexpr ModulEmitterStrategie kAdapterModulStrategie{
    anatomy::AnatomyGenus::Adapter,  "Adapter", "cache_engine/abi/adapter_module_abi_v1.hpp",
    "COMDARE_DEFINE_ADAPTER_MODULE", 11,        "comdare_anatomy_perm_auto_adapter_"};
/// Hybrid (Gattung HeuristikAdapter, Reroute-Genus): GENAU EIN Makro-Argument = das Ziel-Genus. Die Dock-
/// Zahl ist KEINE Aritaet und KEIN Strategie-Datum (F-17: dynamisch je Planer, s. Datei-Kopf).
inline constexpr ModulEmitterStrategie kHybridModulStrategie{anatomy::AnatomyGenus::FunctionInterfaceReroute,
                                                             "Hybrid",
                                                             "hybrid/hybrid_module_abi_v1.hpp",
                                                             "COMDARE_DEFINE_HYBRID_MODULE",
                                                             1,
                                                             "comdare_anatomy_perm_auto_hybrid_"};

/// Die GESCHLOSSENE Liste aller Strategien -- Index == Genus-Wert (SearchAlgorithm=0 .. FunctionInterface-
/// Reroute=5), dieselbe Ordnung wie kGenusBuildSlotCounts. Eine siebte Gattung traegt sich HIER ein.
inline constexpr std::array<ModulEmitterStrategie, 6> kAlleModulStrategien{
    kSearchAlgorithmModulStrategie, kSetModulStrategie,  kSequenceModulStrategie,
    kAdapterModulStrategie,         kViewModulStrategie, kHybridModulStrategie};

/// modul_strategie_fuer(genus) -- FAIL-CLOSED: nullptr fuer eine Kennung, die keine Strategie traegt (eine
/// Gattungs-Kennung aus XML/Planer ist ein int, kein Beweis -- dieselbe Wache wie genus_is_build_bound).
[[nodiscard]] constexpr ModulEmitterStrategie const* modul_strategie_fuer(anatomy::AnatomyGenus g) noexcept {
    for (auto const& s : kAlleModulStrategien)
        if (s.genus == g) return &s;
    return nullptr;
}

namespace detail {
[[nodiscard]] consteval bool modul_strategien_index_treu() noexcept {
    for (std::size_t i = 0; i < kAlleModulStrategien.size(); ++i)
        if (static_cast<std::size_t>(kAlleModulStrategien[i].genus) != i) return false;
    return true;
}
} // namespace detail

static_assert(detail::modul_strategien_index_treu(),
              "STEMPEL-2/L4: kAlleModulStrategien ist Index == Genus-Wert geordnet (wie kGenusBuildSlotCounts).");
static_assert(kAlleModulStrategien.size() == experiment::kGenusBuildSlotCounts.size(),
              "STEMPEL-2/L4: je Eintrag der Bau-Bindung (D1) genau EINE Emitter-Strategie -- die Andock-Flaeche "
              "deckt alle bau-gebundenen Genera, keinen mehr und keinen weniger.");
// CROSS-PINS gegen den CEB-Bau-Entscheid: die Makro-Aritaet der fuenf ABI-sichtbaren Genera IST die
// Slot-Zahl der Bau-Bindung (GenusBindingTraits via genus_build_slot_count) -- eine Strategie mit anderer
// Aritaet emittierte ein Makro, das der Kompositions-Fabrik compile-hart widerspraeche.
static_assert(kSearchAlgorithmModulStrategie.makro_aritaet ==
              experiment::genus_build_slot_count(anatomy::AnatomyGenus::SearchAlgorithm));
static_assert(kSetModulStrategie.makro_aritaet == experiment::genus_build_slot_count(anatomy::AnatomyGenus::Set));
static_assert(kSequenceModulStrategie.makro_aritaet ==
              experiment::genus_build_slot_count(anatomy::AnatomyGenus::Sequence));
static_assert(kAdapterModulStrategie.makro_aritaet ==
              experiment::genus_build_slot_count(anatomy::AnatomyGenus::Adapter));
static_assert(kViewModulStrategie.makro_aritaet == experiment::genus_build_slot_count(anatomy::AnatomyGenus::View));
// Der Hybrid: die BAU-Aritaet des Reroute-Genus ist der Dock-Deckel (32, HY-A3) -- das ist die Zahl der
// Docks, die eine Hybrid-Binary tragen DARF, NICHT die Argument-Zahl ihres Makros. Das Makro nimmt genau
// das Ziel-Genus. Beide Zahlen sind hier bewusst getrennt gepinnt, damit niemand die 32 in eine
// Emissions-Konstante verwandelt (F-17).
static_assert(kHybridModulStrategie.makro_aritaet == 1,
              "STEMPEL-2/F-17: COMDARE_DEFINE_HYBRID_MODULE nimmt GENAU EIN Argument (das Ziel-Genus); die "
              "Dock-Zahl ist ein Planer-Datum und nie eine Emitter-Aritaet.");
static_assert(experiment::genus_build_slot_count(anatomy::AnatomyGenus::FunctionInterfaceReroute) ==
                  ::comdare::cache_engine::hybrid::kRerouteGenusCtSlotCount,
              "STEMPEL-2/F-17: die Bau-Aritaet des Reroute-Genus bleibt der Dock-DECKEL der Hybrid-Bindung "
              "(HY-A3) -- eine Obergrenze, keine Emissions-Konstante.");

// -----------------------------------------------------------------------------------------------------
// (2) EINGANG: der CEB-Bau-Entscheid (D1-Verdict) -- die Andock-Flaeche reicht ihn durch, nie daneben.
// -----------------------------------------------------------------------------------------------------

/// modul_emission_zulassung(genus, aritaet) -- darf fuer diese Gattung mit dieser Aritaet emittiert
/// werden? Das Urteil kommt AUS genus_build_verdict (genus_build_admission.hpp); hier wird nichts
/// nachgebaut. Eine Emission fuer ein abgelehntes Verdict ist ein Widerspruch: die Kompositions-Fabrik
/// braeche compile-hart, der Planer haette die Zelle als nicht_gebaut zu fuehren.
[[nodiscard]] constexpr experiment::GenusBuildVerdict modul_emission_zulassung(anatomy::AnatomyGenus g,
                                                                               std::size_t           aritaet) noexcept {
    return experiment::genus_build_verdict(g, aritaet);
}

static_assert(modul_emission_zulassung(anatomy::AnatomyGenus::Set, kSetModulStrategie.makro_aritaet).zugelassen());
static_assert(
    !modul_emission_zulassung(anatomy::AnatomyGenus::Set, kSequenceModulStrategie.makro_aritaet).zugelassen(),
    "STEMPEL-2/L4: ein Sequence-Argumentsatz an der Set-Strategie ist ein Aritaets-Fehler, kein Emitter-Fall.");

// -----------------------------------------------------------------------------------------------------
// (3) DIE STEMPEL-ANHAENGUNG -- EINMAL geschrieben, byte-gleich zur SA-Weiche in render_adhoc_module_source.
// -----------------------------------------------------------------------------------------------------

/// append_anatomy_version_stamp(src, organ, system, measurement) -- haengt die A-11-Stempel-Zeile AN.
/// Leere Organ-Zeile = KEIN Stempel (die Diagnose-/Fake-Klasse von emit_adhoc_modules: solche Module sind
/// seit A-11 NICHT ladbar, und genau das ist die Aussage -- sie bildet die Negativ-Seite der Vertragspaare).
/// Ohne Mess-Zeile die 2-arg-Kurzform (system, organ); mit Mess-Zeile die 3-arg _M-Vollform (measurement,
/// system, organ) -- S-6a: Kategorien-Ordnung MESS, SYSTEM, ORGAN. Stempel-Strings sind C-literal-sicher
/// (nur =@;.+_ [] und alnum, keine Quotes/Backslashes; adhoc_emitter.hpp) -- kein Escaping.
constexpr void append_anatomy_version_stamp(std::string& src, std::string_view organ_stamp,
                                            std::string_view system_stamp, std::string_view measurement_stamp) {
    if (organ_stamp.empty()) return;
    if (measurement_stamp.empty()) {
        src += "COMDARE_ANATOMY_VERSION_STAMP(\"";
        src += system_stamp;
        src += "\", \"";
        src += organ_stamp;
        src += "\")\n";
    } else {
        src += "COMDARE_ANATOMY_VERSION_STAMP_M(\"";
        src += measurement_stamp;
        src += "\", \"";
        src += system_stamp;
        src += "\", \"";
        src += organ_stamp;
        src += "\")\n";
    }
}

namespace detail {
/// Dezimal-Rendering eines nicht-negativen Index ohne std::to_string (das ist nicht constexpr; die
/// Kernfunktion soll in consteval-Proben laufen koennen -- s. Vertrags-Wachen unten).
[[nodiscard]] constexpr std::string dezimal(int wert) {
    if (wert <= 0) return "0";
    std::string s;
    while (wert > 0) {
        s.insert(s.begin(), static_cast<char>('0' + (wert % 10)));
        wert /= 10;
    }
    return s;
}
} // namespace detail

// -----------------------------------------------------------------------------------------------------
// (4) DIE EINE RENDERING-KERNFUNKTION
// -----------------------------------------------------------------------------------------------------

/// render_modul_source(strategie, idx, macro_args, organ, system, measurement, zusatz_includes) -- der
/// kompilierbare Modul-.cpp-Quelltext EINER Permutation einer Gattung.
///   SearchAlgorithm: DELEGATION an render_adhoc_module_source (adhoc_emitter.hpp) -- byte-gleich zur
///     bestehenden Emission (Umbrella-Include, ADHOC-Makro, dieselbe Stempel-Weiche).
///   alle anderen: Kopfzeile + ABI-Include + zusatz_includes (Kompositions-Header, Muster shape_include
///     der Shaped-Schwester) + DEFINE-Makro mit den Argumenten + ANGEHAENGTER Stempel im Hand-Fixture-
///     Muster (Include des Stempel-Makro-Traegers anatomy_module_abi_v1.hpp, dann die Stempel-Zeile).
/// Die DEFINE-Makros werden nur AUFGERUFEN, nie umgebaut (Weiche A).
[[nodiscard]] constexpr std::string render_modul_source(ModulEmitterStrategie const& s, int idx,
                                                        std::string_view macro_args, std::string_view organ_stamp = {},
                                                        std::string_view                  system_stamp      = {},
                                                        std::string_view                  measurement_stamp = {},
                                                        std::span<std::string_view const> zusatz_includes   = {}) {
    if (s.genus == anatomy::AnatomyGenus::SearchAlgorithm)
        return render_adhoc_module_source(idx, macro_args, organ_stamp, system_stamp, measurement_stamp);
    std::string src = "// AUTO-GENERATED by modul_emitter (Stempel Teil 2, Weiche A) -- Gattung ";
    src += s.gattung_etikett;
    src += " -- Permutation ";
    src += detail::dezimal(idx);
    src += " -- DO NOT EDIT\n#include <";
    src += s.abi_include;
    src += ">\n";
    for (std::string_view inc : zusatz_includes) {
        src += "#include <";
        src += inc;
        src += ">\n";
    }
    src += "\n";
    src += s.define_makro;
    src += "(\n    ";
    src += macro_args;
    src += ")\n";
    if (!organ_stamp.empty()) {
        // A-11-Muster der Hand-Fixtures: der Stempel-Makro-Traeger wird NACH dem Gattungs-Makro inkludiert
        // (die Gattungs-ABI-Header ziehen nur die decl), dann die angehaengte Stempel-Zeile.
        src += "#include <cache_engine/abi/anatomy_module_abi_v1.hpp>\n";
        append_anatomy_version_stamp(src, organ_stamp, system_stamp, measurement_stamp);
    }
    return src;
}

// -----------------------------------------------------------------------------------------------------
// (5) DIE VERTRAGS-PRAEDIKATE UEBER DAS ERZEUGNIS (constexpr; consteval-Proben unten)
// -----------------------------------------------------------------------------------------------------

/// erzeugnis_traegt_define_makro(quelle, makro) -- steht der Makro-AUFRUF im Erzeugnis?
[[nodiscard]] constexpr bool erzeugnis_traegt_define_makro(std::string_view quelle, std::string_view makro) noexcept {
    std::string_view const aufruf_praefix = makro;
    auto const             pos            = quelle.find(aufruf_praefix);
    return pos != std::string_view::npos && pos + aufruf_praefix.size() < quelle.size() &&
           quelle[pos + aufruf_praefix.size()] == '(';
}

/// erzeugnis_ist_gestempelt(quelle) -- die Weiche-A-Form: eine COMDARE_ANATOMY_VERSION_STAMP-Zeile steht
/// NACH einem COMDARE_DEFINE_-Makro-Aufruf (angehaengt, nie davor, nie im Makro). Quelle ohne Stempel: false.
[[nodiscard]] constexpr bool erzeugnis_ist_gestempelt(std::string_view quelle) noexcept {
    auto const define = quelle.find("COMDARE_DEFINE_");
    if (define == std::string_view::npos) return false;
    auto const stempel = quelle.find("COMDARE_ANATOMY_VERSION_STAMP", define);
    return stempel != std::string_view::npos;
}

namespace detail {
/// consteval-Probe der Kernfunktion an einer Container-Strategie: gestempelt gdw. Organ-Zeile vorhanden.
[[nodiscard]] consteval bool kernfunktion_haengt_stempel_an() {
    std::string const mit  = render_modul_source(kSetModulStrategie, 3, "int", "kern=x@1.0.0.c", "sys=code@1.0.0.c");
    std::string const voll = render_modul_source(kSetModulStrategie, 3, "int", "kern=x@1.0.0.c", "sys=code@1.0.0.c",
                                                 "measurement_tooling=wallclock@1.0.0.c");
    std::string const ohne = render_modul_source(kSetModulStrategie, 3, "int");
    return erzeugnis_ist_gestempelt(mit) && erzeugnis_ist_gestempelt(voll) && !erzeugnis_ist_gestempelt(ohne) &&
           erzeugnis_traegt_define_makro(mit, "COMDARE_DEFINE_SET_MODULE") &&
           erzeugnis_traegt_define_makro(ohne, "COMDARE_DEFINE_SET_MODULE") &&
           mit.find("COMDARE_ANATOMY_VERSION_STAMP(\"sys=code@1.0.0.c\", \"kern=x@1.0.0.c\")") != std::string::npos &&
           voll.find("COMDARE_ANATOMY_VERSION_STAMP_M(\"measurement_tooling=wallclock@1.0.0.c\", "
                     "\"sys=code@1.0.0.c\", \"kern=x@1.0.0.c\")") != std::string::npos;
}
} // namespace detail

static_assert(detail::kernfunktion_haengt_stempel_an(),
              "STEMPEL-2/L4-VERTRAG: die Kernfunktion haengt den Stempel NACH dem DEFINE-Makro an (2-arg ohne "
              "Mess-Zeile, 3-arg _M mit Mess-Zeile; Folge MESS, SYSTEM, ORGAN) und laesst eine Quelle ohne "
              "Organ-Zeile stempellos.");

// -----------------------------------------------------------------------------------------------------
// (6) ARGUMENT-BILDUNG JE GATTUNG -- die Achsen-Aliasse der Komposition in Makro-Reihenfolge (T0..Tn).
//     Byte-gleiche Fuge ",\n    " wie adhoc_macro_args (DRY im Textformat).
// -----------------------------------------------------------------------------------------------------

namespace detail {
class MakroArgumente {
public:
    template <class T>
    void add() {
        if (!s_.empty()) s_ += ",\n    ";
        s_ += strip_all_elaborated(type_name<T>());
    }
    [[nodiscard]] std::string fertig() && { return std::move(s_); }

private:
    std::string s_;
};
} // namespace detail

/// set_macro_args<C>() -- die 13 Set-Slots (SetComposition<T0..T12>, set_composition.hpp).
template <class C>
[[nodiscard]] std::string set_macro_args() {
    detail::MakroArgumente a;
    a.template add<typename C::search_algo>();
    a.template add<typename C::cache_traversal>();
    a.template add<typename C::path_compression>();
    a.template add<typename C::node_type>();
    a.template add<typename C::memory_layout>();
    a.template add<typename C::allocator>();
    a.template add<typename C::prefetch>();
    a.template add<typename C::concurrency>();
    a.template add<typename C::serialization>();
    a.template add<typename C::index_organization>();
    a.template add<typename C::io_dispatch>();
    a.template add<typename C::migration_policy>();
    a.template add<typename C::filter>();
    return std::move(a).fertig();
}

/// sequence_macro_args<C>() -- die 9 Sequence-Slots (8 geteilte + growth_policy, sequence_composition.hpp).
template <class C>
[[nodiscard]] std::string sequence_macro_args() {
    detail::MakroArgumente a;
    a.template add<typename C::memory_layout>();
    a.template add<typename C::allocator>();
    a.template add<typename C::prefetch>();
    a.template add<typename C::concurrency>();
    a.template add<typename C::serialization>();
    a.template add<typename C::value_handle>();
    a.template add<typename C::io_dispatch>();
    a.template add<typename C::migration_policy>();
    a.template add<typename C::growth_policy>();
    return std::move(a).fertig();
}

/// view_macro_args<C>() -- die 5 View-Slots (2 geteilte + extent/layout/accessor, view_composition.hpp).
template <class C>
[[nodiscard]] std::string view_macro_args() {
    detail::MakroArgumente a;
    a.template add<typename C::memory_layout>();
    a.template add<typename C::value_handle>();
    a.template add<typename C::extent_policy>();
    a.template add<typename C::layout_policy>();
    a.template add<typename C::accessor_policy>();
    return std::move(a).fertig();
}

/// adapter_macro_args<C>() -- die 11 Adapter-Slots (10 geteilte/delegierte + inner_container).
template <class C>
[[nodiscard]] std::string adapter_macro_args() {
    detail::MakroArgumente a;
    a.template add<typename C::search_algo>();
    a.template add<typename C::cache_traversal>();
    a.template add<typename C::memory_layout>();
    a.template add<typename C::allocator>();
    a.template add<typename C::prefetch>();
    a.template add<typename C::concurrency>();
    a.template add<typename C::serialization>();
    a.template add<typename C::value_handle>();
    a.template add<typename C::io_dispatch>();
    a.template add<typename C::migration_policy>();
    a.template add<typename C::inner_container>();
    return std::move(a).fertig();
}

/// modul_macro_args<G, C>() -- die Argument-Bildung als Strategie-Weiche ueber das Genus.
template <anatomy::AnatomyGenus G, class C>
[[nodiscard]] std::string modul_macro_args() {
    if constexpr (G == anatomy::AnatomyGenus::SearchAlgorithm)
        return adhoc_macro_args<C>();
    else if constexpr (G == anatomy::AnatomyGenus::Set)
        return set_macro_args<C>();
    else if constexpr (G == anatomy::AnatomyGenus::Sequence)
        return sequence_macro_args<C>();
    else if constexpr (G == anatomy::AnatomyGenus::View)
        return view_macro_args<C>();
    else if constexpr (G == anatomy::AnatomyGenus::Adapter)
        return adapter_macro_args<C>();
    else
        static_assert(G == anatomy::AnatomyGenus::SearchAlgorithm,
                      "STEMPEL-2: der Hybrid hat keine Achsen-Komposition -- seine Argument-Bildung ist das "
                      "Ziel-Genus (hybrid_modul_emitter.hpp), nicht eine Slot-Liste.");
}

// -----------------------------------------------------------------------------------------------------
// (7) ORGAN-ZEILE JE GATTUNG (Stempel-Speisung) -- dieselbe Grammatik + dieselbe Form-Wache wie
//     organ_stamp_line<Comp> (anatomy_version_stamp.hpp); Slot-Namen = die Kompositions-Aliasse.
// -----------------------------------------------------------------------------------------------------

/// SlotStampbar<T> -- traegt der Achsen-Typ name() + algo_version (Registry-Wrapper-API)?
template <class T>
concept SlotStampbar = requires {
    { T::name() } -> std::convertible_to<std::string_view>;
    { T::algo_version } -> std::convertible_to<std::string_view>;
};

template <class C>
concept SetStampbar = SlotStampbar<typename C::search_algo> && SlotStampbar<typename C::cache_traversal> &&
                      SlotStampbar<typename C::path_compression> && SlotStampbar<typename C::node_type> &&
                      SlotStampbar<typename C::memory_layout> && SlotStampbar<typename C::allocator> &&
                      SlotStampbar<typename C::prefetch> && SlotStampbar<typename C::concurrency> &&
                      SlotStampbar<typename C::serialization> && SlotStampbar<typename C::index_organization> &&
                      SlotStampbar<typename C::io_dispatch> && SlotStampbar<typename C::migration_policy> &&
                      SlotStampbar<typename C::filter>;

template <class C>
concept SequenceStampbar = SlotStampbar<typename C::memory_layout> && SlotStampbar<typename C::allocator> &&
                           SlotStampbar<typename C::prefetch> && SlotStampbar<typename C::concurrency> &&
                           SlotStampbar<typename C::serialization> && SlotStampbar<typename C::value_handle> &&
                           SlotStampbar<typename C::io_dispatch> && SlotStampbar<typename C::migration_policy> &&
                           SlotStampbar<typename C::growth_policy>;

template <class C>
concept ViewStampbar = SlotStampbar<typename C::memory_layout> && SlotStampbar<typename C::value_handle> &&
                       SlotStampbar<typename C::extent_policy> && SlotStampbar<typename C::layout_policy> &&
                       SlotStampbar<typename C::accessor_policy>;

template <class C>
concept AdapterStampbar = SlotStampbar<typename C::search_algo> && SlotStampbar<typename C::cache_traversal> &&
                          SlotStampbar<typename C::memory_layout> && SlotStampbar<typename C::allocator> &&
                          SlotStampbar<typename C::prefetch> && SlotStampbar<typename C::concurrency> &&
                          SlotStampbar<typename C::serialization> && SlotStampbar<typename C::value_handle> &&
                          SlotStampbar<typename C::io_dispatch> && SlotStampbar<typename C::migration_policy> &&
                          SlotStampbar<typename C::inner_container>;

/// SearchAlgorithm: dieselbe Bedingung, unter der emit_adhoc_modules die ECHTEN Zeilen emittiert -- der
/// Kern-Slot traegt die API; organ_stamp_line<C> verlangt sie dann fuer alle 18 (compile-hart).
template <class C>
concept SearchAlgorithmStampbar = SlotStampbar<typename C::search_algo>;

/// kGenusStampbar<G, C> -- ist die Komposition C in der Gattung G stempelbar (alle Slots tragen die API)?
template <anatomy::AnatomyGenus G, class C>
inline constexpr bool kGenusStampbar = false;
template <class C>
inline constexpr bool kGenusStampbar<anatomy::AnatomyGenus::SearchAlgorithm, C> = SearchAlgorithmStampbar<C>;
template <class C>
inline constexpr bool kGenusStampbar<anatomy::AnatomyGenus::Set, C> = SetStampbar<C>;
template <class C>
inline constexpr bool kGenusStampbar<anatomy::AnatomyGenus::Sequence, C> = SequenceStampbar<C>;
template <class C>
inline constexpr bool kGenusStampbar<anatomy::AnatomyGenus::View, C> = ViewStampbar<C>;
template <class C>
inline constexpr bool kGenusStampbar<anatomy::AnatomyGenus::Adapter, C> = AdapterStampbar<C>;

/// ContainerOrganMetaMetas -- die Organ-Meta-Meta-Menge der Container-/Hybrid-Organ-Zeilen: BEWUSST LEER und
/// BEWUSST NICHT abi::OrganMetaMetas. Jene ist die VOLLMENGE der SearchAlgorithm-Anatomie (E-10/ORG-19, Zug
/// bau/e10-38a2-org19, landet VOR diesem Zug): ab E-10 S4 ist sie nicht mehr leer (DiskIoOrganMetaMeta), und
/// organ_stamp_line<Comp> waehlt daraus JE COMP ueber den persistence_target-Slot. Keine Container-Anatomie
/// (Set/Sequence/View/Adapter) und kein Hybrid fuehrt diesen Slot -- eine Kopplung an die Vollmenge haette nach
/// der E-10-Landung jede dieser Zeilen stumm um ';[disk_io=...]' verlaengert (Segmentzahl != Aritaet, Stempel-
/// Blindstelle ohne Traeger). Ein Meta-Meta-Glied einer Container-Gattung traegt sich HIER ein, mit eigener
/// Selektion je Komposition (W4/W7-Posten), nie ueber die SA-Vollmenge. F-1 der Fremd-Refutation 27.08.2026
/// (Koeder-Beweis: ~/backups-workflow/20260826-stempel-teil2/refutation/, koeder-f1.diff + rot-f1/gruen-f1).
using ContainerOrganMetaMetas = ::comdare::cache_engine::measurement::MetaMetaMembers<>;

namespace detail {
/// Die EINE Zeilenbildung ueber ein Eintrags-Array: Form-Wache (FLAG-GRAMMATIK v2, dieselbe wie an der
/// SA-Organ-Zeile) + build_axis_version_stamp_line + Organ-Meta-Meta-Klammer-Anhang aus ContainerOrganMetaMetas
/// (leer per Deklaration, s. oben -- append_meta_meta_suffix laesst die Zeile dann BYTE-IDENTISCH).
template <std::size_t N>
[[nodiscard]] std::string
organ_zeile_aus(std::array<::comdare::cache_engine::measurement::AxisVersionEntry, N> const& entries) {
    std::string line = ::comdare::cache_engine::measurement::build_axis_version_stamp_line(entries);
    ::comdare::cache_engine::abi::append_meta_meta_suffix(
        line, ::comdare::cache_engine::abi::meta_meta_stamp_suffix_from_members<ContainerOrganMetaMetas>());
    return line;
}
} // namespace detail

/// set_organ_stamp_line<C>() -- "search_algo=...;cache_traversal=...;...;filter=..." (13 Eintraege).
template <class C>
    requires SetStampbar<C>
[[nodiscard]] std::string set_organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    static constexpr std::array<AxisVersionEntry, 13> entries{{
        {"search_algo", C::search_algo::name(), C::search_algo::algo_version},
        {"cache_traversal", C::cache_traversal::name(), C::cache_traversal::algo_version},
        {"path_compression", C::path_compression::name(), C::path_compression::algo_version},
        {"node_type", C::node_type::name(), C::node_type::algo_version},
        {"memory_layout", C::memory_layout::name(), C::memory_layout::algo_version},
        {"allocator", C::allocator::name(), C::allocator::algo_version},
        {"prefetch", C::prefetch::name(), C::prefetch::algo_version},
        {"concurrency", C::concurrency::name(), C::concurrency::algo_version},
        {"serialization", C::serialization::name(), C::serialization::algo_version},
        {"index_organization", C::index_organization::name(), C::index_organization::algo_version},
        {"io_dispatch", C::io_dispatch::name(), C::io_dispatch::algo_version},
        {"migration_policy", C::migration_policy::name(), C::migration_policy::algo_version},
        {"filter", C::filter::name(), C::filter::algo_version},
    }};
    static_assert(entries.size() == kSetModulStrategie.makro_aritaet,
                  "STEMPEL-2/L3: die Set-Organ-Zeile traegt ALLE 13 Set-Slots (Stempel-Blindstelle sonst).");
    static_assert(::comdare::cache_engine::measurement::axis_version_entries_are_wellformed(entries),
                  "Eine Set-Achse traegt eine UNPARSBARE algo_version (Flag-Grammatik v2 oder Sentinel 0.0.0).");
    return detail::organ_zeile_aus(entries);
}

/// sequence_organ_stamp_line<C>() -- 9 Eintraege (8 geteilte + growth_policy).
template <class C>
    requires SequenceStampbar<C>
[[nodiscard]] std::string sequence_organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    static constexpr std::array<AxisVersionEntry, 9> entries{{
        {"memory_layout", C::memory_layout::name(), C::memory_layout::algo_version},
        {"allocator", C::allocator::name(), C::allocator::algo_version},
        {"prefetch", C::prefetch::name(), C::prefetch::algo_version},
        {"concurrency", C::concurrency::name(), C::concurrency::algo_version},
        {"serialization", C::serialization::name(), C::serialization::algo_version},
        {"value_handle", C::value_handle::name(), C::value_handle::algo_version},
        {"io_dispatch", C::io_dispatch::name(), C::io_dispatch::algo_version},
        {"migration_policy", C::migration_policy::name(), C::migration_policy::algo_version},
        {"growth_policy", C::growth_policy::name(), C::growth_policy::algo_version},
    }};
    static_assert(entries.size() == kSequenceModulStrategie.makro_aritaet,
                  "STEMPEL-2/L3: die Sequence-Organ-Zeile traegt ALLE 9 Sequence-Slots.");
    static_assert(::comdare::cache_engine::measurement::axis_version_entries_are_wellformed(entries),
                  "Eine Sequence-Achse traegt eine UNPARSBARE algo_version.");
    return detail::organ_zeile_aus(entries);
}

/// view_organ_stamp_line<C>() -- 5 Eintraege (2 geteilte + extent/layout/accessor).
template <class C>
    requires ViewStampbar<C>
[[nodiscard]] std::string view_organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    static constexpr std::array<AxisVersionEntry, 5> entries{{
        {"memory_layout", C::memory_layout::name(), C::memory_layout::algo_version},
        {"value_handle", C::value_handle::name(), C::value_handle::algo_version},
        {"extent_policy", C::extent_policy::name(), C::extent_policy::algo_version},
        {"layout_policy", C::layout_policy::name(), C::layout_policy::algo_version},
        {"accessor_policy", C::accessor_policy::name(), C::accessor_policy::algo_version},
    }};
    static_assert(entries.size() == kViewModulStrategie.makro_aritaet,
                  "STEMPEL-2/L3: die View-Organ-Zeile traegt ALLE 5 View-Slots.");
    static_assert(::comdare::cache_engine::measurement::axis_version_entries_are_wellformed(entries),
                  "Eine View-Achse traegt eine UNPARSBARE algo_version.");
    return detail::organ_zeile_aus(entries);
}

/// adapter_organ_stamp_line<C>() -- 11 Eintraege (10 geteilte/delegierte + inner_container).
template <class C>
    requires AdapterStampbar<C>
[[nodiscard]] std::string adapter_organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    static constexpr std::array<AxisVersionEntry, 11> entries{{
        {"search_algo", C::search_algo::name(), C::search_algo::algo_version},
        {"cache_traversal", C::cache_traversal::name(), C::cache_traversal::algo_version},
        {"memory_layout", C::memory_layout::name(), C::memory_layout::algo_version},
        {"allocator", C::allocator::name(), C::allocator::algo_version},
        {"prefetch", C::prefetch::name(), C::prefetch::algo_version},
        {"concurrency", C::concurrency::name(), C::concurrency::algo_version},
        {"serialization", C::serialization::name(), C::serialization::algo_version},
        {"value_handle", C::value_handle::name(), C::value_handle::algo_version},
        {"io_dispatch", C::io_dispatch::name(), C::io_dispatch::algo_version},
        {"migration_policy", C::migration_policy::name(), C::migration_policy::algo_version},
        {"inner_container", C::inner_container::name(), C::inner_container::algo_version},
    }};
    static_assert(entries.size() == kAdapterModulStrategie.makro_aritaet,
                  "STEMPEL-2/L3: die Adapter-Organ-Zeile traegt ALLE 11 Adapter-Slots.");
    static_assert(::comdare::cache_engine::measurement::axis_version_entries_are_wellformed(entries),
                  "Eine Adapter-Achse traegt eine UNPARSBARE algo_version.");
    return detail::organ_zeile_aus(entries);
}

/// modul_organ_stamp_line<G, C>() -- die Organ-Zeile als Strategie-Weiche ueber das Genus
/// (SearchAlgorithm = die bestehende 18er-Zeile organ_stamp_line<C>, unveraendert).
template <anatomy::AnatomyGenus G, class C>
    requires(kGenusStampbar<G, C>)
[[nodiscard]] std::string modul_organ_stamp_line() {
    if constexpr (G == anatomy::AnatomyGenus::SearchAlgorithm)
        return ::comdare::cache_engine::abi::organ_stamp_line<C>();
    else if constexpr (G == anatomy::AnatomyGenus::Set)
        return set_organ_stamp_line<C>();
    else if constexpr (G == anatomy::AnatomyGenus::Sequence)
        return sequence_organ_stamp_line<C>();
    else if constexpr (G == anatomy::AnatomyGenus::View)
        return view_organ_stamp_line<C>();
    else
        return adapter_organ_stamp_line<C>();
}

// -----------------------------------------------------------------------------------------------------
// (8) DER EMITTER JE GATTUNG -- dieselbe Schleife wie emit_adhoc_modules (for_each_composition_type der
//     Gattungs-Engines, E-24 C2), stampbare Kompositionen tragen die ECHTEN Zeilen (Organ je Genus +
//     system_stamp_line), alle anderen bleiben Emissions-only (nicht ladbar seit A-11 -- die Aussage).
// -----------------------------------------------------------------------------------------------------

/// emit_modules<G, Engine>(strategie, out_dir, zusatz_includes) -- schreibt je Komposition des Engine ein
/// Modul-.cpp (Dateiname <datei_praefix><idx>.cpp, Loader-Pattern comdare_anatomy_perm_*) und liefert die
/// Pfade. SearchAlgorithm DELEGIERT an emit_adhoc_modules (EINE Strecke; identische Dateien).
template <anatomy::AnatomyGenus G, class Engine>
[[nodiscard]] std::vector<std::filesystem::path> emit_modules(ModulEmitterStrategie const&      s,
                                                              std::filesystem::path const&      out_dir,
                                                              std::span<std::string_view const> zusatz_includes = {}) {
    if constexpr (G == anatomy::AnatomyGenus::SearchAlgorithm) {
        (void)zusatz_includes; // der Umbrella-Include der SA-Strecke ist Teil der bestehenden Kernfunktion
        return emit_adhoc_modules<Engine>(out_dir);
    } else {
        std::error_code ec;
        std::filesystem::create_directories(out_dir, ec);
        std::vector<std::filesystem::path> files;
        int                                idx = 0;
        Engine::for_each_composition_type([&]<class C>() {
            std::filesystem::path const f = out_dir / (std::string{s.datei_praefix} + detail::dezimal(idx) + ".cpp");
            std::ofstream               out(f, std::ios::trunc);
            if constexpr (kGenusStampbar<G, C>) {
                std::string const organ  = modul_organ_stamp_line<G, C>();
                std::string const system = ::comdare::cache_engine::abi::system_stamp_line();
                out << render_modul_source(s, idx, modul_macro_args<G, C>(), organ, system, {}, zusatz_includes);
            } else {
                out << render_modul_source(s, idx, modul_macro_args<G, C>(), {}, {}, {}, zusatz_includes);
            }
            files.push_back(f);
            ++idx;
        });
        return files;
    }
}

// -----------------------------------------------------------------------------------------------------
// (9) DER CT-VERTRAG "ModulEmitter je Gattung" (H-23 L4 = D1) -- die Andock-Flaeche als Concept.
// -----------------------------------------------------------------------------------------------------

/// ModulEmitterVertrag<E> -- ein Emitter nennt sein Genus (CT) und emittiert in ein Verzeichnis (RT):
/// Eingang ist die Bau-Entscheidung des Planers (Genus + Achsen-Kombination = die Engine bzw. HybridConfig),
/// Ausgang sind die Pfade der geschriebenen, kompilierbaren Quelldateien.
template <class E>
concept ModulEmitterVertrag = requires(E const& e, std::filesystem::path const& out) {
    { E::genus } -> std::convertible_to<anatomy::AnatomyGenus>;
    { e.emittieren(out) } -> std::same_as<std::vector<std::filesystem::path>>;
};

/// GattungsModulEmitter<G, Engine> -- der Vertrags-Traeger je Gattung ueber eine Gattungs-Engine.
template <anatomy::AnatomyGenus G, class Engine>
struct GattungsModulEmitter {
    static constexpr anatomy::AnatomyGenus genus = G;
    static_assert(modul_strategie_fuer(G) != nullptr, "STEMPEL-2/L4: dieses Genus traegt keine Emitter-Strategie.");

    [[nodiscard]] std::vector<std::filesystem::path> emittieren(std::filesystem::path const& out) const {
        return emit_modules<G, Engine>(*modul_strategie_fuer(G), out, zusatz_includes);
    }

    std::span<std::string_view const> zusatz_includes{}; ///< Kompositions-Header (Muster shape_include)
};

} // namespace comdare::cache_engine::builder::codegen
