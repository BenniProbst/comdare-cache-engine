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
// ZWEITE PRAEZISIERUNG (CI 16260, Job 385992, prod2/GenuineIntel): auch ein TEIL-Fenster ist KEINE
// Messung. "irgendwie gelaufen" (0 < t_running < t_enabled) genuegt nicht -- auf prod2 (4 GP-Counter je
// Thread unter HT, NMI-Watchdog, CI-Nachbarn) bekam ein Event Hardware-Zeit NUR ausserhalb des Chase,
// las ehrlich 0 und wurde als "ehrliche Absage" verbucht: im SELBEN Prozess kippte der Befund zwischen
// events=4/4 und 2/4 (dtlb_misses=0 in einem voll gemessenen 16384-Seiten-Chase ist physikalisch
// unmoeglich -- die 0 stammte aus einem nicht gemessenen Fenster-Anteil). Deshalb urteilt die 0 seither
// NUR aus einem VOLL gelaufenen Fenster (t_running == t_enabled), das Event ist GEPINNT (alles-oder-
// nichts-Scheduling: voll auf der PMU oder ERROR-Zustand, keine Teil-Fenster), jedes Fenster bekommt
// einen FRISCHEN fd (keine kumulative Zeit-Ambiguitaet, ERROR endet mit dem Fenster), und der Verlauf
// steht dem Aufrufer als PmcBissFenster zur Verfuegung (Multiplex-Beweis ausgewiesen statt verschluckt).
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
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#endif

namespace comdare::cache_engine::measurement {

/// GEOMETRIE des Koeders -- EINE Quelle fuer den Kern, seine Selbst-Beweise und die Tests (T-3: die Tests
/// frieren ihre eigenen Literale ein und pruefen sie GEGEN diese Konstanten, nie umgekehrt).
///   kPmcKoederBytes   64 MiB  -- groesser als jeder heutige L2 und groesser als der L3-Anteil eines Kerns;
///                               der Chase faellt bis in den Hauptspeicher durch.
///   kPmcKoederSchritt 4096 B  -- je Kettenglied eine EIGENE 4-KiB-Seite: die Kette laeuft ueber
///                               kPmcKoederSlots = 16384 verschiedene Seiten, mehr als jeder L2-DTLB haelt
///                               (Zen 5: 4096 Eintraege, Golden-Cove-STLB: 2048). Jeder Sprung ist damit ein
///                               Page-Walk -- gleich, ob die Seiten frisch oder warm sind.
inline constexpr std::size_t kPmcKoederBytes   = 64u * 1024u * 1024u;
inline constexpr std::size_t kPmcKoederSchritt = 4096u;
inline constexpr std::size_t kPmcKoederSlots   = kPmcKoederBytes / kPmcKoederSchritt;
static_assert(kPmcKoederSlots == 16384u, "pmc_koeder: 64 MiB / 4096 B = 16384 Kettenglieder (Geometrie-Anker).");
static_assert(kPmcKoederSlots > 4096u,
              "pmc_koeder: die Kette MUSS mehr Seiten beruehren, als der groesste bekannte L2-DTLB (4096 Eintraege) "
              "halten kann -- sonst beisst der DTLB-Koeder nur ueber die Erstberuehrung frischer Seiten "
              "(Geschichtsabhaengigkeit, #114-Re-Run 25.08.2026).");

/// Der KOEDER. Ein Pointer-Chase ueber 64 MiB, ein Kettenglied je 4-KiB-Seite, damit L1D-Read-Misses,
/// Last-Level-Misses UND DTLB-Misses real anfallen. Die Kette ist eine Permutation (jeder Slot genau einmal),
/// also nicht vom Prefetcher vorherzusehen, und der Rueckgabewert wird vom Aufrufer verbraucht -- sonst
/// optimierte der Compiler den ganzen Koeder weg und der Zaehler saehe nichts.
/// KEIN Zufall aus /dev/urandom: die Kette muss REPRODUZIERBAR beissen, sonst wackelt die Erkennung.
///
/// SEITEN DIREKT VOM KERNEL, KEIN ALLOKATOR-ZUSTAND (Befund 25.08.2026, #114-Re-Run; Beweisort
/// backups-workflow/20260825-diagnose-e07/, src_diag/b10_k5_diag3_koeder.cpp): bis dahin stand hier
/// `std::vector<std::size_t> kette(kSlots, 0)` mit kSlots = 64 MiB / 4096 -- also 16384 ELEMENTE zu 8 Byte
/// = 128 KiB = 32 Seiten, nicht 64 MiB. Diese Arbeitsmenge passt in jeden DTLB; die dtlb_misses (Page-Walks)
/// kamen ausschliesslich aus der Erstberuehrung FRISCHER Seiten: Aufruf 1 (anon-mmap, 1/33 Seiten resident)
/// 197 Walks, Aufruf 3 (glibc gab denselben Heap-Chunk zurueck, 33/33 resident) 4, Aufruf 4: 0. Die
/// Erkennung hing an der Allokator-Geschichte des Prozesses (im selben Prozess events=3/4, dann 2/4; unter
/// Last 160/300 Prozesspaare ungleich). Deshalb: (a) die Geometrie wie deklariert -- ein Kettenglied je
/// Seite ueber 64 MiB; (b) mmap statt Heap, damit kein Allokator-Zustand die Seiten vorwaermt, und
/// MADV_NOHUGEPAGE, damit "je Sprung eine neue Seite" auch unter THP=always wahr bleibt (eine 2-MiB-Seite
/// deckte 512 Glieder mit EINEM TLB-Eintrag). Gemessen (prod1, Zen 5, dtlb_misses je Fenster): warme Seiten
/// 74k Walks (1.6-1.9 ms), frische Seiten 110k-133k (10-14 ms); vorher 0-209.
/// Scheitert mmap (ENOMEM), liefert der Koeder den Werkzeugfehler-Sentinel, den pmc_event_beisst fail-closed
/// als "kein Biss, kein Wiederholungsgrund" liest.
[[nodiscard]] inline std::uint64_t pmc_koeder_pointer_chase() noexcept {
#if defined(__linux__)
    void* const roh = ::mmap(nullptr, kPmcKoederBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (roh == MAP_FAILED) return 0xFFFFFFFFFFFFFFFFULL;
    (void)::madvise(roh, kPmcKoederBytes, MADV_NOHUGEPAGE);
    auto* const           basis       = static_cast<std::size_t*>(roh);
    constexpr std::size_t kElemJeSlot = kPmcKoederSchritt / sizeof(std::size_t); // Slot i bei basis[i * kElemJeSlot]
    // Ein einfacher, deterministischer Zyklus ueber alle Slots: Schrittweite teilerfremd zu kPmcKoederSlots.
    constexpr std::size_t kSchritt = 4099u; // Primzahl, teilerfremd zu kPmcKoederSlots (2^14)
    std::size_t           pos      = 0;
    for (std::size_t i = 0; i < kPmcKoederSlots; ++i) {
        std::size_t const next   = (pos + kSchritt) % kPmcKoederSlots;
        basis[pos * kElemJeSlot] = next;
        pos                      = next;
    }
    std::uint64_t summe = 0;
    std::size_t   p     = 0;
    for (std::size_t i = 0; i < kPmcKoederSlots * 4; ++i) {
        p = basis[p * kElemJeSlot];
        summe += p;
    }
    (void)::munmap(roh, kPmcKoederBytes);
    return summe;
#else
    return 0;
#endif
}

/// FENSTER-DECKEL der Wiederholung ungueltiger Fenster (leer, Teilzeit, unplanbar; s. pmc_event_beisst).
/// Klein und endlich: ein Host, dessen PMU ueber so viele Koeder-Fenster hinweg keinem einzigen Event
/// ein VOLLES Fenster gibt, ist ehrlich unbrauchbar (fail-closed) -- der Deckel schuetzt nur gegen das
/// transiente Multiplexing-/Belegungs-Loch.
inline constexpr unsigned kPmcKoederFenster = 5u;

/// STATISTIK EINER BISS-PROBE (CI 16260, Job 385992): der Fenster-Verlauf, damit der Aufrufer eine
/// ehrliche 0 (voll gemessenes Fenster ohne Biss) von einer NICHT-MESSUNG (PMU verdraengt oder belegt)
/// unterscheiden kann. Ohne diese Unterscheidung truegen beide Faelle dieselbe nackte 0 -- genau daran
/// kippte prod2 im selben Prozess zwischen events=4/4 und 2/4.
struct PmcBissFenster {
    unsigned gefahren     = 0;     ///< gefahrene Koeder-Fenster insgesamt (<= kPmcKoederFenster)
    unsigned leer         = 0;     ///< Fenster ganz ohne Hardware-Zeit (t_running == 0)
    unsigned teilzeit     = 0;     ///< Fenster mit 0 < t_running < t_enabled und Wert 0 -- Multiplex-BEWEIS
    unsigned unplanbar    = 0;     ///< Fenster, in denen das gepinnte Event nie auf die PMU kam (read == EOF)
    bool     voll_fenster = false; ///< true, wenn ein VOLL gelaufenes Fenster das Urteil getragen hat
};

/// Oeffnet EIN Event self-monitoring (pid=0, cpu=-1, exclude_kernel -- die Event-IDENTITAET (type,
/// config) ist exakt die Anforderungsform der realen Quelle), laesst den Koeder laufen, liest zurueck.
/// true NUR, wenn der Zaehler geoeffnet hat UND ein Fenster einen Wert > 0 traegt (K13: der Koeder MUSS
/// beissen).
///
/// FENSTER-GUELTIGKEIT (CI 16073/382856 + CI 16260/385992): weder ein leeres NOCH ein Teil-Fenster ist
/// eine Messung. Unter PMU-Multiplexing/-Belegung (NMI-Watchdog, fremde perf-Nutzer, parallele
/// ctest-Nachbarn) kann ein Event ein Fenster ganz verpassen (t_running == 0) ODER nur einen Anteil
/// bekommen (0 < t_running < t_enabled) -- im Teil-Fenster kann die 0 schlicht heissen, dass der Chase
/// gerade nicht beobachtet wurde (prod2: dtlb_misses=0 ueber 16384 Seiten). Deshalb je Fenster:
///   attr.pinned = 1                 => alles-oder-nichts: das Event sitzt VOLL auf der PMU oder faellt
///                                      in den ERROR-Zustand (read() == EOF); Teil-Fenster verschwinden
///                                      strukturell. Die Scheduling-Prioritaet aendert die Event-
///                                      Identitaet nicht (type/config unveraendert).
///   FRISCHER fd je Fenster          => Zeiten/Werte sind Fenster-lokal (keine Kumulation), und ein
///                                      ERROR-Zustand endet mit dem Fenster statt am fd zu kleben.
///   wert > 0                        => BISS (ein gezaehlter Wert ist positiv beweisend) -- fertig.
///   wert == 0, t_running == t_enabled > 0 => VOLL gemessen und nicht gebissen: die ehrliche Absage --
///                                      fertig, kein weiterer Versuch.
///   t_running == 0                  => LEERES Fenster, keine Messung -> naechstes Fenster.
///   0 < t_running < t_enabled       => TEIL-Fenster (Multiplex-Beweis), keine Messung -> naechstes
///                                      Fenster (mit pinned nur als Randlage erreichbar; dennoch
///                                      klassifiziert, nie als 0 verbucht).
///   read() == EOF                   => Fenster UNPLANBAR (pinned-ERROR: PMU belegt) -> naechstes Fenster.
/// Alles gedeckelt auf kPmcKoederFenster; nach dem Deckel gilt fail-closed (kein Biss): eine dauerhaft
/// verdraengte oder belegte PMU wird nie behauptet. Werkzeugfehler (open/read scheitert hart, mmap-
/// Sentinel) bleiben fail-closed und sind KEIN Wiederholungsgrund.
///
/// `fenster` (optional) erhaelt den Verlauf (PmcBissFenster) -- der Aufrufer kann damit die ehrliche 0
/// von der Nicht-Messung unterscheiden und den Multiplex-Beweis ausweisen (V-1: Nenner statt nackter
/// Zahl).
[[nodiscard]] inline bool pmc_event_beisst(PmcEventSpec const& ev, PmcBissFenster* fenster = nullptr) noexcept {
#if defined(__linux__)
    struct LeseForm { // read_format-Layout ohne PERF_FORMAT_GROUP: Wert, dann die zwei Zeiten
        std::uint64_t wert;
        std::uint64_t zeit_aktiv;    // TIME_ENABLED: Event war logisch eingeschaltet
        std::uint64_t zeit_gelaufen; // TIME_RUNNING: Event sass real auf der PMU
    };
    PmcBissFenster stat;
    bool           biss = false;
    for (unsigned f = 0; f < kPmcKoederFenster; ++f) {
        struct ::perf_event_attr attr;
        std::memset(&attr, 0, sizeof(attr)); // Muellbits in Reserve-Feldern => EINVAL; immer memset
        attr.size           = sizeof(attr);
        attr.type           = ev.type;
        attr.config         = ev.config;
        attr.disabled       = 1;
        attr.exclude_kernel = 1;
        attr.exclude_hv     = 1;
        attr.pinned         = 1; // alles-oder-nichts-Scheduling (s. Kopf); Event ist sein eigener Leader
        attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        long const fd       = ::syscall(__NR_perf_event_open, &attr, /*pid=*/0, /*cpu=*/-1, /*group_fd=*/-1,
                                        /*flags=*/0UL);
        if (fd < 0) break; // oeffnet nicht (ENOENT/EACCES/...): fail-closed, kein Wiederholungsgrund
        int const fdi = static_cast<int>(fd);
        ++stat.gefahren;
        (void)::ioctl(fdi, PERF_EVENT_IOC_ENABLE, 0);
        std::uint64_t const koeder = pmc_koeder_pointer_chase();
        (void)::ioctl(fdi, PERF_EVENT_IOC_DISABLE, 0);
        LeseForm        lf{};
        ::ssize_t const n = ::read(fdi, &lf, sizeof(lf));
        (void)::close(fdi);
        // koeder wird verbraucht, damit der Pointer-Chase nicht wegoptimiert wird (er ist der ganze
        // Punkt). Werkzeugfehler (mmap-Sentinel, harter read-Fehler) sind fail-closed, kein Retry.
        if (koeder == 0xFFFFFFFFFFFFFFFFULL) break;
        if (n == 0) { // EOF: pinned-ERROR -- das Event kam in diesem Fenster nie auf die PMU
            ++stat.unplanbar;
            continue;
        }
        if (n != static_cast<::ssize_t>(sizeof(lf))) break; // harter Lesefehler: Werkzeugfehler
        if (lf.wert > 0) {
            biss              = true; // gezaehlt ist gezaehlt -- der Koeder hat gebissen
            stat.voll_fenster = lf.zeit_gelaufen == lf.zeit_aktiv;
            break;
        }
        if (lf.zeit_gelaufen == 0) { // leeres Fenster: keine Messung
            ++stat.leer;
            continue;
        }
        if (lf.zeit_gelaufen < lf.zeit_aktiv) { // Teil-Fenster: Multiplex-Beweis, KEINE Messung
            ++stat.teilzeit;
            continue;
        }
        stat.voll_fenster = true; // voll gelaufen und 0 gezaehlt: die ehrliche Absage steht
        break;
    }
    if (fenster != nullptr) *fenster = stat;
    return biss;
#else
    (void)ev;
    if (fenster != nullptr) *fenster = PmcBissFenster{};
    return false;
#endif
}

} // namespace comdare::cache_engine::measurement
