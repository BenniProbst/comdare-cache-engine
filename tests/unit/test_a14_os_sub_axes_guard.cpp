// Paket A14 / OS-U1 -- Wachen-Test der DREI operating_system-Unter-Achsen (Owner-Entscheid E3, 02.08.2026).
//
// **Zweck:** Er beweist POSITIV, dass die FINALEN DREI Unter-Achsen os_version/kernel/build an der
// Haupt-Achse operating_system haengen (Eltern-Bezug ueber den TYP, nicht ueber einen String), dass die
// FINAL-DREI-Wache und die Namensfallen-Wachen wirklich scharf sind, und dass die Einhaengung
// STEMPEL-NEUTRAL bleibt (A-15: RT-Unter-Achsen stehen nie im Binary-Stempel).
//
// **Warum ein eigener Test und nicht nur die static_asserts im Header:** ein Header, den niemand
// inkludiert, wird nie uebersetzt -- seine Wachen sind dann Dekoration. Diese TU ist der Ort, an dem
// operating_system_sub_axes.hpp im Default-ctest-Sweep tatsaechlich durch den Compiler laeuft. Die
// Registry-Seite (XML-Emission) deckt separat das Byte-Gate test_system_axis_registry_roundtrip.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner Wachen-Test; keine Aenderung an kCompositionAxisNames,
// golden_fullpilot_320, POD-Layout oder Stempel-Zeilen.

#include <cache_engine/abi/anatomy_version_stamp.hpp>     // A-15-Gegenprobe: system_stamp_line()
#include <cache_engine/abi/system_axis_code_versions.hpp> // Bump-Verbots-Wache: kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/operating_system_sub_axes.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp> // Abgrenzung: gleiche Mechanik, andere Eltern

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <type_traits>

namespace cem  = ::comdare::cache_engine::measurement;
namespace cabi = ::comdare::cache_engine::abi;

// -- Block A (compile-time): Concept, Eltern-TYP und Tiefe ------------------------------------------
static_assert(cem::OperatingSystemSubAxisConcept<cem::OsVersionSubAxis>);
static_assert(cem::OperatingSystemSubAxisConcept<cem::KernelSubAxis>);
static_assert(cem::OperatingSystemSubAxisConcept<cem::BuildSubAxis>);
// Eine Unter-Achse IST eine Voll-Achse (Layer-Modell D4) -- die Gegenprobe gehoert hierher, weil sie
// sonst nur im Header behauptet waere.
static_assert(cem::CebSystemAxisConcept<cem::OsVersionSubAxis>);
static_assert(cem::CebSubAxisConcept<cem::KernelSubAxis>);
static_assert(std::same_as<cem::BuildSubAxis::parent_axis, cem::OperatingSystemAxisTag>);
static_assert(cem::axis_depth_v<cem::OsVersionSubAxis> == 1);
static_assert(cem::axis_depth_v<cem::OperatingSystemAxisTag> == 0);
// Der Tag ist zustandslos und vtable-frei (Anti-Runtime-Switch-Doktrin haelt auch am Repraesentanten).
static_assert(std::is_empty_v<cem::OperatingSystemAxisTag>);
static_assert(!std::is_polymorphic_v<cem::OperatingSystemAxisTag>);
static_assert(std::is_empty_v<cem::BuildSubAxis>);

// -- Block B (compile-time): ABGRENZUNG zu den target_isa-Unter-Achsen ------------------------------
// Dieselbe Mechanik (CebSubAxis + option_source), ANDERE Eltern-Achse. Wer beim Kopieren die Eltern
// vergisst, haengt eine OS-Unter-Achse an target_isa -- das faellt hier auf.
static_assert(cem::NumaNodeSubAxis::parent_axis_label() != cem::OsVersionSubAxis::parent_axis_label());
static_assert(!std::same_as<cem::PageSubAxis::parent_axis, cem::OperatingSystemAxisTag>);

namespace {

// -- Block C (Laufzeit): die FINALEN DREI, ihre Labels und ihre Options-Herkunft ---------------------
TEST(A14OsSubAxes, FinalDreiLabelsUndOptionsHerkunft) {
    ASSERT_EQ(cem::kOperatingSystemSubAxisLabels.size(), 3u)
        << "A-08/K-04 + Owner E-12: GENAU DREI Unter-Achsen; update_zustand ist in 'build' gemergt.";
    EXPECT_EQ(cem::kOperatingSystemSubAxisLabels[0], std::string_view{"os_version"});
    EXPECT_EQ(cem::kOperatingSystemSubAxisLabels[1], std::string_view{"kernel"});
    EXPECT_EQ(cem::kOperatingSystemSubAxisLabels[2], std::string_view{"build"});
    // option_source statt CT-Options-Katalog: die zulaessigen Werte haengen an der Maschine.
    for (auto const& label : cem::kOperatingSystemSubAxisLabels) EXPECT_FALSE(label.empty());
    EXPECT_EQ(cem::OsVersionSubAxis::option_source(), std::string_view{"machine_resolved"});
    EXPECT_EQ(cem::KernelSubAxis::option_source(), std::string_view{"machine_resolved"});
    EXPECT_EQ(cem::BuildSubAxis::option_source(), std::string_view{"machine_resolved"});
}

// -- Block D (Laufzeit): der Eltern-Bezug ist ABGELEITET, nicht wiederholt ---------------------------
TEST(A14OsSubAxes, ElternBezugKommtAusDemTypNichtAusEinemLiteral) {
    // Das Eltern-Label wird aus der Haupt-Achse gezogen -- ein Rename dort zoege hier automatisch mit.
    EXPECT_EQ(cem::OperatingSystemAxisTag::axis_label(), cem::LinuxOperatingSystem::axis_label());
    EXPECT_EQ(cem::OsVersionSubAxis::parent_axis_label(), cem::LinuxOperatingSystem::axis_label());
    EXPECT_EQ(cem::KernelSubAxis::parent_axis_label(), cem::LinuxOperatingSystem::axis_label());
    EXPECT_EQ(cem::BuildSubAxis::parent_axis_label(), cem::LinuxOperatingSystem::axis_label());
    // Die Haupt-Achse bleibt bei GENAU DREI Familien -- die Instanz wohnt in den Unter-Achsen (OP-10-Schnitt).
    EXPECT_EQ(cem::kAllOperatingSystemIds.size(), 3u);
}

// -- Block E (Laufzeit): NAMENSFALLEN ---------------------------------------------------------------
TEST(A14OsSubAxes, NamensfallenBuildUndOsSindAusgeschlossen) {
    // "build" ist der OS-Patch-/Update-Stand, NICHT der build_version-Suffix (Ledger 70.6), NICHT der
    // build_type ("+bt=") und NICHT die build_target_complex-Klammer.
    EXPECT_NE(cem::BuildSubAxis::axis_label(), std::string_view{"build_version"});
    EXPECT_NE(cem::BuildSubAxis::axis_label(), std::string_view{"build_type"});
    EXPECT_NE(cem::BuildSubAxis::axis_label(), std::string_view{"build_target_complex"});
    // "os_version" ist die INSTANZ, nicht die Familie.
    EXPECT_NE(cem::OsVersionSubAxis::axis_label(), cem::LinuxOperatingSystem::axis_label());
    EXPECT_NE(cem::OsVersionSubAxis::axis_label(), std::string_view{"os"});
    // Der Verbots-Katalog ist wirklich gefuellt und enthaelt die gemergte vierte Achse (E-12).
    bool update_zustand_verboten = false;
    bool update_status_verboten  = false;
    for (auto const& forbidden : cem::detail::kOperatingSystemSubAxisForbiddenLabels) {
        if (forbidden == std::string_view{"update_zustand"}) update_zustand_verboten = true;
        if (forbidden == std::string_view{"update_status"}) update_status_verboten = true;
    }
    EXPECT_TRUE(update_zustand_verboten) << "Owner E-12: 'Wir mergen den Update Zustand in Build'.";
    EXPECT_TRUE(update_status_verboten);
    // Und kein gueltiges Label steht im Verbots-Katalog (die consteval-Wache noch einmal zur Laufzeit).
    for (auto const& label : cem::kOperatingSystemSubAxisLabels)
        for (auto const& forbidden : cem::detail::kOperatingSystemSubAxisForbiddenLabels) EXPECT_NE(label, forbidden);
}

// -- Block F (Laufzeit): STEMPEL-NEUTRALITAET (A-15 + Bump-Verbots-Wache) ------------------------------------
TEST(A14OsSubAxes, UnterAchsenStehenNichtImSystemStempel) {
    std::string const line = cabi::system_stamp_line();
    // A-15: RT-Unter-Achsen stehen NIE im Binary-Stempel. Taucht hier ein Unter-Achsen-Label auf, ist
    // der SHA512 aller Neubauten gegen den Bestand verschoben -- exakt die Owner-E3-verbotene Folge.
    for (auto const& label : cem::kOperatingSystemSubAxisLabels)
        EXPECT_EQ(line.find(std::string{label}), std::string::npos)
            << "A-15-Bruch: '" << label << "' steht in der System-Stempel-Zeile: " << line;
    // Die Haupt-Achse selbst steht dort weiterhin -- die Zeile ist nicht etwa leer geworden.
    EXPECT_NE(line.find(std::string{cem::LinuxOperatingSystem::axis_label()}), std::string::npos) << line;
    // Bump-Verbots-Gegenprobe: die Tabelle traegt weiterhin GENAU DREI System-Haupt-Achsen; die Unter-Achsen
    // haben KEINEN Eintrag bekommen (ein Eintrag hier waere ein Stempel-Byte-Ereignis).
    EXPECT_EQ(cabi::kSystemAxisCodeCount, 3u);
    for (auto const& entry : cabi::kSystemAxisCodeVersions)
        for (auto const& label : cem::kOperatingSystemSubAxisLabels) EXPECT_NE(entry.axis, label);
}

} // namespace
