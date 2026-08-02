// measurement/operating_system_axis.hpp -- operating_system als CEB-System-Haupt-Achse
// (O-8 Schritt 4 / Paket A3; Auspraegungs-Katalog OP-10, Owner-KERN "GENAU DREI Haupt-Achsen").
//
// ROLLE: die Betriebssystem-FAMILIE, unter der gebaut und gemessen wird. Sie tritt mit A3 in
// kSystemAxisOrder ein -- bis dahin existierte "operating_system" AUSSCHLIESSLICH als String im
// Ordnungs-Header (Kommentar + der static_assert "tritt erst mit Paket A3 ein"), ohne Achsen-Typ.
// Ohne diesen Header ist der Ordnungs-Eintritt nicht baubar: die Emitter-Tabelle des Registry-
// Generators ist TYP-getrieben (kSystemAxisEmitters traegt axis_label() REALER Typen, nie Literale).
//
// KATALOG (OP-10, quellen-gestuetzt aus RF-3-Flotte + 8er-Docker-Doktrin): GENAU DREI Klassen-
// Identitaeten -- linux / windows / macos. Das sind FAMILIEN, keine Distributionen: die reale Flotte
// (Windows 11 + Windows Server 2022, die Linux-Docker-Matrix, 2x macOS) faellt vollstaendig in diese
// drei. Distribution und Version (debian-13/trixie, windows-server-2022, macOS-Version) sind
// AUSDRUECKLICH KEINE Auspraegungen dieser Achse: die Achse traegt die KLASSEN-Identitaet.
//
// WO DIE INSTANZ WOHNT (A14/OS-U1, Owner-Entscheid E3 vom 02.08.2026): in den DREI Unter-Achsen
// os_version / kernel / build (operating_system_sub_axes.hpp; "build" traegt den Update-Zustand
// mit -- Owner E-12: "Wir mergen den Update Zustand in Build"). Sie sind stage="runtime" mit
// option_source="machine_resolved": die Werte werden je Maschine erhoben, die <machines>-Deklaration
// (O-4b, Schritt 5) ist die ERWARTUNG dafuer, nicht die Quelle. Das ist die systematische Form dessen,
// was OP-10 als "STEMPEL-VARIABLE" (analog der BUILD-Version nach Ledger 69.2/70.6) beschrieben hat --
// kein Widerspruch zu OP-10, sondern seine Typ-Fassung.
// Wer eine Distribution als vierte Auspraegung DIESER Achse einfuegen will, hat den Schnitt weiterhin
// missverstanden: die Familie ist die Klasse, os_version die Instanz.
//
// STEMPEL-NEUTRAL (A-15 + Bump-Verbots-Wache): die drei Unter-Achsen sind RT-Unter-Achsen und stehen deshalb
// NIE im Binary-Stempel. Weder ihre Einfuehrung noch ihre Werte duerfen
// abi::kSystemAxisCodeVersions["operating_system"] bumpen -- ein Bump verschoebe den SHA512 aller
// Neubauten gegen den Bestand und erzwaenge exakt den Neubau, den Owner-E3 ausschliesst.
//
// WIRKUNG: NUR Stempel-Identitaet (RF-6-Linie, wie target_isa): gleiche Kombination => gleicher
// Stempel. KEINE Laufzeit-/Dispatch-Wirkung, KEIN Eintrag in der binary_id (binary_id="never" --
// System-Achsen permutieren die Organ-Komposition nicht).
// Metaprog: CRTP + Concept, static-dispatch, keine vtable (Muster TargetIsaSystemAxis).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// operating_system -- eigene Haupt-System-Achse (Betriebssystem-FAMILIE). Jede Auspraegung ist eine
/// Klassen-Identitaet; Distro/Version bleiben Stempel-Variablen (siehe Kopf).
template <class Derived>
struct OperatingSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "operating_system"; }

    /// Deklarative Familien-Kennung (Ordner-/Sidecar-/Stempel-Etikett): "linux" | "windows" | "macos".
    [[nodiscard]] static constexpr std::string_view os_family_id() noexcept { return Derived::do_os_family_id(); }

    /// POSIX-Familie? Rein deklarativ (Stempel-/Doku-Merkmal), KEIN Dispatch-Schalter: der Bau waehlt
    /// nichts anhand dieses Flags aus -- es macht die Familien-Verwandtschaft im Design-Space sichtbar.
    [[nodiscard]] static constexpr bool is_posix_family() noexcept { return Derived::do_is_posix_family(); }

protected:
    constexpr OperatingSystemAxis() noexcept = default;
};

template <class A>
concept OperatingSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, OperatingSystemAxis<A>> &&
    std::is_empty_v<OperatingSystemAxis<A>> && (!std::is_polymorphic_v<OperatingSystemAxis<A>>) && requires {
        { A::os_family_id() } -> std::same_as<std::string_view>;
        { A::is_posix_family() } -> std::same_as<bool>;
    };

// -- Die drei Auspraegungen (OP-10). Jede eine leere CRTP-Struct (Design-Space-Vokabular); der Planer
//    bzw. die XML waehlt, nichts ist gepinnt. --

/// linux -- die Docker-Matrix-Familie (8er-Docker-Doktrin) und der Default der Bau-/Mess-Flotte.
struct LinuxOperatingSystem final : OperatingSystemAxis<LinuxOperatingSystem> {
    [[nodiscard]] static constexpr std::string_view do_os_family_id() noexcept { return "linux"; }
    [[nodiscard]] static constexpr bool             do_is_posix_family() noexcept { return true; }
};

/// windows -- deckt Windows 11 UND Windows Server 2022 (beides Instanzen derselben Klassen-Identitaet;
/// die Unterscheidung traegt die Stempel-Variable, nicht die Achse).
struct WindowsOperatingSystem final : OperatingSystemAxis<WindowsOperatingSystem> {
    [[nodiscard]] static constexpr std::string_view do_os_family_id() noexcept { return "windows"; }
    [[nodiscard]] static constexpr bool             do_is_posix_family() noexcept { return false; }
};

/// macos -- die beiden macOS-Maschinen der Flotte.
struct MacosOperatingSystem final : OperatingSystemAxis<MacosOperatingSystem> {
    [[nodiscard]] static constexpr std::string_view do_os_family_id() noexcept { return "macos"; }
    [[nodiscard]] static constexpr bool             do_is_posix_family() noexcept { return true; }
};

/// CEB-Default = linux (die Bau-/Mess-Flotte laeuft heute dort) -- beweglicher Startwert, KEIN Pin.
using DefaultOperatingSystem = LinuxOperatingSystem;

/// Single-Source der gueltigen os_family_ids (analog kAllTargetIsaIds/kAllSimdIds).
inline constexpr std::array<std::string_view, 3> kAllOperatingSystemIds = {
    LinuxOperatingSystem::os_family_id(), WindowsOperatingSystem::os_family_id(), MacosOperatingSystem::os_family_id()};

static_assert(OperatingSystemAxisConcept<LinuxOperatingSystem>);
static_assert(OperatingSystemAxisConcept<WindowsOperatingSystem>);
static_assert(OperatingSystemAxisConcept<MacosOperatingSystem>);
static_assert(LinuxOperatingSystem::axis_label() == std::string_view{"operating_system"});
// Katalog-Anker (OP-10): GENAU DREI Familien. Eine vierte Auspraegung bricht hier compile-time -- das
// ist der Ort, an dem die Distro-als-Achse-Verwechslung auffaellt. Die Distribution ist seit A14/OS-U1
// die Unter-Achse os_version (operating_system_sub_axes.hpp), NIE eine vierte Familie hier. Erst eine
// echte vierte FAMILIE (z.B. bsd) waere ein bewusstes CT-Ereignis an dieser Zeile.
static_assert(kAllOperatingSystemIds.size() == 3);
static_assert(DefaultOperatingSystem::os_family_id() == std::string_view{"linux"});
static_assert(LinuxOperatingSystem::is_posix_family() && MacosOperatingSystem::is_posix_family() &&
              !WindowsOperatingSystem::is_posix_family());
// Abgrenzung: operating_system ist eine EIGENE Haupt-Achse, nicht die Ziel-ISA und nicht der Host-Deskriptor.
static_assert(LinuxOperatingSystem::do_axis_label() != std::string_view{"target_isa"});
static_assert(LinuxOperatingSystem::do_axis_label() != std::string_view{"hardware"});

} // namespace comdare::cache_engine::measurement
