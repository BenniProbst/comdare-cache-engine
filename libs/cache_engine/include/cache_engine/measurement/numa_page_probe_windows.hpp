#pragma once
// measurement/numa_page_probe_windows.hpp -- prozessfreie Windows-Erhebung fuer OD-10-RT.
//
// WAS: Die Windows-Spezialisierung liest beide Achsen aus dem eigenen Prozess:
//   numa_node -- GetLogicalProcessorInformationEx(RelationNumaNode, ...) liefert je NUMA-Knoten einen
//                Eintrag mit seiner NodeNumber. Die Nummern werden gesammelt, sortiert und auf
//                Duplikate geprueft -- exakt dieselbe Ordnungs-Zusicherung wie in der Linux-Zelle,
//                damit die Werte-Menge der Achse plattform-unabhaengig dieselbe Form hat.
//   page      -- GetSystemInfo liefert dwPageSize als Basis-Seite, GetLargePageMinimum die einzige vom
//                System angebotene grosse Seite. Rueckgabe 0 heisst dokumentiert "keine grossen Seiten"
//                und ist deshalb KEIN Fehler, sondern die vollstaendige Antwort "nur die Basis-Seite".
//
// K2 PROZESS-FREI: der Zugriff bleibt im laufenden Prozess und benutzt genau diese Kernel32-
// Schnittstellen. Es wird kein WMI befragt (das waere ein COM-/Dienst-Umweg mit eigener Fehlerwelt)
// und kein Hilfsprogramm gestartet.
//
// K4 FEHLERKLASSEN: Lehnt GetLogicalProcessorInformationEx den Aufruf mit einem anderen Fehler als
// ERROR_INSUFFICIENT_BUFFER ab, ist die OS-Faehigkeit auf diesem Host nicht verfuegbar --
// BetriebssystemFeatureFehlt. Ein erfolgreicher Aufruf, dessen Inhalt unbrauchbar ist (kein einziger
// Knoten, oder eine Basis-Seitengroesse 0), ist dagegen QuelleKorrupt: die Schnittstelle war da, nur
// ihr Ergebnis traegt keine Aussage. Es gibt in dieser Zelle keinen erfundenen Datei-Zugangsfehler --
// der injizierte Kontext wird bewusst nicht gelesen, weil Windows die Werte nicht ueber Pfade anbietet.
//
// 'N/A STATT NULL': eine leere Knoten-Menge wird NIE als Erfolg gemeldet. Sie waere von "ein Knoten
// mit der Nummer 0" nicht mehr unterscheidbar, und genau diese Verschmelzung schliesst die Doktrin aus.
//
// A-15 STEMPEL-NEUTRAL: die gelesenen Zahlen werden nur in den RT-Traeger geschrieben. Dieser
// Blatt-Header kennt keinen ABI-, Registry- oder Stempel-Pfad.

#include <cache_engine/measurement/numa_page_probe.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace comdare::cache_engine::measurement {

[[nodiscard]] inline NumaPageTopology NumaPageProbe<WindowsOperatingSystem>::collect(NumaPageProbeContext const& ctx) {
#if defined(_WIN32)
    static_cast<void>(ctx);

    // -- numa_node: erst die noetige Puffergroesse erfragen, dann den Puffer fuellen --------------
    // JEDER Ausgang ist ausdruecklich belegt und keiner haengt an einem STALE GetLastError(): der
    // Fehlercode wird nur DIREKT nach einem als FALSE gemeldeten Aufruf gelesen. Nach einem Erfolg
    // ist er undefiniert -- ihn dort auszuwerten waere genau die Sorte stiller Fehlklassifikation,
    // gegen die die K4-Trennung gebaut ist.
    NumaNodeSetResult nodes =
        std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    DWORD length = 0;
    if (::GetLogicalProcessorInformationEx(RelationNumaNode, nullptr, &length) != FALSE) {
        // Dokumentiert unmoeglich (der nullptr-Aufruf MUSS mit ERROR_INSUFFICIENT_BUFFER scheitern).
        // Trifft er doch ein, ist die Schnittstelle da, aber ihr Ergebnis unbrauchbar -> QuelleKorrupt.
        nodes = std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    } else if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        // Die Schnittstelle traegt auf diesem Host nicht -> D1-Befund (Vorbelegung, hier nur benannt).
    } else if (length == 0) {
        nodes = std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    } else {
        std::vector<unsigned char> buffer(static_cast<std::size_t>(length));
        auto* const                first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
        if (::GetLogicalProcessorInformationEx(RelationNumaNode, first, &length) == FALSE) {
            nodes = std::unexpected(NumaPageProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
        } else {
            NumaNodeSet ids;
            DWORD       offset = 0;
            bool        broken = false;
            while (offset < length) {
                auto const* const entry =
                    reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX const*>(buffer.data() + offset);
                if (entry->Size == 0 || entry->Size > length - offset) {
                    broken = true;
                    break;
                }
                if (entry->Relationship == RelationNumaNode) {
                    auto const number = static_cast<std::uint64_t>(entry->NumaNode.NodeNumber);
                    if (number > kMaxNumaNodeId || ids.size() >= kMaxNumaNodeCount) {
                        broken = true;
                        break;
                    }
                    ids.push_back(static_cast<std::uint32_t>(number));
                }
                offset += entry->Size;
            }
            std::sort(ids.begin(), ids.end());
            // Leere Menge, Ueberlauf-/Deckel-Bruch und Duplikate sind ALLE QuelleKorrupt: die
            // Schnittstelle hat geantwortet, ihre Antwort traegt aber keine gueltige Werte-Menge.
            // 'n/a statt Null': eine leere Liste wird NIE als Erfolg gemeldet.
            if (!broken && !ids.empty() && std::adjacent_find(ids.begin(), ids.end()) == ids.end())
                nodes = std::move(ids);
            else
                nodes = std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt});
        }
    }

    // -- page: Basis-Seite plus die EINE angebotene grosse Seite -----------------------------------
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);
    PageCapabilitySetResult pages = std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    auto const              base  = static_cast<std::uint64_t>(info.dwPageSize);
    if (base > 0 && base <= kMaxPageSizeBytes && (base & (base - 1U)) == 0U) {
        PageCapabilitySet capabilities;
        capabilities.push_back(PageSizeCapability{base, PageSizeAvailability::Basis});
        auto const large = static_cast<std::uint64_t>(::GetLargePageMinimum());
        // 0 == dokumentiert "dieses System kennt keine grossen Seiten": vollstaendige Antwort, kein
        // Fehler. Ein Wert <= Basis oder keine Zweierpotenz waere dagegen ein kaputtes Ergebnis.
        if (large > 0) {
            if (large <= base || large > kMaxPageSizeBytes || (large & (large - 1U)) != 0U) {
                return NumaPageTopology{std::move(nodes),
                                        std::unexpected(NumaPageProbeError{HardwareProbeErrorClass::QuelleKorrupt})};
            }
            // Windows fuehrt keinen Pool-Zaehler: die grosse Seite ist angeboten, ihre Belegbarkeit
            // haengt am SeLockMemoryPrivilege des Aufrufers. Das ist genau die Stufe NurUnterstuetzt --
            // PoolBelegt zu melden waere eine Belegbarkeits-Behauptung ohne Beleg.
            capabilities.push_back(PageSizeCapability{large, PageSizeAvailability::NurUnterstuetzt});
        }
        pages = std::move(capabilities);
    }
    return NumaPageTopology{std::move(nodes), std::move(pages)};
#else
    static_cast<void>(ctx);
    return detail::numa_page_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
