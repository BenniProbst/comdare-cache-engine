// measurement/meta_meta_admission.hpp -- die Section-37-Zulassung auf META-META-EBENE
// (Lane C, Paket C-2, Bauplan-v3 D2.9, 26.07.2026).
//
// AUFTRAG D2.9, WORTGETREU: "C-2 NUR ADDITIV: admit_organ_on_machine hat den realen Aufrufer
// build_orchestrator.hpp:460-461; Meta-Meta-Hebung = neue Ueberladung, Alt-Signatur bleibt;
// build_orchestrator.hpp bleibt unberuehrt."
//
// WAS GEHOBEN WIRD: die Zulassungs-FRAGE ist auf beiden Ebenen dieselbe -- "traegt die Maschine, was das
// Programm verlangt?". Auf der FLAG-Ebene beantwortet sie admit_organ_on_machine(organ_required,
// machine_signature) ueber Teilmengen von SimdFeatureFlag (simd_build_gate.hpp:153-159). Auf der
// META-META-Ebene ist die Traegermenge eine TYP-Menge, und die Teilmengen-Relation ist die Halbordnung
// subsumes_v (meta_meta_identity.hpp). Diese Datei fuegt die zweite Ueberladung hinzu -- und NUR die.
//
// KEINE ZWEITE HALBORDNUNG: die Ueberladung delegiert an subsumes_v und rechnet nichts selbst. Es gibt
// genau eine Relation im Projekt; hier steht nur die Uebersetzung ihres Ergebnisses in die gemeinsame
// Fehlerklassen-Sprache (CompilerCompilerErrorClass::HardwareErweiterungFehlt), damit beide Ebenen
// dasselbe Urteil in derselben Form aussprechen.
//
// WARUM EIN EIGENER HEADER (und nicht simd_build_gate.hpp): meta_meta_identity.hpp zieht
// <boost/mp11.hpp>. simd_build_gate.hpp wird von build_orchestrator.hpp UND
// profile_facade/profile_run_facade.cpp gezogen; ein Boost-Include dort erbte dieselbe Hermetik-Luecke,
// die im Hub-Header bereits vermieden wurde (Generator-Target ohne Boost::mp11-Link, CI-3-Praezedenz
// tests/unit/CMakeLists.txt:3476-3477). Als eigener Header ohne Konsument bleibt simd_build_gate.hpp
// BYTE-STABIL und build_orchestrator.hpp UNBERUEHRT (Bauplan D3.4). Wer die Meta-Meta-Zulassung braucht,
// inkludiert diesen Header bewusst -- und dessen Target linkt dann auch Boost::mp11.
//
// INERT: dieser Header hat heute KEINEN Produktions-Konsumenten (nur den unregistrierten Lane-C-Test).
// Die Scharfschaltung ist Paket C-3a und gegatet auf C-3b + O-4 (Owner-Vorab-GO Ledger §69.9).
//
// Metaprog: rein constexpr/Concept, keine vtable, kein Runtime-Switch, kein std::variant.

#pragma once

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/hardware_meta_meta_axis.hpp>
#include <cache_engine/measurement/meta_meta_identity.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp>

#include <optional>

namespace comdare::cache_engine::measurement {

/// Jedes Glied einer Identitaets-Menge MUSS eine wohlgeformte System-Meta-Meta-Achse sein.
///
/// Das ist die HAERTUNG, die A1 offen liess: subsumes_v allein akzeptiert BELIEBIGE Typen als
/// Mengen-Glieder (seine Beweise laufen ueber lokale Platzhalter-Structs). Fuer die Zulassung an einer
/// Bau-Naht ist das zu schwach -- dort darf nur eine echte Achse stehen.
template <class Set>
inline constexpr bool identity_members_are_axes_v = mp::mp_all_of<Set, is_system_meta_meta_axis>::value;

/// Section-37-Zulassung auf META-META-EBENE (die neue Ueberladung; die Flag-Variante in
/// simd_build_gate.hpp:153-159 bleibt unveraendert daneben stehen).
///
/// Semantik identisch zur Flag-Ebene: kein Fehler, wenn die Maschinen-Identitaet die Programm-Identitaet
/// TRAEGT (Programm ist Teilmenge der Maschine); sonst D1 HardwareErweiterungFehlt. Die Basis-Identitaet
/// CpuOnlyIdentity ist die leere Menge und laeuft damit ueberall -- exakt die Owner-Aussage
/// "ALLE Teil-Identitaeten mit kleinerer Hardware-Verwendung sind gueltig; Basis = CPU-only".
///
/// Aufruf: admit_organ_on_machine<MaschinenIdentitaet, ProgrammIdentitaet>()
template <class MachineIdentity, class ProgramIdentity>
    requires identity_members_are_axes_v<MachineIdentity> && identity_members_are_axes_v<ProgramIdentity>
[[nodiscard]] constexpr std::optional<CompilerCompilerErrorClass> admit_organ_on_machine() noexcept {
    if constexpr (subsumes_v<MachineIdentity, ProgramIdentity>)
        return std::nullopt;
    else
        return CompilerCompilerErrorClass::HardwareErweiterungFehlt;
}

// -- Wohlgeformtheit + Aequivalenz zur Flag-Ebene (alles compile-time) ----------------------------

/// Die Basis-Identitaet ist die leere Menge -- sie erfuellt die Achsen-Auflage trivial.
static_assert(identity_members_are_axes_v<CpuOnlyIdentity>);
static_assert(identity_members_are_axes_v<MetaMetaSet<SimdExtensionHardwareFamily>>);
// Gegenprobe: ein Nicht-Achsen-Typ wird als Mengen-Glied ABGELEHNT (die Haertung greift).
static_assert(!identity_members_are_axes_v<MetaMetaSet<SimdAvx2Option>>,
              "Eine Unter-Achsen-Option ist keine Meta-Meta und darf keine Identitaets-Menge bilden.");

// Basis laeuft ueberall: leere Programm-Menge -> immer zugelassen.
static_assert(!admit_organ_on_machine<MetaMetaSet<SimdExtensionHardwareFamily>, CpuOnlyIdentity>().has_value());
static_assert(!admit_organ_on_machine<CpuOnlyIdentity, CpuOnlyIdentity>().has_value());
// Reflexiv: die eigene Identitaet ist immer zugelassen.
static_assert(
    !admit_organ_on_machine<MetaMetaSet<SimdExtensionHardwareFamily>, MetaMetaSet<SimdExtensionHardwareFamily>>()
         .has_value());
// ABWAERTS nein: Hardware nicht vorhanden -> D1 HardwareErweiterungFehlt, dieselbe Klasse wie auf der
// Flag-Ebene. Das ist die Aequivalenz-Aussage der Hebung.
static_assert(admit_organ_on_machine<CpuOnlyIdentity, MetaMetaSet<SimdExtensionHardwareFamily>>() ==
                  CompilerCompilerErrorClass::HardwareErweiterungFehlt,
              "Meta-Meta-Ebene muss dieselbe Fehlerklasse sprechen wie die Flag-Ebene (Section 37).");
// Die Alt-Signatur bleibt unangetastet erreichbar (Ueberladung, keine Ersetzung): leere Anforderung ->
// kein Fehler. Diese Zeile ist die Wache dagegen, dass die Hebung die Flag-Variante verdeckt.
static_assert(
    !admit_organ_on_machine(std::span<SimdFeatureFlag const>{}, std::span<SimdFeatureFlag const>{}).has_value());

} // namespace comdare::cache_engine::measurement
