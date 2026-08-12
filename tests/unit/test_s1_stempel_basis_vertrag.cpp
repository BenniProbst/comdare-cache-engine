// tests/unit/test_s1_stempel_basis_vertrag.cpp -- S-1 (Stempel-Strecke): der Vertragstest der
// Stempel-Basisklasse (abi/stempel_basis.hpp) nach T-1..T-9.
//
// WAS DIESER TEST BEWEIST (Abnahmen A-P1..A-P7 des S-1-Schnitts):
//   A-P1  Die dreiwertige Zulassungsmatrix (7 KON7-04-Interfaces x 4 Traeger = 28 Zellen) stimmt Zelle
//         fuer Zelle mit der FREMDEN Quelle ueberein (Marke KON7-04; Tabelle unten ist eine
//         UNABHAENGIGE Abschrift, kein Re-Read des Prueflings). ASSERT der 7/4/28 VOR jeder Schleife.
//   T-4   Gegeneingaenge: absichtlich vertragslose/teilvertragliche Probe-Structs werden ABGELEHNT --
//         je Pflicht-Klasse einer (fehlender sha ohne BewusstLeer, unparsbare Zeile, stille Leere,
//         organ_zeile an der CEB [KON7-07-Riegel], fehlendes angeschlossene() am Hybrid, falscher
//         Traeger, kein CRTP). Faellt eine Pflicht aus dem Concept, wird genau hier ROT.
//   A-P2/P3  GENAU 10 gebundene Baustein-Typen (5 KON6-03 + AnatomyVersionLines + 2 Naht + Legende +
//         ToolingListe; fremdquellige Liste, ASSERT ==10 vor der Schleife) -- und
//         PlanRegistryTrioAnnotation bleibt AUSSEN (!ist_stempel_baustein, Explore: FAELLT RAUS).
//   A-P4  Byte-Orakel Planer: planner_version_stamp() == PlanerStempel::gesamt_stempel(), Praefix-Anker
//         "planner@1.0.0.c isa=x86_64 os=" (VOR der Delegation eingefroren, T-5).
//   A-P5  Byte-Orakel CEB: ceb_version_stamp() == CebStempel::gesamt_stempel(); sha-Laenge == 128
//         (sha512_len-Doktrin, anatomy_module_abi_v1_decl.hpp:224-225).
//   A-P6  Dock-Nenner == 5; jedes Literal ist VersionsLiteral-Baustein.
//   A-P7  g1-Block: vier gelabelte Zeilen, jede non-empty; Zeile 1 == planner_version_stamp()
//         (transitiv == PlanerStempel-Kompositum).
//
// T-1 KOEDER-PROTOKOLL (Tag-1-ROT; jede Mutation ausgefuehrt, literal ROT gesehen, zurueckgenommen,
// danach GRUEN -- Bau-Session 12.08.2026, gcc-release, Wuerfel: Zelle 17/28, Interface 7/7, Typ 2/10,
// Literal 3/5, Zeichen-Pos 5):
//   K1  A-P1+  Matrixzelle fingerprint_sha@Planer -> ERLAUBT (stempel_basis.hpp):
//       "A-P1: mindestens eine der 28 Matrixzellen weicht von der KON7-04-Zuordnung ab" UND
//       "T-4: fehlende fingerprint_sha-Pflicht MUSS ablehnen" -- beide ROT.
//   K2  A-P1-  angeschlossene-Pflicht aus dem Concept gestrichen (return true):
//       "angeschlossene() ist Hybrid-PFLICHT (KON47-02-Signatur) -- ihr Fehlen MUSS ablehnen" ROT.
//   K3  A-P2+  Trait-Zeile StampLineLiteral entfernt (anatomy_stamp_entries.hpp):
//       "A-P2/P3: mindestens ein Baustein-Typ hat seine Trait-Anbindung verloren" ROT.
//   K4  A-P2-  probeweise Spezialisierung fuer PlanRegistryTrioAnnotation eingeworfen:
//       "PlanRegistryTrioAnnotation ist KEIN Stempel-Baustein (Explore: FAELLT RAUS)" ROT.
//   K5  A-P4+  1 Zeichen im Planer-Kompositum gekippt ("planner@" -> "plannxr@"):
//       "A-P4: das Planer-Kompositum hat den eingefrorenen Praefix verlassen (Byte-Ereignis!)" ROT.
//   K6  A-P4-  kPlannerVersion -> "1.0.0" (ohne 'c'): BEIDE Wachen ROT -- Basis-Vertrag
//       (planner_version.hpp static_assert(StempelVertrag<...>)) UND Bestands-Wache "kPlannerVersion
//       ohne CPU-Flag ... COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf" (Erbin-Wachen bleiben scharf).
//   K7  A-P5+  constexpr organ_zeile() in CebStempel eingeworfen: static_assert unter der Definition
//       ROT ("... u.a. ist organ_zeile hier VERBOTEN (KON7-07)") -- der Riegel beisst.
//   K8  A-P5-  kSystemZeileBewusstLeer gestrichen bei leer bleibender system_zeile: derselbe
//       Vertrags-Assert ROT (stille Leere unmoeglich).
//   K9  A-P6+  kSequenceDockVersion -> "1.0.0": algo_semver-Wache (pruef_dock_version.hpp:154) UND
//       P6-Trait-Assert (:241) UND Test-Assert -- alle ROT.
//   K10 A-P6-  sechstes Fantasie-Literal "9.9.9.c" in die P6-Liste: Nenner-Asserts (Header "der
//       Dock-Nenner ist FUENF" + Test "fuehrt nicht genau die fuenf Dock-Literale") ROT.
//   K11 A-P7+  g1_build_type_label() probeweise geleert: Laufzeit-ROT
//       "Expected: (zeilen[2].size()) > (string_view{\"build-type=\"}.size()), actual: 11 vs 11".
//
// T-9: Codex-Gegenlese-Pass ist fuer die FOLGEWELLE vorgemerkt (nicht Teil dieser TU).
//
// DECKUNG, beide Mengen (GOAL-Pruefung 4): GEPRUEFT = Vertragsflaeche/Matrix, Riegel (organ@CEB,
// stille Leere, Fehlgrammatik), Byte-Delegation Planer+CEB, Baustein-Bindung (10), Dock-Nenner (5),
// g1-Ausgabeblock. NICHT GEPRUEFT (WARTE/eigene Posten, deklarierte Luecken): CEB-System-Fuellwert
// (KON8-03), Planer-SHA-Wert, Tier-/Hybrid-ERBIN am Bestand (W-B/W-D -- die Tier-/Hybrid-Probes unten
// pruefen NUR die Vertragsflaeche mit Test-lokalen Werten), SotaStampLines (W-A).

#include <builder/ceb_version_stamp.hpp>
#include <builder/pruef_dock/pruef_dock_version.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <cache_engine/abi/anatomy_stamp_entries.hpp>
#include <cache_engine/abi/stempel_basis.hpp>
#include <cache_engine/abi/system_cell_values.hpp>
#include <cache_engine/abi/toolchain_stamp_glied.hpp>
#include <profile_facade/g1_binary_version_stamp.hpp>
#include <profile_facade/planner/planner_version.hpp>
#include <profile_facade/toolchain_stamp_naht.hpp>

#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

namespace {

namespace abi = ::comdare::cache_engine::abi;
namespace ceb = ::comdare::cache_engine::builder;
namespace pfd = ::comdare::cache_engine::builder::pruef_dock;
namespace pfg = ::comdare::cache_engine::builder::profile_facade;
namespace pff = ::comdare::cache_engine::profile_facade;
namespace pl  = ::comdare::cache_engine::planner;
using std::string_view;

// ================================================================================================
// A-P1 / T-3: die 28 Matrixzellen gegen die FREMDE Quelle (Marke KON7-04)
// ================================================================================================
// UNABHAENGIGE Abschrift der Owner-Zuordnung -- bewusst NICHT aus kStempelZulassungsMatrix abgelesen:
//   version_xyz NUR Planer -- mess+system CEB/Tier/Hybrid -- organ Tier/Hybrid -- fingerprint_sha +
//   gesamt_stempel alle vier -- angeschlossene NUR Hybrid (RT-Hook).
// P == Pflicht, V == Verboten (ERLAUBT ist in KON7-04 heute unbelegt; die Dreiwertigkeit ist
// KON8-07-Form und wird ueber die Enum-Werte unten mitgeprueft).
struct FremdZelle {
    abi::StempelInterface i;
    abi::StempelTraeger   t;
    abi::StempelZulassung z;
};
inline constexpr std::size_t kFremdInterfaceAnzahl = 7;
inline constexpr std::size_t kFremdTraegerAnzahl   = 4;
inline constexpr std::size_t kFremdZellenAnzahl    = 28;
// ASSERT der 7 und der 28 VOR jeder Schleife (T-3):
static_assert(kFremdInterfaceAnzahl == 7 && kFremdTraegerAnzahl == 4 &&
              kFremdInterfaceAnzahl * kFremdTraegerAnzahl == kFremdZellenAnzahl);
static_assert(abi::kStempelInterfaceAnzahl == kFremdInterfaceAnzahl,
              "T-3: der Pruefling fuehrt nicht die 7 KON7-04-Interfaces");
static_assert(abi::kStempelTraegerAnzahl == kFremdTraegerAnzahl,
              "T-3: der Pruefling fuehrt nicht die 4 Traeger");

inline constexpr auto P  = abi::StempelZulassung::Pflicht;
inline constexpr auto V  = abi::StempelZulassung::Verboten;
inline constexpr auto tP = abi::StempelTraeger::Planer;
inline constexpr auto tC = abi::StempelTraeger::Ceb;
inline constexpr auto tT = abi::StempelTraeger::Tier;
inline constexpr auto tH = abi::StempelTraeger::Hybrid;
inline constexpr auto iV = abi::StempelInterface::VersionXyz;
inline constexpr auto iM = abi::StempelInterface::MessZeile;
inline constexpr auto iS = abi::StempelInterface::SystemZeile;
inline constexpr auto iO = abi::StempelInterface::OrganZeile;
inline constexpr auto iF = abi::StempelInterface::FingerprintSha;
inline constexpr auto iG = abi::StempelInterface::GesamtStempel;
inline constexpr auto iA = abi::StempelInterface::Angeschlossene;

inline constexpr std::array<FremdZelle, kFremdZellenAnzahl> kFremdMatrix{{
    {iV, tP, P}, {iV, tC, V}, {iV, tT, V}, {iV, tH, V}, // version_xyz NUR Planer
    {iM, tP, V}, {iM, tC, P}, {iM, tT, P}, {iM, tH, P}, // mess_zeile CEB/Tier/Hybrid
    {iS, tP, V}, {iS, tC, P}, {iS, tT, P}, {iS, tH, P}, // system_zeile CEB/Tier/Hybrid
    {iO, tP, V}, {iO, tC, V}, {iO, tT, P}, {iO, tH, P}, // organ_zeile Tier/Hybrid (CEB=V ist der Riegel)
    {iF, tP, P}, {iF, tC, P}, {iF, tT, P}, {iF, tH, P}, // fingerprint_sha alle vier
    {iG, tP, P}, {iG, tC, P}, {iG, tT, P}, {iG, tH, P}, // gesamt_stempel alle vier
    {iA, tP, V}, {iA, tC, V}, {iA, tT, V}, {iA, tH, P}, // angeschlossene NUR Hybrid (RT-Hook)
}};
static_assert(kFremdMatrix.size() == kFremdZellenAnzahl, "T-3: Nenner-ASSERT vor der Schleife");

static_assert(
    [] {
        for (FremdZelle const& z : kFremdMatrix)
            if (abi::stempel_zulassung(z.i, z.t) != z.z) return false;
        return true;
    }(),
    "A-P1: mindestens eine der 28 Matrixzellen weicht von der KON7-04-Zuordnung ab "
    "(kStempelZulassungsMatrix, stempel_basis.hpp)");

// ================================================================================================
// T-4: Gegeneingaenge -- der Vertrag LEHNT AB (je Pflicht-Klasse ein vertragswidriger Probe-Struct)
// ================================================================================================
inline constexpr char kProbeSha128[] =
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static_assert(string_view{kProbeSha128}.size() == 128);

// Test-lokale wohlgeformte Zeilen (NUR Vertragsflaechen-Probe; die ECHTEN Tier-/Hybrid-Erbinnen am
// Bestand sind W-B/W-D und werden hier ausdruecklich NICHT gebaut).
constexpr string_view probe_mess_zeile() noexcept {
    return "measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]";
}
constexpr string_view probe_system_zeile() noexcept {
    return "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;external_utils=code@1.0.0.c;[simd=code@1.0.0.c]";
}
constexpr string_view probe_organ_zeile() noexcept { return "search_algo=k_ary@1.0.0.c;filter=bloom@2.3.4.c"; }

constexpr std::array<string_view, 6> tier_probe_teile() noexcept {
    return {probe_organ_zeile(), "\n", probe_system_zeile(), "\n", probe_mess_zeile(), kProbeSha128};
}
inline constexpr auto kTierProbeKomp = abi::stempel_kompositum<&tier_probe_teile>();

// POSITIV Tier-Flaeche: alle Pflichten erfuellt => Vertrag wahr.
struct TierProbe : abi::StempelBasis<TierProbe, abi::StempelTraeger::Tier> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return probe_system_zeile(); }
    static constexpr string_view organ_zeile() noexcept { return probe_organ_zeile(); }
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kTierProbeKomp.view(); }
};
static_assert(abi::StempelVertrag<TierProbe, abi::StempelTraeger::Tier>,
              "T-4-Gegenprobe (positiv): die volle Tier-Vertragsflaeche muss angenommen werden");

// POSITIV Hybrid-Flaeche: zusaetzlich der angeschlossene()-RT-Hook (KON47-02-Signatur).
struct HybridProbe : abi::StempelBasis<HybridProbe, abi::StempelTraeger::Hybrid> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return probe_system_zeile(); }
    static constexpr string_view organ_zeile() noexcept { return probe_organ_zeile(); }
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kTierProbeKomp.view(); }
    static constexpr std::array<string_view, 1> kAngeschlossene{probe_organ_zeile()};
    [[nodiscard]] std::span<string_view const>  angeschlossene() const noexcept { return kAngeschlossene; }
};
static_assert(abi::StempelVertrag<HybridProbe, abi::StempelTraeger::Hybrid>,
              "T-4-Gegenprobe (positiv): die volle Hybrid-Vertragsflaeche muss angenommen werden");

// NEGATIV 1: voellig vertragslos -- fuer ALLE vier Traeger abgelehnt.
struct Vertragslos {};
static_assert(!abi::StempelVertrag<Vertragslos, abi::StempelTraeger::Planer> &&
              !abi::StempelVertrag<Vertragslos, abi::StempelTraeger::Ceb> &&
              !abi::StempelVertrag<Vertragslos, abi::StempelTraeger::Tier> &&
              !abi::StempelVertrag<Vertragslos, abi::StempelTraeger::Hybrid>);

// NEGATIV 2: Planer OHNE fingerprint_sha-BewusstLeer-Erklaerung (sha fehlt einfach) -- der
// Koeder- der A-P1: wird die fingerprint-Pflicht aus dem Concept gestrichen, kippt DIESER Assert.
constexpr std::array<string_view, 2> planer_neg_teile() noexcept { return {"planner@", "1.0.0.c"}; }
inline constexpr auto kPlanerNegKomp = abi::stempel_kompositum<&planer_neg_teile>();
struct PlanerOhneShaErklaerung : abi::StempelBasis<PlanerOhneShaErklaerung, abi::StempelTraeger::Planer> {
    static constexpr string_view version_xyz() noexcept { return "1.0.0.c"; }
    static constexpr string_view gesamt_stempel() noexcept { return kPlanerNegKomp.view(); }
    // fingerprint_sha() fehlt ABSICHTLICH -- Pflicht verletzt.
};
static_assert(!abi::StempelVertrag<PlanerOhneShaErklaerung, abi::StempelTraeger::Planer>,
              "T-4: fehlende fingerprint_sha-Pflicht MUSS ablehnen (Koeder-Flaeche der A-P1)");

// NEGATIV 3: CEB MIT organ_zeile -- der KON7-07-Riegel (A-P5-Koeder+ als Dauer-Negativprobe).
constexpr std::array<string_view, 4> ceb_neg_teile() noexcept {
    return {"ceb-measurement=", probe_mess_zeile(), ";sha512=", kProbeSha128};
}
inline constexpr auto kCebNegKomp = abi::stempel_kompositum<&ceb_neg_teile>();
struct CebMitOrganZeile : abi::StempelBasis<CebMitOrganZeile, abi::StempelTraeger::Ceb> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return {}; }
    static constexpr bool        kSystemZeileBewusstLeer      = true;
    static constexpr string_view kSystemZeileBewusstLeerGrund = "Probe";
    static constexpr string_view organ_zeile() noexcept { return probe_organ_zeile(); } // VERBOTEN
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kCebNegKomp.view(); }
};
static_assert(!abi::StempelVertrag<CebMitOrganZeile, abi::StempelTraeger::Ceb>,
              "KON7-07-Riegel: eine CEB mit organ_zeile() MUSS abgelehnt werden");

// NEGATIV 4: unparsbare Mess-Zeile (geklebte Gruppe, F4-Fehlform) -- der EINE Scanner beisst.
struct CebMitFehlgrammatik : abi::StempelBasis<CebMitFehlgrammatik, abi::StempelTraeger::Ceb> {
    static constexpr string_view mess_zeile() noexcept { return "a=b@1.0.0[c=d@1.0.0]"; }
    static constexpr string_view system_zeile() noexcept { return {}; }
    static constexpr bool        kSystemZeileBewusstLeer      = true;
    static constexpr string_view kSystemZeileBewusstLeerGrund = "Probe";
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kCebNegKomp.view(); }
};
static_assert(!abi::StempelVertrag<CebMitFehlgrammatik, abi::StempelTraeger::Ceb>,
              "M/S/O-Zeilen laufen durch stamp_line_is_wellformed -- die geklebte Gruppe MUSS ablehnen");

// NEGATIV 5: STILLE Leere -- system_zeile leer OHNE kSystemZeileBewusstLeer (A-P5-Koeder- als
// Dauer-Negativprobe: stille Leere ist unmoeglich).
struct CebMitStillerLeere : abi::StempelBasis<CebMitStillerLeere, abi::StempelTraeger::Ceb> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return {}; } // leer, aber UNERKLAERT
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kCebNegKomp.view(); }
};
static_assert(!abi::StempelVertrag<CebMitStillerLeere, abi::StempelTraeger::Ceb>,
              "Leerwert NUR mit kXxxBewusstLeer + Grund -- stille Leere MUSS ablehnen");

// NEGATIV 6: Hybrid OHNE angeschlossene() -- die gewuerfelte Koeder--Flaeche der A-P1 (Interface 7):
// wird die angeschlossene-Pflicht aus dem Concept gestrichen, wird DIESER Assert falsch-gruen und der
// Test damit ROT.
struct HybridOhneAngeschlossene : abi::StempelBasis<HybridOhneAngeschlossene, abi::StempelTraeger::Hybrid> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return probe_system_zeile(); }
    static constexpr string_view organ_zeile() noexcept { return probe_organ_zeile(); }
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kTierProbeKomp.view(); }
};
static_assert(!abi::StempelVertrag<HybridOhneAngeschlossene, abi::StempelTraeger::Hybrid>,
              "angeschlossene() ist Hybrid-PFLICHT (KON47-02-Signatur) -- ihr Fehlen MUSS ablehnen");

// NEGATIV 7: richtige Flaeche, FALSCHER Traeger (Tier-Flaeche als CEB geschlossen) -- CRTP-Anbindung
// und Matrix gehoeren zusammen (organ_zeile ist an der CEB verboten).
static_assert(!abi::StempelVertrag<TierProbe, abi::StempelTraeger::Ceb>,
              "eine Tier-Flaeche darf nicht als CEB durchgehen (Traeger-Bindung der Matrix)");

// NEGATIV 8: ohne CRTP-Anbindung (kein StempelBasis-Erbe) -- ANBINDEN ist Teil des Vertrags.
struct OhneCrtpBasis {
    static constexpr string_view version_xyz() noexcept { return "1.0.0.c"; }
    static constexpr string_view fingerprint_sha() noexcept { return {}; }
    static constexpr bool        kFingerprintShaBewusstLeer      = true;
    static constexpr string_view kFingerprintShaBewusstLeerGrund = "Probe";
    static constexpr string_view gesamt_stempel() noexcept { return kPlanerNegKomp.view(); }
};
static_assert(!abi::StempelVertrag<OhneCrtpBasis, abi::StempelTraeger::Planer>,
              "ohne CRTP-Anbindung an StempelBasis gibt es keinen Vertrag (sechste Fassung daneben)");

// NEGATIV 9: gesamt_stempel OHNE die eigene Mess-Zeile als Teilstring (Kompositum nennt sie nicht).
constexpr std::array<string_view, 2> ceb_ohne_mess_teile() noexcept { return {";sha512=", kProbeSha128}; }
inline constexpr auto kCebOhneMessKomp = abi::stempel_kompositum<&ceb_ohne_mess_teile>();
struct CebGesamtOhneMess : abi::StempelBasis<CebGesamtOhneMess, abi::StempelTraeger::Ceb> {
    static constexpr string_view mess_zeile() noexcept { return probe_mess_zeile(); }
    static constexpr string_view system_zeile() noexcept { return {}; }
    static constexpr bool        kSystemZeileBewusstLeer      = true;
    static constexpr string_view kSystemZeileBewusstLeerGrund = "Probe";
    static constexpr string_view fingerprint_sha() noexcept { return kProbeSha128; }
    static constexpr string_view gesamt_stempel() noexcept { return kCebOhneMessKomp.view(); }
};
static_assert(!abi::StempelVertrag<CebGesamtOhneMess, abi::StempelTraeger::Ceb>,
              "gesamt_stempel MUSS jede eigene nicht-leere Zeile als Teilstring enthalten "
              "(ordnungsneutral, S-6-Sperre bleibt gewahrt)");

// ================================================================================================
// A-P2/P3: GENAU 10 gebundene Baustein-Typen (fremdquellige Liste) + der Nicht-Baustein
// ================================================================================================
using GebundeneBausteine =
    std::tuple<abi::detail::StampSegment,                                 // 1 (KON6-03)
               abi::detail::StampLineLiteral<4>,                          // 2 (KON6-03, NTTP-Traeger)
               abi::CompletedSystemStampLine<4>,                          // 3 (KON6-03)
               abi::ToolchainStampParts,                                  // 4 (KON6-03)
               abi::AnatomyStampEntryV1,                                  // 5 (KON6-03)
               abi::AnatomyVersionLines,                                  // 6
               pff::PermToolchainGliedWert,                               // 7 (Naht)
               pff::PermToolchainAchsen,                                  // 8 (Naht)
               ceb::CebComboLegend<6>,                                    // 9 (Legende, "[all]")
               ceb::detail::CebToolingList>;                              // 10 (ToolingListe)
inline constexpr std::size_t kGebundeneBausteinAnzahl = 10;
// ASSERT ==10 VOR der Schleife (fremdquelliger Nenner: KON6-03 + Explore-Liste des Schnitts):
static_assert(std::tuple_size_v<GebundeneBausteine> == kGebundeneBausteinAnzahl,
              "A-P2/P3: der Baustein-Nenner ist ZEHN -- wer bindet oder loest, zieht diese Liste nach");

template <std::size_t... I>
constexpr bool alle_gebunden(std::index_sequence<I...>) noexcept {
    return (abi::StempelBaustein<std::tuple_element_t<I, GebundeneBausteine>> && ...);
}
static_assert(alle_gebunden(std::make_index_sequence<kGebundeneBausteinAnzahl>{}),
              "A-P2/P3: mindestens ein Baustein-Typ hat seine Trait-Anbindung verloren");

// Rollen-Zuordnung je Typ (der Rollen-Tag ist Teil der Anbindung, nicht Zierde):
static_assert(abi::ist_stempel_baustein<abi::detail::StampSegment>::rolle == abi::StempelBausteinRolle::Segment);
static_assert(abi::ist_stempel_baustein<abi::detail::StampLineLiteral<4>>::rolle ==
              abi::StempelBausteinRolle::ZeilenLiteral);
static_assert(abi::ist_stempel_baustein<abi::CompletedSystemStampLine<4>>::rolle ==
              abi::StempelBausteinRolle::VervollstaendigteZeile);
static_assert(abi::ist_stempel_baustein<abi::ToolchainStampParts>::rolle == abi::StempelBausteinRolle::GliedParts);
static_assert(abi::ist_stempel_baustein<abi::AnatomyStampEntryV1>::rolle == abi::StempelBausteinRolle::AbiPod);
static_assert(abi::ist_stempel_baustein<abi::AnatomyVersionLines>::rolle == abi::StempelBausteinRolle::AbiPod);
static_assert(abi::ist_stempel_baustein<pff::PermToolchainGliedWert>::rolle == abi::StempelBausteinRolle::NahtGlied);
static_assert(abi::ist_stempel_baustein<pff::PermToolchainAchsen>::rolle == abi::StempelBausteinRolle::NahtGlied);
static_assert(abi::ist_stempel_baustein<ceb::CebComboLegend<6>>::rolle == abi::StempelBausteinRolle::Legende);
static_assert(abi::ist_stempel_baustein<ceb::detail::CebToolingList>::rolle ==
              abi::StempelBausteinRolle::ToolingListe);

} // namespace

// A-P2/P3-Koeder- (Dauer-Negativprobe): PlanRegistryTrioAnnotation FAELLT RAUS (Explore-Entscheid des
// Schnitts -- Plan-Kopf-Annotation, kein Stempel-Baustein). Vorwaertsdeklaration genuegt dem Trait;
// eine probeweise eingeworfene Spezialisierung macht den Assert unten ROT.
namespace comdare::cache_engine::planner {
struct PlanRegistryTrioAnnotation;
} // namespace comdare::cache_engine::planner

namespace {

// (im anonymen Namensraum, damit der lokale abi-Alias die globale <cxxabi.h>-Alias-Kollision schattiert)
static_assert(!abi::StempelBaustein<::comdare::cache_engine::planner::PlanRegistryTrioAnnotation>,
              "PlanRegistryTrioAnnotation ist KEIN Stempel-Baustein (Explore: FAELLT RAUS) -- wer sie "
              "anbindet, weitet die KON6-03-Menge ohne Schnitt-Entscheid");

// ================================================================================================
// A-P4: Byte-Orakel PLANER (T-5: VOR der Delegation eingefroren)
// ================================================================================================
// Praefix-Anker aus der FREMDEN Quelle (Schnitt-Text), nicht aus dem Pruefling abgelesen:
inline constexpr string_view kPlanerPraefixAnker = "planner@1.0.0.c isa=x86_64 os=";
static_assert(pl::PlanerStempel::gesamt_stempel().substr(0, kPlanerPraefixAnker.size()) == kPlanerPraefixAnker,
              "A-P4: das Planer-Kompositum hat den eingefrorenen Praefix verlassen (Byte-Ereignis!)");
static_assert(pl::PlanerStempel::version_xyz() == string_view{"1.0.0.c"});

TEST(S1StempelBasisVertrag, PlanerDelegiertByteIdentisch) {
    std::string const stamp = pl::planner_version_stamp();
    EXPECT_EQ(stamp, std::string{pl::PlanerStempel::gesamt_stempel()});
    ASSERT_GE(stamp.size(), kPlanerPraefixAnker.size());
    EXPECT_EQ(stamp.substr(0, kPlanerPraefixAnker.size()), std::string{kPlanerPraefixAnker});
}

// ================================================================================================
// A-P5: Byte-Orakel CEB + sha-Laenge + deklarierte System-Luecke
// ================================================================================================
static_assert(ceb::CebStempel::fingerprint_sha().size() == 128,
              "A-P5: sha512_len-Doktrin (decl.hpp:224-225) -- der CEB-Fingerprint traegt 128 hex");
static_assert(ceb::CebStempel::system_zeile().empty() && ceb::CebStempel::kSystemZeileBewusstLeer &&
                  !ceb::CebStempel::kSystemZeileBewusstLeerGrund.empty(),
              "A-P5: die System-Luecke der CEB ist DEKLARIERT (W10-C3-Riegel-Konstante), nie still");

TEST(S1StempelBasisVertrag, CebDelegiertByteIdentisch) {
    std::string const stamp = ceb::ceb_version_stamp();
    EXPECT_EQ(stamp, std::string{ceb::CebStempel::gesamt_stempel()});
    EXPECT_EQ(stamp.rfind("ceb-measurement=", 0), 0u);
    EXPECT_NE(stamp.find(";sha512="), std::string::npos);
    EXPECT_EQ(ceb::CebStempel::fingerprint_sha().size(), 128u);
    // ORDNUNGSNEUTRAL: die eigene Mess-Zeile und der sha reisen als Teilstring im Gesamt-Stempel.
    EXPECT_NE(stamp.find(std::string{ceb::CebStempel::mess_zeile()}), std::string::npos);
    EXPECT_NE(stamp.find(std::string{ceb::CebStempel::fingerprint_sha()}), std::string::npos);
}

// ================================================================================================
// A-P6: der Dock-Nenner (FUENF) + VersionsLiteral-Bausteine
// ================================================================================================
// Fremdquellige Namensliste (KON6-03 / pruef_dock_version.hpp:58-64), ASSERT ==5 VOR der Schleife:
inline constexpr std::array<string_view, 5> kFremdDockLiterale{
    pfd::kSearchAlgorithmDockVersion, pfd::kSetDockVersion, pfd::kSequenceDockVersion, pfd::kAdapterDockVersion,
    pfd::kViewDockVersion};
static_assert(kFremdDockLiterale.size() == 5, "A-P6: Nenner-ASSERT vor der Schleife");
static_assert(pfd::kPruefDockVersionsLiteralBausteine.size() == kFremdDockLiterale.size(),
              "A-P6: die P6-Baustein-Liste fuehrt nicht genau die fuenf Dock-Literale");
static_assert(
    [] {
        for (std::size_t i = 0; i < kFremdDockLiterale.size(); ++i)
            if (pfd::kPruefDockVersionsLiteralBausteine[i] != kFremdDockLiterale[i]) return false;
        return true;
    }(),
    "A-P6: die P6-Baustein-Liste nennt andere Literale als die fuenf Dock-Konstanten");
static_assert(
    [] {
        for (string_view const v : kFremdDockLiterale)
            if (!abi::ist_versions_literal_baustein(v)) return false;
        return true;
    }(),
    "A-P6: ein Dock-Literal besteht die VersionsLiteral-Baustein-Politik nicht");

TEST(S1StempelBasisVertrag, DockNennerIstFuenf) {
    EXPECT_EQ(pfd::kPruefDockVersionsLiteralBausteine.size(), 5u);
    for (string_view const v : pfd::kPruefDockVersionsLiteralBausteine) EXPECT_FALSE(v.empty());
}

// ================================================================================================
// A-P7: der g1-Einordnungs-Block -- vier gelabelte Zeilen, jede non-empty, Zeile 1 == Planer-Rolle
// ================================================================================================
TEST(S1StempelBasisVertrag, G1BlockVierZeilenNonEmptyUndPlanerRolle) {
    std::string const block = pfg::g1_binary_version_block("+ext=avx2+cxx=gcc:15.3.0+opt=O3+ceb=9.1");
    std::array<std::string, 4> zeilen{};
    std::size_t                n     = 0;
    std::size_t                start = 0;
    for (std::size_t pos = 0; pos < block.size() && n < zeilen.size(); ++pos) {
        if (block[pos] != '\n') continue;
        zeilen[n++] = block.substr(start, pos - start);
        start       = pos + 1;
    }
    ASSERT_EQ(n, 4u) << "g1-Block traegt nicht genau vier '\\n'-terminierte Zeilen: " << block;
    for (std::string const& z : zeilen) EXPECT_FALSE(z.empty());
    // Zeile 1 == die PLANER-Rolle, transitiv aus dem Erbin-Kompositum (P7-Anker):
    EXPECT_EQ(zeilen[0], pl::planner_version_stamp());
    EXPECT_EQ(zeilen[0], std::string{pl::PlanerStempel::gesamt_stempel()});
    EXPECT_EQ(zeilen[1].rfind("ceb-contract=", 0), 0u);
    EXPECT_EQ(zeilen[2].rfind("build-type=", 0), 0u);
    EXPECT_GT(zeilen[2].size(), string_view{"build-type="}.size()) << "build-type-Label darf nie leer sein";
    EXPECT_EQ(zeilen[3].rfind("build-version=", 0), 0u);
}

// ================================================================================================
// Matrix-Spiegel als Laufzeit-Test (sichtbare Ausgabe der 28-Zellen-Deckung im ctest-Protokoll)
// ================================================================================================
TEST(S1StempelBasisVertrag, MatrixDeckt28ZellenGegenFremdquelle) {
    ASSERT_EQ(kFremdMatrix.size(), 28u);
    std::size_t geprueft = 0;
    for (FremdZelle const& z : kFremdMatrix) {
        EXPECT_EQ(abi::stempel_zulassung(z.i, z.t), z.z)
            << "Zelle (interface=" << static_cast<int>(z.i) << ", traeger=" << static_cast<int>(z.t) << ")";
        ++geprueft;
    }
    EXPECT_EQ(geprueft, 28u);
}

} // namespace
