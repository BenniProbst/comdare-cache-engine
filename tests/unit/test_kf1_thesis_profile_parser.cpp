// test_kf1_thesis_profile_parser — KF-1 (2026-06-02)
// Standalone-Test (kein gtest) fuer XmlConfigParser::parse_thesis_profile + xml_reader-DOM.
// Liest das echte cacheline_study.profile.xml und prueft die geparste Struktur.
//
// Build (standalone):
//   g++ -std=c++23 test_kf1_thesis_profile_parser.cpp \
//       ../../libs/common/serialization/xml_config_parser/xml_config_parser.cpp \
//       -I../../libs/common/serialization/xml_config_parser -o test_kf1.exe

#include "xml_config_parser.hpp"
#include "xml_reader.hpp"

#include <iostream>
#include <string>

namespace cx = comdare::builder::xml;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

int main(int argc, char** argv) {
    // Pfad zum Profil: arg[1] oder Default relativ zur Repo-Struktur.
    std::string profile =
        (argc > 1) ? argv[1] : "../../libs/cache_engine/algorithm_profiles/thesis_profiles/cacheline_study.profile.xml";

    // 0) DOM-Reader-Smoke (xml_reader.hpp)
    {
        auto doc = comdare::common::xml::parse_document(
            R"(<?xml version="1.0"?><!-- c --><root a="1"><child>x y</child><rep>p</rep><rep>q</rep></root>)");
        check_true("xml_reader: parse_document liefert root", doc.has_value());
        if (doc) {
            check_eq("xml_reader: root.tag", doc->tag, std::string{"root"});
            check_eq("xml_reader: root@a", doc->attr("a"), std::string{"1"});
            check_eq("xml_reader: child.text_tokens", doc->child("child")->text_tokens().size(), std::size_t{2});
            check_eq("xml_reader: 2x <rep>", doc->children_named("rep").size(), std::size_t{2});
        }
    }

    // 0b) F26.1 (WP-3, 2026-07-16): UTF-8-BOM am Dokument-Anfang wird uebersprungen — vorher stilles
    //     nullopt fuer ein wohlgeformtes, BOM-behaftetes Dokument (Windows-Editor/PowerShell-Faelle).
    {
        std::string const bom_doc = std::string{"\xEF\xBB\xBF"} + R"(<?xml version="1.0"?><root a="1"/>)";
        auto              doc     = comdare::common::xml::parse_document(bom_doc);
        check_true("xml_reader F26.1: BOM-Dokument parst (kein stilles nullopt)", doc.has_value());
        if (doc) {
            check_eq("xml_reader F26.1: BOM root.tag", doc->tag, std::string{"root"});
            check_eq("xml_reader F26.1: BOM root@a", doc->attr("a"), std::string{"1"});
        }
        // BOM mitten im Dokument bleibt Inhalt (nur der Dokument-ANFANG wird geschaelt).
        auto doc2 = comdare::common::xml::parse_document("<root>\xEF\xBB\xBFtext</root>");
        check_true("xml_reader F26.1: BOM im Text bleibt Text", doc2.has_value() && !doc2->text.empty());
    }

    // 0c) F26.2 (WP-3, 2026-07-16): Close-Tag-Name muss dem offenen Tag entsprechen — ein falscher/zu
    //     frueher Close-Tag ist jetzt ein Parse-Fehler (vorher: stilles Re-Parenting, teil-leeres DOM).
    {
        // Falscher Close-Name auf dem Wurzel-Element.
        auto bad1 = comdare::common::xml::parse_document("<root a='1'><child>x</child></wurzel>");
        check_true("xml_reader F26.2: falscher Root-Close => nullopt", !bad1.has_value());
        // Zu frueher Close-Tag eines Eltern-Elements im Kind (die INC-D-Fehlerklasse </phase> vs </phases>).
        auto bad2 = comdare::common::xml::parse_document("<phases><phase name='p'>x</phases></phases>");
        check_true("xml_reader F26.2: zu frueher Eltern-Close im Kind => nullopt", !bad2.has_value());
        // Gegenprobe: korrekt geschachtelt parst weiterhin.
        auto good = comdare::common::xml::parse_document("<phases><phase name='p'>x</phase></phases>");
        check_true("xml_reader F26.2: korrekt geschachtelt parst", good.has_value());
        check_eq("xml_reader F26.2: 1 Kind", good ? good->children.size() : std::size_t{0}, std::size_t{1});
    }

    // 1) Thesis-Profil
    cx::XmlConfigParser parser;
    auto                tp = parser.parse_thesis_profile(profile);
    check_true("parse_thesis_profile liefert Profil", tp.has_value());
    if (!tp) {
        std::cout << "FATAL: Profil nicht geladen (" << profile << ")\n";
        return 1;
    }

    check_eq("id", tp->id, std::string{"cacheline_study"});
    check_eq("schema_version", tp->schema_version, 1);
    check_eq("base_tiers", tp->base_tiers.size(), std::size_t{8});
    check_eq("base_tiers[0].id", tp->base_tiers.front().id, std::string{"art"});
    check_true("base_tiers[0].profile_ref enthaelt art.profile.xml",
               tp->base_tiers.front().profile_ref.find("art.profile.xml") != std::string::npos);
    // BEWUSSTE Pin-Fortschreibung 9 -> 6 (A1-Fehlplatzierungs-Fix, S3-Resolver-Aktivierung 2026-07-20): isa +
    // simd_extension waren SYSTEM-Achsen faelschlich in der permute_axes-Organ-Position (der aktive S3-Resolver
    // klassifizierte sie als V-UNREG-AXIS) -> kanonisch in den <system_axes>-Block relokiert (isa->target_isa,
    // simd_extension->extension_hardware/simd). page_type (Baum-Knoten-PageKind = axis_01_page_type Build-Variante,
    // §52-B10) verliess die Organ-Position ganz (ZUKUNFT, kein Parser-Kanal). Rest = 6 echte Organ-Achsen
    // (memory_layout cache_traversal node_type prefetch allocator + der cacheline-Sonderzweig).
    check_eq("permute_axes", tp->permute_axes.size(), std::size_t{6});

    // cacheline-Unterachse finden
    cx::ThesisAxisSpec const* cl = nullptr;
    for (auto const& a : tp->permute_axes)
        if (a.ref == "cacheline") cl = &a;
    check_true("cacheline-Achse vorhanden", cl != nullptr);
    if (cl) {
        check_eq("cacheline.per_organ", cl->per_organ.size(), std::size_t{4}); // kanonische 4 Organe
        // BEWUSSTE Pin-Fortschreibung 3 -> 4 (C1, GO4/#8 F-C, 2026-07-12): line_size 32 additiv im Profil.
        check_eq("cacheline.line_sizes", cl->line_sizes.size(), std::size_t{4}); // 64 128 256 32
        check_eq("cacheline.alignments", cl->alignments.size(), std::size_t{3});
        check_eq("cacheline.sw_prefetch_hints", cl->sw_prefetch_hints.size(), std::size_t{5});
    }

    // A1-Fehlplatzierungs-Fix (2026-07-20): isa + simd_extension sind KEINE Organ-Kompositions-Achsen (nicht unter
    // den 17 kCompositionAxisNames) -> sie stehen NICHT mehr in permute_axes, sondern kanonisch im <system_axes>-
    // Block. Gegenprobe: in permute_axes NICHT auffindbar; im system_axes-POD die relokierten Werte.
    bool isa_in_permute = false, simd_in_permute = false;
    for (auto const& a : tp->permute_axes) {
        if (a.ref == "isa") isa_in_permute = true;
        if (a.ref == "simd_extension") simd_in_permute = true;
    }
    check_true("isa NICHT mehr in permute_axes (jetzt target_isa-System-Achse, A1)", !isa_in_permute);
    check_true("simd_extension NICHT mehr in permute_axes (jetzt extension_hardware-System-Achse, A1)",
               !simd_in_permute);
    // isa->target_isa {x86_64}: der Alt-isa-Wert isa_amd64 kanonisiert auf den x86_64-Baustein.
    check_eq("system_axes.target_isa (Alt-Ref isa)", tp->target_isa.isa.size(), std::size_t{1});
    // simd_extension->extension_hardware/simd {no_extension,avx2,avx512}: der Alt-Stufen-Spread {SSE42/AVX2/AVX512}
    // (SSE4.2 -> no_extension, kein Registry-Aequivalent).
    check_eq("system_axes.simd (Alt-Ref simd_extension, Stufen-Spread)", tp->extension_hardware.simd_options.size(),
             std::size_t{3});

    check_eq("workloads", tp->workloads.size(), std::size_t{6}); // A..F
    check_eq("telemetry_mode", tp->telemetry_mode, std::string{"on"});
    check_true("telemetry_silent", tp->telemetry_silent);
    check_eq("thread_counts", tp->thread_counts.size(), std::size_t{1}); // 1 (label-only gepinnt, T8-Phantom-Fix)
    check_eq("hw_prefetcher", tp->hw_prefetcher.size(), std::size_t{3});
    check_eq("repetitions", tp->repetitions, 3);
    check_true("repetitions nicht interpoliert", !tp->repetitions_interpolate);
    check_eq("fixed_conditions[turbo]",
             tp->fixed_conditions.count("turbo") ? tp->fixed_conditions.at("turbo") : std::string{"?"},
             std::string{"off"});
    check_eq("modes", tp->modes.size(), std::size_t{3});

    // Modus pruefling_replace: replaces_axes nicht leer
    cx::ThesisMode const* pr = nullptr;
    for (auto const& m : tp->modes)
        if (m.name == "pruefling_replace") pr = &m;
    check_true("Modus pruefling_replace vorhanden", pr != nullptr);
    if (pr) {
        check_eq("pruefling_replace.pruefling", pr->pruefling, std::string{"prtart"});
        check_true("pruefling_replace.replaces_axes nicht leer", !pr->replaces_axes.empty());
        // Pin 2 -> 3 (Kanonisierung 2026-07-12): isa simd_extension cacheline.
        check_eq("pruefling_replace.active_axes", pr->active_axes.size(), std::size_t{3});
    }

    check_eq("static_axes_from", tp->static_axes_from, std::string{"base_tier"});

    std::cout << "\n==== KF-1 Parser-Test: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
