// test_axis_error_taxonomy — Fehlerklassifizierungs-Framework INC-29.0 (Task #29).
//
// Beweist POSITIV die Taxonomie-Kontrakte von measurement/axis_error.hpp: die zwei disjunkten Enums
// (D1 CompilerCompilerErrorClass, D2 SampleStatus), ihre Count-Single-Source + Drift-Guards, und die
// CSV-Token-Vokabeln (OD-1: Ok->Zahl, N-A->"n/a", Failed->"failed"). Reiner compile-time-Test
// (static_assert) + ein Runtime-Smoke fuer die constexpr-Etikett-Funktionen. ADDITIV, golden-neutral.

#include <cache_engine/measurement/axis_error.hpp>

#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>

namespace cem = ::comdare::cache_engine::measurement;

// ── compile-time: POD-/Trennungs-/Drift-Garantien (spiegeln die Header-static_asserts, hier als Test-Gate) ──
static_assert(std::is_same_v<std::underlying_type_t<cem::CompilerCompilerErrorClass>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<cem::SampleStatus>, std::uint8_t>);
// E-24 C5/FK-8: 5 -> 7 (GattungsBindungFehlt=5, GattungsSlotAritaet=6). RF-3 (Paragraf 70.3) hatte 4 -> 5.
static_assert(cem::kCompilerCompilerErrorClassCount == 7);
static_assert(cem::kSampleStatusCount == 4); // D2 unberuehrt (disjunkte Taxonomie)
static_assert(static_cast<std::uint8_t>(cem::SampleStatus::Ok) == 0);
static_assert(cem::SampleStatus::Failed != cem::SampleStatus::NotApplicable);
// Drift-Guard: ein neuer Enum-Wert ohne Count-Bump bricht HIER (nicht still).
static_assert(cem::kSampleStatusCount == static_cast<std::size_t>(cem::SampleStatus::Failed) + 1);
static_assert(cem::kCompilerCompilerErrorClassCount ==
              static_cast<std::size_t>(cem::CompilerCompilerErrorClass::GattungsSlotAritaet) + 1);
// D2/OD-1: die entscheidenden Zell-Vokabeln sind zementiert (compile-time).
static_assert(cem::sample_status_token(cem::SampleStatus::Failed) == std::string_view{"failed"});
static_assert(cem::sample_status_token(cem::SampleStatus::NotApplicable) == std::string_view{"n/a"});
static_assert(cem::sample_status_token(cem::SampleStatus::SourceUnavailable) == std::string_view{"n/a"});

TEST(AxisErrorTaxonomy, D2_TokenTrennungIstEindeutig) {
    // "Messung nie als Nullen": Failed traegt "failed", NIE "0"/leer; N-A traegt ehrlich "n/a".
    EXPECT_EQ(cem::sample_status_token(cem::SampleStatus::Failed), std::string_view{"failed"});
    EXPECT_EQ(cem::sample_status_token(cem::SampleStatus::NotApplicable), std::string_view{"n/a"});
    EXPECT_NE(cem::sample_status_token(cem::SampleStatus::Failed),
              cem::sample_status_token(cem::SampleStatus::NotApplicable));
    // Out-of-range-Cast -> sicherer Default "failed" (nie stille Null).
    EXPECT_EQ(cem::sample_status_token(static_cast<cem::SampleStatus>(99)), std::string_view{"failed"});
}

TEST(AxisErrorTaxonomy, D1_KlassenEtikettenSindStabilUndVollstaendig) {
    EXPECT_EQ(cem::error_class_label(cem::CompilerCompilerErrorClass::HardwareErweiterungFehlt),
              std::string_view{"hardware_erweiterung_fehlt"});
    EXPECT_EQ(cem::error_class_label(cem::CompilerCompilerErrorClass::KonfigXmlParse),
              std::string_view{"konfig_xml_parse"});
    // RF-3 (§70.3): das Etikett der fuenften Klasse ist ebenso zementiert wie die vier Bestands-Etiketten.
    EXPECT_EQ(cem::error_class_label(cem::CompilerCompilerErrorClass::BetriebssystemFeatureFehlt),
              std::string_view{"betriebssystem_feature_fehlt"});
    // jede der 5 Klassen liefert ein nicht-leeres Etikett != "unbekannt" (die Schleife zaehlt ueber die
    // Count-Single-Source, faengt die neue Klasse also automatisch mit). ANMERKUNG RF-3: `seen` wird
    // hier nur befuellt, PAARWEISE Distinktheit prueft diese Schleife NICHT -- der Beweis dafuer steht
    // im RF-3-Guard tests/unit/test_rf3_betriebssystem_feature_fehlt.cpp.
    std::string_view seen[cem::kCompilerCompilerErrorClassCount];
    for (std::size_t i = 0; i < cem::kCompilerCompilerErrorClassCount; ++i) {
        seen[i] = cem::error_class_label(static_cast<cem::CompilerCompilerErrorClass>(i));
        EXPECT_FALSE(seen[i].empty());
        EXPECT_NE(seen[i], std::string_view{"unbekannt"});
    }
}
