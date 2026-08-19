#pragma once
// V41.F.6.1 R5.G — adhoc_emitter: schreibt pro Permutation eines PermutationEngine ein KOMPILIERBARES
// Modul-.cpp (Umbrella-Include + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC mit den 18 FQ-Achsen-Typen).
//
// Das ist die wiederverwendbare Kern-Funktion des R5.G-Auto-Emitters: aus einer Engine über den
// gemergten Permutations-Raum (for_each_composition_type) werden alle Modul-.cpp generiert, die dann
// per CMake als Permutations-DLLs gebaut werden. Bausteine (type_name + ADHOC-Makro + Umbrella) sind
// einzeln verifiziert; diese Datei kombiniert sie zur File-Emission.

#include "type_name.hpp"

// A-11/golden-102: die ECHTEN Stempel-Zeilen-Helfer fuer emit_adhoc_modules (organ_stamp_line<C> /
// system_stamp_line). builder/ -> abi/ ist die erlaubte Include-Richtung (Praezedenz: der Loader).
#include <cache_engine/abi/anatomy_version_stamp.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::codegen {

/// strip_all_elaborated (2026-06-03, L-LAZY-E2E) — entfernt ALLE (auch INNEN-liegenden) MSVC-Elaborated-Type-
/// Keywords aus einem FQ-Typ-Namen. type_name<T>() schält nur den ÄUSSEREN "class "/"struct " (type_name.hpp);
/// MSVC rendert in __FUNCSIG__ aber AUCH jedes verschachtelte Template-Argument so, z.B.
/// `ObservableNodeType<class NS::Node4NodeType>`. Würde das innere `class ` in den emittierten C++-Quelltext
/// geschrieben, ist es ein Syntaxfehler (C2760 „class hier nicht erwartet", Befund 2026-06-03 bei den
/// Observable*<…>-Wrappern node/layout/serialization/telemetry). Da der Emitter den Namen als ECHTEN Quelltext
/// schreibt, ist hier (host-seitig, std::string) der richtige Ort, die Tokens überall zu entfernen.
/// SICHER: "class "/"struct "/"enum "/"union " (mit folgendem Trenner) sind in __FUNCSIG__ ausschließlich
/// Elaborated-Specifier — ein Identifier kann kein Leerzeichen enthalten → keine Falsch-Treffer.
[[nodiscard]] inline std::string strip_all_elaborated(std::string_view t) {
    static constexpr std::string_view kws[] = {"class ", "struct ", "enum ", "union "};
    std::string                       out;
    out.reserve(t.size());
    for (std::size_t i = 0; i < t.size();) {
        bool matched = false;
        for (std::string_view kw : kws)
            if (i + kw.size() <= t.size() && t.substr(i, kw.size()) == kw) {
                i += kw.size();
                matched = true;
                break;
            }
        if (!matched) out += t[i++];
    }
    return out;
}

/// adhoc_macro_args<C>() -- die 18 FQ-Achsen-Typ-Namen einer Composition C als Komma-getrennter Block
/// (Argument fuer COMDARE_DEFINE_ANATOMY_MODULE_ADHOC). Reihenfolge = AdHocComposition<T0..T17>
/// (15 Such-Achsen + queuing q1/q2 + persistence_target; Bau-INC-2c: telemetry / Bau-INC-2d: isa sind
/// System-Achsen, kein Slot mehr). Die Zahl stand hier auf 17, waehrend der Rumpf seit STRUKT-R ORG-18
/// achtzehn Typen anhaengt -- dieselbe Nachzug-Luecke, die A8.2 in der Stempel-Zeile geschlossen hat.
/// Jeder Name wird via strip_all_elaborated von INNEN-liegenden
/// "class "/"struct "-Keywords befreit (sonst Syntaxfehler bei Wrapper<class Inner>-Slots).
template <class C>
[[nodiscard]] std::string adhoc_macro_args() {
    std::string s;
    auto        add = [&](std::string_view t) {
        if (!s.empty()) s += ",\n    ";
        s += strip_all_elaborated(t);
    };
    add(type_name<typename C::search_algo>());
    add(type_name<typename C::cache_traversal>());
    add(type_name<typename C::mapping>());
    add(type_name<typename C::path_compression>());
    add(type_name<typename C::node_type>());
    add(type_name<typename C::memory_layout>());
    add(type_name<typename C::allocator>());
    add(type_name<typename C::prefetch>());
    add(type_name<typename C::concurrency>());
    add(type_name<typename C::serialization>());
    add(type_name<typename C::value_handle>());
    // Bau-INC-2d: C::isa entfaellt (Target-ISA-System-Achse, build-config-Codepfad, kein Kompositions-Slot).
    add(type_name<typename C::index_organization>());
    add(type_name<typename C::io_dispatch>());
    add(type_name<typename C::migration_policy>());
    add(type_name<typename C::filter>());
    add(type_name<typename C::queuing_q1>());         // Doc 30 §8.0: queuing q1 (buffer_strategy)
    add(type_name<typename C::queuing_q2>());         // Doc 30 §8.0: queuing q2 (flush_policy)
    add(type_name<typename C::persistence_target>()); // STRUKT-R ORG-18: T17 persistence_target
    return s;
}

/// render_adhoc_module_source — der kompilierbare Modul-.cpp-Quelltext einer Permutation aus dem bereits
/// zusammengesetzten FQ-Achsen-Typ-Argument-Block. Reine Laufzeit-Funktion (kein Template) → wird sowohl
/// vom Template-Pfad emit_adhoc_modules<Engine> ALS AUCH von der F.4-Tools-Facade (ICodegenTool) genutzt,
/// damit das Emitter-Textformat an EINER Stelle definiert ist (DRY).
[[nodiscard]] inline std::string render_adhoc_module_source(int idx, std::string_view macro_args,
                                                            std::string_view organ_stamp       = {},
                                                            std::string_view system_stamp      = {},
                                                            std::string_view measurement_stamp = {}) {
    std::string src = "// AUTO-GENERATED by R5.G adhoc_emitter — Permutation " + std::to_string(idx) +
                      " — DO NOT EDIT\n"
                      "#include <builder/codegen/all_axes_umbrella.hpp>\n\n"
                      "COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(\n    " +
                      std::string{macro_args} + ")\n";
    // W12-A2 (Section 43): bei nicht-leerem organ_stamp die OPTIONALE Versionierungs-Stempel-Zeile anhaengen.
    // Beide Emitter-Pfade (lazy + Katalog) reichen denselben compose_organ_stamp_line + system_stamp_line durch
    // -> byte-identisch -> die Round-Trip-Byte-Wache bleibt STRIKT. Stempel-Strings sind C-literal-sicher
    // (nur =@;.+_ [] und alnum, keine Quotes/Backslashes) -> kein Escaping. A13-M2 (Owner-Q1): '[' und ']'
    // sind mit dem Meta-Meta-Klammer-Anhang dazugekommen und ebenfalls escaping-frei.
    if (!organ_stamp.empty()) {
        // S6-P1b (Section 43/47): APPEND-ONLY measurement_stamp = die Mess-Tooling-HAUPT-Stempel-Zeile
        // (kMeasurementAxisVersionLine, anatomy_version_stamp.hpp::measurement_stamp_line). LEER (Default:
        // [all]/leere Mess-Tooling-Combo) -> EXAKT die bisherige 2-arg-Makro-Zeile -> der emittierte Quelltext
        // bleibt byte-identisch (golden-CRC 0xF1C1F26A1232073B unberuehrt; Katalog/binary_id haengen NICHT am
        // Tooling, der Stempel != binary_id). Nur bei expliziter Tooling-Wahl fuellt sich der Slot -> DLL-Bytes
        // je Combo verschieden, binary_id/CRC UNBERUEHRT. NAHT-REALITAET (S6-P1b, ehrlich dokumentiert): die
        // gewaehlte Combo ist im gefilterten Planer-Walk (select_measurement_combo, --measurement-combo je
        // ceb:emit ab count>1) bekannt; der CLI-Konsument, der den Slug->Tooling in DIESEN Emitter durchreicht,
        // ist die CLI-Haertung (POST-v3 #34).
        // A13-M3 (Owner-E2): der frueher hier stehende merge_stamp-Parameter und sein 4-arg-_MERGE-Zweig sind
        // ERSATZLOS ENTFERNT -- die Merge-ZEILE existiert nicht mehr. Es bleiben genau ZWEI Emissions-Formen.
        //
        // [HISTORIK, NACHZUG R-3 07.08.2026] Der Satz oben "Default: [all]/leere Mess-Tooling-Combo -> EXAKT
        // die bisherige 2-arg-Makro-Zeile" beschreibt den Stand VOR der Mess-Naht und wird hier nicht
        // geloescht, weil er die Byte-Bilanz jener Scheibe begruendet. LEBENDE WAHRHEIT auf dem PERM-Pfad:
        // resolve_live_measurement_combo_legend() liefert im UNGESETZT-Fall ausdruecklich "[all]", und
        // measurement_stamp_line_from_combo_legend("[all]") ist die VOLLMENGEN-Zeile -- also NICHT leer.
        // Der Perm-Pfad emittiert damit die 3-arg-Form; die 2-arg-Form bleibt der ce-only-/Katalog-Pfad
        // (make_lazy_adhoc_source_gen ohne Argument), an dem die 320er-Byte-Identitaets-Wachen haengen.
        // FUER R-3 FOLGENLOS: das neunte Preimage-Glied materialisiert INNEN in der Makro-Expansion (wie kFP
        // selbst), nicht im emittierten Quelltext -- beide Emissions-Formen bleiben byte-identisch.
        // S-6a (18.08.2026): die EMITTIERTE Argument-Folge ist auf MESS, SYSTEM, ORGAN gedreht -- beide
        // Formen. Die Makros heissen unveraendert, ihre Parameter-Namen NICHT (anatomy_module_abi_v1.hpp).
        // DAS IST DER DEKLARIERTE GOLDEN-BRUCH dieser Scheibe: der emittierte Quelltext aendert sich, also
        // bewegen sich die 320er-Byte-Identitaets-Wachen und der golden-CRC-Anker 0xF1C1F26A1232073B mit.
        // Sie werden im golden-Regen-Schnitt neu gesetzt, NICHT hier -- ein Regen in derselben Scheibe
        // wuerde den Beweis, dass sich genau diese Bytes bewegt haben, mit dem Beweismittel loeschen.
        if (measurement_stamp.empty()) {
            // Default (kein Mess-Tooling) -> die 2-arg-Kurzform, ab S-6a (system, organ).
            src += "COMDARE_ANATOMY_VERSION_STAMP(\"";
            src += system_stamp;
            src += "\", \"";
            src += organ_stamp;
            src += "\")\n";
        } else {
            // Mess-Tooling gewaehlt -> die 3-arg _M-Vollform, ab S-6a (measurement, system, organ).
            src += "COMDARE_ANATOMY_VERSION_STAMP_M(\"";
            src += measurement_stamp;
            src += "\", \"";
            src += system_stamp;
            src += "\", \"";
            src += organ_stamp;
            src += "\")\n";
        }
    }
    return src;
}

/// emit_adhoc_modules<Engine>(out_dir) — schreibt für JEDE Permutation des Engine ein Modul-.cpp nach
/// out_dir (Dateiname comdare_anatomy_perm_auto_<idx>.cpp) und liefert die geschriebenen Pfade.
/// Jedes .cpp ist standalone kompilierbar (nur Umbrella-Include + ADHOC-Makro).
///
/// A-11/golden-102 (19.08.2026) -- STEMPEL-SPEISUNG DES WERKZEUGS: seit der Stempel-Pflicht weist der
/// Loader stempellose Module mit status 13 ab (Emission ohne Stempel faellt). Fuer REALE Kompositionen
/// (Slots mit name()/algo_version -- jede Vendor-Achse traegt beides) emittiert dieses Werkzeug deshalb
/// die ECHTEN Zeilen (abi::organ_stamp_line<C>() + abi::system_stamp_line(), dieselben Helfer wie die
/// produktiven Pfade). NUR Diagnose-/Fake-Kompositionen ohne diese API (test_d3-FakeComp, int-Slots)
/// bleiben Emissions-only -- ihre Module sind seit A-11 NICHT ladbar, und genau das ist die Aussage.
template <class Engine>
[[nodiscard]] std::vector<std::filesystem::path> emit_adhoc_modules(std::filesystem::path const& out_dir) {
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    std::vector<std::filesystem::path> files;
    int                                idx = 0;
    Engine::for_each_composition_type([&]<class C>() {
        std::filesystem::path const f = out_dir / ("comdare_anatomy_perm_auto_" + std::to_string(idx) + ".cpp");
        std::ofstream               out(f, std::ios::trunc);
        if constexpr (requires {
                          { C::search_algo::name() };
                          { C::search_algo::algo_version };
                      }) {
            std::string const organ  = ::comdare::cache_engine::abi::organ_stamp_line<C>();
            std::string const system = ::comdare::cache_engine::abi::system_stamp_line();
            out << render_adhoc_module_source(idx, adhoc_macro_args<C>(), organ, system);
        } else {
            out << render_adhoc_module_source(idx, adhoc_macro_args<C>());
        }
        files.push_back(f);
        ++idx;
    });
    return files;
}

} // namespace comdare::cache_engine::builder::codegen
