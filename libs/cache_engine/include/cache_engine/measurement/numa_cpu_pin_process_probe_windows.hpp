#pragma once
// measurement/numa_cpu_pin_process_probe_windows.hpp -- prozessfreie Windows-Erhebung fuer OD-11-RT.
//
// WAS: Die Windows-Spezialisierung liest beide Felder aus dem eigenen Prozess:
//   core_class -- GetLogicalProcessorInformationEx(RelationProcessorCore, ...) liefert je physischem
//                 Kern einen PROCESSOR_RELATIONSHIP-Eintrag mit EfficiencyClass und GroupMask. Windows
//                 fuehrt die Effizienz-Klasse als ZAHL (0 = am wenigsten effizient/leistungsstaerkster
//                 Kern-Typ, hoehere Werte = effizientere Typen). Genau ZWEI verschiedene Klassen-Werte
//                 sind der Hybrid-Fall -> HoheLeistung/HoheEffizienz; EINE ist die uniforme Maschine;
//                 MEHR als zwei werden NICHT auf zwei gezwungen, sondern als FormatUnbekannt benannt
//                 (dieselbe "nicht raten"-Regel wie bei den L3-Domaenen der Linux-Zelle).
//   Zuordnung  -- GetProcessAffinityMask liefert die erlaubte Menge, SetThreadAffinityMask setzt und
//                 gibt die VORMASKE zurueck; die Rueckprobe erfolgt ueber ein zweites, sofort wieder
//                 zurueckgesetztes SetThreadAffinityMask, dessen Rueckgabe die tatsaechlich gesetzte
//                 Maske ist. Ein Erfolg ohne diese Rueckprobe hiesse nur "der Aufruf gab nicht 0 zurueck".
//
// GRUPPEN-GRENZE, BENANNT STATT VERSCHWIEGEN: Windows fuehrt Prozessoren in GRUPPEN zu je hoechstens 64.
// Diese Zelle wertet ausschliesslich die Gruppe des aufrufenden Prozesses aus (GroupMask.Group == 0 bei
// den ueblichen Ein-Gruppen-Maschinen). Auf einer Mehr-Gruppen-Maschine waere eine gruppenuebergreifende
// CPU-Id eine Erfindung -- es gibt dort keine global eindeutige Nummer. Kerne fremder Gruppen werden
// deshalb NICHT stillschweigend uebersprungen, sondern machen die Erhebung zu FormatUnbekannt.
//
// K2 PROZESS-FREI: der Zugriff bleibt im laufenden Prozess und benutzt genau diese Kernel32-
// Schnittstellen. Es wird kein WMI befragt (das waere ein COM-/Dienst-Umweg mit eigener Fehlerwelt) und
// kein Hilfsprogramm gestartet.
//
// K4 FEHLERKLASSEN: Lehnt GetLogicalProcessorInformationEx den Aufruf mit einem anderen Fehler als
// ERROR_INSUFFICIENT_BUFFER ab, ist die OS-Faehigkeit auf diesem Host nicht verfuegbar --
// BetriebssystemFeatureFehlt. Ein erfolgreicher Aufruf, dessen Inhalt unbrauchbar ist (kein einziger
// Kern, eine leere Maske), ist dagegen QuelleKorrupt: die Schnittstelle war da, nur ihr Ergebnis traegt
// keine Aussage. Es gibt in dieser Zelle keinen erfundenen Datei-Zugangsfehler -- der injizierte Kontext
// wird bewusst nicht gelesen, weil Windows die Werte nicht ueber Pfade anbietet.
//
// A-15 STEMPEL-NEUTRAL: die gelesenen Zahlen werden nur in den RT-Traeger geschrieben. Dieser
// Blatt-Header kennt keinen ABI-, Registry- oder Stempel-Pfad.

#include <cache_engine/measurement/numa_cpu_pin_process_probe.hpp>

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
namespace detail {

/// Die Zuordnungs-Erprobung der Windows-Zelle. Familien-Name aus demselben Grund wie in der Linux-Zelle:
/// jedes Blatt spricht nur fuer sich.
[[nodiscard]] inline PinningCapabilityResult
numa_cpu_pin_process_probe_pinning_windows(NumaCpuPinProcessProbeContext const& ctx) {
#if defined(_WIN32)
    if (!ctx.erprobe_pinning) return PinningCapability{PinningAvailability::KeineSchnittstelle, 0, 0, false};

    DWORD_PTR prozess_maske = 0;
    DWORD_PTR system_maske  = 0;
    if (::GetProcessAffinityMask(::GetCurrentProcess(), &prozess_maske, &system_maske) == FALSE)
        return std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    if (prozess_maske == 0)
        return std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    PinningCapability result{};
    DWORD_PTR         ziel_bit = 0;
    for (unsigned bit = 0; bit < sizeof(DWORD_PTR) * 8U; ++bit) {
        DWORD_PTR const probe = static_cast<DWORD_PTR>(1) << bit;
        if ((prozess_maske & probe) != 0) {
            if (ziel_bit == 0) {
                ziel_bit               = probe;
                result.gepruefter_kern = bit;
            }
            ++result.erlaubte_kerne;
        }
    }
    if (ziel_bit == 0) return std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});

    // SetThreadAffinityMask liefert die VORMASKE (0 == Fehlschlag, dokumentiert).
    DWORD_PTR const vormaske = ::SetThreadAffinityMask(::GetCurrentThread(), ziel_bit);
    if (vormaske == 0) {
        result.availability            = PinningAvailability::VomKernAbgelehnt;
        result.maske_wiederhergestellt = true; // nichts wurde veraendert
        return result;
    }
    // RUECKPROBE: das Zuruecksetzen liefert die AKTUELL gesetzte Maske -- das ist die einzige Stelle, an
    // der Windows sie herausgibt. Sie muss GENAU das Zielbit sein.
    DWORD_PTR const gesetzt        = ::SetThreadAffinityMask(::GetCurrentThread(), vormaske);
    result.maske_wiederhergestellt = (gesetzt != 0);
    result.availability =
        (gesetzt == ziel_bit) ? PinningAvailability::Durchgesetzt : PinningAvailability::NichtDurchgesetzt;
    return result;
#else
    static_cast<void>(ctx);
    return std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
#endif
}

} // namespace detail

[[nodiscard]] inline ProcessLocalityTopology
NumaCpuPinProcessProbe<WindowsOperatingSystem>::collect(NumaCpuPinProcessProbeContext const& ctx) {
#if defined(_WIN32)
    static_cast<void>(ctx.cpu_root);

    // -- core_class: erst die noetige Puffergroesse erfragen, dann den Puffer fuellen ---------------
    // JEDER Ausgang ist ausdruecklich belegt und keiner haengt an einem STALE GetLastError(): der
    // Fehlercode wird nur DIREKT nach einem als FALSE gemeldeten Aufruf gelesen.
    CoreClassMapResult klassen =
        std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
    DWORD length = 0;
    if (::GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length) != FALSE) {
        // Dokumentiert unmoeglich (der nullptr-Aufruf MUSS mit ERROR_INSUFFICIENT_BUFFER scheitern).
        klassen = std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    } else if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        // Die Schnittstelle traegt auf diesem Host nicht -> D1-Befund (Vorbelegung, hier nur benannt).
    } else if (length == 0) {
        klassen = std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});
    } else {
        std::vector<unsigned char> buffer(static_cast<std::size_t>(length));
        auto* const                first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
        if (::GetLogicalProcessorInformationEx(RelationProcessorCore, first, &length) == FALSE) {
            klassen =
                std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt});
        } else {
            struct WindowsKernKlasse {
                std::uint32_t              efficiency_class = 0;
                std::vector<std::uint32_t> cpu_ids;
            };
            std::vector<WindowsKernKlasse> klassen_roh;
            DWORD                          offset  = 0;
            bool                           broken  = false;
            bool                           fremd   = false;
            while (offset < length) {
                auto const* const entry =
                    reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX const*>(buffer.data() + offset);
                if (entry->Size == 0 || entry->Size > length - offset) {
                    broken = true;
                    break;
                }
                if (entry->Relationship == RelationProcessorCore) {
                    auto const& core = entry->Processor;
                    for (WORD g = 0; g < core.GroupCount; ++g) {
                        if (core.GroupMask[g].Group != 0) {
                            // Mehr-Gruppen-Maschine: eine globale CPU-Id waere eine Erfindung (Kopf-Block).
                            fremd = true;
                            break;
                        }
                        auto const maske = static_cast<std::uint64_t>(core.GroupMask[g].Mask);
                        for (unsigned bit = 0; bit < 64U; ++bit) {
                            if ((maske & (std::uint64_t{1} << bit)) == 0) continue;
                            auto const klasse = static_cast<std::uint32_t>(core.EfficiencyClass);
                            auto const known  = std::find_if(
                                klassen_roh.begin(), klassen_roh.end(),
                                [klasse](WindowsKernKlasse const& k) noexcept { return k.efficiency_class == klasse; });
                            if (known != klassen_roh.end())
                                known->cpu_ids.push_back(bit);
                            else
                                klassen_roh.push_back(WindowsKernKlasse{klasse, {bit}});
                        }
                    }
                    if (fremd) break;
                }
                offset += entry->Size;
            }

            if (broken || fremd) {
                klassen = std::unexpected(NumaCpuPinProcessProbeError{fremd ? HardwareProbeErrorClass::FormatUnbekannt
                                                                      : HardwareProbeErrorClass::QuelleKorrupt});
            } else if (klassen_roh.empty()) {
                // 'n/a statt Null': eine leere Klassen-Liste wird NIE als Erfolg gemeldet.
                klassen = std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});
            } else if (klassen_roh.size() > kMaxDistinctCoreClasses) {
                // Mehr als zwei Effizienz-Klassen liessen sich nur mit einem Schwellwert auf zwei
                // zwingen -- NICHT geraten, sondern benannt.
                klassen = std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::FormatUnbekannt});
            } else {
                // Windows: 0 == leistungsstaerkster Typ, hoehere Werte == effizientere Typen.
                std::sort(klassen_roh.begin(), klassen_roh.end(),
                          [](WindowsKernKlasse const& a, WindowsKernKlasse const& b) noexcept {
                              return a.efficiency_class < b.efficiency_class;
                          });
                CoreClassMap map;
                bool         korrupt = false;
                for (std::size_t i = 0; i < klassen_roh.size(); ++i) {
                    auto& ids = klassen_roh[i].cpu_ids;
                    std::sort(ids.begin(), ids.end());
                    if (ids.empty() || std::adjacent_find(ids.begin(), ids.end()) != ids.end()) {
                        korrupt = true;
                        break;
                    }
                    CoreClassKind const kind = (klassen_roh.size() == 1U) ? CoreClassKind::Uniform
                                               : (i == 0U)                ? CoreClassKind::HoheLeistung
                                                                          : CoreClassKind::HoheEffizienz;
                    map.groups.push_back(CoreClassGroup{kind, std::move(ids)});
                }
                map.source =
                    (klassen_roh.size() == 1U) ? CoreTopologySource::Homogen : CoreTopologySource::HybridPmu;
                if (korrupt)
                    klassen = std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt});
                else
                    klassen = std::move(map);
            }
        }
    }

    return ProcessLocalityTopology{std::move(klassen), detail::numa_cpu_pin_process_probe_pinning_windows(ctx)};
#else
    static_cast<void>(ctx);
    return detail::numa_cpu_pin_process_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
