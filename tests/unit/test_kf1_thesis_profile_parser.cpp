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
    // BEWUSSTE Pin-Fortschreibung 8 -> 9 (Kanonisierung, GO-4/GO-5-Nebenbefund 2026-07-12): die Alt-isa-Achse
    // {X86_SSE42/AVX2/AVX512} zerfaellt kanonisch in isa{isa_amd64} + simd_extension{simd_ext_sse2/avx2/avx512}.
    check_eq("permute_axes", tp->permute_axes.size(), std::size_t{9});

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

    // isa-Achse: kanonisch auf die CPU-Familie gepinnt (isa_amd64); der Alt-SIMD-Stufen-Spread (3 values)
    // lebt kanonisch in der simd_extension-Achse (Kanonisierung 2026-07-12, s.o.).
    cx::ThesisAxisSpec const* isa = nullptr;
    for (auto const& a : tp->permute_axes)
        if (a.ref == "isa") isa = &a;
    check_true("isa-Achse vorhanden", isa != nullptr);
    if (isa) check_eq("isa.values (Familie gepinnt)", isa->values.size(), std::size_t{1});
    cx::ThesisAxisSpec const* simd = nullptr;
    for (auto const& a : tp->permute_axes)
        if (a.ref == "simd_extension") simd = &a;
    check_true("simd_extension-Achse vorhanden", simd != nullptr);
    if (simd) check_eq("simd_extension.values (Stufen-Spread)", simd->values.size(), std::size_t{3});

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
