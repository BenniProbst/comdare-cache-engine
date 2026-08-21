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
// WAS "BEISST" HEISST (K13, unveraendert): "oeffnet" genuegt NICHT. Ein Zaehler kann oeffnen und im ganzen
// Fenster keine Hardware-Zeit bekommen (t_running==0). Deshalb laeuft zwischen ENABLE und READ ein
// cache-feindlicher Pointer-Chase ueber einen Puffer weit jenseits L1; liefert der Zaehler danach 0, hat
// der KOEDER NICHT GEBISSEN und das Event zaehlt als nicht brauchbar.
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

/// Oeffnet EIN Event self-monitoring (pid=0, cpu=-1, exclude_kernel -- exakt die Anforderungsform der
/// realen Quelle), laesst den Koeder laufen, liest zurueck. true NUR, wenn der Zaehler geoeffnet hat UND
/// danach einen Wert > 0 traegt (K13: der Koeder MUSS beissen).
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
    long const fd       = ::syscall(__NR_perf_event_open, &attr, /*pid=*/0, /*cpu=*/-1, /*group_fd=*/-1,
                                    /*flags=*/0UL);
    if (fd < 0) return false;
    int const f = static_cast<int>(fd);
    (void)::ioctl(f, PERF_EVENT_IOC_RESET, 0);
    (void)::ioctl(f, PERF_EVENT_IOC_ENABLE, 0);
    std::uint64_t const koeder = pmc_koeder_pointer_chase();
    (void)::ioctl(f, PERF_EVENT_IOC_DISABLE, 0);
    std::uint64_t wert = 0;
    bool const    ok   = ::read(f, &wert, sizeof(wert)) == static_cast<::ssize_t>(sizeof(wert));
    (void)::close(f);
    // koeder wird verbraucht, damit der Pointer-Chase nicht wegoptimiert wird (er ist der ganze Punkt).
    return ok && wert > 0 && koeder != 0xFFFFFFFFFFFFFFFFULL;
#else
    (void)ev;
    return false;
#endif
}

} // namespace comdare::cache_engine::measurement
