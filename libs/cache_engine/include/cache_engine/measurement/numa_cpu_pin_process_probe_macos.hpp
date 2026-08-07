#pragma once
// measurement/numa_cpu_pin_process_probe_macos.hpp -- prozessfreie macOS-Erhebung fuer OD-11-RT.
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
// WELLE B/2 (2026-08-07) -- OFF-BY-ONE AM ID-DECKEL, und was daraus folgte: die Zelle prueft je Ebene, ob
// der vergebene Id-Block noch unter kMaxCpuId liegt. Sie rechnete dafuer basis+anzahl gegen den Deckel --
// die letzte vergebene Id ist aber basis+anzahl-1. Ein Block, dessen letzte Id GENAU kMaxCpuId ist, fiel
// damit als QuelleKorrupt heraus, obwohl jede seiner Ids zulaessig ist (Beleg: der Listen-Parser der
// Linux-Zelle weist erst end > kMaxCpuId ab, und der Bestandstest haelt fest, dass die Ober-Id selbst
// NICHT abgewiesen wird). Das war die schweigende Sorte Fehler: eine gueltige Maschine haette eine
// Fehlerklasse statt einer Karte bekommen.
// DIE ZWEITE HAELFTE DES BEFUNDS: die Rechnung sass INNERHALB von #if defined(__APPLE__) und war damit auf
// keiner CI-Plattform uebersetzbar, geschweige denn pruefbar. Die Ableitung ist deshalb in
// detail::numa_cpu_pin_process_compose_perflevel_core_classes gezogen (ausserhalb des Guards, wie die
// Listen-Grammatik der Linux-Zelle); Darwin-gebunden bleibt allein das sysctl-LESEN der Anzahlen.
//
// A-15 STEMPEL-NEUTRAL: dieser Blatt-Header erzeugt nur RT-Werte und kennt keinen ABI-, Registry- oder
// Stempel-Pfad.

#include <cache_engine/measurement/numa_cpu_pin_process_probe.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace comdare::cache_engine::measurement {
namespace detail {

/// Der Id-Deckel ist ein ABGESCHLOSSENES Intervall [0, kMaxCpuId] -- die Konstante ist die groesste
/// ZULAESSIGE Id, nicht die erste unzulaessige. Beleg im Bestand: der Listen-Parser der Linux-Zelle weist
/// erst `end > kMaxCpuId` ab (numa_cpu_pin_process_parse_cpu_list), die Id kMaxCpuId selbst ist also
/// gueltig -- und genau das haelt der Bestandstest als Gegenprobe fest ("die zulaessige Ober-Id selbst
/// wird NICHT abgewiesen").
///
/// DARAUS FOLGT DIE RECHNUNG, die hier steht, damit sie nicht an jeder Aufrufstelle neu erfunden wird:
/// ein Block von `anzahl` Ids ab `basis` belegt basis .. basis+anzahl-1. Die letzte Id ist
/// basis+anzahl-1 und NICHT basis+anzahl. Wer die Summe selbst gegen den Deckel prueft, weist einen
/// Block, dessen letzte Id exakt kMaxCpuId ist, faelschlich als QuelleKorrupt ab (Welle B/2, 2026-08-07:
/// genau dieser Off-by-one stand in der macOS-Zelle).
///
/// anzahl == 0 ist KEIN passender Block, sondern eine leere Klasse -- 'n/a statt Null': sie faellt hier
/// als false heraus und wird vom Aufrufer als QuelleKorrupt benannt.
[[nodiscard]] constexpr bool numa_cpu_pin_process_cpu_id_block_fits(std::uint64_t basis,
                                                                    std::uint64_t anzahl) noexcept {
    if (anzahl == 0) return false;
    return basis + anzahl - 1U <= static_cast<std::uint64_t>(kMaxCpuId);
}

/// Die OS-NEUTRALE Ableitung der Kern-Klassen aus den PERFLEVEL-ANZAHLEN. Sie steht -- wie die
/// Listen-Grammatik der Linux-Zelle -- AUSSERHALB des Plattform-Guards: nur die sysctl-LESUNG ist
/// Darwin-gebunden, die Ableitung daraus ist reine Arithmetik und muss auf JEDER Bau-Plattform
/// pruefbar sein. Ohne diese Trennung liesse sich der Deckel-Rand dieser Zelle nirgends testen (die
/// CI baut kein Darwin), und genau dort sass der Off-by-one.
///
/// Darwin nummeriert die Ebenen von der SCHNELLSTEN zur langsamsten (perflevel0 == P) -- die Reihenfolge
/// der Anzahlen ist deshalb bedeutungstragend und wird hier nicht sortiert.
///
/// Review-Befund L-1 (2026-08-07): HybridPmu ist die BENANNTE Linux-cpu_core/cpu_atom-sysfs-Quelle
/// (s. Doc am Enum in numa_cpu_pin_process_probe.hpp) -- diese Zelle erhebt ueber PERFLEVEL-sysctls und
/// darf dieselbe Provenienz-Vokabel nicht mitbenutzen.
///
/// TOTAL: die beiden Ebenen-Wachen (leer, mehr als das Vokabular traegt) stehen hier ERNEUT, obwohl der
/// Aufrufer sie schon vor den sysctl-Aufrufen faehrt. Diese Funktion muss fuer sich allein wahr sein.
[[nodiscard]] inline std::expected<CoreClassMap, HardwareProbeErrorClass>
numa_cpu_pin_process_compose_perflevel_core_classes(std::vector<std::uint32_t> const& anzahlen) {
    if (anzahlen.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (anzahlen.size() > kMaxDistinctCoreClasses) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

    CoreClassMap map;
    map.source = (anzahlen.size() == 1U) ? CoreTopologySource::Homogen : CoreTopologySource::DarwinPerflevel;

    std::uint64_t naechste = 0;
    for (std::size_t ebene = 0; ebene < anzahlen.size(); ++ebene) {
        std::uint32_t const anzahl = anzahlen[ebene];
        // 'n/a statt Null': eine Ebene ohne Prozessoren ist keine leere Klasse, sondern eine kaputte Quelle.
        if (anzahl == 0 || static_cast<std::size_t>(anzahl) > kMaxCpuCount ||
            !numa_cpu_pin_process_cpu_id_block_fits(naechste, anzahl)) {
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        }

        CoreClassGroup gruppe;
        // Darwin: perflevel0 ist die SCHNELLSTE Ebene (dokumentierte Reihenfolge).
        gruppe.kind = (anzahlen.size() == 1U) ? CoreClassKind::Uniform
                      : (ebene == 0U)         ? CoreClassKind::HoheLeistung
                                              : CoreClassKind::HoheEffizienz;
        gruppe.cpu_ids.reserve(anzahl);
        for (std::uint32_t i = 0; i < anzahl; ++i) gruppe.cpu_ids.push_back(static_cast<std::uint32_t>(naechste) + i);
        naechste += anzahl;
        map.groups.push_back(std::move(gruppe));
    }
    return map;
}

} // namespace detail

[[nodiscard]] inline ProcessLocalityTopology
NumaCpuPinProcessProbe<MacosOperatingSystem>::collect(NumaCpuPinProcessProbeContext const& ctx) {
#if defined(__APPLE__)
    static_cast<void>(ctx);

    // Die Zuordnung ist auf dieser Familie nicht durchsetzbar (siehe Kopf-Block) -- das ist eine
    // benannte Stufe und ausdruecklich KEIN Fehler und KEINE erfundene Faehigkeit.
    PinningCapabilityResult pinning = PinningCapability{PinningAvailability::KeineSchnittstelle, 0, 0, false};

    std::uint32_t ebenen  = 0;
    std::size_t   groesse = sizeof(ebenen);
    if (::sysctlbyname("hw.nperflevels", &ebenen, &groesse, nullptr, 0) != 0 || groesse == 0) {
        return ProcessLocalityTopology{
            std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
            std::move(pinning)};
    }
    if (ebenen == 0) {
        return ProcessLocalityTopology{
            std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleKorrupt}), std::move(pinning)};
    }
    // Diese beiden Wachen bleiben VOR den perflevel-sysctls stehen: liefe die Ebenen-Zahl erst in die
    // Lese-Schleife, entschiede ein fehlendes hw.perflevel4.logicalcpu die Fehlerklasse
    // (BetriebssystemFeatureFehlt) statt der AUSDRUCKSGRENZE des Vokabulars (FormatUnbekannt). Die
    // Ableitung unten prueft beides erneut -- sie muss fuer sich allein wahr sein.
    if (static_cast<std::size_t>(ebenen) > kMaxDistinctCoreClasses) {
        // Mehr Leistungs-Ebenen als das Vokabular traegt -- NICHT auf zwei zwingen, sondern benennen.
        return ProcessLocalityTopology{
            std::unexpected(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::FormatUnbekannt}), std::move(pinning)};
    }

    // Darwin-gebunden ist AUSSCHLIESSLICH das Lesen der Anzahlen. Die Ableitung daraus steht in
    // detail::numa_cpu_pin_process_compose_perflevel_core_classes -- ausserhalb dieses Guards und damit
    // auf jeder Bau-Plattform pruefbar.
    std::vector<std::uint32_t> anzahlen;
    anzahlen.reserve(ebenen);
    for (std::uint32_t ebene = 0; ebene < ebenen; ++ebene) {
        std::string const name   = "hw.perflevel" + std::to_string(ebene) + ".logicalcpu";
        std::uint32_t     anzahl = 0;
        std::size_t       len    = sizeof(anzahl);
        if (::sysctlbyname(name.c_str(), &anzahl, &len, nullptr, 0) != 0 || len == 0) {
            return ProcessLocalityTopology{
                std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
                std::move(pinning)};
        }
        anzahlen.push_back(anzahl);
    }

    auto karte = detail::numa_cpu_pin_process_compose_perflevel_core_classes(anzahlen);
    if (!karte.has_value()) {
        return ProcessLocalityTopology{std::unexpected(NumaCpuPinProcessProbeError{karte.error()}), std::move(pinning)};
    }
    return ProcessLocalityTopology{std::move(*karte), std::move(pinning)};
#else
    static_cast<void>(ctx);
    return detail::numa_cpu_pin_process_os_feature_missing();
#endif
}

} // namespace comdare::cache_engine::measurement
