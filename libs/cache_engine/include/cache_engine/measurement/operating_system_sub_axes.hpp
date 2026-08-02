// measurement/operating_system_sub_axes.hpp -- die FINALEN DREI Unter-System-Achsen der Haupt-Achse
// operating_system (Paket A14 / OS-U1; Owner-Entscheid E3 vom 02.08.2026).
//
// OWNER-DECKUNG (E3, verbatim): "Das Paket ist JETZT Pflicht, ansonsten muessen alle Binaries bei
// Einfuehrung neu gebaut werden. Es ist die Basis des Systems, um mit den heute von infra installierten
// anderen OS die Bandbreite an binaries und Messungen zu erhoehen." Die drei Namen stehen fest seit
// A-08 (Konsolidierungs-Anker K-04, Konflikt "4 vs 3" aufgeloest) und E-12 (Owner verbatim: "Wir mergen
// den Update Zustand in Build") -- os_version, kernel, build. update_zustand ist damit KEINE vierte
// Unter-Achse, sondern Bestandteil von build.
//
// -- WAS DIE DREI SIND ---------------------------------------------------------------------------
//   os_version -- die Distributions-/Release-Identitaet der OS-INSTANZ innerhalb ihrer Familie
//                 (linux: ID+VERSION_ID aus /etc/os-release; windows: Produktlinie; macos: Produktversion).
//   kernel     -- das Kernel-Release der Instanz (linux/macos: uname-Release; windows: NT-Kernel-Tripel).
//   build      -- der Build-/Patch-Stand INKLUSIVE Update-Zustand (E-12-Merge).
// Die HAUPT-Achse traegt weiterhin ausschliesslich die KLASSEN-Identitaet (drei Familien, OP-10); diese
// drei Unter-Achsen tragen die INSTANZ. Das ist die systematische Form dessen, was OP-10 als
// "Stempel-Variablen" beschrieben hat -- keine Gegenrede zu OP-10, sondern seine Typ-Fassung.
//
// -- CT/RT-SCHNITT (Kanon Haupt=CT / Unter=RT-Draht, stufen-relativ) -----------------------------
// Wie numa_node/page am target_isa-Komplex haben diese drei KEINEN compile-statischen Options-Katalog:
// die zulaessigen Werte haengen an der konkreten Maschine und sind zur Bauzeit nicht erkennbar. Sie
// deklarieren deshalb eine option_source ("machine_resolved") statt einer Options-Liste.
// Die LAUFZEIT-ERHEBUNG der Werte ist AUSDRUECKLICH nicht Teil dieses Headers (Paket OS-U3): hier
// entsteht nur die statische Einhaengung, also das ANGEBOT.
//
// -- STEMPEL-NEUTRALITAET (A-15 + Auflage K5) ----------------------------------------------------
// A-15 gilt hart: RT-Unter-Achsen stehen NIE im Binary-Stempel. Dieser Header aendert deshalb WEDER
// die Stempel-Zeilen NOCH den SHA512 NOCH den build_version-Suffix -- er ist rein additiv.
// AUFLAGE K5 (bindende Wache, Bauplan A14): abi::kSystemAxisCodeVersions["operating_system"] darf durch
// die Einhaengung dieser Unter-Achsen NICHT gebumpt werden. Der in die Binary kompilierte Achsen-CODE
// aendert sich nicht; ein Bump wuerde system_stamp_line und damit den SHA512 ALLER Neubauten gegen den
// Bestand verschieben -- das Skip-Gate verfehlte die Alt-Binaries, also genau die Folge, die Owner-E3
// ausschliesst ("ansonsten muessen alle Binaries ... neu gebaut werden").
//
// -- NAMENSFALLE "build" vs "build_version" ------------------------------------------------------
// Die Unter-Achse heisst "build" (OS-Patch-/Update-Stand der INSTANZ). Sie ist NICHT der
// build_version-Suffix (Ledger 70.6: die Stempel-Variable-Version der System-Achsen), NICHT der
// build_type ("+bt="-Suffix-Segment) und NICHT die build_target_complex-Klammer. Die static_asserts am
// Dateiende halten das fest -- ohne sie entstuende hier die naechste Drift-Klasse.
//
// Metaprog: CRTP + Concept, static-dispatch, keine vtable (Muster TargetIsaSubAxis).

#pragma once

#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// Familien-Repraesentant der ACHSE operating_system fuer Typ-Eltern-Bezuege
/// (CebSubAxis<..., OperatingSystemAxisTag>). KEINE Auspraegung und kein Baustein: er traegt
/// ausschliesslich das Achsen-Label, damit sich die Unter-Achsen auf die ACHSE beziehen koennen statt
/// auf eine willkuerlich herausgegriffene Familie. Das Label wird NICHT wiederholt, sondern aus der
/// bestehenden Single-Source gezogen (Muster TargetIsaAxisTag).
struct OperatingSystemAxisTag final : CebSystemAxis<OperatingSystemAxisTag> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept {
        return LinuxOperatingSystem::axis_label();
    }
};

/// Gemeinsame Schicht der operating_system-Unter-Achsen: Eltern-Typ fest, Options-Herkunft deklariert.
template <class Derived>
struct OperatingSystemSubAxis : CebSubAxis<Derived, OperatingSystemAxisTag> {
    /// Woher die zulaessigen Werte kommen (statt einer CT-Options-Liste). Reines Deklarations-Merkmal.
    [[nodiscard]] static constexpr std::string_view option_source() noexcept { return Derived::do_option_source(); }

protected:
    constexpr OperatingSystemSubAxis() noexcept = default;
};

template <class A>
concept OperatingSystemSubAxisConcept =
    CebSubAxisConcept<A> && std::derived_from<A, OperatingSystemSubAxis<A>> && requires {
        { A::option_source() } -> std::same_as<std::string_view>;
    };

/// os_version -- Distributions-/Release-Identitaet der OS-Instanz INNERHALB ihrer Familie.
/// Die Familie selbst bleibt die Haupt-Achse; diese Achse unterscheidet debian-13 von ubuntu-24.04.
struct OsVersionSubAxis final : OperatingSystemSubAxis<OsVersionSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "os_version"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// kernel -- das Kernel-Release der Instanz. Eigene Achse, weil dieselbe os_version mehrere Kernel
/// fahren kann (und der Kernel die Mess-Relevanz traegt, nicht die Distributions-Marke).
struct KernelSubAxis final : OperatingSystemSubAxis<KernelSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "kernel"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// build -- Build-/Patch-Stand INKLUSIVE Update-Zustand (Owner E-12: "Wir mergen den Update Zustand in
/// Build"). Siehe Namensfallen-Block im Kopf: das ist NICHT der build_version-Suffix.
struct BuildSubAxis final : OperatingSystemSubAxis<BuildSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "build"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// Single-Source der Unter-Achsen-Labels an der Haupt-Achse operating_system. Der Registry-Generator
/// und jeder Spalten-/Dateinamen-Erzeuger ziehen ihre Liste HIER, nie aus Literalen.
inline constexpr std::array<std::string_view, 3> kOperatingSystemSubAxisLabels = {
    OsVersionSubAxis::axis_label(), KernelSubAxis::axis_label(), BuildSubAxis::axis_label()};

namespace detail {

/// FINAL-DREI-Hilfe: die Labels muessen paarweise verschieden sein -- zwei gleiche Labels waeren zwei
/// XML-Zeilen mit derselben id und damit eine still ueberschriebene Unter-Achse.
[[nodiscard]] consteval bool operating_system_sub_axis_labels_are_distinct() {
    for (std::size_t i = 0; i < kOperatingSystemSubAxisLabels.size(); ++i) {
        if (kOperatingSystemSubAxisLabels[i].empty()) return false;
        for (std::size_t j = i + 1; j < kOperatingSystemSubAxisLabels.size(); ++j)
            if (kOperatingSystemSubAxisLabels[i] == kOperatingSystemSubAxisLabels[j]) return false;
    }
    return true;
}

/// NAMENSFALLEN-Katalog: Etiketten, die hier NIE als Unter-Achsen-Label auftauchen duerfen. Jeder
/// Eintrag steht fuer eine reale Verwechslung, nicht fuer eine hypothetische:
///   update_zustand/update_status -- die vierte Achse, die E-12 in "build" gemergt hat;
///   build_version/build_type/build_target_complex -- die drei Bau-Begriffe, mit denen sich "build"
///       kreuzen wuerde (Ledger 70.6-Stempel-Variable, "+bt="-Suffix-Segment, aeussere Komplex-Klammer);
///   os/operating_system/distribution -- die Familie bzw. die Achse selbst; eine Unter-Achse, die so
///       heisst, macht die Instanz zur Klasse und bricht den OP-10-Schnitt.
inline constexpr std::array<std::string_view, 9> kOperatingSystemSubAxisForbiddenLabels = {
    "update_zustand", "update_status",    "build_version", "build_type", "build_target_complex", "os",
    "distribution",   "operating_system", "os_family"};

[[nodiscard]] consteval bool operating_system_sub_axis_labels_avoid_traps() {
    for (auto const& label : kOperatingSystemSubAxisLabels)
        for (auto const& forbidden : kOperatingSystemSubAxisForbiddenLabels)
            if (label == forbidden) return false;
    return true;
}

} // namespace detail

static_assert(OperatingSystemSubAxisConcept<OsVersionSubAxis>);
static_assert(OperatingSystemSubAxisConcept<KernelSubAxis>);
static_assert(OperatingSystemSubAxisConcept<BuildSubAxis>);

// EINHAENGUNGS-WACHE: alle drei haengen wirklich an operating_system, und zwar ueber den TYP. Das Label
// ist ABGELEITET (CebSubAxis), nicht wiederholt -- ein Rename der Haupt-Achse zieht hier automatisch mit.
static_assert(OsVersionSubAxis::parent_axis_label() == std::string_view{"operating_system"});
static_assert(KernelSubAxis::parent_axis_label() == std::string_view{"operating_system"});
static_assert(BuildSubAxis::parent_axis_label() == std::string_view{"operating_system"});
static_assert(std::same_as<OsVersionSubAxis::parent_axis, OperatingSystemAxisTag>);
static_assert(std::same_as<KernelSubAxis::parent_axis, OperatingSystemAxisTag>);
static_assert(std::same_as<BuildSubAxis::parent_axis, OperatingSystemAxisTag>);
// Der Tag traegt exakt das Label der Haupt-Achse (er ist ihr Repraesentant, keine zweite Achse).
static_assert(OperatingSystemAxisTag::axis_label() == LinuxOperatingSystem::axis_label());
static_assert(OperatingSystemAxisTag::axis_label() == std::string_view{"operating_system"});
// Tiefe 1 = direkte Unter-Achse (offene Rekursion aus ceb_sub_axis.hpp, hier als Gegenprobe).
static_assert(axis_depth_v<OsVersionSubAxis> == 1 && axis_depth_v<KernelSubAxis> == 1 &&
              axis_depth_v<BuildSubAxis> == 1);

// FINAL-DREI-WACHE (A-08 / K-04 / Owner E-12). Eine VIERTE Unter-Achse bricht hier compile-time -- das
// ist der Ort, an dem die zurueckgekehrte update_zustand-Achse auffaellt.
static_assert(kOperatingSystemSubAxisLabels.size() == 3,
              "A-08/K-04 + Owner E-12: operating_system hat GENAU DREI Unter-Achsen (os_version, kernel, "
              "build). Der Update-Zustand ist in 'build' GEMERGT ('Wir mergen den Update Zustand in "
              "Build') und ist AUSDRUECKLICH keine vierte Achse. Wer hier eine vierte eintraegt, hebt "
              "einen Owner-Entscheid auf und muss das quittieren, statt die Zahl nachzuziehen.");
static_assert(detail::operating_system_sub_axis_labels_are_distinct(),
              "Zwei operating_system-Unter-Achsen tragen dasselbe Label -- in der Registry-XML waeren das "
              "zwei <sub_axis>-Zeilen mit derselben id, von denen jeder Leser eine still verliert.");

// NAMENSFALLEN-WACHE (Kopf-Block): "build" ist der OS-Patch-Stand, NICHT der build_version-Suffix
// (Ledger 70.6), NICHT der build_type ("+bt=") und NICHT die build_target_complex-Klammer; "os_version"
// ist die Instanz, NICHT die Familie.
static_assert(detail::operating_system_sub_axis_labels_avoid_traps(),
              "Ein operating_system-Unter-Achsen-Label kollidiert mit einem Begriff aus dem Verbots-"
              "Katalog (kOperatingSystemSubAxisForbiddenLabels): update_zustand/update_status sind in "
              "'build' gemergt (E-12), build_version/build_type/build_target_complex sind BAU-Begriffe "
              "und os/distribution/operating_system/os_family sind die Familie, nicht die Instanz.");
static_assert(BuildSubAxis::axis_label() != std::string_view{"build_version"});
static_assert(BuildSubAxis::axis_label() != std::string_view{"build_type"});
static_assert(OsVersionSubAxis::axis_label() != LinuxOperatingSystem::axis_label());
static_assert(OsVersionSubAxis::axis_label() != std::string_view{"os"});

// ABGRENZUNGS-WACHE zu den target_isa-Unter-Achsen: dieselbe Mechanik, ANDERE Eltern-Achse. Die Wache
// steht hier und nicht drueben, weil dieser Header der neue ist.
static_assert(OperatingSystemAxisTag::axis_label() != std::string_view{"target_isa"});
static_assert(OperatingSystemAxisTag::axis_label() != std::string_view{"external_utils"});

// OPTIONS-HERKUNFT: alle drei sind maschinen-aufgeloest (kein CT-Options-Katalog). Der Wert ist das
// Etikett, das die Registry-XML traegt -- er ist an DIESE Konstante gekoppelt, nicht an ein XML-Literal.
static_assert(OsVersionSubAxis::option_source() == std::string_view{"machine_resolved"});
static_assert(KernelSubAxis::option_source() == std::string_view{"machine_resolved"});
static_assert(BuildSubAxis::option_source() == std::string_view{"machine_resolved"});

} // namespace comdare::cache_engine::measurement
