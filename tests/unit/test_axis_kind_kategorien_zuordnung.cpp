// V-01R-Kategorien-Zuordnungs-Gate (A2.5-Fix-Strecke 2 / G1, #15-Bruch, KON101-02).
//
// GEGENSTAND: der AxisKind-Dreh in die Kategorien-Ordnung MESS, SYSTEM, ORGAN (Owner 17.08.,
// KON101-01: "definitiv mit drehen, sonst ergibt es keinen Sinn") und die NEUE CT-Zuordnung
// Unter-Achse -> Haupt-Achse -> Kategorie (axis_category / axis_category_of, topics/axis.hpp).
//
// WARUM ein EIGENER Test neben test_striktheit_axis_dach_guard: der Dach-Guard beweist die
// FAMILIEN-Trennung (wer traegt welchen Diskriminator); dieses Gate beweist die KATEGORIEN-Ebene
// (welcher Diskriminator gehoert zu welchem Realm) UND die schichtuebergreifende NP-01-Auflage
// (KON106-02): die Zuordnung KONSULTIERT die Ordnungs-Single-Source abi::kSystemAxisOrder und
// ersetzt sie nie. Diese Konsultations-Wache kann nicht im Dach leben (topics/ kennt abi/ nicht,
// Layer-Schnitt) -- sie lebt hier, an der einzigen TU, die beide Schichten sieht.
//
// T-1-BELEG (Fix-Strecke 2, 19.08.2026): die Ordnungs-Wache in topics/axis.hpp biss VOR dem Dreh
// literal rot (g1_rot_ordnungswache_vor_dreh.log); der K13-Koeder "system_measurement ist eine
// SYSTEM-Achse" biss am Pin (g1_rot_kategorie_koeder_k13.log); der Naht-Koeder "Ycsb ist SYSTEM"
// biss an dieser Zuordnung (g1_rot_naht_koeder_ycsb.log). Alle drei Gegenproben danach gruen.

#include <topics/axis.hpp>

#include <cache_engine/abi/system_axis_order.hpp>
#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/load_framework_measurement_axis.hpp>
#include <cache_engine/measurement/system_axis.hpp>
#include <system_axes/external_utils_family_axis.hpp>
#include <system_axes/scheduling_system_axis.hpp>
#include <system_axes/target_isa_complex_axis.hpp>
#include "topics/organ_meta_meta_axis.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <string_view>

namespace cet = ::comdare::cache_engine::topics;
namespace cem = ::comdare::cache_engine::measurement;
namespace cea = ::comdare::cache_engine::abi;

// -- Block A: die sechs Diskriminator->Kategorie-Pins am NUTZUNGS-Ort (Doppel zu den Dach-Pins) --
// Die Pins stehen auch in topics/axis.hpp; hier stehen sie ein zweites Mal, damit ein kuenftiger
// Umbau des Dachs (z. B. Aufspaltung der Pins) diese TU nicht stumm macht -- der Test bleibt die
// Wache der KATEGORIEN-SEMANTIK, unabhaengig davon, wie das Dach seine eigenen Wachen schneidet.
static_assert(cet::axis_category(cet::AxisKind::system_measurement) == cet::AxisCategory::mess);
static_assert(cet::axis_category(cet::AxisKind::measurement_meta_meta) == cet::AxisCategory::mess);
static_assert(cet::axis_category(cet::AxisKind::system_config) == cet::AxisCategory::system);
static_assert(cet::axis_category(cet::AxisKind::system_meta_meta) == cet::AxisCategory::system);
static_assert(cet::axis_category(cet::AxisKind::organ) == cet::AxisCategory::organ);
static_assert(cet::axis_category(cet::AxisKind::organ_meta_meta) == cet::AxisCategory::organ);

// -- Block B: LEBENDE Traeger je Realm melden die richtige Kategorie ------------------------------
// MESS: die Blut-Achse (WallClock) und die Mess-Meta-Meta (Ycsb/load_framework, RF-1/R-G).
static_assert(cet::axis_category(cem::WallClockSystemAxis::axis_kind()) == cet::AxisCategory::mess);
static_assert(cet::axis_category(cem::YcsbLoadFrameworkAxis::axis_kind()) == cet::AxisCategory::mess);
static_assert(cet::axis_category_of<cem::WallClockSystemAxis>() == cet::AxisCategory::mess);
static_assert(cet::axis_category_of<cem::YcsbLoadFrameworkAxis>() == cet::AxisCategory::mess);
// SYSTEM: Konfig-Wurzel-Probe (Form wie im Dach-Guard) + der external_utils-Familienvertreter.
// TRIPWIRE-VERTRAEGLICH (C-3): SimdExternalUtilsFamily meldet HEUTE system_config; hebt C-3 sie
// spaeter auf system_meta_meta, bleibt diese KATEGORIEN-Probe wahr (beide sind Kategorie SYSTEM).
// Genau dafuer ist die Kategorien-Ebene da -- der Realm ist stabil, die Feinstufe darf wandern.
namespace {
struct KonfigProbe final : cem::CebSystemAxis<KonfigProbe> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "konfig_probe_g1"; }
};
} // namespace
static_assert(cet::axis_category_of<KonfigProbe>() == cet::AxisCategory::system);
static_assert(cet::axis_category(cem::SimdExternalUtilsFamily::axis_kind()) == cet::AxisCategory::system);
// ORGAN: Dach-Probe mit organ-Diskriminator + Organ-Meta-Meta-Beweisachse (A13-M2-Mechanismus).
namespace {
struct OrganDachProbe final : cet::Axis<OrganDachProbe> {
    [[nodiscard]] static constexpr cet::AxisKind axis_kind() noexcept { return cet::AxisKind::organ; }
};
struct ProofOrganMetaMetaG1 final : cet::OrganMetaMetaAxis<ProofOrganMetaMetaG1> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "proof_omm_g1"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "proof_sub_g1"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};
} // namespace
static_assert(cet::axis_category_of<OrganDachProbe>() == cet::AxisCategory::organ);
static_assert(cet::axis_category_of<ProofOrganMetaMetaG1>() == cet::AxisCategory::organ);
// A2.5-R1 (25.08.2026, L-06 Warnungs-Review, clang -Wunused-const-variable): der Organ-Meta-Meta-
// Vertrag verlangt axis_code_version nur im requires-Ausdruck (unbewertet); die Beweisachse liest
// ihren Stempel HIER und pinnt ihn -- Aussage statt Anwesenheit (T-2).
static_assert(ProofOrganMetaMetaG1::axis_code_version == std::string_view{"1.0.0.c"});

// -- Block C: die Unter->Haupt-KETTE als TYP (V-01R-Kern) -----------------------------------------
// DefaultSchedulingSystemAxis ist eine ECHTE Unter-Achse des Bestands: CebSubAxis<., TargetIsaAxisTag>
// (Paket A1: using parent_axis = ParentAxis). Die Kategorie kommt ueber die KETTE aus der Wurzel --
// nirgends wiederholt. axis_depth (Bestand) kreuzt die Ketten-Laenge gegen dieselbe Typ-Struktur.
static_assert(cem::CebSubAxisConcept<cem::DefaultSchedulingSystemAxis>);
static_assert(cet::SubAxisBezugConcept<cem::DefaultSchedulingSystemAxis>);
static_assert(!cet::SubAxisBezugConcept<cem::TargetIsaAxisTag>); // die Wurzel hat KEINEN Eltern-Bezug
static_assert(cem::axis_depth_v<cem::DefaultSchedulingSystemAxis> == 1);
static_assert(cem::axis_depth_v<cem::TargetIsaAxisTag> == 0);
static_assert(cet::axis_category_of<cem::DefaultSchedulingSystemAxis>() == cet::AxisCategory::system);
static_assert(cet::axis_category_of<cem::TargetIsaAxisTag>() == cet::AxisCategory::system);

// -- Block D: NP-01-KONSULTATION (KON106-02) -------------------------------------------------------
// Die Zuordnung KONSULTIERT kSystemAxisOrder und ersetzt sie nie: die Haupt-Achse, auf die die
// Unter-Achsen-Kette zeigt, MUSS in der Ordnungs-Single-Source stehen -- geprueft ueber die
// bestehende Such-API (system_axis_order_index / is_known_system_axis), NICHT ueber eine hier
// nachgebaute Liste (das waere die am 12.08. verbotene zweite Wahrheit).
static_assert(cea::is_known_system_axis(cem::DefaultSchedulingSystemAxis::parent_axis_label()),
              "NP-01: die Eltern-Achse der Scheduling-Unter-Achse fehlt in kSystemAxisOrder -- die "
              "Kategorien-Zuordnung darf nur auf Haupt-Achsen der Ordnungs-Single-Source zeigen.");
static_assert(cea::is_known_system_axis(cem::TargetIsaAxisTag::axis_label()),
              "NP-01: der target_isa-Familienvertreter muss in kSystemAxisOrder stehen (Konsultation, "
              "keine Nachbildung).");
static_assert(cem::DefaultSchedulingSystemAxis::parent_axis_label() == cem::TargetIsaAxisTag::axis_label(),
              "NP-01: Typ-Kette (parent_axis) und String-Ordnung (kSystemAxisOrder) muessen auf DIESELBE "
              "Haupt-Achse zeigen -- laeuft das auseinander, traegt eine der beiden Quellen Drift.");

// -- Block E: RT-Nenner (Grundgesamtheit + Ordnung, sichtbare Zaehlung) ----------------------------
namespace {
constexpr std::array<cet::AxisKind, cet::kAxisKindCount> kAlleAxisKinds{
    cet::AxisKind::system_measurement,
    cet::AxisKind::measurement_meta_meta,
    cet::AxisKind::system_config,
    cet::AxisKind::system_meta_meta,
    cet::AxisKind::organ,
    cet::AxisKind::organ_meta_meta,
};

[[nodiscard]] std::string_view kategorie_token(cet::AxisCategory c) {
    switch (c) {
        case cet::AxisCategory::mess: return "mess";
        case cet::AxisCategory::system: return "system";
        case cet::AxisCategory::organ: return "organ";
    }
    return {};
}
} // namespace

TEST(AxisKindKategorien, NENNER_JederEnumeratorTraegtGenauEineKategorie) {
    std::size_t mit_kategorie = 0;
    for (auto const k : kAlleAxisKinds)
        if (!kategorie_token(cet::axis_category(k)).empty()) ++mit_kategorie;
    std::cout << "[AXISKIND-KATEGORIE] Enumeratoren mit Kategorie: " << mit_kategorie << "/" << kAlleAxisKinds.size()
              << " (Grundgesamtheit: die im Test einzeln genannten AxisKind-Enumeratoren aus topics/axis.hpp)\n";
    EXPECT_EQ(mit_kategorie, kAlleAxisKinds.size());
}

TEST(AxisKindKategorien, ORDNUNG_OrdinaleFolgenDerKategorienOrdnungMessSystemOrgan) {
    std::size_t geprueft = 0;
    for (std::size_t i = 0; i + 1 < kAlleAxisKinds.size(); ++i) {
        auto const links  = cet::axis_category(static_cast<cet::AxisKind>(i));
        auto const rechts = cet::axis_category(static_cast<cet::AxisKind>(i + 1));
        EXPECT_LE(static_cast<unsigned>(links), static_cast<unsigned>(rechts))
            << "Ordinal " << i << " (" << kategorie_token(links) << ") steht hinter Ordinal " << i + 1 << " ("
            << kategorie_token(rechts) << ") -- der V-01R-Dreh ist verletzt";
        ++geprueft;
    }
    std::cout << "[AXISKIND-KATEGORIE] Ordnungs-Paare monoton geprueft: " << geprueft << "/"
              << (kAlleAxisKinds.size() - 1)
              << " (Grundgesamtheit: benachbarte Ordinal-Paare der Enumeratoren aus topics/axis.hpp)\n";
    EXPECT_EQ(geprueft, kAlleAxisKinds.size() - 1);
}

TEST(AxisKindKategorien, KONSULTATION_TypKetteUndOrdnungsQuelleDeckenSich) {
    // RT-Sicht der NP-01-Wache (die CT-Haelfte steht oben als static_assert): sichtbare Zaehlung,
    // damit der Nenner im Log steht und ein kuenftiger Ordnungs-Umbau hier LAUT wird.
    auto const idx = cea::system_axis_order_index(cem::DefaultSchedulingSystemAxis::parent_axis_label());
    std::cout << "[AXISKIND-KATEGORIE] parent_axis_label='" << cem::DefaultSchedulingSystemAxis::parent_axis_label()
              << "' -> kSystemAxisOrder-Index " << idx << " von " << cea::kSystemAxisOrderCount
              << " (Grundgesamtheit: kSystemAxisOrder, abi/system_axis_order.hpp)\n";
    EXPECT_LT(idx, cea::kSystemAxisOrderCount);
}
