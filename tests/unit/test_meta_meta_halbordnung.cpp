// tests/unit/test_meta_meta_halbordnung.cpp -- Lane C, Paket C-5 (26.07.2026).
//
// HISTORIE (bis 02.08.2026): dieser Test war ABSICHTLICH NICHT in tests/unit/CMakeLists.txt
// eingetragen, weil die Datei damals Lane-A-Konflikt-Zone war (Auftrag Abschnitt 3, Bauplan-v3 D3);
// die Verifikation lief ueber einen Hand-Bau mit dem Include-/Link-Satz eines Muster-Blocks.
//
// SEIT A13-M2 (Review-BEFUND-1, Owner-Entscheid F-WAISEN Variante (b) vom 02.08.2026) IST DIESE TU
// REGISTRIERT. Grund: die Lane-A-Konflikt-Zone existiert nicht mehr, und der Hand-Bau hat genau das
// verschleiert, was er verhindern sollte -- die Verschaerfung von SystemMetaMetaAxisConcept
// (axis_code_version-Pflicht) brach diese TU compile-hart, ohne dass ein Voll-ctest es zeigte
// ("Lokale Voll-Bau-Luecken = falsches Gruen"). Sie steht ab jetzt im normalen Sweep.
// Von den acht nie registrierten Waisen-TUs in tests/unit/ ist bewusst NUR diese eine registriert
// worden (sie ist die einzige, die von A13-M2 beruehrte Header inkludiert); die uebrigen sieben
// (br4_emit, br4_load, kf16_e2e_real_build, test_a9b_active_deklaration_inert,
// test_c3b_kanal_merge_beleg, test_d4b_container_dll, test_rf2_admission_marker_inert) sind
// Bestands-Schuld ausserhalb des Wellen-Scopes und stehen auf der Abschluss-Aufraeumpass-Kandidatenliste.
//
// WAS HIER BEWIESEN WIRD (und warum es nicht in die Header gehoert): die Header tragen jeweils ihre
// EIGENEN Invarianten. Was sie NICHT tragen koennen, ist die Aussage UEBER DIE SCHICHTEN HINWEG --
// dass die Typ-Familie (C-1', hardware_meta_meta_axis.hpp, boost-frei), die Identitaets-Halbordnung
// (A1, meta_meta_identity.hpp, mp11) und die gehobene Zulassung (C-2, meta_meta_admission.hpp) am
// ECHTEN Hub dasselbe bedeuten und dasselbe Urteil liefern wie das flag-genaue Pruef-Dock
// (simd_build_gate.hpp). Genau diese Bruecken-Aussagen stehen hier.

#include <system_axes/external_utils_family_axis.hpp> // C-3: der Hub + sein Glied
// R-G-TRIPWIRE-Include: der HUB zieht load_framework absichtlich NICHT mehr (Ledger 69.1 -- Mess-Realm,
// nicht System-Hub). Wer die Realm-Grenze PRUEFEN will, muss die Achse deshalb selbst hereinholen. Genau
// diese Include-Asymmetrie ist Teil des Belegs: der Hub kennt load_framework nicht.
#include <cache_engine/measurement/load_framework_measurement_axis.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp> // die echten Maschinen-Signaturen
#include <cache_engine/measurement/meta_meta_admission.hpp>    // C-2: die gehobene Zulassung
#include <cache_engine/measurement/meta_meta_identity.hpp>     // A1: MetaMetaSet + subsumes_v
#include <cache_engine/measurement/simd_build_gate.hpp>        // die Flag-Ebene (Pruef-Dock)
#include <cache_engine/measurement/simd_organ_requirement.hpp> // any_organ_declares_required
#include <cache_engine/measurement/simd_organ_sensibility.hpp> // kSensibilityFilterFlags

#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace meas = comdare::cache_engine::measurement;

namespace {

// -------------------------------------------------------------------------------------------------
// Test-LOKALE System-Meta-Metas. Sie zeigen zugleich die Kern-Zusage von C-1': eine NEUE Meta-Meta
// braucht keine Rahmen-Aenderung, nur einen Typ. Keine dieser Klassen wird ausgeliefert.
// -------------------------------------------------------------------------------------------------

/// Eine Meta-Meta, die die AVX-512-Faehigkeit der Maschine repraesentiert (Typ-Gegenstueck zu den
/// avx512-Flags der Flag-Ebene). Volle CT-Haupt-Achse mit eigener RT-Unter-Achse -- Auftrag 1.4.
/// A13-M2 (Review-BEFUND-1, 02.08.2026): axis_code_version ist seit der Verschaerfung von
/// SystemMetaMetaAxisConcept (hardware_meta_meta_axis.hpp, Owner-Entscheid E2 "Meta-Meta-Stempel sind
/// PFLICHT wie alle Hauptachsen") ein PFLICHT-Member. Ohne sie schlagen die
/// static_assert(SystemMetaMetaAxisConcept<...>)-Zeilen im Abschnitt C-1' hart fehl.
/// FLAGBEHAFTET wie der Bestand ("1.0.0.c", Owner-Q3) -- diese drei Test-Typen standen auf der
/// Migrations-Naht-Liste in algo_semver.hpp, Klasse (d), und sind mit A13-M3/C4 mitgezogen worden
/// (grep-Anweisung dort; sie tragen KEINE gated Wache und mussten deshalb aktiv gefunden werden).
struct Avx512MetaMeta final : meas::SystemMetaMetaAxis<Avx512MetaMeta> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "test_avx512"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "test_avx512_level"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};

/// Eine zweite, unabhaengige Meta-Meta (Platzhalter fuer externe Hardware -- GPU/FPGA/NPU-Klasse).
struct GpuMetaMeta final : meas::SystemMetaMetaAxis<GpuMetaMeta> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "test_gpu"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "test_gpu_device"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};

/// Eine Meta-Meta, die SELBST Manager ist -- der Beleg fuer die offene Rekursion (Q-D: NVIDIA-GPUs als
/// Cluster am PCIe). Sie braucht dafuer nichts als ein `meta_metas`-Alias.
struct GpuClusterMetaMeta final : meas::SystemMetaMetaAxis<GpuClusterMetaMeta> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "test_gpu_cluster"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "test_gpu_cluster_fabric"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
    using meta_metas                                                  = meas::MetaMetaMembers<GpuMetaMeta>;
};

// 09.08.2026 (Warnungs-Runde 2, clang -Wunused-const-variable; Klasse MUTANT): der Kommentar oben
// BEHAUPTET fuer alle drei Test-Typen die A13-M3/C4-Migration auf das geflaggte "1.0.0.c" -- geprueft
// wurde davon nichts: GpuMetaMeta::axis_code_version wurde in dieser TU nie gelesen (die Warnung war
// der Beleg; ihre zwei Geschwister erreichten nur zufaellig eine gelesene Stelle). Eine Behauptung
// ueber DREI, die keiner ansieht, ist dieselbe Blindstelle wie die Fuenf-Gattungen-Wache aus Runde 1.
// Jetzt ist die Migrations-Behauptung fuer jeden der drei Typen eine WACHE statt eines Satzes.
static_assert(Avx512MetaMeta::axis_code_version == std::string_view{"1.0.0.c"} &&
                  GpuMetaMeta::axis_code_version == std::string_view{"1.0.0.c"} &&
                  GpuClusterMetaMeta::axis_code_version == std::string_view{"1.0.0.c"},
              "A13-M3/C4: die drei Test-Meta-Metas tragen das geflaggte Owner-Q3-Format 1.0.0.c -- "
              "wer eine davon migriert, zieht diese Wache mit, statt den Kommentar veralten zu lassen.");

// Identitaeten (A1-Vokabular) ueber ECHTEN Meta-Meta-TYPEN -- nicht ueber A1s Platzhaltern.
using IdCpuOnly   = meas::CpuOnlyIdentity;
using IdAvx512    = meas::MetaMetaSet<Avx512MetaMeta>;
using IdGpu       = meas::MetaMetaSet<GpuMetaMeta>;
using IdAvx512Gpu = meas::MetaMetaSet<Avx512MetaMeta, GpuMetaMeta>;

} // namespace

// =================================================================================================
// C-1': die Typ-Familie traegt das echte Glied
// =================================================================================================
static_assert(meas::SystemMetaMetaAxisConcept<meas::SimdExternalUtilsFamily>);
static_assert(meas::SystemMetaMetaAxisConcept<Avx512MetaMeta>);
static_assert(meas::SystemMetaMetaAxisConcept<GpuClusterMetaMeta>);
// Zero-cost: keine vtable, kein Zustand -- die Anti-Runtime-Switch-Sicherung gilt auch fuer Meta-Metas.
static_assert(std::is_empty_v<Avx512MetaMeta> && !std::is_polymorphic_v<Avx512MetaMeta>);
static_assert(std::is_empty_v<GpuClusterMetaMeta> && !std::is_polymorphic_v<GpuClusterMetaMeta>);

TEST(MetaMetaTypFamilie, GliedIstEineEigenstaendigeVollAchse) {
    // Command-Pattern: das Glied ist fuer sich allein gueltig, nicht nur im Verband des Managers.
    EXPECT_TRUE(meas::CebSystemAxisConcept<meas::SimdExternalUtilsFamily>);
    // ... und es spannt eine EIGENE RT-Unter-Achse. Getrennte Klammerung, keine Fusion (Owner Q-A).
    EXPECT_EQ(meas::SimdExternalUtilsFamily::sub_axis_label(), std::string_view{"simd"});
    EXPECT_EQ(meas::SimdExternalUtilsFamily::axis_label(), std::string_view{"external_utils"});
}

TEST(MetaMetaTypFamilie, MarkerIstEinOptInKeineZufallsErkennung) {
    struct SiehtAusWieEineAchse {
        static constexpr std::string_view axis_label() noexcept { return "kein_opt_in"; }
        static constexpr std::string_view sub_axis_label() noexcept { return "auch_nicht"; }
    };
    EXPECT_FALSE(meas::is_system_meta_meta_axis_v<SiehtAusWieEineAchse>);
    EXPECT_FALSE(meas::SystemMetaMetaAxisConcept<SiehtAusWieEineAchse>);
    // Eine Unter-Achsen-OPTION ist keine Meta-Meta -- sie spannt keine eigene Unter-Achse.
    EXPECT_FALSE(meas::SystemMetaMetaAxisConcept<meas::SimdAvx2Option>);
}

TEST(MetaMetaTypFamilie, RGLoadFrameworkIstKeineSystemMetaMeta) {
    // R-G-TRIPWIRE (Ledger 69.1, Owner-KERN): load_framework ist eine Meta-Meta-HAUPT-Achse der
    // MESS-Achsen, NICHT eine System-Meta-Meta unter dem external_utils-Hub. Diese Wache haelt die
    // Realm-Grenze; sie bricht, sobald jemand die alte (superseded) Zuordnung wieder einbaut.
    EXPECT_FALSE(meas::is_system_meta_meta_axis_v<meas::YcsbLoadFrameworkAxis>);
    EXPECT_FALSE(meas::SystemMetaMetaAxisConcept<meas::YcsbLoadFrameworkAxis>);
    EXPECT_FALSE((meas::ExternalUtilsHub::manages<meas::YcsbLoadFrameworkAxis>));
    EXPECT_FALSE((meas::ExternalUtilsHub::releases<meas::YcsbLoadFrameworkAxis>));
    // NACHZUG A13-M2 (Review-BEFUND-1-Folgebefund, 02.08.2026): hier stand bis heute
    //   EXPECT_TRUE(meas::CebSystemAxisConcept<meas::YcsbLoadFrameworkAxis>)
    // mit dem Kommentar "der Umzug in den Mess-Realm ... liegt in A3 / im O-8-Fenster, nicht hier".
    // Der Umzug IST seither vollzogen (K1, O-8 Schritt 4 / Paket A3): die Basis ist
    // MeasurementMetaMetaAxis statt CebSystemAxis, axis_kind() liefert measurement_meta_meta
    // (REALM-ANKER static_assert in load_framework_measurement_axis.hpp). Die alte Erwartung war
    // also nicht "noch nicht dran", sondern SUPERSEDED -- sie ist nur deshalb nie gebrochen, weil
    // diese TU als Waise nie gebaut wurde ("Lokale Voll-Bau-Luecken = falsches Gruen"). Ab jetzt
    // nagelt der Test die NEUE Ordnung fest, in beide Richtungen.
    EXPECT_FALSE(meas::CebSystemAxisConcept<meas::YcsbLoadFrameworkAxis>);
    EXPECT_TRUE(meas::MeasurementMetaMetaAxisConcept<meas::YcsbLoadFrameworkAxis>);
    EXPECT_EQ(meas::YcsbLoadFrameworkAxis::axis_kind(), comdare::cache_engine::topics::AxisKind::measurement_meta_meta);
    // Dieser Gegentest ist in der Mess-Header-Datei selbst ABSICHTLICH nicht moeglich (er zoege
    // ceb_system_axis.hpp in den Include-Graph der Mess-Seite, s. Kommentar dort). In DIESER TU sind
    // beide Realms ohnehin sichtbar -- sie ist damit der einzig realm-saubere Ort fuer die Gegenprobe.
    EXPECT_EQ(meas::YcsbLoadFrameworkAxis::sub_axis_label(), std::string_view{"workload"});
}

// =================================================================================================
// C-2: die Halbordnung auf ECHTEN Meta-Meta-Typen (A1 belegte sie nur an Platzhaltern)
// =================================================================================================
// Reflexiv.
static_assert(meas::subsumes_v<IdCpuOnly, IdCpuOnly>);
static_assert(meas::subsumes_v<IdAvx512Gpu, IdAvx512Gpu>);
// Basis CPU-only ist das MINIMUM: sie laeuft unter jeder Identitaet (Owner: "Basis = CPU-only").
static_assert(meas::subsumes_v<IdAvx512, IdCpuOnly>);
static_assert(meas::subsumes_v<IdGpu, IdCpuOnly>);
static_assert(meas::subsumes_v<IdAvx512Gpu, IdCpuOnly>);
// AUFWAERTS ja: Teil-Identitaeten mit KLEINERER Hardware-Verwendung sind gueltig (Auftrag C-2).
static_assert(meas::subsumes_v<IdAvx512Gpu, IdAvx512>);
static_assert(meas::subsumes_v<IdAvx512Gpu, IdGpu>);
// ABWAERTS nein: Hardware ausgebaut -> das Programm, das sie braucht, laeuft NICHT mehr.
static_assert(!meas::subsumes_v<IdAvx512, IdAvx512Gpu>);
static_assert(!meas::subsumes_v<IdCpuOnly, IdAvx512>);
// UNVERGLEICHBAR: zwei disjunkte Erweiterungen ordnen sich nicht -- deshalb HALBordnung und keine
// Totalordnung. Waere hier irgendwo true, waere die Relation eine Rangfolge und die Doktrin falsch.
static_assert(!meas::subsumes_v<IdAvx512, IdGpu>);
static_assert(!meas::subsumes_v<IdGpu, IdAvx512>);
// ANTISYMMETRIE (die Eigenschaft, die A1 offen liess): gilt subsumes in BEIDE Richtungen, sind die
// Mengen gleich. Kontraposition an echten Paaren: verschiedene Mengen -> nie beidseitig.
static_assert(!(meas::subsumes_v<IdAvx512Gpu, IdAvx512> && meas::subsumes_v<IdAvx512, IdAvx512Gpu>));
static_assert(!(meas::subsumes_v<IdAvx512, IdCpuOnly> && meas::subsumes_v<IdCpuOnly, IdAvx512>));
// TRANSITIV entlang der Kette CpuOnly <= Avx512 <= Avx512Gpu.
static_assert(meas::subsumes_v<IdAvx512, IdCpuOnly> && meas::subsumes_v<IdAvx512Gpu, IdAvx512> &&
              meas::subsumes_v<IdAvx512Gpu, IdCpuOnly>);
// Die Concept-Schreibweise derselben Relation an der Freigabe-Naht.
static_assert(meas::RunnableOn<IdCpuOnly, IdAvx512Gpu>);
static_assert(!meas::RunnableOn<IdAvx512Gpu, IdCpuOnly>);

TEST(MetaMetaHalbordnung, GliederDerHalbordnungSindEchteMetaMetaAchsen) {
    // Die Haertung gegenueber A1: die Menge darf nicht aus beliebigen Typen bestehen (C-2).
    EXPECT_TRUE((meas::identity_members_are_axes_v<IdAvx512Gpu>));
    EXPECT_TRUE((meas::identity_members_are_axes_v<IdCpuOnly>));
    EXPECT_FALSE((meas::identity_members_are_axes_v<meas::MetaMetaSet<meas::SimdAvx2Option>>));
    EXPECT_EQ(meas::meta_meta_count_v<IdCpuOnly>, 0u);
    EXPECT_EQ(meas::meta_meta_count_v<IdAvx512Gpu>, 2u);
}

TEST(MetaMetaHalbordnung, GehobeneZulassungIstEineUEBERLADUNGKeineErsetzung) {
    // D2.9: die Alt-Signatur bleibt erreichbar UND die neue Typ-Ueberladung existiert daneben.
    // (a) Alt-Signatur, Flag-Ebene -- unveraendert.
    EXPECT_FALSE(meas::admit_organ_on_machine({}, meas::Prod1Zen5Signature::signature()).has_value());
    // (b) Neue Ueberladung, Meta-Meta-Ebene -- dieselbe Rueckgabe-Form, dieselbe Fehlerklasse.
    EXPECT_FALSE((meas::admit_organ_on_machine<IdAvx512Gpu, IdAvx512>().has_value()));
    auto const abgelehnt = meas::admit_organ_on_machine<IdCpuOnly, IdAvx512>();
    ASSERT_TRUE(abgelehnt.has_value());
    EXPECT_EQ(*abgelehnt, meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);
}

// =================================================================================================
// C-2-BRUECKE: dieselbe Teilmengen-Semantik wie das flag-genaue Gate -- generalisiert, nicht parallel
// =================================================================================================
TEST(MetaMetaHalbordnung, TypEbeneUrteiltWieDieFlagEbene) {
    // Sachverhalt: ein Programm braucht AVX-512. prod1 (Zen 5) hat es, prod2 (Raptor Lake, fused off)
    // nicht. Die FLAG-Ebene entscheidet das ueber admit_organ_on_machine (simd_build_gate.hpp:153-159).
    static constexpr std::array<meas::SimdFeatureFlag, 1> kRequiresAvx512{{meas::kAvx512F}};

    auto const flag_urteil_prod1 = meas::admit_organ_on_machine(kRequiresAvx512, meas::Prod1Zen5Signature::signature());
    auto const flag_urteil_prod2 =
        meas::admit_organ_on_machine(kRequiresAvx512, meas::Prod2AlderLakeSignature::signature());

    EXPECT_FALSE(flag_urteil_prod1.has_value()) << "prod1/Zen5 besitzt avx512f -> Zulassung erwartet.";
    ASSERT_TRUE(flag_urteil_prod2.has_value()) << "prod2 hat AVX-512 fused-off -> Ablehnung erwartet.";
    EXPECT_EQ(*flag_urteil_prod2, meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);

    // Dieselbe Frage auf der TYP-Ebene, ueber die C-2-Ueberladung: prod1s Identitaet traegt die
    // Avx512-Meta-Meta, prod2s nicht.
    auto const typ_urteil_prod1 = meas::admit_organ_on_machine<meas::MetaMetaSet<Avx512MetaMeta>, IdAvx512>();
    auto const typ_urteil_prod2 = meas::admit_organ_on_machine<IdCpuOnly, IdAvx512>();

    // DIE BRUECKE: beide Ebenen liefern dasselbe Urteil, in derselben Form.
    EXPECT_EQ(typ_urteil_prod1.has_value(), flag_urteil_prod1.has_value());
    EXPECT_EQ(typ_urteil_prod2.has_value(), flag_urteil_prod2.has_value());
    EXPECT_EQ(*typ_urteil_prod2, *flag_urteil_prod2);
}

TEST(MetaMetaHalbordnung, NurDasFreigabeGateIstInertNichtDieSimdAchse) {
    // PRAEZISIERUNG (Bauplan-v3 D2.1): die Formulierung "der Freigabe-Pilot ist inert" laedt zu der
    // FALSCHEN Verallgemeinerung ein, SIMD sei insgesamt inert. Das ist nachweislich nicht so: die
    // -m-Flags erreichen den Compiler ueber permutation_codegen_tool.cpp:43-45/:477-483 und
    // cmake/isa_features.cmake. INERT ist ausschliesslich das Section-37-FREIGABE-GATE.
    //
    // Der Test nagelt deshalb GENAU DREI Dinge fest, jeweils mit eigener Bruchstelle:

    // (1) NACHZUG C-3a: die drei Hooks sind seit der Scharfschaltung NICHT mehr pauschal leer --
    //     dieser Test war die Tripwire dafuer und hat korrekt gebrochen. Der Ist-Stand ist jetzt:
    //     required LEER (der dominante Schalter, Organ-Seite), meaningful ECHT gefuellt (Vereinigung
    //     der Sinnhaftigkeits-Matrix), machine_signature leer SOLANGE die CEB nichts belegt hat.
    // E-10 3e: Hook 1 ist seit Schritt 3 PER BINARY (organ_required_for_axes; der globale Hook ist
    // entfernt) -- die Aggregation der Demo-Binary ist leer, die Vollmengen-Auskunft Hook 2 bleibt echt.
    std::vector<std::pair<std::string, std::string>> const demo_axes{{"search_algo", "k_ary"},
                                                                     {"persistence_target", "persistence_memory_only"}};
    EXPECT_TRUE(meas::organ_required_for_axes(demo_axes).empty());
    EXPECT_FALSE(meas::active_organ_meaningful().empty())
        << "C-3a fuellt Hook 2 mit der echten Vereinigung -- leer waere hier ein Rueckschritt.";
    EXPECT_TRUE(meas::active_machine_signature().empty())
        << "Ohne CEB-Belegung bleibt die Signatur leer (natuerlicher Kill-Switch).";

    // (2) Der DOMINANTE Schalter ist nicht Hook (3), sondern die Organ-Deklaration: solange kein Organ
    //     required-Flags traegt (simd_organ_requirement.hpp:40-50, static_assert :88), kehrt pruef_dock
    //     bei :109 sofort mit NotApplicable zurueck -- UNABHAENGIG davon, welche Maschinen-Signatur
    //     eingesetzt wird. Deshalb waere das blosse Fuellen von :187 ein No-Op (Bauplan D2.3).
    EXPECT_FALSE(meas::any_organ_declares_required());
    auto const mit_echter_signatur =
        meas::pruef_dock(meas::organ_required_for_axes(demo_axes), meas::organ_meaningful_for_axes(demo_axes),
                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
    EXPECT_EQ(mit_echter_signatur.state, meas::SimdGateState::NotApplicable)
        << "Belegt: eine ECHTE Signatur allein schaltet das Gate NICHT scharf.";
    EXPECT_EQ(mit_echter_signatur.effective_count, 0u);

    // (3) Folglich steuert das Gate heute NULL Flags bei -- auf allen drei Routen.
    for (auto route : {meas::SimdRoute::NoExtension, meas::SimdRoute::Avx2, meas::SimdRoute::Avx512})
        EXPECT_TRUE(meas::gate_for_binary(demo_axes, route).flags.empty());
}

// =================================================================================================
// C-3: die Hub-Mechanik am echten Knoten
// =================================================================================================
TEST(ExternalUtilsHub, ManagerVerwaltetUndGibtFrei) {
    EXPECT_EQ(meas::ExternalUtilsHub::meta_metas::size, 1u);
    EXPECT_TRUE((meas::ExternalUtilsHub::manages<meas::SimdExternalUtilsFamily>));
    EXPECT_FALSE((meas::ExternalUtilsHub::manages<Avx512MetaMeta>)) << "test-lokale Meta-Meta ist nicht im Hub.";
    EXPECT_TRUE((meas::ExternalUtilsHub::releases<meas::SimdExternalUtilsFamily>));
    // Eine Unter-Achsen-Option wird nie freigegeben -- sie ist keine Meta-Meta.
    EXPECT_FALSE((meas::ExternalUtilsHub::releases<meas::SimdNoExtOption>));
}

TEST(ExternalUtilsHub, IdentitaetIstDieKonfigurationDerGlieder) {
    // Owner Q-A: der Hub hat KEINE eigene Identitaet, nur die indirekte ueber seine Glieder. Mit A1s
    // Listen-Vokabular ist sie unmittelbar eine MetaMetaSet, auf die subsumes_v anwendbar ist.
    using HubIdentitaet = meas::ExternalUtilsHub::identity_as<meas::MetaMetaSet>;
    static_assert(std::is_same_v<HubIdentitaet, meas::MetaMetaSet<meas::SimdExternalUtilsFamily>>,
                  "Die Hub-Identitaet IST die Glied-Konfiguration.");
    EXPECT_EQ(meas::meta_meta_count_v<HubIdentitaet>, 1u);
    // Die Basis-Identitaet laeuft auch unter der Hub-Identitaet (Aufwaerts-Doktrin am echten Hub).
    EXPECT_TRUE((meas::subsumes_v<HubIdentitaet, meas::CpuOnlyIdentity>));
    // Eine Teil-Identitaet mit GROESSERER Hardware-Verwendung ist NICHT gueltig.
    EXPECT_FALSE((meas::subsumes_v<HubIdentitaet, meas::MetaMetaSet<meas::SimdExternalUtilsFamily, Avx512MetaMeta>>));
    // Und dasselbe Urteil ueber die C-2-Ueberladung.
    EXPECT_FALSE((meas::admit_organ_on_machine<HubIdentitaet, meas::CpuOnlyIdentity>().has_value()));
}

TEST(ExternalUtilsHub, VisitorLaeuftUeberDieGlieder) {
    std::vector<std::string_view> reihenfolge;
    meas::ExternalUtilsHub::for_each([&](auto tag) {
        using Glied = typename decltype(tag)::type;
        reihenfolge.emplace_back(Glied::axis_label());
    });
    ASSERT_EQ(reihenfolge.size(), 1u);
    EXPECT_EQ(reihenfolge[0], std::string_view{"external_utils"});
}

// =================================================================================================
// D4/D5: offene Rekursion und getrennte Klammerung
// =================================================================================================
TEST(MetaMetaRekursion, TiefeIstEmergentUndUnbegrenzt) {
    // Blaetter: Tiefe 0. Der heutige Hub: Tiefe 1.
    EXPECT_EQ((meas::hub_depth_v<meas::SimdExternalUtilsFamily>), 0u);
    EXPECT_EQ((meas::hub_depth_v<meas::ExternalUtilsHub>), 1u);
    // Eine Meta-Meta, die SELBST verwaltet, hebt die Tiefe -- ohne eine Zeile im Rahmen zu aendern.
    EXPECT_EQ((meas::hub_depth_v<GpuClusterMetaMeta>), 1u);
    struct TiefererHub {
        using meta_metas = meas::MetaMetaMembers<GpuClusterMetaMeta>;
    };
    EXPECT_EQ((meas::hub_depth_v<TiefererHub>), 2u);
    struct NochTiefer {
        using meta_metas = meas::MetaMetaMembers<TiefererHub>;
    };
    EXPECT_EQ((meas::hub_depth_v<NochTiefer>), 3u) << "KEIN festes drittes Level (Layer-Modell D4, Q-D).";
}

TEST(MetaMetaRekursion, KlammerungBleibtGetrenntFlacheFormIstNurFuerDieMenge) {
    struct HubMitVerschachteltemGlied {
        using meta_metas = meas::MetaMetaMembers<Avx512MetaMeta, GpuClusterMetaMeta>;
    };
    // Geschachtelte Form: 2 DIREKTE Glieder (die stempel-taugliche Klammer-Struktur).
    EXPECT_EQ((HubMitVerschachteltemGlied::meta_metas::size), 2u);
    // Flache Huelle: 3 Glieder -- der GpuCluster bringt sein eigenes Glied mit.
    using Flach = meas::transitive_members_t<HubMitVerschachteltemGlied>;
    EXPECT_EQ((Flach::size), 3u);
    // Die beiden Formen fallen AUSEINANDER. Waeren sie gleich, waere die Schachtelung eingeebnet und
    // damit die Achsen fusioniert -- genau das verbietet Owner Q-A.
    EXPECT_NE((Flach::size), (HubMitVerschachteltemGlied::meta_metas::size));
    // Die Zwischen-Ebene ist selbst Glied (ein Manager ist auch ein Command).
    EXPECT_TRUE((Flach::contains<GpuClusterMetaMeta>));
    EXPECT_TRUE((Flach::contains<GpuMetaMeta>));
}
