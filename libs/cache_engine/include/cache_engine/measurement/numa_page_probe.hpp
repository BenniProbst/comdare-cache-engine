// measurement/numa_page_probe.hpp -- prozess-freie RT-Erhebung der target_isa-Unter-Achsen
// numa_node und page (Paket OD-10-RT).
//
// WOHER DAS PAKET KOMMT: Commit f6a5dc02 (O-8 Schritt 6) hat numa_node und page erstmals einen TYP
// gegeben und sie ueber CebSubAxis<..., TargetIsaAxisTag> an den target_isa-Komplex gehaengt. Beide
// haben AUSDRUECKLICH keinen compile-statischen Options-Katalog, sondern deklarieren
// option_source() == "machine_resolved" -- die Werte-Menge haengt an der Maschine. Genau diese
// Aufloesung war dort als OD-10 vertagt ("Die LAUFZEIT-Aufloesung der Werte ist AUSDRUECKLICH nicht
// Teil dieses Fensters"). Dieser Header ist sie.
//
// WAS ERHOBEN WIRD (die Werte-Mengen der ZWEI Unter-Achsen, nichts sonst):
//   numa_node -- die NODE-IDS der Maschine. linux: die Kernel-Liste unter <numa_node_root>/online
//                (Grammatik "0", "0-3", "0-1,4,6-7"); windows: die NUMA-Knoten aus
//                GetLogicalProcessorInformationEx(RelationNumaNode); macos: es gibt keine
//                prozessfreie NUMA-Schnittstelle -- das ist ein benannter L6-Befund, keine leere Liste.
//   page      -- die SEITENGROESSEN-FREIGABE als System-Capability. Immer die Basis-Seite
//                (sysconf(_SC_PAGESIZE) bzw. GetSystemInfo/hw.pagesize) und dazu jede vom Kernel
//                gefuehrte Huge-Seite MIT ihrem Pool-Stand. Die Achse ist die System-SEITE der
//                Freigabe; ihre organ-seitige Durchsetzung bleibt der AllocPageHint-NTTP
//                (axes/alloc/alloc_hw_config.hpp) und wird hier NICHT beruehrt.
//
// K2 PROZESS-FREI: ausschliesslich Datei-Reads unter sysfs und Familien-Schnittstellen im eigenen
// Prozess (sysconf, GetLogicalProcessorInformationEx, sysctlbyname). Externe Programme, Shells und
// Prozess-Erzeugung sind kein Teil dieser API -- kein numactl, kein lscpu, kein hwloc-Aufruf. Auch
// libnuma bleibt draussen: sie waere eine zweite Wissensquelle neben sysfs, und ihr Fehlen waere ein
// Link-Fehler statt eines klassifizierten Befunds.
//
// K4 FEHLER-EINORDNUNG (#29, exakt die OS-U3-Aufteilung): HardwareProbeErrorClass heisst historisch
// "Hardware", bezeichnet im Framework aber den ZUGANG zu einer Quelle -- Datei fehlt / unlesbar /
// korrupt / unbekanntes Format reisen deshalb auch hier in dieser bestehenden Domaene. Nur eine auf
// dieser Plattform fehlende FAMILIEN-SCHNITTSTELLE oder ein gescheiterter Familien-Syscall erzeugt
// CompilerCompilerErrorClass::BetriebssystemFeatureFehlt. Es entsteht bewusst KEIN neues Enum: die
// vier vorhandenen Unterscheidungen sind exakt die benoetigten (RF-3-Kollisionslehre -- eine neue
// Klasse in einem fremden Enum zieht dessen Count-Wachen und bricht sie still).
//
// 'N/A STATT NULL' (Owner-Doktrin, feedback_measurement_failure_visibility_csv_failed_not_null_plus_log):
// KEIN Ergebnis dieses Headers ist je eine 0 oder eine leere Liste, die "nichts gefunden" und "nicht
// erhoben" verschmelzen wuerde. Jede der beiden Achsen traegt ENTWEDER ihre Werte-Menge ODER eine
// benannte Fehlerklasse. Ein leerer Vektor ist strukturell unerreichbar: die Parser weisen die leere
// Menge als QuelleKorrupt zurueck.
//
// ABWEICHUNG VOM PRAEZEDENZFALL OS-U3, BENANNT STATT VERSCHWIEGEN: collect() liefert KEIN aeusseres
// std::expected, sondern NumaPageTopology mit ZWEI unabhaengigen std::expected-Feldern. Grund: die
// beiden Achsen haben GETRENNTE Quellen (Node-Baum vs Hugepage-Baum), waehrend OS-U3 alle drei Werte
// aus DERSELBEN OS-Instanz zieht und dort all-or-nothing richtig ist. Ein aeusseres expected wuerde
// hier eine vollstaendig lesbare Seitengroessen-Freigabe hinter einem fehlenden NUMA-Baum
// verschwinden lassen -- genau die stille Degradation, die A4/K4 ausschliessen. Die Fehler-SUMME
// (NumaPageProbeError) bleibt dagegen exakt die von OsProbeError; nur ihre Anzahl-Position im
// Ergebnis-Typ unterscheidet sich.
//
// K5 VERSIONIERTE VERFAHRENS-ID: probe_id() ist eine Eigenschaft des Probe-TYPS, kein frei setzbares
// Feld des Runtime-Ergebnisses. Aendert sich eine Quelle oder die Zusammensetzung der Werte, MUSS die
// Version steigen, damit Messungen aus verschiedenen Erhebungs-Verfahren unterscheidbar bleiben. Der
// Familien-Teil wird aus OsAxis::os_family_id() in einen constexpr-Puffer komponiert und nirgendwo als
// zweites Literal gepflegt (Muster kOsProbeVersion). kNumaPageProbeVersion startet dreistellig mit
// CPU-Flag als "v1.0.0c" -- die A13-M3/C4-Migration ist vollzogen und COMDARE_VERSION_HW_FLAG_ENFORCE
// ist SCHARF, ein flagloses Literal braeche hier sofort. Die zentrale Naht-Liste in algo_semver.hpp
// fuehrt diese Quelle als eigene Klasse; der dort VORNE stehende generische Wachen-grep
// (ce_owned_version_satisfies_cpu_enforce) findet sie, weil beide B12-Wachen unten stehen.
//
// ANSCHLUSS AN machine_resolved (das eigentliche Ziel des Pakets): der Probe-Typ fuehrt die beiden
// Unter-Achsen als TYPEN (numa_axis/page_axis) und verwacht compile-time, dass beide wirklich
// "machine_resolved" deklarieren. Damit ist diese Probe nicht IRGENDEIN Erheber, sondern der von der
// statischen Einhaengung BENANNTE Aufloeser -- und ein Umschreiben der option_source auf einen
// CT-Katalog bricht hier, statt die Probe stillschweigend gegenstandslos zu machen.
// DER PRODUKTIVE KONSUMENT KOMMT GETRENNT UND PLANMAESSIG: die erhobenen Werte-Mengen in die
// Planer-/CEB-Aufloesung und in die Mess-Ausgabe zu tragen ist das Folge-Paket OD-10-RT-K -- exakt so,
// wie OS-U3 seine Werte fuer OS-U4 vorbereitet hat, ohne sie selbst zu verdrahten. Das ist eine
// deklarierte Paket-Grenze, kein vergessener Anschluss.
//
// A-15 STEMPEL-NEUTRALITAET: NumaPageTopology ist ein reines Runtime-Blatt. Dieser Header kennt weder
// ABI-Stempel noch Registry, XML, Generator oder Binary-ID und bietet keine Naht dorthin. Kein
// erhobener Wert und auch keine probe_id wird in den Binary-Stempel geschrieben; die System-Achsen-
// Code-Version "target_isa" bleibt davon unberuehrt, und system_axis_registry.xml bleibt byte-stabil
// (weder Label noch option_source noch Mitgliedschaft der Unter-Achsen werden angefasst).
//
// Metaprog: CT-Familien-Dispatch ueber Template-Spezialisierung, kein Runtime-enum, kein switch ueber
// Familien, keine vtable. Der einzige std::variant ist die Fehler-Summe an der Expected/Result-Naht.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DAS VOKABULAR DER SEITENGROESSEN-FREIGABE
// =================================================================================================

/// WIE eine Seitengroesse freigegeben ist. Die drei Werte sind bewusst unterscheidbar, weil sie drei
/// verschiedene Aussagen ueber die Maschine sind und eine gemeinsame Vokabel sie unwiederbringlich
/// verschmelzen wuerde (dieselbe W-4-Lehre wie AdmissionStatus/BuildCellStatus in axis_error.hpp):
///   Basis           = die Grundseite des Systems -- immer nutzbar, kein Pool noetig,
///   PoolBelegt      = Huge-Seite MIT vorgehaltenen Seiten (nr_hugepages > 0), also real belegbar,
///   NurUnterstuetzt = der Kernel FUEHRT die Groesse, haelt aber KEINE Seiten vor (nr_hugepages == 0).
/// Der Unterschied traegt: "der Kernel kennt 1 GiB-Seiten" und "es liegen 1 GiB-Seiten bereit" sind
/// verschiedene Freigaben, und eine Messreihe, die beides als "1g verfuegbar" ausweist, behauptet
/// eine Belegbarkeit, die es nicht gibt.
enum class PageSizeAvailability : std::uint8_t {
    Basis           = 0,
    PoolBelegt      = 1,
    NurUnterstuetzt = 2,
};
/// Single-Source der Freigabe-Stufenzahl (beide Drift-Wachen unten).
inline constexpr std::size_t kPageSizeAvailabilityCount = 3;

/// Log-/CSV-Etikett je Freigabe-Stufe. Der Fallback heisst BEWUSST nicht "unbekannt": dieses Wort ist
/// in axis_error.hpp bereits doppelt vergeben, und die Disjunktheits-Wache unten soll eine echte
/// Aussage machen statt an einem geteilten Sammelbegriff zu scheitern.
[[nodiscard]] constexpr std::string_view page_size_availability_token(PageSizeAvailability a) noexcept {
    switch (a) {
        case PageSizeAvailability::Basis: return "basis_seite";
        case PageSizeAvailability::PoolBelegt: return "pool_belegt";
        case PageSizeAvailability::NurUnterstuetzt: return "nur_unterstuetzt";
    }
    return "seitenfreigabe_unbekannt";
}

/// EINE freigegebene Seitengroesse. size_bytes ist immer eine echte Zweierpotenz > 0 -- ein 0-Eintrag
/// ist strukturell unerreichbar, weil die Parser ihn als QuelleKorrupt zurueckweisen ('n/a statt Null').
struct PageSizeCapability {
    std::uint64_t        size_bytes   = 0;
    PageSizeAvailability availability = PageSizeAvailability::NurUnterstuetzt;
};

[[nodiscard]] constexpr bool operator==(PageSizeCapability const& a, PageSizeCapability const& b) noexcept {
    return a.size_bytes == b.size_bytes && a.availability == b.availability;
}

// =================================================================================================
// 2. DIE ERGEBNIS-TRAEGER DER BEIDEN UNTER-ACHSEN
// =================================================================================================

/// K4: exakt die Form von OsProbeError/BuildError -- eine typisierte Summe an der Expected/Result-Naht.
/// Die erste Alternative klassifiziert den Quellen-ZUGANG, die zweite ausschliesslich die fehlende
/// OS-Faehigkeit. Diese ausdrueckliche variant-Ausnahme ist Fehler-TRANSPORT, niemals Familien-Dispatch.
/// Die Summe selbst steht seit OD-11-RT EINMAL in axis_error.hpp (ProbeErrorSum); der sprechende Name
/// bleibt hier. Die frueher hier stehende error_domain-Ueberladung ist entfallen -- Begruendung an
/// ProbeErrorSum: sie war signaturgleich zu der der Schwester-Proben und schloss aus, dass zwei
/// Erhebungen je in derselben Uebersetzungseinheit ausgewertet werden.
using NumaPageProbeError = ProbeErrorSum;

/// Das bestehende stabile Etikett der jeweils getragenen #29-Domaene, ohne ein drittes Vokabular zu
/// erfinden. QuelleFehlt bleibt "quelle_fehlt", der L6-Produzent meldet das zementierte
/// "betriebssystem_feature_fehlt".
[[nodiscard]] constexpr std::string_view numa_page_probe_error_label(NumaPageProbeError const& error) noexcept {
    return std::visit(
        [](auto const value) noexcept -> std::string_view {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, HardwareProbeErrorClass>)
                return hardware_probe_label(value);
            else
                return error_class_label(value);
        },
        error);
}

/// Die Werte-Menge der Unter-Achse numa_node: die Node-Ids, aufsteigend und duplikatfrei.
using NumaNodeSet = std::vector<std::uint32_t>;
/// Die Werte-Menge der Unter-Achse page: die Seitengroessen, aufsteigend und duplikatfrei.
using PageCapabilitySet = std::vector<PageSizeCapability>;

using NumaNodeSetResult       = std::expected<NumaNodeSet, NumaPageProbeError>;
using PageCapabilitySetResult = std::expected<PageCapabilitySet, NumaPageProbeError>;

/// Das Ergebnis EINER Erhebung: je Unter-Achse ENTWEDER die Werte-Menge ODER eine benannte Fehlerklasse.
/// Es gibt keinen dritten Zustand und keinen Ersatzwert -- siehe den 'n/a statt Null'-Block im Kopf.
struct NumaPageTopology {
    NumaNodeSetResult       numa_nodes;
    PageCapabilitySetResult page_sizes;
};

// =================================================================================================
// 3. DIE OS-HANDLES ALS INJIZIERBARER KONTEXT
// =================================================================================================

/// Die realen Quellen-Wurzeln. Sie stehen HIER als Single-Source und werden von der ISA-x-OS-Zelle
/// (hardware_probe_factory.hpp) als Zell-Handles weitergereicht, nie ein zweites Mal hingeschrieben.
inline constexpr std::string_view kDefaultNumaNodeRoot = "/sys/devices/system/node";
inline constexpr std::string_view kDefaultHugepageRoot = "/sys/kernel/mm/hugepages";
/// Der Blattname der ONLINE-Liste. Bewusst "online" und NICHT "possible": possible ist die zur Bauzeit
/// des Kernels reservierte OBERGRENZE (auf Ein-Knoten-Maschinen regelmaessig groesser als die Realitaet),
/// online sind die tatsaechlich vorhandenen Knoten. Die Achse traegt die Werte der MASCHINE.
inline constexpr std::string_view kNumaOnlineLeafName = "online";
/// Namens-Kontrakt der Hugepage-Knoten: "hugepages-<KiB>kB" (Kernel-ABI, Documentation/admin-guide/mm).
inline constexpr std::string_view kHugepageDirPrefix = "hugepages-";
inline constexpr std::string_view kHugepageDirSuffix = "kB";
/// Der Pool-Zaehler unter einem Hugepage-Knoten.
inline constexpr std::string_view kHugepagePoolLeafName = "nr_hugepages";

/// Injizierbare, BESITZENDE OS-Handles. std::filesystem::path statt string_view verhindert (wie bei
/// OperatingSystemProbeContext), dass ein Test einen temporaeren Pfad-String eintraegt, dessen Speicher
/// vor collect() bereits abgelaufen ist. windows/macos brauchen fuer ihre Familien-Schnittstellen keinen
/// Pfad, teilen aber denselben Kontext-Typ -- ein zweiter Kontext waere eine zweite Wissensquelle.
struct NumaPageProbeContext {
    std::filesystem::path numa_node_root;
    std::filesystem::path hugepage_root;
};

/// Die echten Handles an genau einer Stelle. Tests ersetzen sie durch Wegwerf-Baeume; die
/// Probe-Spezialisierungen enthalten dadurch keine versteckten, nicht injizierbaren Pfad-Konstanten.
[[nodiscard]] inline NumaPageProbeContext default_numa_page_probe_context() {
    return {std::filesystem::path{kDefaultNumaNodeRoot}, std::filesystem::path{kDefaultHugepageRoot}};
}

namespace detail {

/// Der EINE L6-Ausgang einer nicht-nativen Zelle: BEIDE Achsen melden die fehlende Familien-
/// Schnittstelle. Er steht hier im Public Header und nicht in einem Blatt, damit die drei Blaetter
/// nicht je eine eigene Formulierung derselben Aussage tragen und damit kein Blatt ein anderes
/// inkludieren muss.
[[nodiscard]] inline NumaPageTopology numa_page_os_feature_missing() {
    return NumaPageTopology{
        std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
        std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt})};
}

} // namespace detail

// =================================================================================================
// 4. PLAUSIBILITAETS-DECKEL (fachliche Wachen, nicht arithmetische)
// =================================================================================================
// Sie weisen MUELL ab, nicht kuenftige Hardware. Die Werte liegen bewusst weit ueber jedem realen
// Stand: ein Ueberschreiten ist kein "neuer Server", sondern eine kaputte oder fremde Quelle.

/// Groesste plausible Node-Id. Der Kernel-Deckel MAX_NUMNODES liegt in ueblichen Konfigurationen bei
/// 1024; 4095 laesst Luft, ohne eine 32-Bit-Muellzahl als Node durchgehen zu lassen.
inline constexpr std::uint32_t kMaxNumaNodeId = 4095;
/// Groesste plausible Anzahl Knoten. Verhindert, dass eine kaputte Bereichs-Angabe ("0-4294967295")
/// zu einer Milliarden-Element-Liste expandiert, bevor irgendeine Wache greift.
inline constexpr std::size_t kMaxNumaNodeCount = 4096;
/// Groesste plausible Seitengroesse (16 GiB). Heute existieren 4k/64k/2M/512M/1G/16G je nach ISA.
inline constexpr std::uint64_t kMaxPageSizeBytes = std::uint64_t{1} << 34;
/// Groesster plausibler Hugepage-Pool-Zaehler. Ein hoeherer Wert kann keine reale Reservierung sein.
inline constexpr std::uint64_t kMaxHugepagePoolCount = std::uint64_t{1} << 40;

// =================================================================================================
// 5. DIE VERSIONIERTE VERFAHRENS-ID (K5)
// =================================================================================================

namespace detail {

inline constexpr std::string_view kNumaPageProbeIdPrefix = "numa_page_probe.";
/// K5/A13-M1b: dreistellig, mit CPU-Hardware-Flag, NIE experimentell. COMDARE_VERSION_HW_FLAG_ENFORCE
/// ist scharf (Default 1) -- ein flagloses "v1.0.0" braeche die gated Wache unten sofort.
inline constexpr std::string_view kNumaPageProbeVersion = "v1.0.0c";

/// Ein fester constexpr-Puffer statt std::string: probe_id() muss auch compile-time aus dem von der
/// Achse gelieferten string_view komponierbar sein. 64 Bytes lassen Platz fuer kuenftige echte
/// Familien-IDs, ohne die heutige ID-Laenge als zweite Wissensquelle einzufrieren.
struct NumaPageProbeIdBuffer final {
    std::array<char, 64> bytes{};
    std::size_t          length = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return {bytes.data(), length}; }
};

[[nodiscard]] consteval NumaPageProbeIdBuffer compose_numa_page_probe_id(std::string_view family,
                                                                         std::string_view version) noexcept {
    NumaPageProbeIdBuffer result{};
    std::size_t const     required = kNumaPageProbeIdPrefix.size() + family.size() + 1U + version.size();
    if (required > result.bytes.size()) return result;

    auto append = [&result](std::string_view part) {
        for (char const c : part) result.bytes[result.length++] = c;
    };
    append(kNumaPageProbeIdPrefix);
    append(family);
    result.bytes[result.length++] = '@';
    append(version);
    return result;
}

[[nodiscard]] constexpr std::size_t numa_page_probe_at_count(std::string_view id) noexcept {
    std::size_t count = 0;
    for (char const c : id)
        if (c == '@') ++count;
    return count;
}

[[nodiscard]] constexpr std::string_view numa_page_probe_family_part(std::string_view id) noexcept {
    if (!id.starts_with(kNumaPageProbeIdPrefix)) return {};
    std::size_t const at = id.find('@', kNumaPageProbeIdPrefix.size());
    if (at == std::string_view::npos) return {};
    return id.substr(kNumaPageProbeIdPrefix.size(), at - kNumaPageProbeIdPrefix.size());
}

} // namespace detail

/// K5-Helfer: schneidet den rohen vX.Y.Z-Teil hinter '@' aus der komponierten Verfahrens-ID. Die
/// separate genau-ein-'@'-Wache unten verhindert, dass ein mehrdeutiger Rest akzeptiert wird.
[[nodiscard]] constexpr std::string_view numa_page_probe_version_part(std::string_view id) noexcept {
    std::size_t const at = id.find('@');
    if (at == std::string_view::npos) return {};
    return id.substr(at + 1U);
}

// =================================================================================================
// 6. DIE CT-FAMILIEN-FACTORY
// =================================================================================================

/// TOTALITAETS-WACHE: bewusst nur deklariert. Eine neue OS-Achsen-Auspraegung ohne ausdrueckliche
/// Probe-Entscheidung bleibt ein unvollstaendiger Typ und kann nicht still auf eine Familie fallen.
template <class OsAxis>
struct NumaPageProbe;

template <>
struct NumaPageProbe<LinuxOperatingSystem> {
    using os_axis = LinuxOperatingSystem;
    /// DER ANSCHLUSS: die beiden Unter-Achsen, deren machine_resolved-Werte hier aufgeloest werden --
    /// als TYPEN, nicht als Etiketten. Die Wachen am Dateiende pruefen, dass sie es wirklich sind.
    using numa_axis = NumaNodeSubAxis;
    using page_axis = PageSubAxis;

    inline static constexpr detail::NumaPageProbeIdBuffer kProbeId =
        detail::compose_numa_page_probe_id(os_axis::os_family_id(), detail::kNumaPageProbeVersion);

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
        return "sysfs_node_baum_und_sysconf_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static NumaPageTopology collect(NumaPageProbeContext const& context);
};

template <>
struct NumaPageProbe<WindowsOperatingSystem> {
    using os_axis   = WindowsOperatingSystem;
    using numa_axis = NumaNodeSubAxis;
    using page_axis = PageSubAxis;

    inline static constexpr detail::NumaPageProbeIdBuffer kProbeId =
        detail::compose_numa_page_probe_id(os_axis::os_family_id(), detail::kNumaPageProbeVersion);

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
        return "logical_processor_information_ex_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static NumaPageTopology collect(NumaPageProbeContext const& context);
};

template <>
struct NumaPageProbe<MacosOperatingSystem> {
    using os_axis   = MacosOperatingSystem;
    using numa_axis = NumaNodeSubAxis;
    using page_axis = PageSubAxis;

    inline static constexpr detail::NumaPageProbeIdBuffer kProbeId =
        detail::compose_numa_page_probe_id(os_axis::os_family_id(), detail::kNumaPageProbeVersion);

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
        return "sysctl_hw_pagesize_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static NumaPageTopology collect(NumaPageProbeContext const& context);
};

/// Der Vertrag jeder entschiedenen Familien-Zelle. os_axis bindet die Familien-Wahl als TYP,
/// numa_axis/page_axis binden die AUFGELOESTEN Unter-Achsen als TYP; collect liefert je Achse genau
/// einen Traeger oder einen klassifizierten K4-Fehler, nie einen Ersatzwert.
template <class Probe>
concept NumaPageProbeConcept = requires(NumaPageProbeContext const& context) {
    typename Probe::os_axis;
    typename Probe::numa_axis;
    typename Probe::page_axis;
    requires OperatingSystemAxisConcept<typename Probe::os_axis>;
    requires TargetIsaSubAxisConcept<typename Probe::numa_axis>;
    requires TargetIsaSubAxisConcept<typename Probe::page_axis>;
    requires std::same_as<Probe, NumaPageProbe<typename Probe::os_axis>>;
    { Probe::probe_id() } -> std::same_as<std::string_view>;
    { Probe::has_native_probe() } -> std::same_as<bool>;
    { Probe::not_implemented_reason() } -> std::same_as<std::string_view>;
    { Probe::collect(context) } -> std::same_as<NumaPageTopology>;
};

namespace detail {

/// Der gesamte K5-Vertrag in einer CT-Wache: Single-Source-Praefix, die Familie direkt aus dem
/// Achsen-Typ, genau ein Trenner und eine echte, dreistellige, ce-eigene, nicht-experimentelle Version.
template <class OsAxis>
[[nodiscard]] consteval bool numa_page_probe_id_contract_is_satisfied() noexcept {
    std::string_view const id      = NumaPageProbe<OsAxis>::probe_id();
    std::string_view const version = numa_page_probe_version_part(id);
    AlgoSemVer const       parsed  = parse_algo_semver(version);
    return id.starts_with(kNumaPageProbeIdPrefix) && numa_page_probe_family_part(id) == OsAxis::os_family_id() &&
           numa_page_probe_at_count(id) == 1U && ce_owned_version_is_wellformed(version) && !parsed.is_sentinel() &&
           !parsed.experimental && version.find('e') == std::string_view::npos;
}

template <class OsAxis>
[[nodiscard]] consteval bool numa_page_probe_version_satisfies_cpu_enforce() noexcept {
    return ce_owned_version_satisfies_cpu_enforce(numa_page_probe_version_part(NumaPageProbe<OsAxis>::probe_id()));
}

/// ANSCHLUSS-WACHE: haengt die Zelle wirklich an den BEIDEN target_isa-Unter-Achsen, und deklarieren
/// diese wirklich noch machine_resolved? Wuerde eine von ihnen auf einen CT-Options-Katalog umgestellt,
/// waere diese Probe gegenstandslos -- sie soll dann brechen, nicht stumm weiterlaufen.
template <class OsAxis>
[[nodiscard]] consteval bool numa_page_probe_is_anchored() noexcept {
    using Probe = NumaPageProbe<OsAxis>;
    return std::same_as<typename Probe::numa_axis, NumaNodeSubAxis> &&
           std::same_as<typename Probe::page_axis, PageSubAxis> &&
           Probe::numa_axis::option_source() == std::string_view{"machine_resolved"} &&
           Probe::page_axis::option_source() == std::string_view{"machine_resolved"} &&
           Probe::numa_axis::parent_axis_label() == std::string_view{"target_isa"} &&
           Probe::page_axis::parent_axis_label() == std::string_view{"target_isa"};
}

} // namespace detail

static_assert(NumaPageProbeConcept<NumaPageProbe<LinuxOperatingSystem>>);
static_assert(NumaPageProbeConcept<NumaPageProbe<WindowsOperatingSystem>>);
static_assert(NumaPageProbeConcept<NumaPageProbe<MacosOperatingSystem>>);

static_assert(detail::numa_page_probe_id_contract_is_satisfied<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::numa_page_probe_id_contract_is_satisfied<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::numa_page_probe_id_contract_is_satisfied<MacosOperatingSystem>(),
              "K5: die macOS-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");

#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::numa_page_probe_version_satisfies_cpu_enforce<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::numa_page_probe_version_satisfies_cpu_enforce<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::numa_page_probe_version_satisfies_cpu_enforce<MacosOperatingSystem>(),
              "K5: die macOS-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
#endif

// Paarweise verschieden, alle drei Vergleiche: zwei Familien duerfen nie als dasselbe Verfahren im Log
// oder Messergebnis erscheinen. Die IDs bleiben trotzdem vollstaendig aus os_family_id() abgeleitet.
static_assert(NumaPageProbe<LinuxOperatingSystem>::probe_id() != NumaPageProbe<WindowsOperatingSystem>::probe_id() &&
              NumaPageProbe<LinuxOperatingSystem>::probe_id() != NumaPageProbe<MacosOperatingSystem>::probe_id() &&
              NumaPageProbe<WindowsOperatingSystem>::probe_id() != NumaPageProbe<MacosOperatingSystem>::probe_id());
// Und disjunkt zur OS-U3-Verfahrens-Familie: dieselbe Maschine erhebt zwei verschiedene Dinge, und die
// Provenienz-Kette muss sie unterscheiden koennen (das Praefix ist die einzige Trennung).
static_assert(!NumaPageProbe<LinuxOperatingSystem>::probe_id().starts_with(std::string_view{"os_probe."}));
static_assert(detail::kNumaPageProbeIdPrefix != std::string_view{"os_probe."});

// -- Der ANSCHLUSS an die statische Einhaengung (f6a5dc02) ----------------------------------------
static_assert(detail::numa_page_probe_is_anchored<LinuxOperatingSystem>(),
              "OD-10-RT: die Linux-Zelle loest nicht mehr die beiden machine_resolved-Unter-Achsen "
              "numa_node/page am target_isa-Komplex auf -- dann erhebt sie etwas anderes als das, was "
              "die statische Einhaengung angeboten hat.");
static_assert(detail::numa_page_probe_is_anchored<WindowsOperatingSystem>());
static_assert(detail::numa_page_probe_is_anchored<MacosOperatingSystem>());
// NAMENSFALLE (uebernommen aus target_isa_sub_axes.hpp, hier als Gegenprobe an der RT-Seite): die
// Seiten-Unter-Achse heisst "page" -- axis_01_page_type ist der Baum-Knoten-PageKind und eine voellig
// andere Achse, und der AllocPageHint-NTTP ist die ORGAN-seitige Durchsetzung, nicht die System-Freigabe.
static_assert(NumaPageProbe<LinuxOperatingSystem>::page_axis::axis_label() == std::string_view{"page"});
static_assert(NumaPageProbe<LinuxOperatingSystem>::page_axis::axis_label() != std::string_view{"page_type"});
static_assert(NumaPageProbe<LinuxOperatingSystem>::numa_axis::axis_label() == std::string_view{"numa_node"});
static_assert(NumaPageProbe<LinuxOperatingSystem>::numa_axis::axis_label() != std::string_view{"numa_topology"});

// -- Die K4-Domaenen, textuell und semantisch an den Bestand gebunden ------------------------------
static_assert(error_domain(NumaPageProbeError{HardwareProbeErrorClass::QuelleFehlt}) == ErrorDomain::HardwareProbe);
static_assert(numa_page_probe_error_label(NumaPageProbeError{HardwareProbeErrorClass::QuelleFehlt}) ==
              std::string_view{"quelle_fehlt"});
static_assert(numa_page_probe_error_label(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt}) ==
              std::string_view{"quelle_korrupt"});
static_assert(numa_page_probe_error_label(NumaPageProbeError{HardwareProbeErrorClass::FormatUnbekannt}) ==
              std::string_view{"format_unbekannt"});
static_assert(error_domain(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              ErrorDomain::CompilerCompiler);
static_assert(numa_page_probe_error_label(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              std::string_view{"betriebssystem_feature_fehlt"});

// -- Das Freigabe-Vokabular: BEIDE Drift-Richtungen (RF-3-Lehre) + volle Disjunktheit --------------
static_assert(std::is_same_v<std::underlying_type_t<PageSizeAvailability>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<PageSizeAvailability> && std::is_trivially_copyable_v<PageSizeCapability>);
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt.
static_assert(kPageSizeAvailabilityCount == static_cast<std::size_t>(PageSizeAvailability::NurUnterstuetzt) + 1);
static_assert(page_size_availability_token(static_cast<PageSizeAvailability>(kPageSizeAvailabilityCount)) ==
                  std::string_view{"seitenfreigabe_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Freigabe-Stufe");
// Die tragende Unterscheidung: "der Kernel kennt die Groesse" und "es liegen Seiten bereit" duerfen nie
// zusammenfallen -- sonst behauptet eine Messreihe eine Belegbarkeit, die es nicht gibt.
static_assert(page_size_availability_token(PageSizeAvailability::PoolBelegt) !=
              page_size_availability_token(PageSizeAvailability::NurUnterstuetzt));
static_assert(page_size_availability_token(PageSizeAvailability::Basis) !=
              page_size_availability_token(PageSizeAvailability::PoolBelegt));
// Token-Disjunktheit gegen ALLE bestehenden Zell-/Fehler-Vokabeln (axis_error.hpp), jede Stufe einzeln
// plus der Fallback -- ein Log-Grep auf diese Etiketten darf keine fremde Domaene mittreffen.
static_assert(probe_label_ist_disjunkt(page_size_availability_token(PageSizeAvailability::Basis)));
static_assert(probe_label_ist_disjunkt(page_size_availability_token(PageSizeAvailability::PoolBelegt)));
static_assert(probe_label_ist_disjunkt(page_size_availability_token(PageSizeAvailability::NurUnterstuetzt)));
static_assert(probe_label_ist_disjunkt(
    page_size_availability_token(static_cast<PageSizeAvailability>(kPageSizeAvailabilityCount))));
// ... und gegen die Erhebungs-Klassen, gegen die probe_label_ist_disjunkt selbst nicht prueft.
static_assert(page_size_availability_token(PageSizeAvailability::Basis) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleFehlt));
static_assert(page_size_availability_token(PageSizeAvailability::NurUnterstuetzt) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleKorrupt));

// -- Die Deckel sind echte Deckel (eine 0 waere ein Deckel, der alles verbietet) -------------------
static_assert(kMaxNumaNodeId > 0 && kMaxNumaNodeCount > 0 && kMaxPageSizeBytes > 0);
static_assert(kMaxPageSizeBytes >= (std::uint64_t{1} << 30), "1-GiB-Seiten muessen unter dem Deckel liegen.");
static_assert(kMaxNumaNodeCount > kMaxNumaNodeId / 2, "Der Anzahl-Deckel darf den Id-Deckel nicht entwerten.");

// -- Der Default-Traeger behauptet nichts ('n/a statt Null', strukturell) --------------------------
// Ein default-konstruiertes Ergebnis traegt in BEIDEN Achsen einen leeren Erfolgs-Vektor -- deshalb
// gibt es keinen Pfad, der ihn erzeugt: jede collect-Zelle belegt beide Felder, und die Parser weisen
// die leere Menge als QuelleKorrupt zurueck. Diese Wache haelt fest, dass der Default NICHT die
// Bedeutung "keine Knoten / keine Seiten" tragen darf.
static_assert(std::is_default_constructible_v<NumaPageTopology>);
static_assert(PageSizeCapability{}.size_bytes == 0,
              "Ein default-konstruiertes PageSizeCapability ist KEIN gueltiger Wert -- 0 Bytes ist die "
              "unerreichbare Nicht-Groesse, nicht 'die kleinste Seite'.");

} // namespace comdare::cache_engine::measurement

// Die Definitionen liegen in Plattform-Blaettern. Jedes Blatt ist auf jeder Plattform uebersetzbar und
// inkludiert seinerseits diesen Public Header, sodass auch eine direkte Blatt-Inklusion gueltig ist.
#include <cache_engine/measurement/numa_page_probe_linux.hpp>
#include <cache_engine/measurement/numa_page_probe_macos.hpp>
#include <cache_engine/measurement/numa_page_probe_windows.hpp>
