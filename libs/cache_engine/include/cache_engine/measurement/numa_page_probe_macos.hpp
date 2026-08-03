#pragma once
// measurement/numa_page_probe_macos.hpp -- prozessfreie macOS-Erhebung fuer OD-10-RT.
//
// WAS: Die macOS-Spezialisierung liest beide Achsen ueber sysctlbyname:
//   page      -- hw.pagesize ist die Basis-Seitengroesse der laufenden Instanz.
//   numa_node -- macOS bietet KEINE prozessfreie NUMA-Schnittstelle an. Es gibt weder einen sysfs-
//                aehnlichen Knoten-Baum noch ein dokumentiertes sysctl, das Knoten-Ids liefert; die
//                Speicher-Topologie ist in Darwin nicht als Anwender-API exponiert. Das ist deshalb
//                der D1-Befund BetriebssystemFeatureFehlt und ausdruecklich KEINE leere Knoten-Menge:
//                "es gibt keine Schnittstelle" und "es gibt keine Knoten" sind verschiedene Aussagen,
//                und nur die erste ist hier wahr.
//
// BENANNTE LUECKE STATT FALSCHER VOLLSTAENDIGKEIT (A8-Doktrin "ehrliche Nicht-Implementierung"):
// macOS kennt auf x86_64 Superpages (VM_FLAGS_SUPERPAGE_SIZE_2MB), bietet aber KEINE aufzaehlbare
// Freigabe-Liste dafuer an -- es gibt kein Gegenstueck zum Hugepage-Baum. Die Seiten-Achse meldet
// deshalb ausschliesslich die Basis-Seite. Das ist eine wahre, aber ENGE Antwort, und sie steht hier,
// damit niemand sie fuer eine vollstaendige Freigabe-Erhebung haelt. Eine Superpage-Aussage ohne
// aufzaehlbare Quelle waere geraten, und Raten ist teurer als eine benannte Luecke.
//
// K2 PROZESS-FREI: ausschliesslich sysctlbyname im eigenen Prozess. system_profiler und sysctl(8)
// waeren Prozess-Aufrufe und damit Doktrin-verboten.
//
// K4 FEHLERKLASSEN: Ein fehlgeschlagenes sysctlbyname ist BetriebssystemFeatureFehlt (die Familien-
// Schnittstelle traegt nicht). Ein erfolgreicher Aufruf mit unbrauchbarem Inhalt (Groesse 0, keine
// Zweierpotenz, ueber dem Deckel) ist QuelleKorrupt. Es gibt in dieser Zelle keinen Datei-Zugang und
// deshalb auch keinen erfundenen Datei-Zugangsfehler; der injizierte Kontext bleibt ungelesen.
//
// A-15 STEMPEL-NEUTRAL: dieser Blatt-Header erzeugt nur RT-Werte und kennt keinen ABI-, Registry- oder
// Stempel-Pfad.

#include <cache_engine/measurement/numa_page_probe.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <utility>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace comdare::cache_engine::measurement {

[[nodiscard]] inline NumaPageTopology NumaPageProbe<MacosOperatingSystem>::collect(NumaPageProbeContext const& ctx) {
#if defined(__APPLE__)
    static_cast<void>(ctx);

    // numa_node: benannte Nicht-Verfuegbarkeit, keine leere Menge (siehe Kopf-Block).
    NumaNodeSetResult nodes =
        std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});

    std::uint64_t base = 0;
    std::size_t   size = sizeof(base);
    if (::sysctlbyname("hw.pagesize", &base, &size, nullptr, 0) != 0 || size == 0) {
        return NumaPageTopology{std::move(nodes), std::unexpected(NumaPageProbeError{
                                                      CompilerCompilerErrorClass::BetriebssystemFeatureFehlt})};
    }
    if (base == 0 || base > kMaxPageSizeBytes || (base & (base - 1U)) != 0U) {
        return NumaPageTopology{std::move(nodes),
                                std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt})};
    }

    PageCapabilitySet capabilities;
    capabilities.push_back(PageSizeCapability{base, PageSizeAvailability::Basis});
    return NumaPageTopology{std::move(nodes), std::move(capabilities)};
#else
    static_cast<void>(ctx);
    return detail::numa_page_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
