#pragma once
// measurement/numa_page_probe_linux.hpp -- prozessfreie Linux-Erhebung fuer OD-10-RT.
//
// WAS: Diese Datei bindet die Linux-Spezialisierung der CT-Factory an genau drei Quellen:
//   <numa_node_root>/online              -> die Node-Ids (Kernel-Listen-Grammatik "0-3,7"),
//   sysconf(_SC_PAGESIZE)                -> die Basis-Seitengroesse,
//   <hugepage_root>/hugepages-<KiB>kB/   -> die vom Kernel gefuehrten Huge-Seiten samt nr_hugepages.
// Die Wurzeln kommen ausschliesslich aus NumaPageProbeContext; dadurch ist der gesamte Datei-Teil ohne
// Zugriff auf die Live-Maschine testbar.
//
// K2 PROZESS-FREI: nur Datei-Reads und sysconf. Die sysfs-Auswertung ist reine C++-Logik -- kein
// numactl, kein lscpu, kein hwloc, keine Shell. libnuma bleibt bewusst draussen (siehe Kopf des
// Public Headers): sie waere eine zweite Wissensquelle neben sysfs, und ihr Fehlen waere ein
// Link-Fehler statt eines klassifizierten Befunds.
//
// WARUM DIE PARSER NICHT GEGUARDET SIND (die tragende Test-Entscheidung): der komplette sysfs-Teil ist
// portables std::filesystem plus String-Auswertung. Er steht deshalb AUSSERHALB von #if defined(__linux__)
// und ist auf jeder Bau-Plattform aufrufbar. Nur die Basis-Seitengroesse braucht die Familien-
// Schnittstelle sysconf -- also genau ein Syscall, nicht der halbe Header. Damit sind die tragenden
// Tests (Listen-Grammatik, Namens-Kontrakt, Pool-Zahl, Komposition, alle K4-Negativ-Proben)
// hardware-FREI: sie laufen gegen einen Fixture-Baum im Test-Tempdir, nicht gegen die Maschine.
//
// K4 FEHLERKLASSEN, und warum die Grenze genau hier liegt:
//   QuelleFehlt      -- Verzeichnis/Datei existiert nicht. Fuer den NUMA-Baum ist das ein Befund (ein
//                       Kernel ohne CONFIG_NUMA bietet die Achse nicht an), fuer den HUGEPAGE-Baum ist
//                       es der erwartete Normalfall "keine Huge-Seiten" -- dort bleibt die Basis-Seite
//                       eine VOLLSTAENDIGE Antwort, kein Fehler. Der Unterschied ist ausdruecklich, weil
//                       die beiden Baeume verschiedene Dinge bedeuten.
//   QuelleUnlesbar   -- existiert, aber kein Zugriff (oder ein Verzeichnis-Iterationsfehler).
//   QuelleKorrupt    -- vorhanden und lesbar, aber inhaltlich unbrauchbar: leer, absteigend, doppelt,
//                       ueber dem Plausibilitaets-Deckel, Huge-Seite nicht groesser als die Basis-Seite.
//   FormatUnbekannt  -- die Kennung passt zu keinem unterstuetzten Format: ein Fremdzeichen in der
//                       Listen-Grammatik, ein Verzeichnisname ohne den hugepages-<KiB>kB-Kontrakt.
//                       NIE interpretieren, nie raten.
// Nur ein gescheitertes sysconf erzeugt den D1-Befund BetriebssystemFeatureFehlt.
//
// 'N/A STATT NULL': kein Parser gibt je eine leere Menge als Erfolg zurueck, und keiner setzt einen
// Ersatzwert. Eine Quelle liefert entweder eine nichtleere, geordnete Werte-Menge oder eine benannte
// Fehlerklasse.
//
// A-15 STEMPEL-NEUTRAL: die erhobenen Zahlen verlassen diesen Blatt-Header nur als RT-Ergebnis. Diese
// Datei kennt keinen Registry-, ABI- oder Stempel-Header und schreibt kein Byte in diese Pfade.

#include <cache_engine/measurement/numa_page_probe.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

[[nodiscard]] inline std::string_view numa_page_trim(std::string_view value) noexcept {
    auto const is_space = [](char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

/// Liest eine sysfs-Datei vollstaendig. Die Trennung existiert/lesbar ist die A4-Kernaussage: eine
/// FEHLENDE Quelle ist ein anderer Zustand als eine vorhandene, auf die kein Zugriff besteht.
[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
numa_page_read_file(std::filesystem::path const& path) {
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

/// Konsumiert eine Dezimalzahl. Getrennte Fehlerklassen mit Absicht: ein NICHT-Ziffern-Anfang ist ein
/// FORMAT-Bruch (die Grammatik passt nicht), ein Ueberlauf ist ein INHALTS-Bruch (die Grammatik passt,
/// die Zahl ist Muell). Der Deckel-Vergleich bleibt beim Aufrufer, weil er je Feld verschieden ist.
[[nodiscard]] inline std::expected<std::uint64_t, HardwareProbeErrorClass>
numa_page_take_uint(std::string_view& text) noexcept {
    if (text.empty() || text.front() < '0' || text.front() > '9')
        return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

    std::uint64_t value = 0;
    while (!text.empty() && text.front() >= '0' && text.front() <= '9') {
        auto const digit = static_cast<std::uint64_t>(text.front() - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        value = value * 10U + digit;
        text.remove_prefix(1);
    }
    return value;
}

/// Parst die KERNEL-LISTEN-GRAMMATIK ("0", "0-3", "0-1,4,6-7") in eine aufsteigende, duplikatfreie
/// Id-Menge. Der Kernel emittiert diese Liste immer aufsteigend und ohne Ueberlappung; jede Abweichung
/// ist deshalb ein Befund (QuelleKorrupt) und kein zu tolerierender Stil.
///
/// WARUM SO STRENG: die Node-Ids sind die WERTE-MENGE einer Achse. Eine tolerant sortierte oder still
/// deduplizierte Liste wuerde aus einer kaputten Quelle eine plausible Achse machen -- genau die
/// Klasse Fehler, gegen die die K4-Trennung gebaut ist.
[[nodiscard]] inline std::expected<NumaNodeSet, HardwareProbeErrorClass> numa_page_parse_id_list(std::string_view raw) {
    std::string_view text = numa_page_trim(raw);
    if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    // Fremdzeichen sind ein FORMAT-Bruch: diese Datei ist nicht die Kernel-Liste, die wir lesen.
    for (char const c : text)
        if ((c < '0' || c > '9') && c != '-' && c != ',')
            return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

    NumaNodeSet   nodes;
    bool          first = true;
    std::uint64_t last  = 0;
    while (true) {
        auto begin = numa_page_take_uint(text);
        if (!begin.has_value()) return std::unexpected(begin.error());
        std::uint64_t end = *begin;
        if (!text.empty() && text.front() == '-') {
            text.remove_prefix(1);
            auto parsed_end = numa_page_take_uint(text);
            if (!parsed_end.has_value()) return std::unexpected(parsed_end.error());
            end = *parsed_end;
        }
        if (end < *begin) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // absteigend
        if (end > kMaxNumaNodeId) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (!first && *begin <= last) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // Ueberlappung
        if (nodes.size() + (end - *begin + 1U) > kMaxNumaNodeCount)
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

        for (std::uint64_t id = *begin; id <= end; ++id) nodes.push_back(static_cast<std::uint32_t>(id));
        first = false;
        last  = end;

        if (text.empty()) break;
        if (text.front() != ',') return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
        text.remove_prefix(1);
        if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // Komma am Ende
    }
    if (nodes.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return nodes;
}

/// Parst den NAMENS-KONTRAKT eines Hugepage-Knotens ("hugepages-2048kB" -> 2 MiB in Bytes). Ein Name
/// ohne diesen Kontrakt ist FormatUnbekannt -- er wird nicht geraten und nicht uebersprungen: ein
/// stilles Ueberspringen wuerde eine unbekannte Groesse aus der Freigabe-Menge verschwinden lassen.
[[nodiscard]] inline std::expected<std::uint64_t, HardwareProbeErrorClass>
numa_page_parse_hugepage_dir_name(std::string_view name) {
    if (!name.starts_with(kHugepageDirPrefix) || !name.ends_with(kHugepageDirSuffix))
        return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

    std::string_view digits =
        name.substr(kHugepageDirPrefix.size(), name.size() - kHugepageDirPrefix.size() - kHugepageDirSuffix.size());
    auto kib = numa_page_take_uint(digits);
    if (!kib.has_value()) return std::unexpected(kib.error());
    if (!digits.empty()) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt); // Rest hinter der Zahl
    if (*kib == 0) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (*kib > kMaxPageSizeBytes / 1024U) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    std::uint64_t const bytes = *kib * 1024U;
    // Seitengroessen sind Zweierpotenzen. Ein anderer Wert ist keine Seite, sondern ein kaputter Knoten.
    if ((bytes & (bytes - 1U)) != 0U) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return bytes;
}

/// Parst den Pool-Zaehler nr_hugepages. 0 ist ein GUELTIGER Wert (Groesse gefuehrt, kein Pool) -- genau
/// diese Null traegt hier Bedeutung und ist deshalb kein Fehlerfall, sondern die Stufe NurUnterstuetzt.
[[nodiscard]] inline std::expected<std::uint64_t, HardwareProbeErrorClass>
numa_page_parse_pool_count(std::string_view raw) {
    std::string_view text = numa_page_trim(raw);
    if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    auto value = numa_page_take_uint(text);
    if (!value.has_value()) return std::unexpected(value.error());
    if (!text.empty()) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    if (*value > kMaxHugepagePoolCount) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return value;
}

/// Liest die ONLINE-Knotenliste. PORTABEL (kein Plattform-Guard): reiner Datei-Zugriff.
[[nodiscard]] inline std::expected<NumaNodeSet, HardwareProbeErrorClass>
numa_page_read_online_nodes(NumaPageProbeContext const& ctx) {
    if (ctx.numa_node_root.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);
    auto text = numa_page_read_file(ctx.numa_node_root / std::string{kNumaOnlineLeafName});
    if (!text.has_value()) return std::unexpected(text.error());
    return numa_page_parse_id_list(*text);
}

/// EIN roher Hugepage-Knoten, bevor er gegen die Basis-Seite geprueft wird.
struct HugepageNode {
    std::uint64_t size_bytes = 0;
    std::uint64_t pool_count = 0;
};

/// Liest die Hugepage-Knoten. PORTABEL (kein Plattform-Guard): reiner Verzeichnis-/Datei-Zugriff.
///
/// FEHLENDE WURZEL IST KEIN FEHLER: ein Kernel ohne Hugepage-Unterstuetzung bietet diesen Baum nicht
/// an, und die Antwort "nur die Basis-Seite" ist dann VOLLSTAENDIG. Eine VORHANDENE, aber nicht
/// iterierbare Wurzel bleibt dagegen QuelleUnlesbar -- sonst verschwaende ein Rechte-Problem als
/// "keine Huge-Seiten". Die Namen werden gesammelt und SORTIERT, bevor sie geparst werden: die
/// Verzeichnis-Reihenfolge des Dateisystems ist nicht deterministisch, das Ergebnis muss es sein.
[[nodiscard]] inline std::expected<std::vector<HugepageNode>, HardwareProbeErrorClass>
numa_page_read_hugepage_nodes(NumaPageProbeContext const& ctx) {
    std::vector<HugepageNode> nodes;
    if (ctx.hugepage_root.empty()) return nodes;

    std::error_code ec;
    bool const      exists = std::filesystem::exists(ctx.hugepage_root, ec);
    if (ec) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    if (!exists) return nodes; // erwarteter Normalfall: keine Huge-Seiten auf dieser Maschine

    std::vector<std::string>            names;
    std::filesystem::directory_iterator it(ctx.hugepage_root, ec);
    if (ec) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    for (auto const& entry : it) {
        if (names.size() >= kMaxNumaNodeCount) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        names.push_back(entry.path().filename().string());
    }
    std::sort(names.begin(), names.end());

    for (auto const& name : names) {
        auto size_bytes = numa_page_parse_hugepage_dir_name(name);
        if (!size_bytes.has_value()) return std::unexpected(size_bytes.error());
        auto text = numa_page_read_file(ctx.hugepage_root / name / std::string{kHugepagePoolLeafName});
        if (!text.has_value()) return std::unexpected(text.error());
        auto pool = numa_page_parse_pool_count(*text);
        if (!pool.has_value()) return std::unexpected(pool.error());
        nodes.push_back(HugepageNode{*size_bytes, *pool});
    }
    return nodes;
}

/// Setzt die FREIGABE-MENGE aus Basis-Seite und Huge-Knoten zusammen. PORTABEL und ohne IO -- das ist
/// die Stelle, an der die Plausibilitaet der Gesamt-Menge entschieden wird, und sie ist deshalb
/// hardware-frei testbar.
///
/// Regeln, jede mit einem realen Grund:
///   - Basis 0 oder keine Zweierpotenz -> QuelleKorrupt (eine 0-Byte-Seite gibt es nicht),
///   - Huge-Seite <= Basis-Seite       -> QuelleKorrupt (sie waere dann keine Huge-Seite),
///   - doppelte Groesse                -> QuelleKorrupt (zwei Knoten fuer dieselbe Seite),
///   - Pool > 0 -> PoolBelegt, sonst NurUnterstuetzt (die Unterscheidung, die die Achse traegt).
/// Das Ergebnis ist aufsteigend sortiert und beginnt IMMER mit der Basis-Seite.
[[nodiscard]] inline std::expected<PageCapabilitySet, HardwareProbeErrorClass>
numa_page_compose_capabilities(std::uint64_t base_page_bytes, std::vector<HugepageNode> nodes) {
    if (base_page_bytes == 0 || base_page_bytes > kMaxPageSizeBytes)
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if ((base_page_bytes & (base_page_bytes - 1U)) != 0U)
        return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    std::sort(nodes.begin(), nodes.end(),
              [](HugepageNode const& a, HugepageNode const& b) noexcept { return a.size_bytes < b.size_bytes; });

    PageCapabilitySet result;
    result.push_back(PageSizeCapability{base_page_bytes, PageSizeAvailability::Basis});
    for (auto const& node : nodes) {
        if (node.size_bytes <= base_page_bytes) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (node.size_bytes == result.back().size_bytes) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        result.push_back(PageSizeCapability{node.size_bytes, node.pool_count > 0U
                                                                 ? PageSizeAvailability::PoolBelegt
                                                                 : PageSizeAvailability::NurUnterstuetzt});
    }
    return result;
}

/// Die Seitengroessen-Achse in einem Zug: Basis-Seite (vom Aufrufer, weil nur sie einen Syscall
/// braucht) plus die sysfs-Knoten. PORTABEL.
[[nodiscard]] inline PageCapabilitySetResult numa_page_collect_pages(NumaPageProbeContext const& ctx,
                                                                     std::uint64_t               base_page_bytes) {
    auto nodes = numa_page_read_hugepage_nodes(ctx);
    if (!nodes.has_value()) return std::unexpected(NumaPageProbeError{nodes.error()});
    auto composed = numa_page_compose_capabilities(base_page_bytes, std::move(*nodes));
    if (!composed.has_value()) return std::unexpected(NumaPageProbeError{composed.error()});
    return *composed;
}

/// Die Knoten-Achse in einem Zug. PORTABEL.
[[nodiscard]] inline NumaNodeSetResult numa_page_collect_nodes(NumaPageProbeContext const& ctx) {
    auto nodes = numa_page_read_online_nodes(ctx);
    if (!nodes.has_value()) return std::unexpected(NumaPageProbeError{nodes.error()});
    return *nodes;
}

} // namespace detail

[[nodiscard]] inline NumaPageTopology NumaPageProbe<LinuxOperatingSystem>::collect(NumaPageProbeContext const& ctx) {
#if defined(__linux__)
    long const raw_page_size = ::sysconf(_SC_PAGESIZE);
    if (raw_page_size <= 0) {
        // Die Familien-Schnittstelle selbst hat versagt -- das ist ein D1-Befund und KEIN Datei-Zugang.
        // Der Knoten-Baum bleibt davon unberuehrt und wird trotzdem gelesen.
        return NumaPageTopology{
            detail::numa_page_collect_nodes(ctx),
            std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt})};
    }
    return NumaPageTopology{detail::numa_page_collect_nodes(ctx),
                            detail::numa_page_collect_pages(ctx, static_cast<std::uint64_t>(raw_page_size))};
#else
    static_cast<void>(ctx);
    return detail::numa_page_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
