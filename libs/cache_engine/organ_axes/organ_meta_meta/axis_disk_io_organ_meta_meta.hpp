#pragma once
// organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp -- ORG-19-IO: die ERSTE Organ-Meta-Meta-Achse
// (E-10/#38a2 Stempel-Haelfte + #86; Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19 Schritt 1a, 26.08.2026).
//
// GEGENSTAND: der Typ traegt die stempelnde Identitaet des Organ-Meta-Meta-Glieds "disk_io" (Owner-Wort
// "Festplatten IO"): seine axis_code_version stempelt im Klammer-Anhang der ORGAN-Zeile (Glied [3] des
// Tier-Fingerprints) -- und zwar NUR fuer Compositions mit persistence_target = DiskWritebackTarget
// (Selektion JE COMP via abi/organ_meta_meta_selection.hpp, Entscheid D-1). MemoryOnly-Compositions
// (die gesamte Flotte all_axes_golden) bleiben BYTE-IDENTISCH (leerer Anhang, kein "[]").
//
// KATEGORIE-HOME: organ_axes/organ_meta_meta/ (S-18/#16-Home-Prinzip; Wurzel-Vorbild SimdExternalUtilsFamily
// in system_axes/). TABU-Manifest: +1 Datei (832 -> 833), im Commit deklariert -- Owner-Rahmen KON108-01
// Frage 4 + KON120-02 B1 + Board #86/#133; Invalidieren ist Ziel, Pflicht ist nur der LAUTE Bruch.
//
// LOCK-DEKLARATION (FIX-3, VERIFY-STEMPEL-IDENTITAET 26.08.2026): der Versions-Scan des axis_version.lock
// matcht AUSSCHLIESSLICH das Token 'algo_version' -- diese Datei ist DIGEST-ONLY-Record der Kategorie
// organ_meta_meta. Ein kuenftiger axis_code_version-Bump aendert im Lock NUR den Digest (kein Versions-
// Feld-Tripwire); die Bump-Sichtbarkeit laeuft ueber die Stempel-/golden-Anker (Glied [3]).
//
// LAYER: NUR topics/ (organ_axes -> topics erlaubt; Praezedenz persistence_target/strategy_base). Die
// Bindung Strategie-Typ -> Meta-Meta lebt NICHT hier, sondern in abi/ (D-1: organ_axes darf measurement/
// nicht einbinden -- 0 Treffer 'cache_engine/measurement/' unter organ_axes/ bleibt wahr).

#include <topics/organ_meta_meta_axis.hpp>

#include <array>
#include <string_view>

namespace comdare::cache_engine::organ_meta_meta {

/// ORG-19-IO -- die Organ-Meta-Meta "disk_io". Stempel-Form nach dem gelandeten System-Vorbild
/// "[simd=code@1.0.0.c]": die Organ-Zeile eines IO-Traegers endet auf ";[disk_io=code@1.0.0.c]".
struct DiskIoOrganMetaMeta final : ::comdare::cache_engine::topics::OrganMetaMetaAxis<DiskIoOrganMetaMeta> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "disk_io"; }
    /// Konvention "die Familie spannt ihre gleichnamige Unter-Achse" (external_utils_family_axis-Vorbild).
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "disk_io"; }
    /// Flag-Grammatik v2, CPU-only-Scope 'c'. Wachen: Renderer-Engpass (meta_meta_stamp_suffix.hpp,
    /// wohlgeformt unbedingt + cpu_pflicht flag-gated) + FIX-7(b)-Direkt-Asserts im k1-Test.
    static constexpr std::string_view axis_code_version = "1.0.0.c";

    /// D-5/KN-3: heute GENAU EIN Unter-Achsen-Wert "staging" -- DiskWritebackTarget misst ausschliesslich
    /// die Rueckschreib-VORBEREITUNG (has_device_writeback_path()==false); "device" folgt erst mit dem
    /// echten Geraetepfad (Registry = ANGEBOT, kein Phantom-Wert; No-Bloat V10).
    static constexpr std::array<std::string_view, 1> kSubAxisOptions{{"staging"}};

    /// D-1 Single-Source der Comp-Bindung: Traeger ist der persistence_target-Slot mit GENAU diesem
    /// Strategie-name(). Die Trait-Spezialisierung (abi/organ_meta_meta_selection.hpp) static_asserted
    /// die Gleichheit gegen DiskWritebackTarget::name() -- eine Drift bricht dort compile-hart.
    static constexpr std::string_view                kCarrierAxis = "persistence_target";
    static constexpr std::array<std::string_view, 1> kCarrierValues{{"persistence_disk_writeback"}};

    /// D-7 (KON110-06): Kanal-Token-DEKLARATION am Typ -- ORG-19 wird als eigener Genus-Kanal im
    /// Kanalwerk registriert; der KANAL-BAU faehrt NACH dem Trigger (#90). kanal_id_schema fuehrt keine
    /// Genus-Liste (nur die Hierarchie achse->genus->kategorie), darum ist das Token hier deklariert.
    [[nodiscard]] static constexpr std::string_view genus_kanal_token() noexcept { return "disk_io"; }
};

static_assert(::comdare::cache_engine::topics::OrganMetaMetaAxisConcept<DiskIoOrganMetaMeta>);
static_assert(DiskIoOrganMetaMeta::axis_kind() == ::comdare::cache_engine::topics::AxisKind::organ_meta_meta);
// Realm-ANKER (Muster ProofOrganMetaMeta, test_striktheit_axis_dach_guard): KEINE Organ-HAUPT-Achse --
// ORG-19 permutiert NIE die binary_id (18+1-Schnitt: kCompositionAxisNames bleibt 18).
static_assert(DiskIoOrganMetaMeta::axis_kind() != ::comdare::cache_engine::topics::AxisKind::organ);
static_assert(DiskIoOrganMetaMeta::axis_label() == std::string_view{"disk_io"});

} // namespace comdare::cache_engine::organ_meta_meta
