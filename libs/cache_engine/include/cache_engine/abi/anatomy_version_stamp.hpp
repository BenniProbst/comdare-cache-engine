// abi/anatomy_version_stamp.hpp -- compile-time-Ableitung der Organ-Stempel-Zeile aus einer Composition
// (Bau W12-A, Section 43, Inkrement 4-Wiring).
//
// Section 43: die Tier-Binary traegt ihre kOrganAxisVersionLine einkompiliert. Diese Zeile wird aus den 17
// Kompositions-Achsen-Typen der AdHocComposition abgeleitet (jede exponiert name() + algo_version), in
// kanonischer compose-Ordnung (Entscheid W12-A-5). NUR Haupt-Achsen (Section 42.b).
//
// BYTE-SICHER (Round-Trip-Wache): Dieser Helfer wird IM Makro COMDARE_DEFINE_ANATOMY_MODULE aus dem
// Composition-Typ aufgerufen -- der emittierte .cpp-QUELLTEXT bleibt unveraendert (weiterhin nur
// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<typen>)), die Emitter-Round-Trip-Byte-Wache (test_lazy_adhoc_source_gen)
// bleibt gruen. Der Stempel lebt im kompilierten Binary. SEPARATE Welt zur .algos-Sig (X.Y.Z-Voll-Form).

#pragma once

#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>             // A13-M2: Klammer-Anhang der Meta-Metas (Owner-Q1)
#include <cache_engine/abi/system_axis_code_versions.hpp>          // A2 (G2-4): kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/axis_version_stamp.hpp>         // AxisVersionEntry + build_axis_version_stamp_line
#include <cache_engine/measurement/external_utils_family_axis.hpp> // A13-M2: ExternalUtilsHub (System-Meta-Meta-Glieder)
#include <cache_engine/measurement/measurement_framework_registry.hpp> // O-8 Schritt 9: load_framework-Segment
#include <cache_engine/measurement/measurement_tooling_registry.hpp>   // K7b-2: kMeasurementToolingRegistry (Vollmenge)

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::abi {

/// Die Zahl der Organ-HAUPT-Achsen (ORG-18, topics/axis.hpp:18 "18 Slots"). Sie steht hier als
/// benannte Konstante und nicht als nackte 17/18 im Array-Typ, damit der static_assert unten die
/// Drift fangen kann, die A8.2 gerade behoben hat.
///
/// WARUM NICHT kCompositionAxisNames direkt: das Array wohnt in builder/experiment_tree/ und dieser
/// Header in abi/ -- ein Include waere eine Layer-Inversion (abi darf nicht auf builder zeigen). Die
/// Kopplung laeuft deshalb ueber die Zahl plus die Namens-Liste im Array unten, nicht ueber einen
/// Include. Wer ORG-18 aendert, aendert BEIDE Orte.
inline constexpr std::size_t kOrganAxisCount = 18;

/// A13-M2 (Owner-E2, OP-11-Rueckbau): die Glied-Reihenfolge der ORGAN-Realm-Meta-Metas -- die SINGLE-SOURCE.
/// HEUTE LEER: es gibt keine Organ-Meta-Meta, der Klammer-Anhang ist damit leer und die Organ-Zeile
/// BYTE-IDENTISCH (no-op). Gebaut ist der MECHANISMUS (Owner-Forderung E-10/nr798-B3org "ORGAN-analoges
/// Meta-Meta-Array + erweiterbarer Compile-Raum-Stempel"): wer eine Organ-Meta-Meta baut
/// (topics::OrganMetaMetaAxis), traegt sie HIER ein und sie stempelt, ohne dass eine Zeile Emitter-Code
/// angefasst wird.
/// WARUM DIE LISTE HIER (abi/) UND NICHT IN topics/ NEBEN DER WURZEL: MetaMetaMembers ist ein
/// measurement-Layer-Typ, und topics/ darf nicht auf measurement/ zeigen (Layer-Inversion). Die WURZEL
/// (topics/organ_meta_meta_axis.hpp) und die LISTE liegen deshalb bewusst getrennt; es gibt trotzdem nur
/// EINE Liste.
using OrganMetaMetas = ::comdare::cache_engine::measurement::MetaMetaMembers<>;

/// organ_stamp_line<Comp>() -- die kOrganAxisVersionLine "achse=algo@X.Y.Z;..." aus den 18 benannten
/// Achsen-Aliassen einer Composition, in kanonischer compose-Ordnung (== AdHocComposition-Alias-Ordnung
/// == kCompositionAxisNames). Jede Achse muss name() + algo_version tragen.
///
/// A8.2 (O-8 Schritt 7, OP-11): die Zeile traegt jetzt ACHTZEHN Eintraege -- persistence_target ist
/// DAZUGEKOMMEN. Das 17er-Array war ein golden-neutral bedingter Nicht-Nachzug der ORG-18-Welle: der
/// Stempel durfte damals nicht brechen, also blieb die 18. Achse draussen. Das OD-1-Stempel-PRINZIP
/// verlangt aber ALLE Organ-Achsen mit je-Achse-Algorithmus-Version; eine fehlende 18. Achse waere
/// eine Stempel-BLINDSTELLE, in der eine persistence_target-Drift unsichtbar bliebe.
/// OP-11-RUECKBAU (A13-M2, Owner-Entscheid E2 vom 02.08.2026). HISTORIE, hier woertlich erhalten
/// (Doku-Doktrin: supersedieren, nie loeschen) -- an dieser Stelle stand bis zum 02.08.2026:
///   "KEINE Meta-Meta-Eintraege in dieser Zeile (RF-7: je Typ EINE Array-Zeile, Typ-Trennung) --
///    load_framework stempelt in der MESS-Zeile, die System-Meta-Metas in der System-Sphaere."
/// (Quelle des Verbots: OP-11, super docs/sessions/20260727-PLAN-o8-fenster-atomar-ultracode.md:813-822.)
/// SUPERSEDED durch den Owner-Wortlaut: "Da eine Meta-Meta-Achse immer zu den Mess-Achsen, System-Achsen
/// oder ORGAN-Achsen gehoert, wird sie auch einfach dynamisch ans Ende der Kette in den bestehenden Zeilen
/// angehaengt." Die Organ-Zeile KANN ab jetzt Meta-Metas tragen -- als geklammerten Anhang an ihrem Ende,
/// gespeist aus abi::OrganMetaMetas.
/// RF-7 BLEIBT GUELTIG: je Achsen-Typ EINE Array-Zeile. Eine Organ-Meta-Meta stempelt im ORGAN-Realm und
/// NIE zeilen-fremd; load_framework bleibt in der Mess-Zeile, die System-Meta-Metas in der System-Zeile.
/// HEUTE: abi::OrganMetaMetas ist LEER -> der Anhang ist leer -> diese Zeile ist BYTE-IDENTISCH (no-op).
///
/// BLOCKER (W12-A, Live-Code-Befund): die REALEN AdHocComposition-Achsen-Typen sind STRATEGIE-Typen
/// (z.B. ObservableComposedContainer<...>) und tragen KEIN name()/algo_version -- nur die Registry-WRAPPER
/// (StaticAxisVariants_*) tragen sie. Diese Funktion ist daher (noch) NICHT auf reale Module anwendbar; sie
/// beweist die Format-/Ordnungs-Logik gegen name()/algo_version-tragende Typen (Test: Mock-Composition). Die
/// Metadaten-Quelle fuer den realen Modul-Stempel ist eine offene Architektur-Frage (Registry-basiert im
/// Emitter vs. Wrapper-Typ-Liste durchs Makro) -- beide beruehren die Emitter-Round-Trip-Byte-Wache.
template <class Comp>
[[nodiscard]] inline std::string organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::array<AxisVersionEntry, kOrganAxisCount> const entries{{
        {"search_algo", Comp::search_algo::name(), Comp::search_algo::algo_version},
        {"cache_traversal", Comp::cache_traversal::name(), Comp::cache_traversal::algo_version},
        {"mapping", Comp::mapping::name(), Comp::mapping::algo_version},
        {"path_compression", Comp::path_compression::name(), Comp::path_compression::algo_version},
        {"node_type", Comp::node_type::name(), Comp::node_type::algo_version},
        {"memory_layout", Comp::memory_layout::name(), Comp::memory_layout::algo_version},
        {"allocator", Comp::allocator::name(), Comp::allocator::algo_version},
        {"prefetch", Comp::prefetch::name(), Comp::prefetch::algo_version},
        {"concurrency", Comp::concurrency::name(), Comp::concurrency::algo_version},
        {"serialization", Comp::serialization::name(), Comp::serialization::algo_version},
        {"value_handle", Comp::value_handle::name(), Comp::value_handle::algo_version},
        {"index_organization", Comp::index_organization::name(), Comp::index_organization::algo_version},
        {"io_dispatch", Comp::io_dispatch::name(), Comp::io_dispatch::algo_version},
        {"migration_policy", Comp::migration_policy::name(), Comp::migration_policy::algo_version},
        {"filter", Comp::filter::name(), Comp::filter::algo_version},
        {"queuing_q1", Comp::queuing_q1::name(), Comp::queuing_q1::algo_version},
        {"queuing_q2", Comp::queuing_q2::name(), Comp::queuing_q2::algo_version},
        // A8.2 (OP-11): der 18. Slot. Er stand als einziger nicht in dieser Zeile.
        {"persistence_target", Comp::persistence_target::name(), Comp::persistence_target::algo_version},
    }};
    static_assert(entries.size() == kOrganAxisCount,
                  "Die Organ-Stempel-Zeile muss ALLE Organ-Haupt-Achsen tragen (ORG-18). Genau diese "
                  "Luecke war A8.2: das Array stand auf 17 und liess persistence_target aus, wodurch "
                  "eine Drift dieser Achse im Stempel unsichtbar blieb.");
    std::string line = build_axis_version_stamp_line(entries);
    // A13-M2 (OP-11-Rueckbau): der Organ-Meta-Meta-Klammer-Anhang ANS ENDE. abi::OrganMetaMetas ist heute leer
    // -> append_meta_meta_suffix laesst die Zeile BYTE-IDENTISCH. Der Mechanismus ist damit gebaut, ohne
    // ein einziges Byte zu bewegen (Beweis: die Organ-Golden-Anker in test_m_w12 blieben unveraendert).
    append_meta_meta_suffix(line, meta_meta_stamp_suffix_from_members<OrganMetaMetas>());
    return line;
}

/// system_stamp_line() -- die kSystemAxisVersionLine (Section 43, Entscheid W12-A-1). Die frueher hier
/// dokumentierte ZWEIPHASIGKEIT ist mit W10-C4 VOLLZOGEN:
///   PHASE 1 (bis W10): diese Funktion liefert die STATISCHEN System-Achsen-ALGORITHMUS-Versionen -- die
///     Code-Identitaet der Achsen, NICHT die gewaehlte Zelle. Das war eine bewusste Luecke, keine
///     Vergesslichkeit: der Tier-Emitter ist system-blind (W4-B-Invariante).
///   PHASE 2 (seit W10-C4, 04.08.2026): die CEB-Naht kennt die Zelle (perm_compile) und reicht sie als
///     Compile-Define COMDARE_SYSTEM_CELL_VALUES herein; das Stempel-Makro vervollstaendigt die hier
///     gerenderte Zeile consteval um die Zellwerte ("code" -> "code.<token>"). Der Emitter ist dabei NICHT
///     entblindet worden -- er schreibt weiter dasselbe Literal.
/// DIESE FUNKTION BLEIBT DESHALB DIE PHASE-1-FORM. Wer die VOLLE, gebaute Zeile braucht, ruft den
/// Vervollstaendiger: abi/system_cell_values.hpp ist die Single-Source der Zellwert-Grammatik und traegt
/// beide Formen (consteval complete_system_stamp_line_array fuer die Makro-Naht, complete_system_stamp_line
/// fuer den Laufzeit-Zwilling der CEB). Die WERTE-Aufloesung selbst wohnt eine Schicht weiter aussen in
/// profile_facade/system_cell_values_naht.hpp -- diese abi-Schicht kennt die Achsen-Zellen nicht und soll
/// sie nicht kennen.
/// Format identisch zur Organ-Zeile ("achse=algo@X.Y.Z"); Algorithmus-Marker "code" = die statische
/// Code-Identitaet der System-Achse, der Zellwert haengt als NAMENS-Anteil davor am '@' (Owner-Q2).
///
/// A13-M2 (Owner-Entscheid E2 + Antwort Q1 vom 02.08.2026): HINTER die drei Haupt-Achsen-Segmente haengt die
/// Zeile jetzt den KLAMMER-ANHANG der System-Meta-Metas -- heute "[simd=code@1.0.0c]", also VIER Eintraege statt
/// drei. Die Glieder kommen aus der EINEN Typliste ExternalUtilsHub::meta_metas (keine zweite Liste); die
/// EBENE steckt in der Klammer-Tiefe, nicht im Namen. Owner-E2 woertlich: Meta-Metas werden "einfach dynamisch
/// ans Ende der Kette in den bestehenden Zeilen angehaengt" -- keine Sonderzeile, kein Sonderfeld.
/// BYTE-FOLGE (beabsichtigt, nicht Nebeneffekt): die System-Zeile aendert sich fuer JEDE Binary -> alle
/// kuenftigen Fingerprints/Lager-Keys verschieben sich. Das ist der geplante M2-Anteil des einen
/// Fingerprint-Global-Shifts VOR Voll-Bau-4 (vorher existiert kein schuetzenswerter Bestand).
[[nodiscard]] inline std::string system_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    // A2 (G2-4 Schritt 3): die Achsen + Code-Versionen aus der Single-Source system_axis_code_versions.hpp
    // (frueher hartkodiert als {"<achse>","code","v1"}); "code" bleibt der Achsen-Marker, die Version ist je
    // Achse bump-bar. A3 (O-8 Schritt 4): die Zeile traegt GENAU DREI HAUPT-Achsen-Segmente statt fuenf -- die
    // Schleife zieht das aus kSystemAxisCodeCount automatisch nach, hier war KEIN Edit noetig. Genau dafuer
    // wurde die Hartkodierung damals aufgeloest.
    // Render: "v1.0.0c" -> "1.0.0c" (seit A13-M3/C4; bis dahin render-neutral "v1.0.0" -> "1.0.0" wie "v1").
    std::array<AxisVersionEntry, kSystemAxisCodeCount> entries{};
    for (std::size_t i = 0; i < kSystemAxisCodeCount; ++i)
        entries[i] = {kSystemAxisCodeVersions[i].axis, "code", kSystemAxisCodeVersions[i].version};
    std::string line = build_axis_version_stamp_line(entries);
    // A13-M2: der Meta-Meta-Klammer-Anhang ANS ENDE. Single-Source der Glieder == ExternalUtilsHub::meta_metas
    // (external_utils_family_axis.hpp:149) -- ein spaeteres gpu/fpga/npu-Glied erscheint hier ohne Edit.
    append_meta_meta_suffix(line, meta_meta_stamp_suffix<::comdare::cache_engine::measurement::ExternalUtilsHub>());
    return line;
}

/// measurement_stamp_line(tooling) -- die kMeasurementAxisVersionLine (Section 43, Section 47: Mess-Tooling == HAUPT,
/// W12-A3). Traegt GENAU die gewaehlte Mess-Tooling-HAUPT-Wahl {wallclock/macro/micro} (die collector-Achse,
/// Plan-D1: von binary_id="never" zur Fan-out-HAUPT promotet, binary_id-relevant NUR ueber diesen Version-Line-
/// Stempel) als EINEN Eintrag "measurement_tooling=<tooling>@X.Y.Z". Analog zu system_stamp_line(): dieselbe
/// AxisVersionEntry/build_axis_version_stamp_line-Welt, dieselbe X.Y.Z-Voll-Form (SEPARATE Welt zur .algos-Sig).
///
/// Section-43-INVARIANTE, PRAEZISIERT (O-8 Schritt 9 / RF-1): NUR Haupt-Achsen. Die Ablaufmethodik
/// (run_methodology debug/measure/release) bleibt UNTER-Achse (Laufzeit-Sweep) und NIE Stempel-Bestandteil.
/// Beim Last-Framework ist zu TRENNEN, was der alte Text zusammenwarf: die Framework-WAHL ist seit A3 eine
/// Meta-Meta-HAUPT-Achse des Mess-Realms und damit STEMPELBAR (sie steht als erstes Segment in dieser Zeile);
/// die Workload-WERTE (ycsb_a..f, Ranges) bleiben RT-Unter-Achse und werden NIE gestempelt. Der Algorithmus-
/// Marker == die gewaehlte Tooling-id; die Version == die statische Code-Identitaet der Mess-Tooling-Achse
/// ("v1" -> 1.0.0). Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert; die Makro-
/// Materialisierung legt dann measurement_line auf "" mit measurement_len==0).
/// O-8 Schritt 9 (RF-1 / Ledger 70.1, IV.2.4 K3): das load_framework-SEGMENT der Mess-Zeile.
///
/// SCHARFSCHALTUNG des in Schritt 0A inert angelegten version-Feldes: die Mess-Framework-Achse wird
/// jetzt gestempelt. load_framework hat mit A3 die System-Welt ERSATZLOS verlassen und ist
/// Meta-Meta-HAUPT-Achse des MESS-Realms -- sein Segment gehoert deshalb NUR in diese Zeile und NIE in
/// die System- oder Organ-Zeile (RF-7: je Achsen-Typ EINE Array-Zeile).
///
/// POSITION -- OP-3-RUECKBAU (A13-M2, Owner-Entscheid E2 vom 02.08.2026): load_framework stand seit O-8
/// Schritt 9 als ERSTES Segment VOR measurement_tooling (OP-3, Manager-Entscheid vom 27.07.2026, Quelle
/// super docs/sessions/20260727-PLAN-o8-fenster-atomar-ultracode.md:781-783). Der Owner-Wortlaut vom
/// 02.08.2026 verdraengt ihn: "Da eine Meta-Meta-Achse immer zu den Mess-Achsen, System-Achsen oder
/// Organ-Achsen gehoert, wird sie auch einfach dynamisch ANS ENDE DER KETTE in den bestehenden Zeilen
/// angehaengt." load_framework IST die Meta-Meta-HAUPT-Achse des Mess-Realms (measurement_meta_meta_axis.hpp)
/// und steht deshalb ab jetzt AM ENDE -- und zwar in derselben KLAMMER-Form wie die System-Meta-Metas
/// (Owner-Q1): "measurement_tooling=<t>@X.Y.Z;[load_framework=<id>@X.Y.Z]".
/// EINE Regel fuer alle Realms: waere das Mess-Meta-Meta ein klammerloses Segment, traege der Entry-POD es
/// als Ebene 0 == Haupt-Achse -- ein Konsument (Lager-Identitaet/G-E6/A2) koennte es dann nicht mehr von
/// einer Mess-HAUPT-Achse unterscheiden, waehrend er es im System-Realm sehr wohl kann.
///
/// WARUM NUR BEI NICHT-LEERER ZEILE: eine leere Mess-Zeile heisst "kein Mess-Tooling einkompiliert",
/// und die Makro-Materialisierung verlaesst sich darauf (measurement_line == "", measurement_len == 0).
/// Ein Segment in eine sonst leere Zeile zu schreiben wuerde aus "nichts gemessen" ein "etwas
/// gemessen" machen -- die Zeile bleibt deshalb leer, wenn sie leer ist.
[[nodiscard]] inline ::comdare::cache_engine::measurement::AxisVersionEntry load_framework_stamp_entry() noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
    // Heute genau EIN reales Last-Framework (kMeasurementFrameworkCount == 1, static_assert-gesichert);
    // id und Version kommen aus der Registry, nicht aus Literalen.
    auto const& info = cm::measurement_framework_info(cm::MeasurementFramework::Ycsb);
    return {"load_framework", info.id, info.version};
}

/// A13-M2: der Mess-Meta-Meta-Klammer-Anhang. EINE Stelle fuer beide Ueberladungen (Einzel- und Mengen-Form)
/// -- sonst existierte die Owner-E2-Ordnung zweimal und koennte driften (genau die O-8-Schritt-12-Falle).
[[nodiscard]] inline std::string measurement_meta_meta_suffix() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    std::array<AxisVersionEntry, 1> const metas{{load_framework_stamp_entry()}};
    return meta_meta_stamp_suffix_from(std::span<AxisVersionEntry const>{metas});
}

[[nodiscard]] inline std::string measurement_stamp_line(std::string_view tooling) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    if (tooling.empty()) return {};
    // A2 (G2-4 Schritt 4): die Code-Version aus der Tooling-Registry (Lookup per id) statt der "v1"-Hartkodierung;
    // bekannte id -> "v1.0.0c" (Render "1.0.0c", seit A13-M3/C4), unbekannte id -> "v0.0.0"-Sentinel (@0.0.0, nur
    // ungueltige ids; A13-M1b: dreistellig nach Owner-Q3, byte-neutral zum frueheren "v0").
    // A13-M2 (OP-3-Rueckbau, Owner-E2): load_framework steht NICHT mehr vorne, sondern als KLAMMER-Anhang
    // ANS ENDE.
    std::array<AxisVersionEntry, 1> const entries{{
        {"measurement_tooling", tooling, ::comdare::cache_engine::measurement::tooling_version_for_id(tooling)},
    }};
    std::string                           line = build_axis_version_stamp_line(entries);
    append_meta_meta_suffix(line, measurement_meta_meta_suffix());
    return line;
}

/// measurement_stamp_line(toolings) -- K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form der
/// kMeasurementAxisVersionLine. Statt EINER Tooling-Wahl traegt die Zeile die MENGE der gewaehlten Mess-Tools als N
/// Eintraege "measurement_tooling=<t>@1.0.0c" (';'-getrennt, Eingabe-Reihenfolge; Section-64-Vollmengen-Provenienz).
/// Leere Tokens werden uebersprungen; leere/leer-gefilterte Menge -> leere Zeile. Dieselbe X.Y.Z-Voll-Form / SEPARATE
/// Welt zur .algos-Sig wie die Einzel-Form (build_axis_version_stamp_line). binary_id-NEUTRAL (Mess-Achse
/// binary_id="never" -> der Stempel lebt nur im Version-Line/Binary, nie in der binary_id/CRC).
[[nodiscard]] inline std::string measurement_stamp_line(std::span<std::string_view const> toolings) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::vector<AxisVersionEntry> entries;
    entries.reserve(toolings.size());
    // A13-M2 (OP-3-Rueckbau, Owner-E2 "ans Ende der Kette"): load_framework wandert als KLAMMER-Anhang ans
    // ZEILEN-ENDE -- aber nur, wenn die Zeile ueberhaupt entsteht. Die Leer-Semantik ("kein Mess-Tooling
    // einkompiliert" => leere Zeile) bleibt unberuehrt; append_meta_meta_suffix laesst eine leere Zeile leer.
    for (std::string_view const t : toolings)
        if (!t.empty())
            // A2 (G2-4 Schritt 4): Code-Version per id-Lookup (Registry) statt "v1"-Hartkodierung; Sentinel "v0.0.0"
            // fuer unbekannte ids. Die gueltigen ids (wallclock/macro/micro) rendern seit A13-M3/C4 "1.0.0c"
            // -- deklariertes Byte-Ereignis der Q3-Migration, nicht mehr render-neutral.
            entries.push_back(
                {"measurement_tooling", t, ::comdare::cache_engine::measurement::tooling_version_for_id(t)});
    if (entries.empty()) return {};
    std::string line = build_axis_version_stamp_line(entries);
    append_meta_meta_suffix(line, measurement_meta_meta_suffix());
    return line;
}

/// measurement_stamp_line_full_set() -- K7b-2 (Section 64-D1-B): die VOLLE Mess-Tooling-Vollmenge
/// {wallclock,macro,micro} (Single-Source kMeasurementToolingRegistry, Registry-Reihenfolge) als Mengen-Stempel. Das
/// ist der Section-64-[all]-Default: eine [all]-CEB misst mit dem vollen Tooling-Angebot -> ihre Provenienz traegt
/// ALLE Tools. Genau kMeasurementToolingCount Eintraege, immer non-empty (die Registry ist nie leer, static_assert).
[[nodiscard]] inline std::string measurement_stamp_line_full_set() {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    std::array<std::string_view, kMeasurementToolingCount> ids{};
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) ids[i] = kMeasurementToolingRegistry[i].id;
    return measurement_stamp_line(std::span<std::string_view const>{ids});
}

/// measurement_stamp_line_from_combo_legend(legend) -- die Mess-Tooling-Stempel-Zeile aus einer Planer-Combo-Legende
/// (S6-P1b Env-Bruecke: COMDARE_MEASUREMENT_COMBO traegt die vom Planer gewaehlte [a,b,c]-Legende, z.B. "[wallclock]").
/// LEER oder "[all]" (das volle Mess-System, kein Tooling-spezifischer Fan-out) -> "" (kein Stempel; der byte-stabile
/// Default-Pfad -> emittierte Quelltexte byte-identisch). Sonst: die inneren Tooling-ids (ohne die []-Klammern) als
/// Stempel-Tooling. So reist die gewaehlte Combo bis in die emittierte DLL-Source, ohne den Emitter zu entblinden.
[[nodiscard]] inline std::string measurement_stamp_line_from_combo_legend(std::string_view legend) {
    // Leere Legende = "keine Legende gereicht" -> byte-stabil leer (der from_env-UNGESETZT-Zweig entscheidet dort
    // ueber die Vollmengen-Default-Semantik, NICHT dieser reine Legenden-Parser).
    if (legend.empty()) return {};
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    // Section 64-D1-B (2026-07-22): [all] / leer-innen -> die VOLLE 3-Tool-Vollmenge (Vollmengen-Provenienz), NICHT
    // mehr "" (das war die Regression: die [all]-Lane emittierte leere Mess-Provenienz).
    if (inner.empty() || inner == "all") return measurement_stamp_line_full_set();
    // Sonst: die inneren Tooling-ids als MENGE (komma-getrennt, Eingabe-Reihenfolge) -> N-Eintrags-Stempel.
    std::vector<std::string_view> toks;
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start) toks.push_back(inner.substr(start, end - start));
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return measurement_stamp_line(std::span<std::string_view const>{toks});
}

// A13-M3 (Owner-E2 02.08.2026, "Merge Zeile kann daher nicht existieren"): der frueher hier stehende
// merge_stamp_line-Renderer (K6a/Section 59, "der DRITTE Tier-Binary-Stempel") ist ERSATZLOS ENTFERNT --
// kein Deprecation-Stub (DV-1 = (a)). Begruendung: "darf nicht existieren" ist eine Existenz-Aussage; ein
// stehen gelassener, toter Stempel-Renderer waere genau der dritte Ableitungsweg in Wartestellung, den die
// O-8-Schritt-12-Lehre meint. Die Merge-DURCHFUEHRUNG bleibt vollstaendig erhalten
// (profile_facade/merge_plan.hpp -- Owner-Q2: die Merge-Strategie WIRD durchgefuehrt); sie lebt im Stempel
// ab jetzt ueber das 'e'-Experimentalflag und die erweiterten hierarchischen Namen.

} // namespace comdare::cache_engine::abi
