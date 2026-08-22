#pragma once
// measurement/pmc_event_biss.hpp -- DER PROBE-KERN: oeffnet EIN Event, laesst den Koeder laufen, liest zurueck.
//
// HERKUNFT (#83, C-1(c)-Warnung, Owner-GO KON103 17.08.2026): dieser Kern stand bis #83 WOERTLICH in
// profile_facade/planner/pmc_host_probe.hpp (detail_pmc_probe::koeder_pointer_chase +
// RealePmcHostStrategie::event_beisst). Der neue CEB-Gegeneingang builder/pmc_startup_pruefung.hpp braucht
// DENSELBEN Biss-Beweis am LAUF-Host -- und builder/ darf profile_facade/ nicht inkludieren (die
// Include-Richtung im Haus laeuft profile_facade -> builder, am Objekt gezaehlt: 5 Treffer hin, 0 zurueck).
// Eine ABSCHRIFT des Kerns in builder/ waere die klassische zweite Wahrheit (ein Koeder-Wechsel liesse den
// Gegeneingang still anders beissen als die Planer-Probe). Deshalb wandert der Kern HIERHER, eine Ebene
// unter beide Verbraucher -- exakt das Muster von pmc_cache_cfg (die Quelle behaelt ihre Struktur, sie
// liest nur ihre Substanz von hier; ABSCHRIFT SCHLAEGT LOESCHUNG). Die Planer-Probe delegiert seither.
//
// WAS "BEISST" HEISST (K13): "oeffnet" genuegt NICHT. Ein Zaehler kann oeffnen und im ganzen Fenster keine
// Hardware-Zeit bekommen (t_running==0). Deshalb laeuft zwischen ENABLE und READ ein cache-feindlicher
// Pointer-Chase ueber einen Puffer weit jenseits L1; liefert der Zaehler nach einem GELAUFENEN Fenster 0,
// hat der KOEDER NICHT GEBISSEN und das Event zaehlt als nicht brauchbar. PRAEZISIERUNG (CI 16073,
// Job 382856; in der W2-Landung aus der Planer-Probe hierher portiert, damit BEIDE Verbraucher identisch
// urteilen): eine 0 aus einem Fenster OHNE Hardware-Zeit ist dagegen KEINE Messung -- unter
// PMU-Multiplexing (fremde perf-Nutzer, parallele ctest-Nachbarn der CI-Zellen) kam derselbe Host so in
// EINEM Job auf events=3/4, 4/4 und 2/4, und die Erkennung wackelte genau so, wie es der Satz "die Kette
// muss REPRODUZIERBAR beissen" verbietet. pmc_event_beisst() misst deshalb die Gueltigkeit des Fensters
// mit (read_format TIME_ENABLED|TIME_RUNNING) und wiederholt leere Fenster gedeckelt, bevor es urteilt;
// erst ein gelaufenes Fenster entscheidet, fail-closed bleibt.
//
// PLATTFORM: Nicht-Linux liefert unbedingt false (kein perf_event_open) -- fail-closed, keine Behauptung.
//
// header-only, C++23. ASCII-only. Kein Zustand.

#include <cstddef>
#include <cstdint>

#include <cache_engine/measurement/pmc_event_set.hpp> // PmcEventSpec

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#include <vector>
#endif

namespace comdare::cache_engine::measurement {

/// Der KOEDER. Ein Pointer-Chase ueber einen Puffer weit jenseits jeder L1-Groesse, mit grosser Schrittweite,
/// damit L1D-Read-Misses, Last-Level-Misses UND DTLB-Misses real anfallen. Die Kette ist eine Permutation
/// (jeder Slot genau einmal), also nicht vom Prefetcher vorherzusehen, und der Rueckgabewert wird vom
/// Aufrufer verbraucht -- sonst optimierte der Compiler den ganzen Koeder weg und der Zaehler saehe nichts.
/// KEIN Zufall aus /dev/urandom: die Kette muss REPRODUZIERBAR beissen, sonst wackelt die Erkennung.
[[nodiscard]] inline std::uint64_t pmc_koeder_pointer_chase() noexcept {
#if defined(__linux__)
    // 64 MiB: groesser als jeder heutige L2 und groesser als der L3-Anteil eines Kerns -- der Chase faellt
    // damit bis in den Hauptspeicher durch. 4096 Byte Schrittweite trifft je Sprung eine neue Seite und
    // erzeugt so auch DTLB-Misses.
    constexpr std::size_t    kBytes  = 64u * 1024u * 1024u;
    constexpr std::size_t    kStride = 4096u;
    constexpr std::size_t    kSlots  = kBytes / kStride;
    std::vector<std::size_t> kette(kSlots, 0);
    // Ein einfacher, deterministischer Zyklus ueber alle Slots: Schrittweite teilerfremd zu kSlots.
    constexpr std::size_t kSchritt = 4099u; // Primzahl, teilerfremd zu kSlots (2^14)
    std::size_t           pos      = 0;
    for (std::size_t i = 0; i < kSlots; ++i) {
        std::size_t const next = (pos + kSchritt) % kSlots;
        kette[pos]             = next;
        pos                    = next;
    }
    std::uint64_t summe = 0;
    std::size_t   p     = 0;
    for (std::size_t i = 0; i < kSlots * 4; ++i) {
        p = kette[p];
        summe += p;
    }
    return summe;
#else
    return 0;
#endif
}

/// FENSTER-DECKEL der Leer-Fenster-Wiederholung (s. pmc_event_beisst). Klein und endlich: ein Host, dessen
/// PMU ueber so viele Koeder-Fenster hinweg keinem einzigen Event Hardware-Zeit gibt, ist ehrlich
/// unbrauchbar (fail-closed) -- der Deckel schuetzt nur gegen das transiente Multiplexing-Loch.
inline constexpr unsigned kPmcKoederFenster = 5u;

/// Oeffnet EIN Event self-monitoring (pid=0, cpu=-1, exclude_kernel -- exakt die Anforderungsform der
/// realen Quelle), laesst den Koeder laufen, liest zurueck. true NUR, wenn der Zaehler geoeffnet hat UND
/// ein GELAUFENES Fenster einen Wert > 0 traegt (K13: der Koeder MUSS beissen).
///
/// FENSTER-LEERE IST KEINE MESSUNG (CI 16073, Job 382856): unter PMU-Multiplexing (fremde perf-Nutzer
/// auf dem Kern, z. B. parallele ctest-Nachbarn der CI-Zelle) kann der Zaehler oeffnen und im ganzen
/// Koeder-Fenster keine Hardware-Zeit bekommen -- read() liefert dann 0, obwohl das Event auf diesem
/// Host funktioniert. Derselbe Runner sah so in EINEM Job events=3/4, 4/4 und 2/4: die Erkennung
/// wackelte lauf-zu-lauf, was der Probe-Kopf ausdruecklich verbietet. Deshalb misst die Probe ihre
/// eigene Gueltigkeit mit (read_format TIME_ENABLED|TIME_RUNNING):
///   zeit_gelaufen == 0 => Fenster leer, es GIBT keine Messung => naechstes Koeder-Fenster
///                        (gedeckelt auf kPmcKoederFenster; wert/zeiten kumulieren ueber den fd);
///   zeit_gelaufen  > 0 => es WURDE gemessen: wert > 0 ist der Biss, wert == 0 die ehrliche Absage
///                        (das Event zaehlt auf diesem Host nicht) -- kein weiterer Versuch.
/// Nach dem Deckel gilt fail-closed (kein Biss): eine dauerhaft verdraengte PMU wird nie behauptet.
[[nodiscard]] inline bool pmc_event_beisst(PmcEventSpec const& ev) noexcept {
#if defined(__linux__)
    struct ::perf_event_attr attr;
    std::memset(&attr, 0, sizeof(attr)); // Muellbits in Reserve-Feldern => EINVAL; immer memset
    attr.size           = sizeof(attr);
    attr.type           = ev.type;
    attr.config         = ev.config;
    attr.disabled       = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv     = 1;
    attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    long const fd       = ::syscall(__NR_perf_event_open, &attr, /*pid=*/0, /*cpu=*/-1, /*group_fd=*/-1,
                                    /*flags=*/0UL);
    if (fd < 0) return false;
    int const f = static_cast<int>(fd);
    struct LeseForm { // read_format-Layout ohne PERF_FORMAT_GROUP: Wert, dann die zwei Zeiten
        std::uint64_t wert;
        std::uint64_t zeit_aktiv;    // TIME_ENABLED: Event war logisch eingeschaltet
        std::uint64_t zeit_gelaufen; // TIME_RUNNING: Event sass real auf der PMU
    };
    bool biss = false;
    for (unsigned fenster = 0; fenster < kPmcKoederFenster; ++fenster) {
        (void)::ioctl(f, PERF_EVENT_IOC_ENABLE, 0);
        std::uint64_t const koeder = pmc_koeder_pointer_chase();
        (void)::ioctl(f, PERF_EVENT_IOC_DISABLE, 0);
        LeseForm   lf{};
        bool const ok = ::read(f, &lf, sizeof(lf)) == static_cast<::ssize_t>(sizeof(lf));
        // koeder wird verbraucht, damit der Pointer-Chase nicht wegoptimiert wird (er ist der ganze
        // Punkt). Werkzeugfehler (read schlaegt fehl) ist fail-closed und KEIN Wiederholungsgrund.
        if (!ok || koeder == 0xFFFFFFFFFFFFFFFFULL) break;
        if (lf.wert > 0) {
            biss = true; // ein gelaufenes Fenster hat gezaehlt -- der Koeder hat gebissen
            break;
        }
        if (lf.zeit_gelaufen > 0) break; // gemessen und NICHT gebissen: die ehrliche 0 steht
        // zeit_gelaufen == 0: Fenster leer (Multiplexing-Verdraengung) -- naechstes Fenster.
    }
    (void)::close(f);
    return biss;
#else
    (void)ev;
    return false;
#endif
}

} // namespace comdare::cache_engine::measurement
