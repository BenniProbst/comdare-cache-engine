#pragma once
// measurement/operating_system_probe_windows.hpp -- prozessfreie Windows-Erhebung fuer OS-U3.
//
// WAS: Die Windows-Spezialisierung liest die echte NT-Version mit RtlGetVersion aus ntdll. Major und
// Minor belegen os_version, das Major.Minor.Build-Tripel belegt kernel, und die Build-Nummer belegt
// build. Ein vorhandener Service-Pack-Stand wird aus demselben EX-Traeger angehaengt; dafuer ist keine
// zweite Schnittstelle erforderlich.
//
// K2 PROZESS-FREI: Der Zugriff bleibt im laufenden Prozess und benutzt genau die ntdll-Schnittstelle.
// Die manifestabhaengige Legacy-Versions-API wird absichtlich nicht verwendet.
//
// K4 FEHLERKLASSEN: Fehlt ntdll/RtlGetVersion auf der Bauplattform oder lehnt die Schnittstelle den
// Aufruf ab, entsteht ausschliesslich BetriebssystemFeatureFehlt. Ein erfolgreich gelesener, aber
// unbrauchbarer Null-Stand ist dagegen QuelleKorrupt: die Schnittstelle war vorhanden, nur ihr Inhalt
// traegt kein Ergebnis. Es gibt in dieser Zelle keinen erfundenen Datei-Zugangsfehler.
//
// A-15 STEMPEL-NEUTRAL: Die gelesenen Zahlen werden nur in den RT-Traeger geschrieben. Dieser
// Blatt-Header kennt keinen ABI-, Registry- oder Stempel-Pfad.

#include <cache_engine/measurement/operating_system_probe.hpp>

#include <expected>
#include <string>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#include <winternl.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

#if defined(_WIN32)
/// ABI-Traeger des von RtlGetVersion akzeptierten RTL_OSVERSIONINFOEXW-Layouts. Einige User-Mode-SDKs
/// deklarieren nur RTL_OSVERSIONINFOW, obwohl ntdll den verlaengerten Size-Kontrakt unterstuetzt. Die
/// lokale, feldgleiche Form haelt den Service-Pack-Zugriff deshalb SDK-portabel, ohne eine Zusatz-API.
struct WindowsRtlOsVersionInfoExW {
    ULONG  dwOSVersionInfoSize;
    ULONG  dwMajorVersion;
    ULONG  dwMinorVersion;
    ULONG  dwBuildNumber;
    ULONG  dwPlatformId;
    WCHAR  szCSDVersion[128];
    USHORT wServicePackMajor;
    USHORT wServicePackMinor;
    USHORT wSuiteMask;
    UCHAR  wProductType;
    UCHAR  wReserved;
};

static_assert(std::is_standard_layout_v<WindowsRtlOsVersionInfoExW>);
static_assert(sizeof(WindowsRtlOsVersionInfoExW) == sizeof(OSVERSIONINFOEXW),
              "Der lokale RTL_OSVERSIONINFOEXW-Traeger muss das dokumentierte EX-Layout behalten.");

using WindowsRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
#endif

} // namespace detail

[[nodiscard]] inline std::expected<OperatingSystemInstance, OsProbeError>
OperatingSystemProbe<WindowsOperatingSystem>::collect(OperatingSystemProbeContext const& ctx) {
#if defined(_WIN32)
    static_cast<void>(ctx);
    HMODULE const ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    FARPROC const raw_rtl_get_version = ::GetProcAddress(ntdll, "RtlGetVersion");
    if (raw_rtl_get_version == nullptr)
        return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});

    auto const rtl_get_version = reinterpret_cast<detail::WindowsRtlGetVersion>(raw_rtl_get_version);
    detail::WindowsRtlOsVersionInfoExW info{};
    info.dwOSVersionInfoSize = static_cast<ULONG>(sizeof(info));
    LONG const status        = rtl_get_version(reinterpret_cast<PRTL_OSVERSIONINFOW>(&info));
    if (status < 0) return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    if (info.dwMajorVersion == 0U || info.dwBuildNumber == 0U)
        return std::unexpected(OsProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    std::string const major        = std::to_string(info.dwMajorVersion);
    std::string const minor        = std::to_string(info.dwMinorVersion);
    std::string const build_number = std::to_string(info.dwBuildNumber);
    std::string       build        = build_number;
    if (info.wServicePackMajor != 0U || info.wServicePackMinor != 0U) {
        build += "-sp";
        build += std::to_string(info.wServicePackMajor);
        build.push_back('.');
        build += std::to_string(info.wServicePackMinor);
    }

    return OperatingSystemInstance{major + '.' + minor, major + '.' + minor + '.' + build_number, std::move(build)};
#else
    static_cast<void>(ctx);
    return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
#endif
}

} // namespace comdare::cache_engine::measurement
