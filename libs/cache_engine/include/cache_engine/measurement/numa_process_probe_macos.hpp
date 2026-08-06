#pragma once
// measurement/numa_process_probe_macos.hpp -- prozessfreie macOS-Erhebung fuer OD-11-RT.
//
// WAS: Die macOS-Spezialisierung liest die Kern-Klassen ueber die PERFLEVEL-sysctls, die Darwin auf
// Apple Silicon fuehrt:
//   hw.nperflevels                      -> die Zahl der Leistungs-Ebenen (1 auf Intel-Macs, 2 auf den
//                                          bisherigen Apple-Silicon-Modellen: P-Cluster und E-Cluster),
//   hw.perflevel<N>.logicalcpu          -> die Zahl der logischen Prozessoren je Ebene.
// Darwin nummeriert die Ebenen von der SCHNELLSTEN zur langsamsten (perflevel0 == P), das ist die
// dokumentierte Reihenfolge und wird hier nicht geraten.
//
// DIE BENANNTE LUECKE, die diese Zelle von der Linux-Zelle unterscheidet (A8-Doktrin "ehrliche
// Nicht-Implementierung"): Darwin gibt KEINE Zuordnung Kern-Id -> Ebene heraus. Es gibt kein Gegenstueck
// zu cpu_core/cpus. Die Kern-Ids werden deshalb aus den ANZAHLEN in Darwin-Reihenfolge vergeben
// (0..n0-1 fuer perflevel0, dann perflevel1) -- und genau das ist eine ABLEITUNG und keine Lesung.
// Weil sie eine Ableitung ist, waere sie als Zuordnung eine Behauptung; die Zelle meldet deshalb
// AUSSCHLIESSLICH die Klassen-GROESSEN und ihre Reihenfolge, und die Zuordnungs-Erprobung sagt fuer
// diese Familie ehrlich KeineSchnittstelle. Eine Kern-Karte ohne belegbare Zuordnung waere geraten, und
// Raten ist teurer als eine benannte Luecke.
//
// WARUM DIE PINNING-STUFE HIER IMMER KeineSchnittstelle IST -- und warum das kein Fehler ist: Darwin
// bietet kein sched_setaffinity. thread_policy_set(THREAD_AFFINITY_POLICY) ist ausdruecklich ein HINWEIS
// an den Scheduler ("affinity tag"), keine Durchsetzung, und auf Apple Silicon ohne Wirkung. Eine
// Erprobung, die einen Hinweis setzt und ihn als Zuordnung meldet, waere genau die Belegbarkeits-
// Behauptung ohne Beleg, die die Doktrin ausschliesst. KeineSchnittstelle ist die WAHRE Aussage --
// unterschieden von VomKernAbgelehnt (dort gibt es die Schnittstelle und sie sagt nein).
//
// K2 PROZESS-FREI: ausschliesslich sysctlbyname im eigenen Prozess. sysctl(8) und system_profiler waeren
// Prozess-Aufrufe und damit Doktrin-verboten.
//
// K4 FEHLERKLASSEN: Ein fehlgeschlagenes sysctlbyname auf hw.nperflevels ist BetriebssystemFeatureFehlt
// (die Familien-Schnittstelle traegt nicht -- so auf aelteren Darwin-Staenden). Ein erfolgreicher Aufruf
// mit unbrauchbarem Inhalt (0 Ebenen, 0 Prozessoren je Ebene, mehr Ebenen als das Vokabular traegt) ist
// dagegen QuelleKorrupt bzw. FormatUnbekannt: die Schnittstelle war da, ihr Ergebnis traegt aber keine
// verwendbare Aussage. Es gibt in dieser Zelle keinen Datei-Zugang und deshalb auch keinen erfundenen
// Datei-Zugangsfehler; der injizierte Kontext bleibt ungelesen.
//
// A-15 STEMPEL-NEUTRAL: dieser Blatt-Header erzeugt nur RT-Werte und kennt keinen ABI-, Registry- oder
// Stempel-Pfad.

#include <cache_engine/measurement/numa_process_probe.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace comdare::cache_engine::measurement {

[[nodiscard]] inline ProcessLocalityTopology
NumaProcessProbe<MacosOperatingSystem>::collect(NumaProcessProbeContext const& ctx) {
#if defined(__APPLE__)
    static_cast<void>(ctx);

    // Die Zuordnung ist auf dieser Familie nicht durchsetzbar (siehe Kopf-Block) -- das ist eine
    // benannte Stufe und ausdruecklich KEIN Fehler und KEINE erfundene Faehigkeit.
    PinningCapabilityResult pinning = PinningCapability{PinningAvailability::KeineSchnittstelle, 0, 0, false};

    std::uint32_t ebenen = 0;
    std::size_t   groesse = sizeof(ebenen);
    if (::sysctlbyname("hw.nperflevels", &ebenen, &groesse, nullptr, 0) != 0 || groesse == 0) {
        return ProcessLocalityTopology{
            std::unexpected(NumaProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
            std::move(pinning)};
    }
    if (ebenen == 0) {
        return ProcessLocalityTopology{
            std::unexpected(NumaProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt}), std::move(pinning)};
    }
    if (static_cast<std::size_t>(ebenen) > kMaxDistinctCoreClasses) {
        // Mehr Leistungs-Ebenen als das Vokabular traegt -- NICHT auf zwei zwingen, sondern benennen.
        return ProcessLocalityTopology{
            std::unexpected(NumaProcessProbeError{HardwareProbeErrorClass::FormatUnbekannt}), std::move(pinning)};
    }

    CoreClassMap map;
    map.source            = (ebenen == 1U) ? CoreTopologySource::Homogen : CoreTopologySource::HybridPmu;
    std::uint32_t naechste = 0;
    for (std::uint32_t ebene = 0; ebene < ebenen; ++ebene) {
        std::string const name  = "hw.perflevel" + std::to_string(ebene) + ".logicalcpu";
        std::uint32_t     anzahl = 0;
        std::size_t       len    = sizeof(anzahl);
        if (::sysctlbyname(name.c_str(), &anzahl, &len, nullptr, 0) != 0 || len == 0) {
            return ProcessLocalityTopology{
                std::unexpected(NumaProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
                std::move(pinning)};
        }
        // 'n/a statt Null': eine Ebene ohne Prozessoren ist keine leere Klasse, sondern eine kaputte Quelle.
        if (anzahl == 0 || static_cast<std::size_t>(anzahl) > kMaxCpuCount ||
            static_cast<std::uint64_t>(naechste) + anzahl > kMaxCpuId) {
            return ProcessLocalityTopology{
                std::unexpected(NumaProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt}), std::move(pinning)};
        }

        CoreClassGroup gruppe;
        // Darwin: perflevel0 ist die SCHNELLSTE Ebene (dokumentierte Reihenfolge).
        gruppe.kind = (ebenen == 1U)  ? CoreClassKind::Uniform
                      : (ebene == 0U) ? CoreClassKind::HoheLeistung
                                      : CoreClassKind::HoheEffizienz;
        for (std::uint32_t i = 0; i < anzahl; ++i) gruppe.cpu_ids.push_back(naechste + i);
        naechste += anzahl;
        map.groups.push_back(std::move(gruppe));
    }
    return ProcessLocalityTopology{std::move(map), std::move(pinning)};
#else
    static_cast<void>(ctx);
    return detail::numa_process_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
