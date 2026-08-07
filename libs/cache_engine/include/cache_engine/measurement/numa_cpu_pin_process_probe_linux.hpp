#pragma once
// measurement/numa_cpu_pin_process_probe_linux.hpp -- prozessfreie Linux-Erhebung fuer OD-11-RT.
//
// WAS: Diese Datei bindet die Linux-Spezialisierung der CT-Factory an genau vier Quellen:
//   <cpu_root>/online                                  -> die logischen Prozessoren der Maschine,
//   <pmu_root>/cpu_core/cpus + cpu_atom/cpus           -> die HYBRIDE Kern-Klassen-Zuordnung (Intel),
//   <cpu_root>/cpuN/cache/index3/{size,shared_cpu_list}-> die L3-DOMAENEN (AMD-X3D und Verwandte),
//   sched_getaffinity/sched_setaffinity                -> die Zuordnungs-FAEHIGKEIT dieses Threads.
// Die Wurzeln kommen ausschliesslich aus NumaCpuPinProcessProbeContext; dadurch ist der gesamte Datei-Teil
// ohne Zugriff auf die Live-Maschine testbar.
//
// K2 PROZESS-FREI: nur Datei-Reads und Scheduler-Syscalls im eigenen Prozess. Die sysfs-Auswertung ist
// reine C++-Logik -- kein taskset, kein numactl, kein lscpu, kein hwloc, keine Shell. hwloc bleibt
// bewusst draussen (wie libnuma bei OD-10-RT): sie waere eine zweite Wissensquelle neben sysfs, und ihr
// Fehlen waere ein Link-Fehler statt eines klassifizierten Befunds.
//
// WARUM DIE PARSER NICHT GEGUARDET SIND (die tragende Test-Entscheidung, uebernommen aus
// numa_page_probe_linux.hpp): der komplette sysfs-Teil ist portables std::filesystem plus
// String-Auswertung. Er steht deshalb AUSSERHALB von #if defined(__linux__) und ist auf jeder
// Bau-Plattform aufrufbar. Nur die Zuordnungs-Erprobung braucht die Familien-Schnittstelle
// sched_getaffinity/sched_setaffinity -- also genau zwei Syscall-Namen, nicht der halbe Header. Damit
// sind die tragenden Tests (Listen-Grammatik, PMU-Paar, L3-Domaenen, Klassifikations-Kette, alle
// K4-Negativ-Proben) hardware-FREI: sie laufen gegen einen Fixture-Baum im Test-Tempdir und liefern
// auf einer eingespeisten HYBRID-Fassung nachweislich etwas anderes als auf einer uniformen.
//
// DIE DETEKTIONS-KETTE, in dieser Reihenfolge und mit diesem Grund:
//   1. HYBRID-PMU (cpu_core/cpu_atom). Die EINZIGE Quelle, die Kern-Klassen selbst BENENNT -- der
//      Kernel fuehrt zwei getrennte PMUs, weil die Kerne verschiedene Zaehler haben. Beide vorhanden
//      und disjunkt -> HoheLeistung/HoheEffizienz, fertig.
//   2. L3-DOMAENEN. Wenn es keine benannte Klassifikation gibt, ist die naechste MESSBARE (nicht
//      geratene) Ungleichheit die L3-Groesse je Domaene. Genau ZWEI verschiedene Groessen ->
//      GrosserCache/KleinerCache. EINE Groesse -> die Maschine ist in dieser Hinsicht homogen.
//   3. HOMOGEN. Eine Klasse ueber alle Online-Prozessoren -- eine ECHTE Antwort, keine Leere.
// AUSDRUECKLICH NICHT IN DER KETTE (Kopf des Public Headers): acpi_cppc/highest_perf und cpu_capacity.
// Beide liefern einen RANG je Kern, keine KLASSE; die Umrechnung braeuchte einen Schwellwert, und ein
// Schwellwert ist geraten. Auf prod1 traegt EINE L3-Domaene acht verschiedene CPPC-Raenge -- eine
// Klassen-Ableitung daraus haette dort acht Kern-Klassen erfunden. Der Live-Test belegt das am Objekt.
//
// K4 FEHLERKLASSEN, und warum die Grenze genau hier liegt:
//   QuelleFehlt      -- Verzeichnis/Datei existiert nicht. Fuer die ONLINE-Liste ist das ein
//                       Zugangs-Fehler (ohne sie gibt es keine Grundmenge), fuer die HYBRID-PMUs und
//                       fuer den L3-Baum ist es der erwartete Normalfall "diese Maschine fuehrt das
//                       nicht" -- dort geht die Kette eine Stufe weiter, statt einen Fehler zu melden.
//                       Der Unterschied ist ausdruecklich, weil die Quellen verschiedene Dinge bedeuten.
//   QuelleUnlesbar   -- existiert, aber kein Zugriff (oder ein Verzeichnis-Iterationsfehler).
//   QuelleKorrupt    -- vorhanden und lesbar, aber inhaltlich unbrauchbar: leere Liste, absteigend,
//                       doppelt, ueber dem Deckel, genau EINE Hybrid-PMU statt zwei, ueberlappende
//                       Klassen, eine Klasse ausserhalb der Online-Menge, L3-Groesse 0.
//   FormatUnbekannt  -- die Kennung passt zu keinem unterstuetzten Format: ein Fremdzeichen in der
//                       Listen-Grammatik, eine Cache-Groesse ohne den K-Suffix, ODER mehr als zwei
//                       verschiedene L3-Domaenen-Groessen. NIE interpretieren, nie raten.
// Nur ein gescheitertes sched_getaffinity erzeugt den D1-Befund BetriebssystemFeatureFehlt.
//
// 'N/A STATT NULL': kein Parser gibt je eine leere Menge als Erfolg zurueck, und keiner setzt einen
// Ersatzwert. Eine Quelle liefert entweder eine nichtleere, geordnete Werte-Menge oder eine benannte
// Fehlerklasse. "Uniform" ist dabei ein ECHTER Wert, kein Ersatz fuer eine fehlgeschlagene Erhebung.
//
// WARUM DIE LISTEN-GRAMMATIK HIER EIN ZWEITES MAL STEHT (und was das absichert): sie ist mit der von
// numa_page_probe_linux.hpp identisch -- der Kernel benutzt dasselbe cpumask-Listenformat fuer
// node/online, pmu/cpus und cache/shared_cpu_list. Der DECKEL ist es aber nicht (kMaxCpuId 65535 gegen
// kMaxNumaNodeId 4095), und ein geteilter Parser wuerde die CPU-Liste einer Grossmaschine still als
// QuelleKorrupt abweisen. Statt die Duplikation zu verschweigen, sichert der Test sie ab: eine
// DRIFT-GEGENPROBE faehrt beide Parser ueber dieselben vier realen Kernel-Formen und verlangt Gleichheit.
//
// A-15 STEMPEL-NEUTRAL: die erhobenen Zahlen verlassen diesen Blatt-Header nur als RT-Ergebnis. Diese
// Datei kennt keinen Registry-, ABI- oder Stempel-Header und schreibt kein Byte in diese Pfade.

#include <cache_engine/measurement/numa_cpu_pin_process_probe.hpp>

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
#include <sched.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

/// Die Werte-Menge einer CPU-Liste: aufsteigend, duplikatfrei, nichtleer.
using CpuIdSet = std::vector<std::uint32_t>;

[[nodiscard]] inline std::string_view numa_cpu_pin_process_trim(std::string_view value) noexcept {
    auto const is_space = [](char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

/// Liest eine sysfs-Datei vollstaendig. Die Trennung existiert/lesbar ist die A4-Kernaussage: eine
/// FEHLENDE Quelle ist ein anderer Zustand als eine vorhandene, auf die kein Zugriff besteht.
[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
numa_cpu_pin_process_read_file(std::filesystem::path const& path) {
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
numa_cpu_pin_process_take_uint(std::string_view& text) noexcept {
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

/// Parst die KERNEL-LISTEN-GRAMMATIK ("0", "0-3", "0-7,16-23") in eine aufsteigende, duplikatfreie
/// CPU-Menge. Der Kernel emittiert diese Liste immer aufsteigend und ohne Ueberlappung; jede Abweichung
/// ist deshalb ein Befund (QuelleKorrupt) und kein zu tolerierender Stil.
///
/// WARUM SO STRENG: die CPU-Ids sind die ZUORDNUNG der Achsen-Werte. Eine tolerant sortierte oder still
/// deduplizierte Liste wuerde aus einer kaputten Quelle eine plausible Kern-Klasse machen -- genau die
/// Klasse Fehler, gegen die die K4-Trennung gebaut ist.
[[nodiscard]] inline std::expected<CpuIdSet, HardwareProbeErrorClass>
numa_cpu_pin_process_parse_cpu_list(std::string_view raw) {
    std::string_view text = numa_cpu_pin_process_trim(raw);
    if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    // Fremdzeichen sind ein FORMAT-Bruch: diese Datei ist nicht die Kernel-Liste, die wir lesen.
    for (char const c : text)
        if ((c < '0' || c > '9') && c != '-' && c != ',')
            return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

    CpuIdSet      cpus;
    bool          first = true;
    std::uint64_t last  = 0;
    while (true) {
        auto begin = numa_cpu_pin_process_take_uint(text);
        if (!begin.has_value()) return std::unexpected(begin.error());
        std::uint64_t end = *begin;
        if (!text.empty() && text.front() == '-') {
            text.remove_prefix(1);
            auto parsed_end = numa_cpu_pin_process_take_uint(text);
            if (!parsed_end.has_value()) return std::unexpected(parsed_end.error());
            end = *parsed_end;
        }
        if (end < *begin) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // absteigend
        if (end > kMaxCpuId) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (!first && *begin <= last) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // Ueberlappung
        if (cpus.size() + (end - *begin + 1U) > kMaxCpuCount)
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

        for (std::uint64_t id = *begin; id <= end; ++id) cpus.push_back(static_cast<std::uint32_t>(id));
        first = false;
        last  = end;

        if (text.empty()) break;
        if (text.front() != ',') return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
        text.remove_prefix(1);
        if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // Komma am Ende
    }
    if (cpus.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return cpus;
}

/// Parst die sysfs-CACHE-GROESSE ("98304K" -> 100663296 Bytes). Der Kernel schreibt hier immer Kibibyte
/// mit K-Suffix; ein anderes Format wird NICHT geraten, sondern als FormatUnbekannt gemeldet.
[[nodiscard]] inline std::expected<std::uint64_t, HardwareProbeErrorClass>
numa_cpu_pin_process_parse_cache_size(std::string_view raw) {
    std::string_view text = numa_cpu_pin_process_trim(raw);
    if (text.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (!text.ends_with(kCacheSizeKibSuffix)) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    text.remove_suffix(kCacheSizeKibSuffix.size());

    auto kib = numa_cpu_pin_process_take_uint(text);
    if (!kib.has_value()) return std::unexpected(kib.error());
    if (!text.empty()) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt); // Rest vor dem Suffix
    // 'n/a statt Null': ein 0-Byte-Cache existiert nicht.
    if (*kib == 0) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (*kib > kMaxL3SizeBytes / 1024U) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    return *kib * 1024U;
}

/// Liest die ONLINE-Liste der logischen Prozessoren. PORTABEL (kein Plattform-Guard): reiner Datei-Zugriff.
[[nodiscard]] inline std::expected<CpuIdSet, HardwareProbeErrorClass>
numa_cpu_pin_process_read_online_cpus(NumaCpuPinProcessProbeContext const& ctx) {
    if (ctx.cpu_root.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);
    auto text = numa_cpu_pin_process_read_file(ctx.cpu_root / std::string{kCpuOnlineLeafName});
    if (!text.has_value()) return std::unexpected(text.error());
    return numa_cpu_pin_process_parse_cpu_list(*text);
}

/// Liest die "cpus"-Datei EINER Hybrid-PMU. Ein FEHLENDES Verzeichnis ist hier kein Fehler, sondern die
/// Aussage "diese Maschine fuehrt keine getrennten PMUs" -- der Aufrufer bekommt deshalb ein leeres
/// optional-Aequivalent (leere Menge) und entscheidet, was das bedeutet.
[[nodiscard]] inline std::expected<CpuIdSet, HardwareProbeErrorClass>
numa_cpu_pin_process_read_pmu_cpus(NumaCpuPinProcessProbeContext const& ctx, std::string_view pmu_dir_name) {
    if (ctx.cpu_pmu_root.empty()) return CpuIdSet{};
    std::filesystem::path const leaf = ctx.cpu_pmu_root / std::string{pmu_dir_name} / std::string{kPmuCpuListLeafName};

    std::error_code ec;
    bool const      exists = std::filesystem::exists(leaf, ec);
    if (ec) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    if (!exists) return CpuIdSet{}; // erwarteter Normalfall auf jeder nicht-hybriden Maschine

    auto text = numa_cpu_pin_process_read_file(leaf);
    if (!text.has_value()) return std::unexpected(text.error());
    return numa_cpu_pin_process_parse_cpu_list(*text);
}

/// EINE L3-Domaene, bevor sie gegen die anderen geprueft wird.
struct L3Domain {
    std::uint64_t size_bytes = 0;
    CpuIdSet      cpu_ids;
};

/// Liest die L3-Domaenen ueber alle ONLINE-Prozessoren. PORTABEL (kein Plattform-Guard).
///
/// FEHLENDER L3-KNOTEN IST KEIN FEHLER: nicht jede Maschine (und kein Container-sysfs-Ausschnitt) fuehrt
/// index3. Fehlt er auf ALLEN Prozessoren, gibt es keine L3-Aussage und die Kette geht auf "homogen"
/// weiter. Fehlt er nur auf EINIGEN, ist der Baum inkonsistent -> QuelleKorrupt, denn dann waere jede
/// Domaenen-Aussage nur halb belegt. Die Domaenen werden ueber ihre shared_cpu_list zusammengefasst und
/// aufsteigend nach kleinster CPU-Id sortiert: die Verzeichnis-Reihenfolge des Dateisystems ist nicht
/// deterministisch, das Ergebnis muss es sein.
[[nodiscard]] inline std::expected<std::vector<L3Domain>, HardwareProbeErrorClass>
numa_cpu_pin_process_read_l3_domains(NumaCpuPinProcessProbeContext const& ctx, CpuIdSet const& online) {
    std::vector<L3Domain> domains;
    if (ctx.cpu_root.empty()) return domains;

    std::size_t gefunden = 0;
    for (std::uint32_t const cpu : online) {
        std::filesystem::path const index = ctx.cpu_root / (std::string{kCpuDirPrefix} + std::to_string(cpu)) /
                                            std::string{kCacheDirName} / std::string{kL3CacheIndexDirName};
        std::error_code ec;
        bool const      exists = std::filesystem::exists(index, ec);
        if (ec) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
        if (!exists) continue;
        ++gefunden;

        auto size_text = numa_cpu_pin_process_read_file(index / std::string{kCacheSizeLeafName});
        if (!size_text.has_value()) return std::unexpected(size_text.error());
        auto size_bytes = numa_cpu_pin_process_parse_cache_size(*size_text);
        if (!size_bytes.has_value()) return std::unexpected(size_bytes.error());

        auto shared_text = numa_cpu_pin_process_read_file(index / std::string{kCacheSharedCpuListLeaf});
        if (!shared_text.has_value()) return std::unexpected(shared_text.error());
        auto shared = numa_cpu_pin_process_parse_cpu_list(*shared_text);
        if (!shared.has_value()) return std::unexpected(shared.error());

        // Dieselbe Domaene wird von jedem ihrer Prozessoren gemeldet -- sie wird EINMAL aufgenommen, und
        // eine widerspruechliche Groesse fuer dieselbe Menge ist ein Befund, kein Rundungsproblem.
        auto const known = std::find_if(domains.begin(), domains.end(),
                                        [&shared](L3Domain const& d) noexcept { return d.cpu_ids == *shared; });
        if (known != domains.end()) {
            if (known->size_bytes != *size_bytes) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
            continue;
        }
        if (domains.size() >= kMaxCpuCount) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        domains.push_back(L3Domain{*size_bytes, std::move(*shared)});
    }

    if (gefunden == 0) return std::vector<L3Domain>{}; // gar kein L3-Baum: keine Aussage, kein Fehler
    if (gefunden != online.size()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    std::sort(domains.begin(), domains.end(), [](L3Domain const& a, L3Domain const& b) noexcept {
        return a.cpu_ids.front() < b.cpu_ids.front();
    });
    return domains;
}

/// Prueft, dass eine Klassen-Menge eine echte Teilmenge der Online-Menge ist. Eine Klasse mit einem
/// Prozessor, den es nicht gibt, ist eine kaputte Quelle und kein zu ignorierender Ausreisser.
[[nodiscard]] inline bool numa_cpu_pin_process_is_subset(CpuIdSet const& part, CpuIdSet const& whole) noexcept {
    return std::includes(whole.begin(), whole.end(), part.begin(), part.end());
}

/// Setzt die Klassen-Karte aus den drei Quellen zusammen. PORTABEL und ohne IO -- das ist die Stelle, an
/// der die Klassifikation entschieden wird, und sie ist deshalb hardware-frei testbar.
///
/// Regeln, jede mit einem realen Grund:
///   - beide Hybrid-PMUs nichtleer, disjunkt, beide Teilmenge von online  -> HybridPmu (Stufe 1),
///   - genau EINE Hybrid-PMU nichtleer                                    -> QuelleKorrupt (halbes Paar),
///   - genau ZWEI verschiedene L3-Groessen                                -> L3Domaene (Stufe 2),
///   - MEHR als zwei verschiedene L3-Groessen                             -> FormatUnbekannt (nicht raten),
///   - sonst                                                              -> Homogen (Stufe 3).
/// Das Ergebnis traegt immer mindestens eine Gruppe, jede Gruppe eine nichtleere CPU-Menge.
[[nodiscard]] inline std::expected<CoreClassMap, HardwareProbeErrorClass>
numa_cpu_pin_process_compose_core_classes(CpuIdSet const& online, CpuIdSet const& performance_cpus,
                                  CpuIdSet const& efficiency_cpus, std::vector<L3Domain> const& l3_domains) {
    if (online.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    // -- Stufe 1: die benannte Hybrid-Klassifikation ------------------------------------------------
    if (!performance_cpus.empty() || !efficiency_cpus.empty()) {
        if (performance_cpus.empty() || efficiency_cpus.empty())
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // halbes PMU-Paar
        if (!numa_cpu_pin_process_is_subset(performance_cpus, online) ||
            !numa_cpu_pin_process_is_subset(efficiency_cpus, online))
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        CpuIdSet schnitt;
        std::set_intersection(performance_cpus.begin(), performance_cpus.end(), efficiency_cpus.begin(),
                              efficiency_cpus.end(), std::back_inserter(schnitt));
        if (!schnitt.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // ein Kern in beiden

        CoreClassMap map;
        map.source = CoreTopologySource::HybridPmu;
        map.groups.push_back(CoreClassGroup{CoreClassKind::HoheLeistung, performance_cpus});
        map.groups.push_back(CoreClassGroup{CoreClassKind::HoheEffizienz, efficiency_cpus});
        return map;
    }

    // -- Stufe 2: die messbare L3-Ungleichheit -------------------------------------------------------
    std::vector<std::uint64_t> groessen;
    groessen.reserve(l3_domains.size());
    for (auto const& domain : l3_domains) {
        if (domain.cpu_ids.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (!numa_cpu_pin_process_is_subset(domain.cpu_ids, online))
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        groessen.push_back(domain.size_bytes);
    }
    std::sort(groessen.begin(), groessen.end());
    groessen.erase(std::unique(groessen.begin(), groessen.end()), groessen.end());

    if (groessen.size() > kMaxDistinctCoreClasses) {
        // NICHT GERATEN: mehr als zwei Domaenen-Groessen liessen sich nur mit einem Schwellwert auf
        // gross/klein zwingen. Das Vokabular traegt diese Maschine nicht -- und das wird gesagt.
        return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    }
    if (groessen.size() == kMaxDistinctCoreClasses) {
        std::uint64_t const gross = groessen.back();
        CoreClassMap        map;
        map.source = CoreTopologySource::L3Domaene;
        CpuIdSet grosse;
        CpuIdSet kleine;
        for (auto const& domain : l3_domains) {
            auto& ziel = (domain.size_bytes == gross) ? grosse : kleine;
            ziel.insert(ziel.end(), domain.cpu_ids.begin(), domain.cpu_ids.end());
        }
        std::sort(grosse.begin(), grosse.end());
        std::sort(kleine.begin(), kleine.end());
        if (grosse.empty() || kleine.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (std::adjacent_find(grosse.begin(), grosse.end()) != grosse.end() ||
            std::adjacent_find(kleine.begin(), kleine.end()) != kleine.end())
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt); // ein Kern in zwei Domaenen
        map.groups.push_back(CoreClassGroup{CoreClassKind::GrosserCache, std::move(grosse)});
        map.groups.push_back(CoreClassGroup{CoreClassKind::KleinerCache, std::move(kleine)});
        return map;
    }

    // -- Stufe 3: ehrlich uniform -------------------------------------------------------------------
    CoreClassMap map;
    map.source = CoreTopologySource::Homogen;
    map.groups.push_back(CoreClassGroup{CoreClassKind::Uniform, online});
    return map;
}

/// Die Klassen-Achse in einem Zug. PORTABEL.
[[nodiscard]] inline CoreClassMapResult
numa_cpu_pin_process_collect_core_classes(NumaCpuPinProcessProbeContext const& ctx) {
    auto online = numa_cpu_pin_process_read_online_cpus(ctx);
    if (!online.has_value()) return std::unexpected(NumaCpuPinProcessProbeError{online.error()});

    auto performance = numa_cpu_pin_process_read_pmu_cpus(ctx, kPerformanceCorePmuDirName);
    if (!performance.has_value()) return std::unexpected(NumaCpuPinProcessProbeError{performance.error()});
    auto efficiency = numa_cpu_pin_process_read_pmu_cpus(ctx, kEfficiencyCorePmuDirName);
    if (!efficiency.has_value()) return std::unexpected(NumaCpuPinProcessProbeError{efficiency.error()});

    std::vector<L3Domain> l3;
    if (performance->empty() && efficiency->empty()) {
        // Die L3-Stufe wird nur betreten, wenn die benannte Klassifikation fehlt -- sonst laese die Probe
        // eine Quelle, deren Aussage sie ohnehin verwirft, und ein Lesefehler darin wuerde eine
        // vollstaendig erhobene Hybrid-Karte kippen.
        auto domains = numa_cpu_pin_process_read_l3_domains(ctx, *online);
        if (!domains.has_value()) return std::unexpected(NumaCpuPinProcessProbeError{domains.error()});
        l3 = std::move(*domains);
    }

    auto composed = numa_cpu_pin_process_compose_core_classes(*online, *performance, *efficiency, l3);
    if (!composed.has_value()) return std::unexpected(NumaCpuPinProcessProbeError{composed.error()});
    return *composed;
}

// =================================================================================================
// DIE ZUORDNUNGS-ERPROBUNG (Owner-Bedingung (b): "reicht der Kernel es durch")
// =================================================================================================
// Die EINZIGE Stelle des Pakets, die etwas TUT statt nur zu lesen -- und die einzige, die es kann: keine
// Datei sagt, ob sched_setaffinity durchgesetzt wird. Der Ablauf ist bewusst vollstaendig und in dieser
// Reihenfolge:
//   1. sched_getaffinity  -> die EFFEKTIVE Maske (cpuset/cgroup bereits eingerechnet). Sie ist zugleich
//                            die Antwort auf "welche Kerne darf dieser Lauf ueberhaupt ansteuern".
//   2. CPU_ISSET-Vorpruefung des Zielkerns. OHNE sie scheiterte der Aufruf mit EINVAL, und der Grund
//                            "der Kern liegt ausserhalb meiner cpuset" waere von "der Kernel kann es
//                            nicht" nicht mehr zu unterscheiden.
//   3. sched_setaffinity  -> setzen.
//   4. RUECKPROBE per sched_getaffinity: es muss GENAU EIN Kern uebrig sein, und zwar der gewuenschte.
//                            Ein Erfolg ohne diese Probe hiesse nur "der Syscall gab 0 zurueck".
//   5. Vormaske zurueckschreiben -- und ob DAS gelang, steht als eigenes Feld im Ergebnis.
// Der geprueft Kern ist der KLEINSTE der erlaubten Menge: er existiert per Konstruktion, und die Wahl
// haengt nicht an einer Kern-Klassen-Annahme, die diese Funktion gerade erst begruenden soll.

/// Sie lebt in detail und traegt den Familien-Namen: Windows und macOS haben ihre EIGENE Erprobung mit
/// eigenen Syscalls in ihren eigenen Blaettern. Ein familienloser Name hier waere ein Blatt, das fuer die
/// anderen mitspricht -- genau die Kopplung, die die CT-Familien-Factory ausschliesst.
[[nodiscard]] inline PinningCapabilityResult
numa_cpu_pin_process_probe_pinning_linux(NumaCpuPinProcessProbeContext const& ctx) {
#if defined(__linux__)
    if (!ctx.erprobe_pinning) {
        // Ausdruecklich abgeschaltet: es wird KEIN Erfolg erfunden und keine Faehigkeit behauptet.
        return PinningCapability{PinningAvailability::KeineSchnittstelle, 0, 0, false};
    }

    cpu_set_t erlaubt{};
    CPU_ZERO(&erlaubt);
    if (::sched_getaffinity(0, sizeof(erlaubt), &erlaubt) != 0)
        return std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});

    int const anzahl = CPU_COUNT(&erlaubt);
    if (anzahl <= 0) {
        // Die Schnittstelle hat geantwortet, ihre Antwort traegt aber keine Aussage ('n/a statt Null':
        // eine leere Maske ist kein "0 erlaubte Kerne", sondern eine kaputte Quelle).
        return std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    }

    int ziel = -1;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        if (CPU_ISSET(cpu, &erlaubt)) {
            ziel = cpu;
            break;
        }
    if (ziel < 0) return std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    PinningCapability result{};
    result.erlaubte_kerne  = static_cast<std::uint32_t>(anzahl);
    result.gepruefter_kern = static_cast<std::uint32_t>(ziel);

    cpu_set_t einer{};
    CPU_ZERO(&einer);
    CPU_SET(ziel, &einer);
    if (::sched_setaffinity(0, sizeof(einer), &einer) != 0) {
        // Abgelehnt, und zwar ohne dass etwas veraendert wurde -- die Vormaske steht unangetastet.
        result.availability            = PinningAvailability::VomKernAbgelehnt;
        result.maske_wiederhergestellt = true;
        return result;
    }

    cpu_set_t zurueck{};
    CPU_ZERO(&zurueck);
    bool const rueckprobe_gelesen = (::sched_getaffinity(0, sizeof(zurueck), &zurueck) == 0);
    bool const genau_der_zielkern = rueckprobe_gelesen && CPU_COUNT(&zurueck) == 1 && CPU_ISSET(ziel, &zurueck);
    result.availability =
        genau_der_zielkern ? PinningAvailability::Durchgesetzt : PinningAvailability::NichtDurchgesetzt;

    result.maske_wiederhergestellt = (::sched_setaffinity(0, sizeof(erlaubt), &erlaubt) == 0);
    return result;
#else
    static_cast<void>(ctx);
    return std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
#endif
}

} // namespace detail

[[nodiscard]] inline ProcessLocalityTopology
NumaCpuPinProcessProbe<LinuxOperatingSystem>::collect(NumaCpuPinProcessProbeContext const& ctx) {
#if defined(__linux__)
    // Die beiden Felder sind unabhaengig: eine verweigerte Zuordnung darf die Kern-Klassen-Karte nicht
    // verschwinden lassen (Owner-KERN: die Werte werden trotzdem ausgegeben).
    return ProcessLocalityTopology{detail::numa_cpu_pin_process_collect_core_classes(ctx),
                                   detail::numa_cpu_pin_process_probe_pinning_linux(ctx)};
#else
    static_cast<void>(ctx);
    return detail::numa_cpu_pin_process_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
