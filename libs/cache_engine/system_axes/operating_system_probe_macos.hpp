#pragma once
// system_axes/operating_system_probe_macos.hpp -- prozessfreie macOS-Erhebung fuer OS-U3.
//
// WAS: kern.osrelease liefert den Darwin-Kernel, SystemVersion.plist liefert ProductVersion und
// ProductBuildVersion. Fehlt nur der Build-Schluessel in einer sonst gueltigen plist, liefert
// kern.osversion den dokumentierten Build-Fallback.
//
// K2 PROZESS-FREI: Die native Zelle benutzt ausschliesslich sysctlbyname und einen Datei-Read. Der
// kleine plist-Leser wertet XML-Daten direkt aus und startet kein externes Werkzeug.
//
// K4 FEHLERKLASSEN: Datei-Zugang und Datei-Inhalt bleiben in HardwareProbeErrorClass. Eine auf dieser
// Bauplattform fehlende macOS-Schnittstelle oder ein fehlgeschlagenes sysctlbyname erzeugt dagegen
// BetriebssystemFeatureFehlt. ProductVersion bleibt Pflicht und wird nie aus Darwin-Daten geraten:
// Fehlt die ganze plist, bleibt deshalb QuelleFehlt. kern.osversion kann nur den Build ersetzen, wenn
// ProductVersion bereits aus einer lesbaren, gueltigen plist belegt ist.
//
// A-15 STEMPEL-NEUTRAL: Dieser Blatt-Header erzeugt nur OperatingSystemInstance-Strings zur Laufzeit
// und kennt keinen ABI-, Registry- oder Stempel-Pfad.

#include <system_axes/operating_system_probe.hpp>

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

[[nodiscard]] inline std::string_view macos_os_probe_trim(std::string_view value) noexcept {
    auto const is_space = [](char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
macos_os_probe_read_file(std::filesystem::path const& path) {
    if (path.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

    std::error_code ec;
    bool const      exists = std::filesystem::exists(path, ec);
    if (ec) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    if (!exists) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

    std::ifstream input(path, std::ios::binary);
    if (!input) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    std::string const text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    return text;
}

enum class MacosPlistValueState { Missing, Found, Malformed };

struct MacosPlistValue {
    MacosPlistValueState state = MacosPlistValueState::Missing;
    std::string          value;
};

[[nodiscard]] inline bool macos_os_probe_value_is_wellformed(std::string_view value) noexcept {
    if (value.empty()) return false;
    for (char const c : value) {
        unsigned char const byte = static_cast<unsigned char>(c);
        if (byte < 0x21U || byte > 0x7eU || c == '<' || c == '>' || c == '\'' || c == '"') return false;
    }
    return true;
}

[[nodiscard]] inline MacosPlistValue macos_os_probe_plist_value(std::string_view text, std::string_view wanted_key) {
    std::size_t     search = 0;
    MacosPlistValue result{};
    while (true) {
        std::size_t const key_open = text.find("<key>", search);
        if (key_open == std::string_view::npos) break;
        std::size_t const key_close = text.find("</key>", key_open + 5);
        if (key_close == std::string_view::npos) return {MacosPlistValueState::Malformed, {}};

        std::string_view const key = macos_os_probe_trim(text.substr(key_open + 5, key_close - (key_open + 5)));
        search                     = key_close + 6;
        if (key != wanted_key) continue;
        if (result.state == MacosPlistValueState::Found) return {MacosPlistValueState::Malformed, {}};

        std::size_t const value_open = text.find("<string>", search);
        if (value_open == std::string_view::npos) return {MacosPlistValueState::Malformed, {}};
        // Ein naechster Key vor dem String bedeutet: dieser Key hat keinen String-Wert. Ein spaeterer
        // Wert darf ihm nicht faelschlich zugeordnet werden.
        std::size_t const next_key = text.find("<key>", search);
        if (next_key != std::string_view::npos && next_key < value_open) return {MacosPlistValueState::Malformed, {}};
        std::size_t const value_close = text.find("</string>", value_open + 8);
        if (value_close == std::string_view::npos) return {MacosPlistValueState::Malformed, {}};

        std::string_view const value = macos_os_probe_trim(text.substr(value_open + 8, value_close - (value_open + 8)));
        if (!macos_os_probe_value_is_wellformed(value)) return {MacosPlistValueState::Malformed, {}};
        result = {MacosPlistValueState::Found, std::string{value}};
        search = value_close + 9;
    }
    return result;
}

struct MacosSystemVersion {
    std::string     product_version;
    MacosPlistValue product_build;
};

[[nodiscard]] inline std::expected<MacosSystemVersion, HardwareProbeErrorClass>
macos_os_probe_parse_system_version(std::string_view text) {
    if (macos_os_probe_trim(text).empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (text.find("<plist") == std::string_view::npos || text.find("<dict") == std::string_view::npos)
        return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    if (text.find("</dict>") == std::string_view::npos || text.find("</plist>") == std::string_view::npos)
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    MacosPlistValue product_version = macos_os_probe_plist_value(text, "ProductVersion");
    if (product_version.state != MacosPlistValueState::Found)
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    MacosPlistValue product_build = macos_os_probe_plist_value(text, "ProductBuildVersion");
    if (product_build.state == MacosPlistValueState::Malformed)
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return MacosSystemVersion{std::move(product_version.value), std::move(product_build)};
}

#if defined(__APPLE__)
[[nodiscard]] inline std::expected<std::string, OsProbeError> macos_os_probe_sysctl_string(char const* name) {
    std::size_t size = 0;
    if (::sysctlbyname(name, nullptr, &size, nullptr, 0) != 0)
        return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    if (size == 0) return std::unexpected(OsProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    std::string value(size, '\0');
    if (::sysctlbyname(name, value.data(), &size, nullptr, 0) != 0)
        return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    if (size < value.size()) value.resize(size);
    while (!value.empty() && value.back() == '\0') value.pop_back();
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
    if (value.empty() || value.find('\0') != std::string::npos)
        return std::unexpected(OsProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    return value;
}
#endif

} // namespace detail

[[nodiscard]] inline std::expected<OperatingSystemInstance, OsProbeError>
OperatingSystemProbe<MacosOperatingSystem>::collect(OperatingSystemProbeContext const& ctx) {
#if defined(__APPLE__)
    auto kernel = detail::macos_os_probe_sysctl_string("kern.osrelease");
    if (!kernel.has_value()) return std::unexpected(kernel.error());

    auto text = detail::macos_os_probe_read_file(ctx.macos_system_version_plist_path);
    if (!text.has_value()) return std::unexpected(OsProbeError{text.error()});
    auto system_version = detail::macos_os_probe_parse_system_version(*text);
    if (!system_version.has_value()) return std::unexpected(OsProbeError{system_version.error()});

    std::string build;
    if (system_version->product_build.state == detail::MacosPlistValueState::Found) {
        build = std::move(system_version->product_build.value);
    } else {
        auto fallback = detail::macos_os_probe_sysctl_string("kern.osversion");
        if (!fallback.has_value()) return std::unexpected(fallback.error());
        build = std::move(*fallback);
    }
    return OperatingSystemInstance{std::move(system_version->product_version), std::move(*kernel), std::move(build)};
#else
    static_cast<void>(ctx);
    return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
#endif
}

} // namespace comdare::cache_engine::measurement
