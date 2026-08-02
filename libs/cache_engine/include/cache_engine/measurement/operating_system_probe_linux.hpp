#pragma once
// measurement/operating_system_probe_linux.hpp -- prozessfreie Linux-Erhebung fuer OS-U3.
//
// WAS: Diese Datei bindet die Linux-Spezialisierung der CT-Factory an genau zwei Laufzeit-Quellen:
// uname(2) liefert kernel/build, und os-release liefert ID plus die optionale VERSION_ID fuer
// os_version. Die Pfade kommen ausschliesslich aus OperatingSystemProbeContext; dadurch sind sowohl
// Primaer- als auch Fallback-Quelle ohne Zugriff auf die Live-Maschine testbar.
//
// K2 PROZESS-FREI: Es werden nur ::uname und Datei-Reads benutzt. Die os-release-Auswertung ist reine
// C++-Logik und interpretiert weder eine Shell noch startet sie ein Hilfsprogramm.
//
// K4 FEHLERKLASSEN: Ein fehlender, unlesbarer, kaputter oder fremd formatierter Datei-Inhalt bleibt in
// HardwareProbeErrorClass. Der Enum-Name sagt historisch "Hardware", seine Framework-Domaene meint
// hier bewusst den ZUGANG zur Quelle. Nur eine auf dieser Bauplattform fehlende Linux-Schnittstelle
// oder ein fehlgeschlagenes ::uname erzeugt den D1-Befund BetriebssystemFeatureFehlt. Der Fallback
// greift nur bei QuelleFehlt; sonst wuerde eine vorhandene kaputte Primaerquelle still verschwinden.
//
// A-15 STEMPEL-NEUTRAL: Die drei Strings verlassen diesen Blatt-Header nur als RT-Ergebnis. Diese Datei
// kennt keinen Registry-, ABI- oder Stempel-Header und schreibt kein Byte in diese Pfade.

#include <cache_engine/measurement/operating_system_probe.hpp>

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#if defined(__linux__)
#include <sys/utsname.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

[[nodiscard]] inline std::string_view linux_os_probe_trim(std::string_view value) noexcept {
    auto const is_space = [](char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
linux_os_probe_read_file(std::filesystem::path const& path) {
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

[[nodiscard]] inline bool linux_os_probe_key_is_wellformed(std::string_view key) noexcept {
    if (key.empty()) return false;
    auto const is_upper = [](char c) noexcept { return c >= 'A' && c <= 'Z'; };
    auto const is_digit = [](char c) noexcept { return c >= '0' && c <= '9'; };
    if (!is_upper(key.front()) && key.front() != '_') return false;
    for (char const c : key)
        if (!is_upper(c) && !is_digit(c) && c != '_') return false;
    return true;
}

struct LinuxOsReleaseValue {
    bool        wellformed = false;
    std::string value;
};

/// Dekodiert genau die fuer os-release benoetigte Shell-Teilmenge: nackte Werte sowie ein einzelnes
/// Paar einfacher oder doppelter Quotes. Backslash maskiert im nackten/doppelt gequoteten Wert das
/// Folgezeichen; innerhalb einfacher Quotes bleibt er ein normales Zeichen. Verkettete Shell-Worte
/// oder Substitutionen werden nicht ausgefuehrt -- K2 erlaubt hier ausschliesslich Daten-Auswertung.
[[nodiscard]] inline LinuxOsReleaseValue linux_os_probe_decode_value(std::string_view raw) {
    raw = linux_os_probe_trim(raw);
    if (raw.empty()) return {true, {}};

    char quote = '\0';
    if (raw.front() == '\'' || raw.front() == '"') {
        quote = raw.front();
        if (raw.size() < 2 || raw.back() != quote) return {};
        raw.remove_prefix(1);
        raw.remove_suffix(1);
    }

    std::string decoded;
    decoded.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        char const c = raw[i];
        if (c == '\0' || c == '\n' || c == '\r') return {};
        if (quote == '\0' && (c == '\'' || c == '"')) return {};
        if (quote != '\0' && c == quote) return {};
        if (c == '\\' && quote != '\'') {
            if (++i == raw.size()) return {};
            decoded.push_back(raw[i]);
            continue;
        }
        decoded.push_back(c);
    }
    return {true, std::move(decoded)};
}

[[nodiscard]] inline bool linux_os_probe_component_is_wellformed(std::string_view value) noexcept {
    if (value.empty()) return false;
    for (char const c : value) {
        unsigned char const byte = static_cast<unsigned char>(c);
        if (byte < 0x21U || byte > 0x7eU || c == '\'' || c == '"') return false;
    }
    return true;
}

[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
linux_os_probe_parse_release(std::string_view text) {
    bool        saw_content_line   = false;
    bool        saw_assignment     = false;
    bool        saw_non_assignment = false;
    bool        saw_id             = false;
    bool        saw_version_id     = false;
    std::string id;
    std::string version_id;

    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t const      newline = text.find('\n', pos);
        std::string_view const line    = linux_os_probe_trim(
            text.substr(pos, newline == std::string_view::npos ? std::string_view::npos : newline - pos));

        if (!line.empty() && line.front() != '#') {
            saw_content_line     = true;
            std::size_t const eq = line.find('=');
            if (eq == std::string_view::npos) {
                saw_non_assignment = true;
            } else {
                std::string_view const key = linux_os_probe_trim(line.substr(0, eq));
                if (!linux_os_probe_key_is_wellformed(key)) {
                    saw_non_assignment = true;
                } else {
                    LinuxOsReleaseValue const value = linux_os_probe_decode_value(line.substr(eq + 1));
                    // Eine gueltige KEY=-Zeile mit gebrochener Quote ist erkannt, aber inhaltlich
                    // kaputt. Sie darf nicht als fremdes Format oder gar als fehlende Quelle reisen.
                    if (!value.wellformed) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
                    saw_assignment = true;
                    if (key == "ID") {
                        if (saw_id) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
                        saw_id = true;
                        id     = value.value;
                    } else if (key == "VERSION_ID") {
                        if (saw_version_id) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
                        saw_version_id = true;
                        version_id     = value.value;
                    }
                }
            }
        }

        if (newline == std::string_view::npos) break;
        pos = newline + 1;
    }

    // Leerzeilen und reine Kommentar-Dateien sind vorhanden, aber ohne ein einziges nutzbares Feld.
    if (!saw_content_line) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    // Nicht-Kommentar-Inhalt ohne irgendeine KEY=VALUE-Zeile gehoert nicht zum erwarteten Format.
    if (!saw_assignment && saw_non_assignment) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    // Eine erkannte Datei mit zusaetzlichem kaputtem Inhalt bleibt ein Befund. Der gueltige ID-Wert
    // darf die zweite, nicht interpretierbare Zeile nicht still ueberdecken.
    if (saw_non_assignment) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    // Eine erkannte os-release-Datei ohne das Pflichtfeld ID ist unvollstaendig, nicht "fehlend".
    if (!saw_id || !linux_os_probe_component_is_wellformed(id))
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (saw_version_id && !version_id.empty() && !linux_os_probe_component_is_wellformed(version_id))
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    if (!version_id.empty()) {
        id.push_back('-');
        id += version_id;
    }
    return id;
}

[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
linux_os_probe_version(OperatingSystemProbeContext const& ctx) {
    auto text = linux_os_probe_read_file(ctx.os_release_path);
    if (!text.has_value()) {
        if (text.error() != HardwareProbeErrorClass::QuelleFehlt) return std::unexpected(text.error());
        text = linux_os_probe_read_file(ctx.os_release_fallback_path);
        if (!text.has_value()) return std::unexpected(text.error());
    }
    return linux_os_probe_parse_release(*text);
}

} // namespace detail

[[nodiscard]] inline std::expected<OperatingSystemInstance, OsProbeError>
OperatingSystemProbe<LinuxOperatingSystem>::collect(OperatingSystemProbeContext const& ctx) {
#if defined(__linux__)
    utsname name{};
    if (::uname(&name) < 0)
        return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});

    std::string kernel{name.release};
    std::string build{name.version};
    if (kernel.empty() || build.empty()) return std::unexpected(OsProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    auto os_version = detail::linux_os_probe_version(ctx);
    if (!os_version.has_value()) return std::unexpected(OsProbeError{os_version.error()});
    return OperatingSystemInstance{std::move(*os_version), std::move(kernel), std::move(build)};
#else
    static_cast<void>(ctx);
    return std::unexpected(OsProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
#endif
}

} // namespace comdare::cache_engine::measurement
