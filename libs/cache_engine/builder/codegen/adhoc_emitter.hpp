#pragma once
// V41.F.6.1 R5.G — adhoc_emitter: schreibt pro Permutation eines PermutationEngine ein KOMPILIERBARES
// Modul-.cpp (Umbrella-Include + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC mit den 18 FQ-Achsen-Typen).
//
// Das ist die wiederverwendbare Kern-Funktion des R5.G-Auto-Emitters: aus einer Engine über den
// gemergten Permutations-Raum (for_each_composition_type) werden alle Modul-.cpp generiert, die dann
// per CMake als Permutations-DLLs gebaut werden. Bausteine (type_name + ADHOC-Makro + Umbrella) sind
// einzeln verifiziert; diese Datei kombiniert sie zur File-Emission.

#include "type_name.hpp"

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
        if (measurement_stamp.empty()) {
            // Default (kein Mess-Tooling) -> die bisherige 2-arg-Form (byte-identisch).
            src += "COMDARE_ANATOMY_VERSION_STAMP(\"";
            src += organ_stamp;
            src += "\", \"";
            src += system_stamp;
            src += "\")\n";
        } else {
            // Mess-Tooling gewaehlt -> die 3-arg _M-Vollform.
            src += "COMDARE_ANATOMY_VERSION_STAMP_M(\"";
            src += organ_stamp;
            src += "\", \"";
            src += system_stamp;
            src += "\", \"";
            src += measurement_stamp;
            src += "\")\n";
        }
    }
    return src;
}

/// emit_adhoc_modules<Engine>(out_dir) — schreibt für JEDE Permutation des Engine ein Modul-.cpp nach
/// out_dir (Dateiname comdare_anatomy_perm_auto_<idx>.cpp) und liefert die geschriebenen Pfade.
/// Jedes .cpp ist standalone kompilierbar (nur Umbrella-Include + ADHOC-Makro).
template <class Engine>
[[nodiscard]] std::vector<std::filesystem::path> emit_adhoc_modules(std::filesystem::path const& out_dir) {
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    std::vector<std::filesystem::path> files;
    int                                idx = 0;
    Engine::for_each_composition_type([&]<class C>() {
        std::filesystem::path const f = out_dir / ("comdare_anatomy_perm_auto_" + std::to_string(idx) + ".cpp");
        std::ofstream               out(f, std::ios::trunc);
        out << render_adhoc_module_source(idx, adhoc_macro_args<C>());
        files.push_back(f);
        ++idx;
    });
    return files;
}

} // namespace comdare::cache_engine::builder::codegen
