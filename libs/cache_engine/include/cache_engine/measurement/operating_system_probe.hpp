// measurement/operating_system_probe.hpp -- prozess-freie RT-Erhebung der OS-Unter-Achsen (OS-U3).
//
// WAS ERHOBEN WIRD (FINAL DREI aus A14 / Owner-E-12):
//   linux   -- os_version = "<ID>-<VERSION_ID>" aus os-release (ohne VERSION_ID nur "<ID>"),
//              kernel = utsname.release, build = utsname.version inklusive Build-/Patch-/Update-Stand.
//   macos   -- os_version = ProductVersion aus SystemVersion.plist, kernel = kern.osrelease,
//              build = ProductBuildVersion; fehlt nur dieses Feld in einer sonst gueltigen plist,
//              dient kern.osversion als Build-Fallback. Eine fehlende plist bleibt QuelleFehlt, weil
//              keine erlaubte zweite Quelle den dann ebenfalls fehlenden ProductVersion-Wert ersetzt.
//   windows -- os_version = "<Major>.<Minor>", kernel = "<Major>.<Minor>.<Build>",
//              build = "<Build>" zuzueglich eines ohne Zusatz-API erreichbaren Service-Pack-Standes.
// Die drei std::string-Werte beschreiben die laufende OS-INSTANZ. Die Familie bleibt eine CT-Achse und
// wird ausschliesslich durch OperatingSystemProbe<OsAxis> gewaehlt -- nie durch Runtime-enum, switch
// oder std::variant.
//
// K2 PROZESS-FREI: die Detail-Header verwenden nur Familien-Schnittstellen und Datei-Reads: uname(2)
// plus os-release unter Linux, sysctlbyname plus SystemVersion.plist unter macOS und RtlGetVersion aus
// ntdll unter Windows. Externe Programme, Shells und Prozess-Erzeugung sind kein Teil dieser API.
//
// K4 FEHLER-EINORDNUNG (#29): HardwareProbeErrorClass heisst historisch "Hardware", bezeichnet im
// Framework aber den ZUGANG zu einer Quelle. Genau deshalb reisen Datei fehlt / unlesbar / korrupt /
// unbekanntes Format auch hier in dieser bestehenden Domaene. Eine fehlende Datei ist kein Urteil
// ueber die Maschine und darf nie CompilerCompilerErrorClass werden. Bewusst entsteht kein neues Enum:
// die vier vorhandenen Unterscheidungen sind exakt die benoetigten, und axis_error.hpp wird parallel
// von FK-1/FK-2 bearbeitet. Nur eine fehlende FAMILIEN-SCHNITTSTELLE oder ein gescheiterter Familien-
// Syscall erzeugt CompilerCompilerErrorClass::BetriebssystemFeatureFehlt. OsProbeError spiegelt dabei
// exakt BuildError: eine typisierte std::variant-Summe an der Expected/Result-Naht. Diese ausdrueckliche
// variant-Ausnahme ist Fehler-Transport, niemals Familien-Dispatch; die Familie bleibt CT-gewaehlt.
//
// K5 VERSIONIERTE VERFAHRENS-ID: probe_id() ist eine Eigenschaft des Probe-TYPS, kein frei setzbares
// Feld des Runtime-Ergebnisses. Aendert sich eine Quelle oder die Zusammensetzung der Werte, muss die
// Version steigen, damit Messungen aus verschiedenen Erhebungs-Verfahren unterscheidbar bleiben. Der
// Familien-Teil wird aus OsAxis::os_family_id() in einen constexpr-Puffer komponiert und nirgendwo als
// zweites Literal gepflegt. Das ist dieselbe Verfahrens-Identitaet wie device_id() der HW-Erhebung.
// kOsProbeVersion war eine weitere Migrations-Naht des A13-M2/M3-Fensters: A13-M3/C4 hat sie zusammen
// mit dem uebrigen Bestand auf "v1.0.0c" gezogen (deklariertes probe_id-Byte-Ereignis), und die gated
// ENFORCE-Wache ist seither scharf. Seit A13-M3/C2 fuehrt die zentrale Naht-Liste sie als KLASSE (f)
// (algo_semver.hpp) -- vorher stand die Naht nur hier und fiel durch jedes der dort als bindend
// deklarierten Erhebungs-Kommandos (Befund GA-05/Z-10 vom 03.08.2026).
//
// A-15 STEMPEL-NEUTRALITAET: OperatingSystemInstance ist ein reines Runtime-Blatt. Dieser Header kennt
// weder ABI-Stempel noch Registry, XML, Generator oder Binary-ID und bietet keine Naht dorthin. Kein
// erhobener Wert und auch keine probe_id wird in den Binary-Stempel geschrieben. Die System-Achsen-
// Code-Version "operating_system" bleibt davon unberuehrt.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace comdare::cache_engine::measurement {

/// Die drei FINALEN Runtime-Werte der laufenden OS-Instanz. probe_id() bleibt bewusst am Probe-Typ:
/// dort ist die Verfahrens-Identitaet CT-gebunden und kann nicht als frei veraenderbares Ergebnis-Feld
/// von der tatsaechlich ausgefuehrten Spezialisierung abweichen.
struct OperatingSystemInstance {
    std::string os_version;
    std::string kernel;
    std::string build;
};

/// K4: exakt die Form von BuildError -- eine typisierte Summe an der Expected/Result-Naht. Die erste
/// Alternative klassifiziert den Quellen-ZUGANG, die zweite ausschliesslich die fehlende OS-Faehigkeit.
using OsProbeError = std::variant<HardwareProbeErrorClass, CompilerCompilerErrorClass>;

[[nodiscard]] constexpr ErrorDomain error_domain(OsProbeError const& error) noexcept {
    return std::visit([](auto const value) noexcept { return error_domain(value); }, error);
}

/// Das bestehende stabile Etikett der jeweils getragenen #29-Domaene, ohne ein drittes Vokabular zu
/// erfinden. Insbesondere bleibt QuelleFehlt "quelle_fehlt" und der L6-Produzent meldet das bereits
/// zementierte "betriebssystem_feature_fehlt".
[[nodiscard]] constexpr std::string_view os_probe_error_label(OsProbeError const& error) noexcept {
    return std::visit(
        [](auto const value) noexcept -> std::string_view {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, HardwareProbeErrorClass>)
                return hardware_probe_label(value);
            else
                return error_class_label(value);
        },
        error);
}

using OperatingSystemProbeResult = std::expected<OperatingSystemInstance, OsProbeError>;

/// Injizierbare, BESITZENDE OS-Handles. std::filesystem::path statt string_view verhindert, dass ein
/// Test einen temporaeren Pfad-String eintraegt, dessen Speicher vor collect() bereits abgelaufen ist.
/// Die Linux-Fallback-Reihenfolge bleibt durch zwei getrennte Felder sichtbar; macOS bekommt dieselbe
/// testbare Datei-Naht. Windows braucht fuer RtlGetVersion keinen Pfad, teilt aber denselben Kontext-Typ.
struct OperatingSystemProbeContext {
    std::filesystem::path os_release_path;
    std::filesystem::path os_release_fallback_path;
    std::filesystem::path macos_system_version_plist_path;
};

/// Die echten OS-Handles an genau einer Stelle. Tests ersetzen sie durch Wegwerf-Pfade; die Probe-
/// Spezialisierungen selbst enthalten dadurch keine versteckten, nicht injizierbaren Datei-Konstanten.
[[nodiscard]] inline OperatingSystemProbeContext default_operating_system_probe_context() {
    return {std::filesystem::path{"/etc/os-release"}, std::filesystem::path{"/usr/lib/os-release"},
            std::filesystem::path{"/System/Library/CoreServices/SystemVersion.plist"}};
}

namespace detail {

inline constexpr std::string_view kOsProbeIdPrefix = "os_probe.";
inline constexpr std::string_view kOsProbeVersion  = "v1.0.0c";

/// Ein fester constexpr-Puffer statt std::string: probe_id() muss auch compile-time aus dem von der
/// Achse gelieferten string_view komponierbar sein. 64 Bytes lassen bewusst Platz fuer kuenftige echte
/// Familien-IDs, ohne die heutige ID-Laenge als zweite Wissensquelle einzufrieren.
struct OsProbeIdBuffer final {
    std::array<char, 64> bytes{};
    std::size_t          length = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return {bytes.data(), length}; }
};

[[nodiscard]] consteval OsProbeIdBuffer compose_os_probe_id(std::string_view family,
                                                            std::string_view version) noexcept {
    OsProbeIdBuffer   result{};
    std::size_t const required = kOsProbeIdPrefix.size() + family.size() + 1U + version.size();
    if (required > result.bytes.size()) return result;

    auto append = [&result](std::string_view part) {
        for (char const c : part) result.bytes[result.length++] = c;
    };
    append(kOsProbeIdPrefix);
    append(family);
    result.bytes[result.length++] = '@';
    append(version);
    return result;
}

[[nodiscard]] constexpr std::size_t os_probe_at_count(std::string_view id) noexcept {
    std::size_t count = 0;
    for (char const c : id)
        if (c == '@') ++count;
    return count;
}

[[nodiscard]] constexpr std::string_view os_probe_family_part(std::string_view id) noexcept {
    if (!id.starts_with(kOsProbeIdPrefix)) return {};
    std::size_t const at = id.find('@', kOsProbeIdPrefix.size());
    if (at == std::string_view::npos) return {};
    return id.substr(kOsProbeIdPrefix.size(), at - kOsProbeIdPrefix.size());
}

} // namespace detail

/// K5-Helfer: schneidet den rohen vX.Y.Z-Teil hinter '@' aus der komponierten Verfahrens-ID. Die
/// separate genau-ein-'@'-Wache unten verhindert, dass ein mehrdeutiger Rest akzeptiert wird.
[[nodiscard]] constexpr std::string_view os_probe_version_part(std::string_view id) noexcept {
    std::size_t const at = id.find('@');
    if (at == std::string_view::npos) return {};
    return id.substr(at + 1U);
}

/// TOTALITAETS-WACHE: bewusst nur deklariert. Eine neue OS-Achsen-Auspraegung ohne ausdrueckliche
/// Probe-Entscheidung bleibt ein unvollstaendiger Typ und kann nicht still auf eine Familie fallen.
template <class OsAxis>
struct OperatingSystemProbe;

template <>
struct OperatingSystemProbe<LinuxOperatingSystem> {
    using os_axis = LinuxOperatingSystem;

    inline static constexpr detail::OsProbeIdBuffer kProbeId =
        detail::compose_os_probe_id(os_axis::os_family_id(), detail::kOsProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(__linux__)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(__linux__)
        return {};
#else
        return "uname_und_os_release_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static OperatingSystemProbeResult collect(OperatingSystemProbeContext const& context);
};

template <>
struct OperatingSystemProbe<WindowsOperatingSystem> {
    using os_axis = WindowsOperatingSystem;

    inline static constexpr detail::OsProbeIdBuffer kProbeId =
        detail::compose_os_probe_id(os_axis::os_family_id(), detail::kOsProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(_WIN32)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(_WIN32)
        return {};
#else
        return "rtl_get_version_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static OperatingSystemProbeResult collect(OperatingSystemProbeContext const& context);
};

template <>
struct OperatingSystemProbe<MacosOperatingSystem> {
    using os_axis = MacosOperatingSystem;

    inline static constexpr detail::OsProbeIdBuffer kProbeId =
        detail::compose_os_probe_id(os_axis::os_family_id(), detail::kOsProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(__APPLE__)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(__APPLE__)
        return {};
#else
        return "sysctl_und_system_version_plist_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static OperatingSystemProbeResult collect(OperatingSystemProbeContext const& context);
};

/// Der Vertrag jeder entschiedenen Familien-Zelle. os_axis bindet die Wahl als TYP; collect liefert
/// genau einen vollstaendigen Runtime-Traeger oder einen klassifizierten K4-Fehler, nie einen Ersatzwert.
template <class Probe>
concept OperatingSystemProbeConcept = requires(OperatingSystemProbeContext const& context) {
    typename Probe::os_axis;
    requires OperatingSystemAxisConcept<typename Probe::os_axis>;
    requires std::same_as<Probe, OperatingSystemProbe<typename Probe::os_axis>>;
    { Probe::probe_id() } -> std::same_as<std::string_view>;
    { Probe::has_native_probe() } -> std::same_as<bool>;
    { Probe::not_implemented_reason() } -> std::same_as<std::string_view>;
    { Probe::collect(context) } -> std::same_as<OperatingSystemProbeResult>;
};

namespace detail {

/// Der gesamte K5-Vertrag in einer CT-Wache: Single-Source-Praefix, die Familie direkt aus dem
/// Achsen-Typ, genau ein Trenner und eine echte, dreistellige, ce-eigene, nicht-experimentelle Version.
template <class OsAxis>
[[nodiscard]] consteval bool os_probe_id_contract_is_satisfied() noexcept {
    std::string_view const id      = OperatingSystemProbe<OsAxis>::probe_id();
    std::string_view const version = os_probe_version_part(id);
    AlgoSemVer const       parsed  = parse_algo_semver(version);
    return id.starts_with(kOsProbeIdPrefix) && os_probe_family_part(id) == OsAxis::os_family_id() &&
           os_probe_at_count(id) == 1U && ce_owned_version_is_wellformed(version) && !parsed.is_sentinel() &&
           !parsed.experimental && version.find('e') == std::string_view::npos;
}

template <class OsAxis>
[[nodiscard]] consteval bool os_probe_version_satisfies_cpu_enforce() noexcept {
    return ce_owned_version_satisfies_cpu_enforce(os_probe_version_part(OperatingSystemProbe<OsAxis>::probe_id()));
}

} // namespace detail

static_assert(OperatingSystemProbeConcept<OperatingSystemProbe<LinuxOperatingSystem>>);
static_assert(OperatingSystemProbeConcept<OperatingSystemProbe<WindowsOperatingSystem>>);
static_assert(OperatingSystemProbeConcept<OperatingSystemProbe<MacosOperatingSystem>>);

static_assert(detail::os_probe_id_contract_is_satisfied<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::os_probe_id_contract_is_satisfied<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::os_probe_id_contract_is_satisfied<MacosOperatingSystem>(),
              "K5: die macOS-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");

#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::os_probe_version_satisfies_cpu_enforce<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::os_probe_version_satisfies_cpu_enforce<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::os_probe_version_satisfies_cpu_enforce<MacosOperatingSystem>(),
              "K5: die macOS-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
#endif

// Paarweise verschieden, alle drei Vergleiche: zwei Familien duerfen nie als dasselbe Verfahren im
// Log oder Messergebnis erscheinen. Die IDs bleiben trotzdem vollstaendig aus os_family_id() abgeleitet.
static_assert(OperatingSystemProbe<LinuxOperatingSystem>::probe_id() !=
                  OperatingSystemProbe<WindowsOperatingSystem>::probe_id() &&
              OperatingSystemProbe<LinuxOperatingSystem>::probe_id() !=
                  OperatingSystemProbe<MacosOperatingSystem>::probe_id() &&
              OperatingSystemProbe<WindowsOperatingSystem>::probe_id() !=
                  OperatingSystemProbe<MacosOperatingSystem>::probe_id());

// Der L6-Produzent ist semantisch und textuell an die bestehende #29-Klasse gebunden; eine Quelle
// verwendet dagegen weiterhin das HardwareProbe-Vokabular des Zugangs-Frameworks.
static_assert(error_domain(OsProbeError{HardwareProbeErrorClass::QuelleFehlt}) == ErrorDomain::HardwareProbe);
static_assert(os_probe_error_label(OsProbeError{HardwareProbeErrorClass::QuelleFehlt}) ==
              std::string_view{"quelle_fehlt"});
static_assert(error_domain(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              ErrorDomain::CompilerCompiler);
static_assert(os_probe_error_label(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              std::string_view{"betriebssystem_feature_fehlt"});

} // namespace comdare::cache_engine::measurement

// Die Definitionen liegen in Plattform-Blaettern. Jedes Blatt ist auf jeder Plattform uebersetzbar
// und inkludiert seinerseits diesen Public Header, sodass auch eine direkte Blatt-Inklusion gueltig ist.
#include <cache_engine/measurement/operating_system_probe_linux.hpp>
#include <cache_engine/measurement/operating_system_probe_macos.hpp>
#include <cache_engine/measurement/operating_system_probe_windows.hpp>
