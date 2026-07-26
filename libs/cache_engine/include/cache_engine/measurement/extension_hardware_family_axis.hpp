// measurement/extension_hardware_family_axis.hpp -- extension_hardware als AKTIVER Familien-Knoten der
// 6. CEB-Konfig-System-Haupt-Achse (GN-1/E-4, Q2-Ruling Option C; Roadmap 2026-07-19).
//
// LUECKE (Register E-4 / Audit G2): seit F-SIMD (2026-07-18) traegt NUR die DEPRECATED-Insel
// extension_hardware_system_axis.hpp das Label "extension_hardware" -- SimdSubAxis::parent_axis_label()
// zeigte damit auf einen VERWAISTEN String (kein aktiver CebSystemAxis mit diesem Label). Dieser Header
// schliesst die Luecke: der aktive Haupt-Achsen-Knoten, ANALOG CompilerSystemAxis (compiler -> gcc|clang),
// symmetrisch fuer gcc/clang (die je-Dialekt-Flag-Reflexion liefern die drangehaengten Unter-Achsen-
// Optionen: SimdSubAxis fuehrt gcc_march_flag/clang_march_flag/msvc_march_flag spiegelbildlich zu
// OptimizationLevelSubAxis). Die DEPRECATED-F-SIMD-Insel wird NICHT reaktiviert -- sie bleibt reiner
// Kontrast (test_striktheit_axis_dach_guard Block F); dieser Knoten ist der EINE aktive Traeger des Labels.
//
// STRUKTUR (Ebenen-Symmetrie compiler/opt_level <-> extension_hardware/simd, F-SIMD):
//   extension_hardware (Haupt-System-Achse, DIESER Knoten)
//     -> Familie {simd (heute), gpu (spaeter, Q2: SIMD->GPU)}  -- je Familie eine dynamische Unter-Achse
//        -> simd-Optionen {no_extension, avx2, avx512}         -- SimdSubAxis (simd_sub_axis.hpp)
//
// Die Familien-Kennung ist zugleich das Unter-Achsen-Label (Konvention "extension_hardware.simd=...";
// Single-Source-Muster wie LoadFrameworkSystemAxis::sub_axis_label()=="workload"). binary_id-NEUTRAL
// (system_config, steht nie in kCompositionAxisNames, golden unberuehrt); der -march-WERT kommt aus der
// Unter-Achsen-Option, der ORT ist die CompileFn-Naht (opt-g-Facade), die PROVENIENZ das H-10-Sidecar.
// Metaprog: CRTP + Concept, static-dispatch, keine vtable (Lehrbuch-Pattern, kein Runtime-Switch).
//
// -- LANE C, PAKET C-3 (Bauplan-v3 IV.2.3, 26.07.2026): DIESER KNOTEN WIRD ZUM HUB ----------------
// Auftrag Abschnitt 1.4: "external_utils (Rename von extension_hardware, Schreibweise FINAL) = KOPF/HUB
// fuer ALLE Meta-Meta-Achsen". Nach Ledger §69.1 (R-G) ist der Hub der Kopf der SYSTEM-Meta-Metas
// (SIMD/AVX, externe HW, spaeter GPU/FPGA/NPU) -- load_framework gehoert NICHT dazu, es ist eine
// Meta-Meta-Haupt-Achse der MESS-Achsen und verlaesst die System-Welt ersatzlos (IV.2.1).
// C-3 ergaenzt diesen Header daher um ZWEI additive Dinge:
//   (1) jede Erweiterungs-Hardware-FAMILIE ist ab jetzt eine SYSTEM-META-META (SystemMetaMetaAxis-
//       Schicht) -- also eine volle CT-Haupt-Achse mit eigener RT-Unter-Achse, nicht bloss ein
//       Familien-Etikett;
//   (2) den MANAGER ExtensionHardwareHub (Command-Pattern-Invoker), der die Glieder verwaltet und
//       freigibt, waehrend jedes Glied eine eigenstaendige Instanz bleibt.
// BYTE-NEUTRAL: alle bestehenden Member, Labels und static_asserts bleiben unveraendert. Der Hub-Typ
// emittiert KEIN Label; der Registry-Generator arbeitet mit hartkodierten Bloecken
// (tools/system_axis_registry_gen/main.cpp:215-242) und sieht ihn nicht -- die Roundtrip-Wache
// (registry_roundtrip.cmake:53-55) bleibt gruen (literal belegt: regenerierte XML byte-identisch).
// NAME: nach dem A2-Rename heisst der Kopf external_utils. Bis dahin bleibt das Label byte-stabil
// "extension_hardware"; C-3 nimmt den Rename NICHT vorweg (er gehoert Lane A, Paket A2, O-8-Fenster).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/hardware_meta_meta_axis.hpp> // C-1': System-Meta-Meta-Typ-Familie + Hub-Mechanik
#include <cache_engine/measurement/simd_sub_axis.hpp>           // SimdSubAxis-Familie (die drangehaengte Unter-Achse)

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// extension_hardware -- der aktive Familien-Knoten der Erweiterungs-Hardware-Haupt-Achse. Jede Auspraegung
/// ist eine HARDWARE-FAMILIE (simd heute, gpu spaeter), deren Auspraegungs-Raum die gleichnamige dynamische
/// Unter-Achse traegt. Analog CompilerSystemAxis (Haupt-Knoten, Familien-Kennung je Auspraegung).
///
/// C-3: die Basis ist jetzt SystemMetaMetaAxis statt direkt CebSystemAxis -- eine Erweiterungs-Hardware-
/// Familie IST eine System-Meta-Meta (Auftrag 1.4). SystemMetaMetaAxis leitet seinerseits von
/// CebSystemAxis ab und fuegt nur die leere Marker-Basis hinzu; axis_label(), axis_kind() und alle
/// Concept-Zusagen bleiben identisch, und is_empty_v bleibt wahr (leere Basen sind emptiness-neutral).
template <class Derived>
struct ExtensionHardwareFamilyAxis : SystemMetaMetaAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "extension_hardware"; }

    /// Familien-Kennung (Ordner-/Sidecar-Etikett): "simd" | spaeter "gpu".
    [[nodiscard]] static constexpr std::string_view family_id() noexcept { return Derived::do_family_id(); }

    /// Label der dynamischen Unter-Achse dieser Familie (Single-Source der Serialisierungs-/Permutations-
    /// Konvention "extension_hardware.<sub>=..."; Muster LoadFrameworkSystemAxis::sub_axis_label()).
    /// Per Konvention == family_id (die Familie SPANNT ihre gleichnamige Unter-Achse).
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return Derived::do_family_id(); }

protected:
    constexpr ExtensionHardwareFamilyAxis() noexcept = default;
};

template <class A>
concept ExtensionHardwareFamilyAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, ExtensionHardwareFamilyAxis<A>> &&
    std::is_empty_v<ExtensionHardwareFamilyAxis<A>> && (!std::is_polymorphic_v<ExtensionHardwareFamilyAxis<A>>) &&
    requires {
        { A::family_id() } -> std::same_as<std::string_view>;
        { A::sub_axis_label() } -> std::same_as<std::string_view>;
    };

/// SIMD ist die erste (und aktuell einzige) Erweiterungs-Hardware-Familie (Q2 Option C: SIMD -> GPU).
/// Ihre Auspraegungen {no_extension, avx2, avx512} leben in der SimdSubAxis-Familie (simd_sub_axis.hpp).
struct SimdExtensionHardwareFamily final : ExtensionHardwareFamilyAxis<SimdExtensionHardwareFamily> {
    [[nodiscard]] static constexpr std::string_view do_family_id() noexcept { return "simd"; }
};

/// CEB-Default-Familie -- simd (die einzige angebundene). Beweglicher Startwert, KEIN Pin.
using DefaultExtensionHardwareFamily = SimdExtensionHardwareFamily;

/// Single-Source der gueltigen Familien-Kennungen (Design-Space-Vokabular; gpu folgt mit dem GPU-Increment).
inline constexpr std::array<std::string_view, 1> kAllExtensionHardwareFamilyIds = {
    SimdExtensionHardwareFamily::family_id()};

static_assert(ExtensionHardwareFamilyAxisConcept<SimdExtensionHardwareFamily>);
// TRIPWIRE (C-3): die System-Meta-Metas melden HEUTE noch system_config. topics::AxisKind::
// system_meta_meta existiert seit A1, ist dort aber ausdruecklich additiv und unbenutzt. Die
// Umschaltung ist ein Stempel-/Ordnungs-Vorgang und gehoert in das EINE Byte-Fenster hinter O-8
// (Bauplan IV.4.4) -- sie bricht genau diese Zeile, und das ist der Ort, an dem sie quittiert wird.
static_assert(SimdExtensionHardwareFamily::axis_kind() == topics::AxisKind::system_config);
static_assert(SimdExtensionHardwareFamily::axis_label() == std::string_view{"extension_hardware"});
static_assert(SimdExtensionHardwareFamily::family_id() == std::string_view{"simd"});

// ── GN-1-AUFLOESUNG ("SimdSubAxis dranhaengen"): parent_axis_label() der Unter-Achse zeigt jetzt auf DIESEN
//    aktiven Knoten (nicht mehr auf den verwaisten String der DEPRECATED-Insel). Drift bricht hier. ──
static_assert(SimdNoExtOption::parent_axis_label() == SimdExtensionHardwareFamily::axis_label(),
              "GN-1: SimdSubAxis.parent_axis_label muss auf den aktiven extension_hardware-Knoten aufloesen.");
static_assert(SimdNoExtOption::do_axis_label() == SimdExtensionHardwareFamily::sub_axis_label(),
              "GN-1: die simd-Familie spannt die gleichnamige Unter-Achse (Label-Konvention).");
// Symmetrie gcc/clang (analog CompilerSystemAxis gcc|clang): jede drangehaengte simd-Option reflektiert BEIDE
// Dialekt-Flags compile-time (SimdSubAxisConcept erzwingt gcc_march_flag UND clang_march_flag; hier der Anker).
static_assert(SimdAvx2Option::gcc_march_flag() == SimdAvx2Option::clang_march_flag(),
              "Q3-Symmetrie: gcc/clang teilen die -mavx2-Schreibweise.");
static_assert(SimdAvx512Option::gcc_march_flag() == SimdAvx512Option::clang_march_flag(),
              "Q3-Symmetrie: gcc/clang teilen die -mavx512f-Schreibweise.");

// =================================================================================================
// C-3: DER HUB (Command-Pattern-Manager der SYSTEM-Meta-Meta-Achsen)
// =================================================================================================

/// Der HUB: der KOPF, unter dem ALLE SYSTEM-Meta-Meta-Achsen haengen (Auftrag 1.4, Ledger §69.1).
///
/// COMMAND-PATTERN (GoF), zero-cost: dieser Typ ist der INVOKER/Manager -- er verwaltet die Glieder und
/// gibt sie frei. Jedes Glied ist ein eigenstaendiger, voll ausgestatteter Achsen-TYP (der Command), der
/// seine eigene RT-Unter-Achse spannt und auch ohne den Manager fuer sich allein gueltig ist. Kein Glied
/// wird in den Manager hineinkopiert, keine Achse wird mit einer anderen fusioniert.
///
/// ORDNUNG != BESITZ (Auftrag 1.2): der Hub kennt seine GLIEDER, nicht deren RANG. Die kanonische
/// Achsen-Reihenfolge ist und bleibt kSystemAxisOrder (abi/system_axis_order.hpp, Lane A) -- sie wird hier
/// bewusst NICHT nachgebildet, sonst entstuende die fuenfte Kopie derselben Ordnung (Riss 2).
///
/// IDENTITAET: der Hub hat KEINE eigene. Seine Identitaet IST exakt die Konfiguration seiner Glieder
/// (Owner Q-A: die Komplex-Achse hat nur eine INDIREKTE Identitaet ueber die gewrappten Glieder).
struct ExtensionHardwareHub {
    /// Die Glied-Reihenfolge -- heute genau EIN Glied. load_framework ist hier AUSDRUECKLICH NICHT
    /// enthalten (Ledger §69.1 / R-G: Mess-Realm, nicht System-Hub); gpu/fpga/npu-Familien treten
    /// additiv hier ein, sobald sie als Typ existieren.
    using meta_metas = MetaMetaMembers<SimdExtensionHardwareFamily>;

    /// Verwaltet der Manager dieses Glied? Mengen-PRIMITIVE, ausdruecklich NICHT die Halbordnung:
    /// die Teilmengen-Relation ueber Identitaeten ist und bleibt subsumes_v (meta_meta_identity.hpp).
    template <class M>
    static constexpr bool manages = meta_metas::contains<M>;

    /// FREIGABE (die eigentliche Manager-Rolle, Owner: "Manager verwaltet + gibt frei"): der Hub gibt ein
    /// Glied frei, wenn er es verwaltet UND es ein wohlgeformter System-Meta-Meta-Achsen-Typ ist.
    /// Mehr darf hier NICHT stehen: die maschinen-abhaengige Zulassung ist die Halbordnung
    /// (meta_meta_admission.hpp, delegiert an subsumes_v), die flag-genaue Zulassung bleibt das
    /// Pruef-Dock (simd_build_gate.hpp:102-147) mit unveraendertem Freigabe-Maximum Signatur
    /// GESCHNITTEN Sinnhaftigkeit GESCHNITTEN Route (:141-146).
    ///
    /// PRAEZISIERUNG (Bauplan-v3 D2.1): INERT ist ausschliesslich das SECTION-37-FREIGABE-GATE
    /// (simd_build_gate.hpp), NICHT die SIMD-Achse. Die -m-Flags erreichen den Compiler heute sehr wohl,
    /// aber ueber einen ANDEREN, LEBENDEN Kanal: permutation_codegen_tool.cpp:43-45 simd_flags() ->
    /// :477-483 target_compile_options(perm_<id> ... -mavx2/-mavx512f), plus cmake/isa_features.cmake
    /// AVX2-/AVX512-Zweige. Wer "der Freigabe-Pilot ist inert" liest und daraus "SIMD emittiert nichts"
    /// schliesst, irrt.
    ///
    /// ANTI-DOPPELUNGS-AUFLAGE (build_orchestrator.hpp:457 als Kommentar, hier als Vertrag): der Hub ist
    /// ein FREIGABE-Entscheider, KEINE zweite Flag-Quelle. Wuerde das Gate scharfgeschaltet, emittierte
    /// effective_march_flags() das VOLLE Maximum (auf prod1/filter/Avx512 gemessen: -mgfni
    /// -mavx512bitalg -mavx512vpopcntdq) NEBEN dem einen -mavx512f des Codegen-Kanals -- leere
    /// Schnittmenge, also nicht bloss eine Dopplung, sondern eine ANDERE Flag-Menge und damit anderer
    /// Code. Der Kanal-Merge-Beleg (Paket C-3b) ist deshalb VORBEDINGUNG jeder Scharfschaltung (C-3a,
    /// gegatet auf C-3b + O-4; Owner-Vorab-GO Ledger §69.9) und gehoert NICHT in C-3.
    ///
    /// BYTE-NEUTRALITAET VON C-3 haengt deshalb ausdruecklich NICHT an der Inertheit des Gates, sondern
    /// an einer staerkeren, unabhaengig belegten Aussage: dieser Hub haengt sich an KEINEN der beiden
    /// Flag-Kanaele und an keine Emissions-Naht. Er hat heute NULL Konsumenten.
    template <class M>
    static constexpr bool releases = manages<M> && SystemMetaMetaAxisConcept<M>;

    /// Die Hub-Identitaet im Listen-Vokabular des Aufrufers. Mit MetaMetaSet (meta_meta_identity.hpp)
    /// ergibt das A1s mp_list-Identitaet, auf die subsumes_v direkt anwendbar ist -- ohne dass dieser
    /// Header Boost::mp11 zieht (siehe Boost-Auflage im Kopf von hardware_meta_meta_axis.hpp).
    template <template <class...> class List>
    using identity_as = typename meta_metas::template as<List>;

    /// Compile-time-Iteration ueber die Glieder (Metaprogrammierungs-Interface, reine Fold-Expression).
    template <class Visitor>
    static constexpr void for_each(Visitor&& visitor) {
        for_each_meta_meta(meta_metas{}, static_cast<Visitor&&>(visitor));
    }
};

// -- Hub-Wachen (alles compile-time) --------------------------------------------------------------
// Das Glied ist eine wohlgeformte System-Meta-Meta-Achse -- die eigentliche C-1'-Zusage am echten Typ.
static_assert(SystemMetaMetaAxisConcept<SimdExtensionHardwareFamily>);
// Es bleibt eine EIGENSTAENDIGE Instanz: es erfuellt das volle Achsen-Concept fuer sich allein.
static_assert(CebSystemAxisConcept<SimdExtensionHardwareFamily>);
// Der Manager verwaltet genau seine Glieder -- und nichts sonst.
static_assert(ExtensionHardwareHub::meta_metas::size == 1);
static_assert(ExtensionHardwareHub::manages<SimdExtensionHardwareFamily>);
static_assert(!ExtensionHardwareHub::manages<SimdAvx2Option>, "eine Unter-Achsen-OPTION ist keine Meta-Meta.");
// Freigabe = verwaltet UND wohlgeformt.
static_assert(ExtensionHardwareHub::releases<SimdExtensionHardwareFamily>);
static_assert(!ExtensionHardwareHub::releases<SimdNoExtOption>);
// R-G-TRIPWIRE (Ledger §69.1): der Hub traegt AUSSCHLIESSLICH System-Meta-Metas. Wer load_framework
// oder eine andere Mess-Realm-Achse hier einhaengt, bricht diese Zeile. Die Pruefung laeuft ueber die
// Glied-ANZAHL, damit sie ohne einen #include der Mess-Realm-Achse auskommt (Layer-Trennung).
static_assert(
    std::is_same_v<ExtensionHardwareHub::identity_as<MetaMetaMembers>, MetaMetaMembers<SimdExtensionHardwareFamily>>,
    "R-G (Ledger 69.1): der external_utils-Hub traegt NUR System-Meta-Metas -- load_framework "
    "ist Mess-Realm und darf hier nie erscheinen.");
// Das Glied spannt eine EIGENE RT-Unter-Achse (getrennte Klammerung, Owner Q-A).
static_assert(SimdExtensionHardwareFamily::sub_axis_label() == std::string_view{"simd"});
// Das Glied ist heute ein BLATT (nicht selbst Hub) -- der Hub hat damit Tiefe 1. Die Zahl ist emergent:
// baut jemand spaeter eine Meta-Meta ein, die selbst verwaltet (Q-D: GPU-Cluster am PCIe), waechst sie
// ohne eine einzige Code-Aenderung. KEIN festes drittes Level.
static_assert(hub_depth_v<SimdExtensionHardwareFamily> == 0);
static_assert(hub_depth_v<ExtensionHardwareHub> == 1);
// Solange kein Glied selbst Hub ist, fallen flache und geschachtelte Form zusammen -- das ist der
// SONDERFALL der heutigen Ein-Ebenen-Konfiguration, nicht die Regel (vgl. die Klammerungs-Beweise in
// hardware_meta_meta_axis.hpp, wo beide Formen bewusst auseinanderfallen).
static_assert(transitive_members_t<ExtensionHardwareHub>::size == ExtensionHardwareHub::meta_metas::size);

} // namespace comdare::cache_engine::measurement
