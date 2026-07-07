// test_ap2_katalog_pfad_stubfrei — AP-2-neu/#236 (W4, 2026-07-07): der KATALOG-PFAD ist der einzige
// PRT-ART-Mess-Pfad und materialisiert die ECHTE PrtArtComposition — STUB-FREI. Beweist LITERAL:
//   (1) build_sota_series_a_modules() traegt den prt_art-Eintrag mit FQ-Typ PrtArtComposition +
//       Header compositions/prt_art_reference.hpp (echte Composition, kein Surrogat-Typ);
//   (2) render_sota_module_source() emittiert dafuer eine Modul-Quelle mit COMDARE_DEFINE_ANATOMY_MODULE
//       und OHNE die Alt-Pfad-Marker (90ns-Stub `cycles_per_op`, `PrtArtHashBackend`,
//       `unordered_map`-Surrogat, V18-`prtart_body`-Template);
//   (3) die Composition-Header selbst (prt_art_reference.hpp + prt_art_merge_reference.hpp, als
//       DATEIEN gelesen) sind frei von den Surrogat-Markern — die Abstraktheit (W4) bleibt ERHALTEN,
//       KEINE 19-Achsen-Vervollstaendigung wird gefordert;
//   (4) Kompilier-Beweis: PrtArtComposition existiert compile-time (Include kompiliert in dieser TU).
// Die 3 Alt-Pfade (V18-Template / EE-Adapter-HashBackend / Registry-Factory) sind im prt-art-Repo
// mit QUARANTAENE-Notizen markiert (kein Mess-Pfad); dieser Test ist das ce-seitige Gegenstueck.

#include "sota_catalog.hpp"

#include <compositions/prt_art_reference.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>

namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cmp = comdare::cache_engine::compositions;

namespace {

[[nodiscard]] std::string read_file(std::string const& path) {
    std::ifstream in{path, std::ios::binary};
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Surrogat-/Alt-Pfad-Marker (W4): duerfen weder in der emittierten Modul-Quelle noch in den
// Composition-Headern des Katalog-Pfads vorkommen.
char const* const kAltPfadMarker[] = {"cycles_per_op", "PrtArtHashBackend", "unordered_map", "prtart_body"};

} // namespace

TEST(Ap2KatalogPfad, PrtArtEintragTraegtEchteComposition) {
    auto const mods  = tlz::build_sota_series_a_modules();
    bool       found = false;
    for (auto const& m : mods) {
        if (m.lebewesen != "prt_art") continue;
        found = true;
        EXPECT_NE(m.composition_type.find("PrtArtComposition"), std::string::npos) << m.composition_type;
        EXPECT_EQ(m.header, "compositions/prt_art_reference.hpp");
    }
    ASSERT_TRUE(found) << "prt_art fehlt in der Reihe-A-Katalog-Population";
}

TEST(Ap2KatalogPfad, EmittierteModulQuelleIstStubfrei) {
    auto const mods = tlz::build_sota_series_a_modules();
    for (auto const& m : mods) {
        if (m.lebewesen != "prt_art") continue;
        auto const src = tlz::render_sota_module_source(m.composition_type, m.header);
        EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE"), std::string::npos);
        EXPECT_NE(src.find("PrtArtComposition"), std::string::npos);
        for (auto const* marker : kAltPfadMarker)
            EXPECT_EQ(src.find(marker), std::string::npos) << "Alt-Pfad-Marker in Modul-Quelle: " << marker;
    }
}

TEST(Ap2KatalogPfad, CompositionHeaderSindSurrogatfrei) {
    for (char const* rel : {"compositions/prt_art_reference.hpp", "compositions/prt_art_merge_reference.hpp"}) {
        std::string const path    = std::string{COMDARE_CE_LIB_DIR} + "/" + rel;
        auto const        content = read_file(path);
        ASSERT_FALSE(content.empty()) << path;
        for (auto const* marker : kAltPfadMarker)
            EXPECT_EQ(content.find(marker), std::string::npos)
                << "Surrogat-Marker im Katalog-Pfad-Header " << rel << ": " << marker;
    }
}

TEST(Ap2KatalogPfad, EchteCompositionExistiertCompileTime) {
    // Der Include kompiliert in dieser TU — die ECHTE Composition ist compile-time praesent (W4:
    // Metaprogrammierungs-Ladung; Abstraktheit des Merge-Slots bleibt unberuehrt).
    static_assert(!std::string_view{cmp::PrtArtComposition::name}.empty());
    SUCCEED();
}
