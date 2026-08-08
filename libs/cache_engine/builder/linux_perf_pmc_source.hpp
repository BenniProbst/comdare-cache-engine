#pragma once
// V5-#26 / Task #152 (CE-DL2) — LinuxPerfPmcSource: REALE CPU-Cache-Miss-Quelle via perf_event_open(2) (Linux).
//
// Linux-Spiegel der WindowsPcmPmcSource: Drop-in-Implementierung derselben `IPmcSource`-Naht (pmc_source.hpp),
// die die bereits existierenden 7 pmc_*-Spalten aus echten Performance-Monitoring-Countern statt aus
// `NullPmcSource`-0 befuellt. KEINE Aenderung an POD/Pipeline/PDF/CSV-Schema (pmc_source.hpp:6-9) --
// cache_engine_builder_iterator.hpp lazy_csv_header/format_csv_row bleiben UNVERAENDERT.
//
// WAS SIE REAL LIEFERT (B5/M-2-KORREKTUR 2026-08-06, erweitert M-3a 2026-08-07): der Kopf sagte frueher
// "+6 HW-Counter (... best-effort L2/coherence, RAPL-energy)". Das war die aeltere, zu optimistische Fassung
// und widersprach dem eigenen Code weiter unten. TATSAECHLICH GEOEFFNET werden VIER generische Counter --
// cache_misses_l1 (L1D/READ/MISS), cache_misses_l3 (LAST-LEVEL/READ/MISS, ehrlich LL), dtlb_misses
// (DTLB/READ/MISS) und seit M-3a branch_misses -- plus energy_micro_joules als best-effort RAPL-Snapshot
// (root-/Zonen-abhaengig). cache_misses_l2 und coherence_invalidations bleiben AUCH MIT dem Flag 0: dafuer
// gibt es keinen portablen generischen Counter, und ein RAW-Rateversuch ist bewusst unterblieben
// (Feld-Mapping + Konstruktor unten sagen es woertlich). Diese honest-0-Spalten sind im Anhang als solche
// zu fuehren, nicht als gemessen.
//
// M-3a (2026-08-07) -- branch_misses ist NICHT mehr honest-0: der Satz "branch_misses wird von KEINER
// PMC-Quelle befuellt" stand hier bis zu diesem Paket und ist damit ueberholt. Der Zaehler liegt als
// PERF_COUNT_HW_BRANCH_MISSES unter PERF_TYPE_HARDWARE (NICHT unter PERF_TYPE_HW_CACHE wie die drei
// anderen) -- ein generisches, vom Kernel auf jede Mikroarchitektur abgebildetes Event, das auf Intel
// UND AMD oeffnet. Das ist der Unterschied zum LL-Event, das auf AMD Zen5 mit ENOENT scheitert: dort
// fehlt die generische Abbildung, hier nicht. Fehlschlaege werden wie ueberall pro Feld gemeldet
// (branch_misses_source_available), nie durch eine erfundene 0 verdeckt.
//
// BUILD-SICHERHEIT (kritisch, analog windows_pcm_pmc_source.hpp): die GANZE Implementierung steht hinter
//     #if defined(COMDARE_ENABLE_PMC) && defined(__linux__)
// Ohne dieses Makro kompiliert diese Datei zu EINEM LEEREN Translation-Unit-Inhalt (nur das pragma once
// + Kommentare) — KEIN <linux/perf_event.h> wird inkludiert, kein Symbol entsteht. Auf Windows/MSVC und im
// Default-Build (COMDARE_ENABLE_PMC OFF) ist die Datei daher völlig inert — der Windows-Build bleibt UNBERÜHRT.
//
// AKTIVIERUNG (System-/Build-Schritt, NICHT Teil dieses Scaffolds): -DCOMDARE_ENABLE_PMC=ON auf einem Linux-Ziel.
// Es ist KEINE Vendor-Lib nötig (perf_event_open ist ein Kernel-Syscall) — daher kann der Linux-Zweig der
// Factory/CMake das Makro auch ohne find_library aktivieren. Optional PAPI als sekundäres Backend
// (-DCOMDARE_ENABLE_PAPI=ON + papi.h/libpapi), perf_event_open bleibt primär.
//
// Lebenszyklus-Vertrag (identisch zu WindowsPcmPmcSource / run_observable_perm):
//   begin()  = ioctl RESET+ENABLE der Counter-Gruppe (unmittelbar VOR t0 = steady_clock::now()).
//   end()    = ioctl DISABLE + read() → multiplex-korrigiertes Delta (unmittelbar NACH t1).
//   available() = true sobald MINDESTENS ein Counter live geöffnet werden konnte (ehrliche Teilverfügbarkeit).
//
// perf_event_open(2)-Referenz (man7, verifiziert): kein glibc-Wrapper → syscall(__NR_perf_event_open, ...);
// PERF_TYPE_HW_CACHE config = id | (op<<8) | (result<<16); self-monitoring pid=0,cpu=-1; attr.exclude_kernel=1
// (sonst EACCES unter Default perf_event_paranoid=2); read_format TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING →
// Multiplexing-Skalierung. RAPL ist system-wide (pid=-1) → meist nur mit paranoid<=0/CAP_PERFMON → best-effort.
// EHRLICHKEIT: pro Counter eigenes available-Flag; ein Fehlschlag (EACCES/EPERM/ENOENT/EINVAL) deaktiviert NUR
// dieses Feld (Wert bleibt 0), NIE erfundene/abgeleitete Werte.
//
// ==================================================================================================
// B-5 (2026-08-08) -- DIE ERRNO-KLASSEN WERDEN GETRENNT, UND EINE BESTANDS-BEGRUENDUNG WIRD KORRIGIERT
// ==================================================================================================
// WAS SICH AENDERT. Bis hierher lieferte PerfCounter::open() ein `bool` und warf damit drei
// verschiedene Naturen in EINEN Zustand. Jetzt liefert es measurement::PmcCounterOutcome
// (pmc_counter_outcome.hpp) -- die Trennung ist am Objekt begruendet, nicht theoretisch:
//
//   AM OBJEKT GEMESSEN, prod1 (AMD Ryzen 9 9950X3D, Zen 5, family 26 / model 68, Kernel
//   6.17.0-35-generic, perf_event_paranoid=1, uid 1001), eigene perf_event_open-Sonde:
//     PERF_TYPE_HW_CACHE / LL / READ / MISS   pid=0  cpu=-1  -> errno=2  (ENOENT)
//     amd_l3 type=17 config=0x104             pid=-1 cpu=0   -> errno=13 (EACCES)
//     amd_l3 type=17 config=0x104 per-task    pid=0  cpu=-1  -> errno=22 (EINVAL)
//
//   Drei Zeilen, drei Aussagen: das generische Last-Level-Event ist auf Zen 5 wirklich nicht
//   abgebildet; der ECHTE L3-Zaehler existiert dagegen sehr wohl, ist aber unter paranoid=1
//   gesperrt; und per-Task ist er strukturell nicht oeffenbar, weil ein Uncore-PMU je CCX zaehlt
//   (cpumask 0,8) und keinen Prozess kennt. Ein `false` haette alle drei zu "Quelle nicht da"
//   eingeebnet -- und genau daraus ist die folgende Falschaussage entstanden.
//
// RICHTIGSTELLUNG (belegt, nicht behauptet). ce `5c102e05` begruendet den Verzicht auf den AMD-Weg
// woertlich mit: "die dafuer noetige amd_l3-Uncore-PMU ist auf identischer Hardware/Kernel wie prod1
// als Modul vorhanden, aber NICHT GELADEN -- eine Infra-, keine Code-Frage." Das ist widerlegt:
// `/sys/bus/event_source/devices/amd_l3/type` liefert 17, und `.../amd_l3/format/` traegt die
// vollstaendige Feldbelegung. Der PMU IST geladen. Die Sperre ist eine RECHTE-Sperre (EACCES), und
// die ist eine andere Aussage als ein fehlender Zaehler -- behebbar ohne andere Hardware.
//
// DER AMD-ZWEITPFAD, und was er BEWUSST NICHT TUT. Der amd_l3-PMU wird jetzt versucht, sobald der
// generische LL-Weg mit EventNichtVorhanden scheitert -- Typ DYNAMISCH aus sysfs (heute 17; die Zahl
// ist keine Konstante und wird nirgends hartkodiert), Kodierung am Objekt aus `perf list --details`
// belegt: l3_lookup_state.l3_miss = event=0x4, umask=0x1, und mit der Feldbelegung aus format/
// (event=config:0-7, umask=config:8-15) ergibt das config = 0x104.
// Sein Ergebnis fuellt aber NICHT die Spalte pmc_cache_misses_l3, und das ist der Kern der
// Entscheidung: der Uncore zaehlt SYSTEM-WEIT je CCX (pid=-1 ist zwingend, per-Task gibt EINVAL),
// waehrend der generische LL-Zaehler auf Intel PER-TASK misst. Beide Zahlen unter EINER Ueberschrift
// zu fuehren waere exakt der Datenbruch, den der Ledger (:4506) fuer diesen Posten beschreibt --
// "verschiedene Semantik unter derselben Ueberschrift", nur quer ueber Vendoren statt ueber die Zeit.
// Der Zweitpfad beantwortet deshalb genau die Frage, die der Bestand falsch beantwortet hat: GIBT es
// den Zaehler auf dieser Maschine (dann ZugriffVerweigert) oder gibt es ihn nicht (EventNichtVorhanden)?
// Eine eigene, ehrlich benannte Wert-Spalte fuer den system-weiten Zaehler ist ein Folge-Paket
// (B-6-Natur, RAW-Events je Mikroarchitektur), kein stiller Einbau hier.
//
// INTEL BLEIBT UNBERUEHRT -- strukturell, nicht per Zusicherung: der Zweitpfad wird ausschliesslich
// betreten, NACHDEM der generische LL-Weg mit EventNichtVorhanden gescheitert ist. Auf Intel oeffnet
// dieser Weg, also wird der Zweitpfad dort nie erreicht. Er ist zusaetzlich hinter einer
// Vendor-Pruefung (amd_l3-sysfs-Pfad existiert) gefuehrt, die auf Intel-Maschinen leer laeuft.
// NICHT SELBST GETESTET: prod1 ist AMD; der Intel-Pfad ist hier unveraendert und wurde nicht
// nachgemessen.

#if defined(COMDARE_ENABLE_PMC) && defined(__linux__)

#include <cache_engine/measurement/pmc_counter_outcome.hpp> // B-5: PmcCounterOutcome + errno-Klassifikation
#include <cache_engine/measurement/pmc_source.hpp> // measurement::IPmcSource / PmcCounters (nach A2-Neben Stufe 1)

// Kernel-/POSIX-Header NUR hier (innerhalb des Guards) — sonst würde ein Nicht-Linux-Build sie anfordern.
#include <linux/perf_event.h> // struct perf_event_attr, PERF_* Konstanten
#include <sys/syscall.h>      // __NR_perf_event_open
#include <sys/ioctl.h>        // ioctl(), PERF_EVENT_IOC_*
#include <unistd.h>           // syscall(), read(), close()

#include <cerrno>
#include <cstdint>
#include <cstdio>  // FILE* sysfs-Lesen (RAPL)
#include <cstdlib> // strtoull / strtod
#include <cstring> // memset, strstr

#if defined(COMDARE_ENABLE_PAPI)
// Optionaler sekundärer Pfad (perf_event_open bleibt primär). Nur inkludiert, wenn der Nutzer das Makro UND
// die PAPI-Header bereitstellt. Wir nutzen ausschließlich die in der Referenz belegten, minimalen PAPI-Aufrufe.
#include <papi.h>
#endif

namespace comdare::cache_engine::builder {

namespace detail_linux_perf {

/// Kanonischer man7-Wrapper: es gibt KEINEN glibc-Wrapper für perf_event_open → direkter Syscall.
inline long perf_event_open(struct ::perf_event_attr* attr, ::pid_t pid, int cpu, int group_fd,
                            unsigned long flags) noexcept {
    return ::syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/// PERF_TYPE_HW_CACHE config-Kodierung (man7): config = cache_id | (op_id<<8) | (result_id<<16).
inline std::uint64_t cache_cfg(std::uint32_t id, std::uint32_t op, std::uint32_t res) noexcept {
    return static_cast<std::uint64_t>(id) | (static_cast<std::uint64_t>(op) << 8) |
           (static_cast<std::uint64_t>(res) << 16);
}

/// EIN perf-Counter-fd mit RAII (Resource Acquisition Is Initialization): besitzt genau einen fd, schließt ihn
/// im Destruktor. Move-only (kein Doppel-close, kein fd-Leak über viele Permutationen → kein EMFILE).
class PerfCounter {
public:
    PerfCounter() = default;
    ~PerfCounter() { reset_fd(-1); }
    PerfCounter(PerfCounter const&)            = delete;
    PerfCounter& operator=(PerfCounter const&) = delete;
    PerfCounter(PerfCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    PerfCounter& operator=(PerfCounter&& o) noexcept {
        if (this != &o) {
            reset_fd(o.fd_);
            o.fd_ = -1;
        }
        return *this;
    }

    /// Oeffnet den Counter. Liefert seit B-5 die KLASSE des Ergebnisses (measurement::PmcCounterOutcome)
    /// statt eines bool -- ENOENT ("dieses Event gibt es auf dieser CPU nicht"), EACCES/EPERM ("es gibt es,
    /// aber wir duerfen nicht") und EINVAL ("wir haben es falsch angefordert") sind drei verschiedene
    /// Aussagen, und der Bestand konnte sie strukturell nicht unterscheiden. Die Klassifikation selbst
    /// liegt in pmc_counter_outcome.hpp (dort auch die am Objekt gemessenen errno-Belege).
    ///
    /// DEFAULTS = der bisherige Weg, unveraendert: self-monitoring (pid=0, cpu=-1) mit exclude_kernel.
    /// Die drei Parameter sind NUR fuer Uncore-PMUs da, die per-Task strukturell nicht oeffnen (EINVAL)
    /// und keine Ring-Trennung kennen (exclude_kernel -> ebenfalls EINVAL). Jeder bestehende Aufruf
    /// nutzt die Defaults und verhaelt sich damit byte-gleich zu vorher.
    /// attr.disabled=1 -> Start erst per begin(). Die stderr-Sichtbarkeitszeile (2026-08-06) bleibt und
    /// traegt jetzt zusaetzlich das Klassen-Etikett, damit ein Log-Grep die Natur sieht, nicht nur die Zahl.
    measurement::PmcCounterOutcome open(std::uint32_t type, std::uint64_t config, char const* event_name = "",
                                        ::pid_t pid = 0, int cpu = -1, bool exclude_kernel = true) noexcept {
        struct ::perf_event_attr attr;
        std::memset(&attr, 0, sizeof(attr)); // Muellbits in Reserve-Feldern → EINVAL; immer memset.
        attr.type           = type;
        attr.size           = sizeof(attr); // PFLICHT (Forward-Compat / ENTER-FIELD-Check) → sonst EINVAL.
        attr.config         = config;
        attr.disabled       = 1;                        // gestartet wird per ioctl ENABLE in begin().
        attr.exclude_kernel = exclude_kernel ? 1U : 0U; // unter Default-paranoid(2) noetig (sonst EACCES).
        attr.exclude_hv     = exclude_kernel ? 1U : 0U; // Hypervisor ausschliessen.
        attr.inherit        = 0;                        // keine Kind-Vererbung.
        attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        long const r        = perf_event_open(&attr, pid, cpu, /*group_fd*/ -1, /*flags*/ 0);
        if (r < 0) {
            int const eno = errno; // SOFORT sichern -- fprintf/strerror koennen errno selbst ueberschreiben.
            measurement::PmcCounterOutcome const oc = measurement::classify_pmc_open_errno(eno);
            std::fprintf(stderr,
                         "[PMC-DIAG] perf_event_open fehlgeschlagen: event=%s klasse=%.*s type=%u "
                         "config=%llu pid=%d cpu=%d errno=%d (%s)\n",
                         event_name, static_cast<int>(measurement::pmc_outcome_label(oc).size()),
                         measurement::pmc_outcome_label(oc).data(), type, static_cast<unsigned long long>(config),
                         static_cast<int>(pid), cpu, eno, std::strerror(eno));
            fd_ = -1;
            return oc;
        }
        fd_ = static_cast<int>(r);
        return measurement::PmcCounterOutcome::Offen;
    }

    void reset() const noexcept {
        if (fd_ >= 0) (void)::ioctl(fd_, PERF_EVENT_IOC_RESET, 0);
    }
    void enable() const noexcept {
        if (fd_ >= 0) (void)::ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
    }
    void disable() const noexcept {
        if (fd_ >= 0) (void)::ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
    }

    /// Multiplex-korrigierter Count. ok=false → Wert ungültig (Counter lief nie / read-Fehler) → NICHT als
    /// echte 0 ausgeben. time_running<time_enabled → hochskaliert (scaled=true, Schätzung, nicht exakt).
    std::uint64_t read_scaled(bool& ok, bool& scaled) const noexcept {
        ok     = false;
        scaled = false;
        if (fd_ < 0) return 0;
        struct {
            std::uint64_t value;
            std::uint64_t t_enabled;
            std::uint64_t t_running;
        } d{};
        if (::read(fd_, &d, sizeof(d)) != static_cast<::ssize_t>(sizeof(d))) return 0; // zu klein → ENOSPC
        if (d.t_running == 0) return 0; // Counter lief nie (Multiplexing-Verdrängung) → ungültig.
        ok = true;
        if (d.t_running < d.t_enabled) { // Multiplexing → ehrlich hochrechnen.
            scaled = true;
            return static_cast<std::uint64_t>(static_cast<double>(d.value) * static_cast<double>(d.t_enabled) /
                                              static_cast<double>(d.t_running));
        }
        return d.value; // kein Multiplexing → exakt.
    }

    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }

private:
    void reset_fd(int next) noexcept {
        if (fd_ >= 0) (void)::close(fd_);
        fd_ = next;
    }
    int fd_ = -1;
};

// -- B-5: die DYNAMISCH ermittelten PMU-Kennzahlen aus sysfs --------------------------------------
// Ein PMU-Typ ist KEINE Konstante. Er wird beim Registrieren des Treibers vergeben und kann sich je
// Kernel und Maschine unterscheiden -- auf prod1 traegt amd_l3 heute die 17, aber nichts garantiert
// das. Deshalb wird er gelesen, nie hartkodiert; existiert der Pfad nicht, gibt es diesen PMU auf
// dieser Maschine schlicht nicht (Rueckgabe -1) und der Aufrufer laesst den Zweitpfad aus.

/// Liest eine kleine vorzeichenlose Dezimalzahl aus einer sysfs-Datei. false = nicht lesbar/leer/kein
/// Zahlenpraefix -- in jedem dieser Faelle wird KEIN Ersatzwert geliefert (fail-closed, Praezedenz
/// c1c76c87: der Nenner kommt aus derselben Quelle wie der Zaehler oder er kommt gar nicht).
inline bool read_sysfs_u32(char const* path, std::uint32_t& out) noexcept {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) return false;
    char              buf[64] = {0};
    std::size_t const n       = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    if (n == 0) return false;
    char* endp                 = nullptr;
    errno                      = 0;
    unsigned long long const v = std::strtoull(buf, &endp, 10);
    if (endp == buf || errno != 0 || v > 0xFFFFFFFFULL) return false;
    out = static_cast<std::uint32_t>(v);
    return true;
}

/// Der perf-Typ eines dynamischen PMU aus /sys/bus/event_source/devices/<name>/type.
/// false = dieser PMU existiert auf dieser Maschine nicht (Treiber nicht geladen / andere Hardware).
inline bool pmu_type_from_sysfs(char const* pmu_name, std::uint32_t& out_type) noexcept {
    char path[128] = {0};
    std::snprintf(path, sizeof(path), "/sys/bus/event_source/devices/%s/type", pmu_name);
    return read_sysfs_u32(path, out_type);
}

/// Die ERSTE CPU aus der cpumask eines Uncore-PMU. Ein Uncore-PMU zaehlt je Domaene (auf prod1 je CCX,
/// cpumask "0,8"); der Kernel erwartet, dass man ihn auf einem der genannten Vertreter oeffnet. Wir
/// nehmen den ersten -- fuer die FRAGE, ob der Zaehler ueberhaupt zugaenglich ist, genuegt einer.
/// Format: kommaseparierte Liste, Bereiche mit '-' ("0,8" oder "0-7"). Wir lesen nur die erste Zahl.
inline bool pmu_first_cpu_from_sysfs(char const* pmu_name, int& out_cpu) noexcept {
    char path[128] = {0};
    std::snprintf(path, sizeof(path), "/sys/bus/event_source/devices/%s/cpumask", pmu_name);
    std::uint32_t first = 0;
    if (!read_sysfs_u32(path, first)) return false;
    out_cpu = static_cast<int>(first);
    return true;
}

// -- B-5 Runde 2: DIE CORE-PMU-DOMAENEN DIESER MASCHINE, als MECHANISMUS statt Vendor-Sonderfall ----
//
// DAS PROBLEM, das dieser Block loest. `PERF_TYPE_HW_CACHE` mit cpu=-1 ist AELTER als die
// Hybrid-Architektur: er kennt nur EINEN Core-PMU. Auf einem Alder/Raptor Lake registriert der Kernel
// aber ZWEI -- `cpu_core` (P) und `cpu_atom` (E) -- und KEINEN generischen `cpu`. Ein Zaehler, der nur
// den generischen Weg kennt, oeffnet dort gar nicht. Genau das ist der Zustand, in dem `pmc:intel`
// seit dem 06.08. rot faellt: nicht EIN Zaehler fehlt, sondern ALLE VIER (available = l1d||ll||dtlb||
// branch, und beide Smoke-Tests fallen an genau diesem Praedikat).
//
// DIE LOESUNG STEHT IM KERNEL-HEADER, sie ist nicht rekonstruiert. <linux/perf_event.h> woertlich:
//     attr.config layout for type PERF_TYPE_HARDWARE and PERF_TYPE_HW_CACHE
//     PERF_TYPE_HARDWARE:  0xEEEEEEEE000000AA   AA: hardware event ID
//     PERF_TYPE_HW_CACHE:  0xEEEEEEEE00DDCCBB   BB/CC/DD: cache/op/result ID
//                          EEEEEEEE: PMU type ID
//     #define PERF_PMU_TYPE_SHIFT 32
// Die GENERISCHE Kodierung bleibt also unveraendert -- nur die oberen 32 Bit adressieren den PMU.
// Damit braucht der Hybrid-Weg KEINE rohen event/umask-Werte je Mikroarchitektur (das waere B-6 und
// waere ohne prod2 ein Rateversuch gewesen); er benutzt dieselben generischen Event-IDs wie bisher.
//
// AM OBJEKT VERIFIZIERT auf prod1 (cpu-PMU type=4 aus sysfs), eigene Sonde:
//     L1D/RD/MISS ohne Praefix        type=3 config=0x10000       -> oeffnet, zaehlt
//     L1D/RD/MISS MIT Praefix (cpu)   type=3 config=0x400010000   -> oeffnet, zaehlt
//     BRANCH_MISSES MIT Praefix (cpu) type=0 config=0x400000005   -> oeffnet, zaehlt
//     L1D/RD/MISS MIT Praefix(amd_l3) type=3 config=0x1100010000  -> errno=2, korrekt abgewiesen
// prod1 ist NICHT hybrid -- aber der Mechanismus ist vendorneutral, und das war hier pruefbar.
//
// WARNUNG, die den Bau bestimmt (ebenfalls am Objekt): ein PMU-Typ, den es GAR NICHT GIBT, wird NICHT
// abgewiesen -- `config = (9999 << 32) | ...` oeffnete und lieferte eine Zahl. Der Kernel faellt still
// auf einen anderen PMU zurueck. Das ist ein stiller Rueckfall im Kernel selbst, und daraus folgt hart:
// der Typ wird IMMER aus sysfs gelesen, nie geraten, nie hartkodiert. Existiert der sysfs-Pfad nicht,
// wird gar nicht erst geoeffnet. Sonst truegen Zellen Zahlen vom falschen PMU -- plausibel und falsch.

/// PERF_PMU_TYPE_SHIFT ist erst in neueren Kernel-Headern definiert. Der WERT ist ABI (er steht im
/// oben zitierten Layout-Kommentar und kann sich nicht aendern, ohne die Schnittstelle zu brechen) --
/// die eigene Definition ist deshalb ein Fallback fuer aeltere Header, kein zweiter Wahrheitsort.
#if !defined(PERF_PMU_TYPE_SHIFT)
#define PERF_PMU_TYPE_SHIFT 32
#endif

/// config mit vorangestelltem PMU-Typ. typ==0 laesst die config unveraendert -- der Kernel-Header sagt
/// dazu "If the PMU type ID is 0, the PERF_TYPE_RAW will be applied", weshalb 0 hier NIE gesetzt wird:
/// 0 heisst bei uns "kein Praefix noetig", und dann geht die rohe config raus.
inline std::uint64_t mit_pmu_typ(std::uint32_t typ, std::uint64_t config) noexcept {
    if (typ == 0) return config;
    return (static_cast<std::uint64_t>(typ) << PERF_PMU_TYPE_SHIFT) | config;
}

/// Die Core-PMU-Domaenen, auf denen ein GENERISCHER Zaehler leben kann -- in der Reihenfolge, in der
/// sie versucht werden. Der Name ist der sysfs-Name; der Typ wird zur Laufzeit daraus aufgeloest.
///
/// WARUM DIESE REIHENFOLGE: `cpu` zuerst, weil auf jeder uniformen Maschine (prod1, jeder AMD, jeder
/// nicht-hybride Intel) genau er existiert -- dort aendert sich damit NICHTS, und das ist Absicht.
/// `cpu_core`/`cpu_atom` gibt es nur auf Hybrid, und dort gibt es `cpu` nicht; die Kette faellt also
/// nie durch, sondern trifft auf jeder Maschine genau eine Wahl.
///
/// P/E-GETRENNTE MESSUNG IST HIER NICHT GELOEST, und das sage ich ausdruecklich statt es zu verdecken:
/// der Owner-KERN verlangt, dass P- und E-Core GETRENNT gemessen und ausgewertet werden ("nicht
/// gemittelt, nicht zusammengefasst, nicht 'die erste, die antwortet'"). Dieser Block macht die Zaehler
/// auf Hybrid ueberhaupt erst OEFFENBAR -- welche Domaene gefahren wurde, wandert ins Log, aber die
/// getrennte Erhebung ueber die NUMA-/Core-Unterachse ist eigene Arbeit (Board-Posten P/E-Core).
/// Solange sie nicht steht, ist die gelieferte Zahl EINE Domaene mit Namen, keine anonyme Mischung.
inline constexpr char const* kCorePmuDomaenen[]   = {"cpu", "cpu_core", "cpu_atom"};
inline constexpr std::size_t kCorePmuDomaenenZahl = sizeof(kCorePmuDomaenen) / sizeof(kCorePmuDomaenen[0]);

/// best-effort RAPL-Paket-Energie über /sys/class/powercap/intel-rapl:0/energy_uj (bereits in Mikrojoule;
/// keine scale-Rechnung). KEIN perf nötig. Seit Linux 5.10 oft 0400 (root-only) → non-root liest -1 → energy
/// bleibt 0/available=false. Wraparound via max_energy_range_uj wird zwischen begin()/end() berücksichtigt.
inline bool read_rapl_uj(std::uint64_t& out_uj) noexcept {
    std::FILE* f = std::fopen("/sys/class/powercap/intel-rapl:0/energy_uj", "rb");
    if (f == nullptr) return false; // keine RAPL-Zone / kein Leserecht → ehrlich nicht verfügbar.
    char              buf[64] = {0};
    std::size_t const n       = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    if (n == 0) return false;
    char* endp                 = nullptr;
    errno                      = 0;
    unsigned long long const v = std::strtoull(buf, &endp, 10);
    if (endp == buf || errno != 0) return false;
    out_uj = static_cast<std::uint64_t>(v);
    return true;
}

inline bool read_rapl_max_range_uj(std::uint64_t& out_uj) noexcept {
    std::FILE* f = std::fopen("/sys/class/powercap/intel-rapl:0/max_energy_range_uj", "rb");
    if (f == nullptr) return false;
    char              buf[64] = {0};
    std::size_t const n       = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    if (n == 0) return false;
    char* endp                 = nullptr;
    errno                      = 0;
    unsigned long long const v = std::strtoull(buf, &endp, 10);
    if (endp == buf || errno != 0) return false;
    out_uj = static_cast<std::uint64_t>(v);
    return true;
}

} // namespace detail_linux_perf

/// REALE PMC-Quelle (Linux perf_event_open). begin() = RESET+ENABLE, end() = DISABLE+read → Counter-Delta.
/// Spiegelt WindowsPcmPmcSource: identische 4 Override-Signaturen + privates ready_-Aggregat. `available()`
/// spiegelt EHRLICH, ob >=1 HW-Counter live geöffnet wurde; pro Feld eigenes available-Flag (Teilverfügbarkeit).
/// Feld-Mapping (PmcCounters → perf-Counter):
///   cache_misses_l1         <- PERF_COUNT_HW_CACHE_L1D / OP_READ / RESULT_MISS   (portabel)
///   cache_misses_l3         <- PERF_COUNT_HW_CACHE_LL  / OP_READ / RESULT_MISS   (Last-Level; ehrlich LL)
///   dtlb_misses             <- PERF_COUNT_HW_CACHE_DTLB/ OP_READ / RESULT_MISS   (portabel)
///   branch_misses           <- PERF_TYPE_HARDWARE / PERF_COUNT_HW_BRANCH_MISSES  (M-3a; andere type-Klasse!)
///   cache_misses_l2         <- KEIN portabler generischer Counter → bleibt 0 (kein RAW-Rateversuch)
///   coherence_invalidations <- KEIN portabler generischer Counter → bleibt 0 (kein RAW-Rateversuch)
///   energy_micro_joules     <- best-effort RAPL (powercap energy_uj, Delta) → 0 ohne Zone/Leserecht
class LinuxPerfPmcSource final : public measurement::IPmcSource {
public:
    LinuxPerfPmcSource() noexcept {
        using namespace detail_linux_perf;
        using measurement::PmcCounterOutcome;
        // Jeder Counter wird INDIVIDUELL geöffnet; ein Fehlschlag deaktiviert NUR dieses Feld (nicht die Source).
        // B-5 Runde 2: JEDER generische Zaehler geht jetzt durch die Domaenen-Kette (oeffne_generisch_).
        // Auf uniformen Maschinen ist das der bisherige Weg, Byte fuer Byte; auf Hybrid-Intel ist es der
        // einzige, der ueberhaupt oeffnet -- dort kennt der generische PERF_TYPE_HW_CACHE die getrennten
        // Domaenen cpu_core/cpu_atom nicht und liefert fuer ALLE VIER Zaehler nichts.
        l1d_oc_ = oeffne_generisch_(
            c_l1d_, PERF_TYPE_HW_CACHE,
            cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS),
            "cache_misses_l1", l1d_domaene_);
        ll_oc_ = oeffne_generisch_(
            c_ll_, PERF_TYPE_HW_CACHE,
            cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS),
            "cache_misses_l3_ll", ll_domaene_);
        // B-5: der AMD-ZWEITPFAD. Er wird NUR betreten, wenn der generische Weg das Event auf dieser CPU
        // gar nicht kennt -- also genau im ENOENT-Fall von Zen 5. Auf Intel oeffnet der generische Weg,
        // damit ist ll_oc_ dort Offen und dieser Block strukturell unerreichbar (kein Intel-Risiko).
        // ER FUELLT KEINEN WERT (s. Kopf): der Uncore zaehlt system-weit je CCX, der generische Zaehler
        // per-Task -- beides unter einer Ueberschrift waere der Datenbruch, den B-5 gerade verhindert.
        // Was er liefert, ist die richtige ANTWORT auf die Frage "warum kein Wert": gibt es den Zaehler
        // auf dieser Maschine ueberhaupt (dann ZugriffVerweigert), oder gibt es ihn nicht?
        if (ll_oc_ == PmcCounterOutcome::EventNichtVorhanden) {
            l3_pmu_oc_ = oeffne_amd_l3_uncore_();
            // Die Sonde muss SPRECHEN, sonst ist die Diagnose gebaut und stumm -- und der naechste Leser
            // steht wieder vor derselben Frage wie 5c102e05. Die Zeile nennt die geschaerfte Begruendung.
            std::fprintf(stderr,
                         "[PMC-DIAG] cache_misses_l3: generischer LL-Zaehler auf dieser CPU nicht "
                         "abgebildet; amd_l3-Uncore-Sonde sagt: %.*s (Zelle bleibt n/a -- der Uncore "
                         "zaehlt system-weit je CCX und gehoert nicht in eine per-Task-Spalte)\n",
                         static_cast<int>(measurement::pmc_outcome_label(l3_pmu_oc_).size()),
                         measurement::pmc_outcome_label(l3_pmu_oc_).data());
        }
        dtlb_oc_ = oeffne_generisch_(
            c_dtlb_, PERF_TYPE_HW_CACHE,
            cache_cfg(PERF_COUNT_HW_CACHE_DTLB, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS),
            "dtlb_misses", dtlb_domaene_);
        // M-3a (2026-08-07): vierter Zaehler, exakt nach dem Muster der drei obigen -- ABER unter einer
        // anderen type-Klasse. PERF_COUNT_HW_BRANCH_MISSES ist ein PERF_TYPE_HARDWARE-Event, kein
        // PERF_TYPE_HW_CACHE-Event; es gibt deshalb KEINE cache_cfg()-Kodierung (id|op<<8|res<<16), die
        // config ist der Event-Wert selbst. Das ist kein Sonderweg, sondern die man7-Kodierung fuer diese
        // Klasse -- ein cache_cfg()-Aufruf hier waere ein stiller Fehler (falsche config -> EINVAL oder,
        // schlimmer, ein anderes Event).
        // Auch der HARDWARE-Zaehler laeuft durch die Kette: der Kernel-Header dokumentiert den
        // PMU-Typ-Praefix fuer PERF_TYPE_HARDWARE genauso wie fuer PERF_TYPE_HW_CACHE (am Objekt
        // verifiziert: config=0x400000005 oeffnete und zaehlte).
        branch_oc_ = oeffne_generisch_(c_branch_, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, "branch_misses",
                                       branch_domaene_);
        // L2 + coherence_invalidations: KEIN portabler generischer Counter → bewusst NICHT geöffnet, Feld 0.
        // RAPL: best-effort sysfs-Snapshot; Verfügbarkeit erst bei begin() (Lesbarkeit kann variieren).
        ready_ = l1d_offen_() || ll_offen_() || dtlb_offen_() || branch_offen_();
        // Sichtbarkeit der Domaenen-Wahl: nur, wenn wirklich eine Nicht-Standard-Domaene gefahren wurde.
        // Auf uniformen Maschinen bleibt die Ausgabe damit exakt so still wie vorher (kein Log-Rauschen
        // in der gesamten bestehenden Flotte); auf Hybrid sagt sie, WO gezaehlt wurde.
        if (hat_nichtgenerische_domaene_()) {
            std::fprintf(stderr,
                         "[PMC-DIAG] Core-PMU-Domaenen gefahren: l1=%s ll=%s dtlb=%s branch=%s "
                         "(Hybrid-Maschine -- P/E-getrennte Erhebung ist eigene Arbeit, diese Zahlen "
                         "stammen aus der genannten Domaene)\n",
                         l1d_domaene_, ll_domaene_, dtlb_domaene_, branch_domaene_);
        }
#if defined(COMDARE_ENABLE_PAPI)
        init_papi_fallback_(); // additiv: nur falls perf-Counter ausfielen UND PAPI verfügbar.
#endif
    }

    void begin() noexcept override {
        // Counter-Gruppe auf 0 + zählen an, UNMITTELBAR vor t0 (run_observable_perm Z.165).
        if (l1d_offen_()) {
            c_l1d_.reset();
            c_l1d_.enable();
        }
        if (ll_offen_()) {
            c_ll_.reset();
            c_ll_.enable();
        }
        if (dtlb_offen_()) {
            c_dtlb_.reset();
            c_dtlb_.enable();
        }
        if (branch_offen_()) {
            c_branch_.reset();
            c_branch_.enable();
        }
        if (l3_uncore_offen_()) {
            c_l3_uncore_.reset();
            c_l3_uncore_.enable();
        }
        // RAPL: monotoner Akkumulator → Start-Snapshot für Delta.
        rapl_have_start_ = detail_linux_perf::read_rapl_uj(rapl_start_uj_);
#if defined(COMDARE_ENABLE_PAPI)
        if (!ready_ && papi_ready_) (void)PAPI_start(papi_evtset_); // sekundärer Pfad: zählen an.
#endif
    }

    [[nodiscard]] measurement::PmcCounters end() noexcept override {
        using namespace detail_linux_perf;
        measurement::PmcCounters c; // alle 0, available=false (POD-Default) — ehrliche Basis.
        if (!ready_) {
#if defined(COMDARE_ENABLE_PAPI)
            return end_papi_fallback_(c); // perf nicht live → ggf. PAPI; sonst available=false.
#else
            return c;
#endif
        }
        // Counter stoppen (DISABLE), UNMITTELBAR nach t1 (run_observable_perm Z.170), dann read.
        if (l1d_offen_()) c_l1d_.disable();
        if (ll_offen_()) c_ll_.disable();
        if (dtlb_offen_()) c_dtlb_.disable();
        if (branch_offen_()) c_branch_.disable();
        if (l3_uncore_offen_()) c_l3_uncore_.disable();

        // B-5 (2026-08-08) -- DIE FLAGS SIND JETZT EINE LESE-AUSSAGE, NICHT NUR EINE OEFFNUNGS-AUSSAGE.
        // Der Vorgaenger-Kommentar sagte hier woertlich, ein spaeter leerer read() (Multiplexing-Verdraengung,
        // read_scaled ok=false) sei "eine ANDERE, hier unveraenderte Fehlerklasse (Feld bleibt 0, kein Token)".
        // Das war ein bewusst stehen gelassener STILLER RUECKFALL: der Zaehler oeffnete, lieferte aber nichts,
        // und die Zelle trug trotzdem eine 0 unter einem "Quelle war da"-Flag -- ununterscheidbar von einer
        // echten Nullmessung. Genau die Bauform, die der Owner-KERN verbietet ("Fehlschlag endet in return 0
        // ohne Anzeige"). Die Flags werden deshalb erst UNTEN gesetzt, nachdem gelesen wurde, und nur dann,
        // wenn dieser Zaehler wirklich einen Wert geliefert hat. Ein leerer read() schlaegt jetzt auf
        // NichtGelesen um (-> Zelle "failed", nicht 0).

        bool ok = false, scaled = false;
        bool any = false;
        if (l1d_offen_()) {
            std::uint64_t const v = c_l1d_.read_scaled(ok, scaled);
            if (ok) {
                c.cache_misses_l1                  = v;
                c.cache_misses_l1_source_available = true;
                any                                = true;
                if (scaled) scaled_ = true;
            } else {
                l1d_oc_ = measurement::PmcCounterOutcome::NichtGelesen; // geoeffnet, aber nichts geliefert
            }
        }
        if (ll_offen_()) {
            // LL = Last-Level (CPU-abhängig L2 ODER L3) → ehrlich in das L3-Feld (Last-Level), NICHT L2 erfinden.
            std::uint64_t const v = c_ll_.read_scaled(ok, scaled);
            if (ok) {
                c.cache_misses_l3                  = v;
                c.cache_misses_l3_source_available = true;
                any                                = true;
                if (scaled) scaled_ = true;
            } else {
                ll_oc_ = measurement::PmcCounterOutcome::NichtGelesen;
            }
        }
        if (dtlb_offen_()) {
            std::uint64_t const v = c_dtlb_.read_scaled(ok, scaled);
            if (ok) {
                c.dtlb_misses                  = v;
                c.dtlb_misses_source_available = true;
                any                            = true;
                if (scaled) scaled_ = true;
            } else {
                dtlb_oc_ = measurement::PmcCounterOutcome::NichtGelesen;
            }
        }
        if (branch_offen_()) {
            std::uint64_t const v = c_branch_.read_scaled(ok, scaled);
            if (ok) {
                c.branch_misses                  = v;
                c.branch_misses_source_available = true;
                any                              = true;
                if (scaled) scaled_ = true;
            } else {
                branch_oc_ = measurement::PmcCounterOutcome::NichtGelesen;
            }
        }
        // B-5 Runde 2: der amd_l3-Uncore-Wert. EIGENE Groesse, EIGENE Spalte -- er geht bewusst NICHT
        // nach cache_misses_l3 (per-Task), weil er system-weit je CCX zaehlt. `any` wird von ihm NICHT
        // gesetzt: er ist kein Prueflings-Wert und darf allein keine Zeile zur Messung erklaeren.
        if (l3_uncore_offen_()) {
            std::uint64_t const v = c_l3_uncore_.read_scaled(ok, scaled);
            if (ok) {
                c.l3_miss_uncore_systemweit                  = v;
                c.l3_miss_uncore_systemweit_source_available = true;
                if (scaled) scaled_ = true;
            } else {
                l3_pmu_oc_ = measurement::PmcCounterOutcome::NichtGelesen;
            }
        }
        // best-effort RAPL-Energie-Delta (Mikrojoule). Nur wenn Start+Ende lesbar.
        std::uint64_t rapl_end_uj = 0;
        if (rapl_have_start_ && read_rapl_uj(rapl_end_uj)) {
            std::uint64_t delta_uj;
            if (rapl_end_uj >= rapl_start_uj_) {
                delta_uj = rapl_end_uj - rapl_start_uj_;
            } else {
                // Wraparound: + max_energy_range_uj (modular), sonst best-effort 0.
                std::uint64_t max_range = 0;
                delta_uj                = read_rapl_max_range_uj(max_range) && max_range > rapl_start_uj_
                                              ? (max_range - rapl_start_uj_) + rapl_end_uj
                                              : 0;
            }
            c.energy_micro_joules = delta_uj;
            any                   = any || (delta_uj != 0);
            // B5/M-2-KORREKTUR-3 (2026-08-06, Owner-KERN "stiller Rueckfall ist verboten", verallgemeinert
            // aus dem L3-Befund): das RAPL-Delta wurde HIER WIRKLICH GELESEN (Start- und Ende-Snapshot beide
            // erfolgreich) -- unabhaengig davon, ob es zufaellig 0 betraegt. Ohne dieses Flag wuerde ein
            // fehlendes RAPL-Zonen-Leserecht (root-only seit Linux 5.10, s. Kopf-Kommentar) denselben
            // Zahlenwert 0 erzeugen wie ein echtes Null-Delta -- exakt der stille Rueckfall, den l2/l3/
            // coherence bereits nicht mehr zeigen.
            c.energy_micro_joules_source_available = true;
        }
        // L2 + coherence_invalidations bleiben EHRLICH 0 (kein portabler generischer Counter).
        c.available = any; // true sobald >=1 Counter echt geliefert hat.
        return c;
    }

    [[nodiscard]] bool available() const noexcept override {
#if defined(COMDARE_ENABLE_PAPI)
        return ready_ || papi_ready_;
#else
        return ready_;
#endif
    }
    [[nodiscard]] std::string_view name() const noexcept override {
#if defined(COMDARE_ENABLE_PAPI)
        if (!ready_ && papi_ready_) return "papi-linux (perf-fallback)";
#endif
        return "linux-perf-pmc";
    }

private:
    detail_linux_perf::PerfCounter c_l1d_{};       ///< L1-D read miss
    detail_linux_perf::PerfCounter c_ll_{};        ///< Last-Level (L3) read miss
    detail_linux_perf::PerfCounter c_dtlb_{};      ///< dTLB read miss
    detail_linux_perf::PerfCounter c_branch_{};    ///< Branch miss (M-3a, PERF_TYPE_HARDWARE)
    detail_linux_perf::PerfCounter c_l3_uncore_{}; ///< B-5 R2: amd_l3-Uncore, SYSTEM-WEIT je CCX
    // B-5: die vier bool-Flags sind KLASSEN geworden. Value-Initialisierung liefert Offen (== 0), deshalb
    // wird jedes Feld im Konstruktor unbedingt aus dem open()-Ergebnis gesetzt -- ein nicht gesetztes Feld
    // stuende sonst still auf "geoeffnet". pmc_outcome_hat_wert() ist die Single-Source der Frage
    // "traegt dieser Zaehler einen Wert"; die vier Praedikate unten sind nur ihre benannte Anwendung.
    measurement::PmcCounterOutcome l1d_oc_    = measurement::PmcCounterOutcome::UnbekannterFehler;
    measurement::PmcCounterOutcome ll_oc_     = measurement::PmcCounterOutcome::UnbekannterFehler;
    measurement::PmcCounterOutcome dtlb_oc_   = measurement::PmcCounterOutcome::UnbekannterFehler;
    measurement::PmcCounterOutcome branch_oc_ = measurement::PmcCounterOutcome::UnbekannterFehler;
    /// Das Ergebnis der amd_l3-Sonde. Nur belegt, wenn der generische LL-Weg mit EventNichtVorhanden
    /// scheiterte -- sonst bleibt es EventNichtVorhanden ("nicht gefragt, weil nicht noetig").
    /// Es fuellt bewusst KEINEN Wert (s. Kopf), sondern schaerft die BEGRUENDUNG im Log.
    measurement::PmcCounterOutcome l3_pmu_oc_ = measurement::PmcCounterOutcome::EventNichtVorhanden;
    /// Auf WELCHER Core-PMU-Domaene der jeweilige Zaehler wirklich geoeffnet hat. Auf uniformen
    /// Maschinen durchgaengig "generisch"; auf Hybrid der sysfs-Name (cpu_core/cpu_atom). Zeigt an,
    /// WO gezaehlt wurde -- eine Zahl von einer Hybrid-Maschine bleibt damit nie anonym.
    char const* l1d_domaene_    = "generisch";
    char const* ll_domaene_     = "generisch";
    char const* dtlb_domaene_   = "generisch";
    char const* branch_domaene_ = "generisch";

    [[nodiscard]] bool l1d_offen_() const noexcept { return measurement::pmc_outcome_hat_wert(l1d_oc_); }
    [[nodiscard]] bool ll_offen_() const noexcept { return measurement::pmc_outcome_hat_wert(ll_oc_); }
    [[nodiscard]] bool dtlb_offen_() const noexcept { return measurement::pmc_outcome_hat_wert(dtlb_oc_); }
    [[nodiscard]] bool branch_offen_() const noexcept { return measurement::pmc_outcome_hat_wert(branch_oc_); }
    [[nodiscard]] bool l3_uncore_offen_() const noexcept { return measurement::pmc_outcome_hat_wert(l3_pmu_oc_); }

    /// Wurde irgendein Zaehler auf einer anderen als der generischen Domaene geoeffnet? Genau dann ist
    /// die Maschine hybrid (oder verhaelt sich so), und genau dann lohnt die Log-Zeile.
    [[nodiscard]] bool hat_nichtgenerische_domaene_() const noexcept {
        auto ist_generisch = [](char const* d) noexcept { return std::strcmp(d, "generisch") == 0; };
        return !(ist_generisch(l1d_domaene_) && ist_generisch(ll_domaene_) && ist_generisch(dtlb_domaene_) &&
                 ist_generisch(branch_domaene_));
    }

    /// B-5 Runde 2: oeffnet einen GENERISCHEN Zaehler auf der ersten Core-PMU-Domaene, die ihn hergibt.
    ///
    /// ABLAUF. Erst der Weg, den es immer schon gab: ohne PMU-Praefix. Auf jeder uniformen Maschine
    /// traegt er, und dann passiert hier NICHTS Neues -- kein Verhaltenswechsel auf prod1, kein
    /// Verhaltenswechsel auf irgendeinem nicht-hybriden Intel. Erst wenn er mit EventNichtVorhanden
    /// scheitert (der Hybrid-Fall: der generische Weg kennt die getrennten Domaenen nicht), wird die
    /// Kette aus kCorePmuDomaenen durchlaufen -- jede Domaene mit ihrem AUS SYSFS GELESENEN Typ.
    ///
    /// WAS DIESE FUNKTION NICHT TUT: raten. Existiert der sysfs-Pfad einer Domaene nicht, wird sie
    /// uebersprungen statt mit einem geratenen Typ geoeffnet -- der Kernel wuerde einen unbekannten Typ
    /// naemlich NICHT abweisen, sondern still auf einen anderen PMU zurueckfallen (am Objekt gemessen,
    /// s. Block oben). Ein solcher Rueckfall ist genau die plausible falsche Zahl, die B-5 verhindert.
    ///
    /// out_domaene traegt den Namen der Domaene, auf der es geklappt hat -- damit eine Zahl von einer
    /// Hybrid-Maschine nie anonym bleibt (s. P/E-Hinweis an kCorePmuDomaenen).
    [[nodiscard]] static measurement::PmcCounterOutcome oeffne_generisch_(detail_linux_perf::PerfCounter& c,
                                                                          std::uint32_t type, std::uint64_t config,
                                                                          char const*  event_name,
                                                                          char const*& out_domaene) noexcept {
        using namespace detail_linux_perf;
        using measurement::PmcCounterOutcome;
        out_domaene                   = "generisch";
        PmcCounterOutcome const erste = c.open(type, config, event_name);
        if (erste != PmcCounterOutcome::EventNichtVorhanden) return erste; // Erfolg ODER anderer Grund

        // Der generische Weg kennt dieses Event hier nicht -> die Domaenen-Kette. Auf einer uniformen
        // Maschine ist "cpu" die einzige existierende und liefert dasselbe Ergebnis noch einmal; auf
        // Hybrid greifen cpu_core/cpu_atom, die der generische Weg nicht sehen konnte.
        PmcCounterOutcome letzte = erste;
        for (std::size_t i = 0; i < kCorePmuDomaenenZahl; ++i) {
            std::uint32_t typ = 0;
            // KEIN Rateversuch: existiert der sysfs-Pfad nicht, wird diese Domaene uebersprungen.
            // Das ist die Sicherung gegen die Kernel-Falle aus dem Block oben -- ein UNBEKANNTER
            // PMU-Typ wird vom Kernel NICHT abgewiesen, sondern faellt still auf einen anderen PMU
            // zurueck (am Objekt gemessen: Typ 9999 mit L1D-config oeffnete und lieferte eine Zahl).
            // Ohne diese Zeile truege eine Zelle Zahlen vom falschen PMU -- plausibel und falsch.
            if (!pmu_type_from_sysfs(kCorePmuDomaenen[i], typ)) continue;
            if (typ == 0) continue; // 0 hiesse laut Kernel-Header PERF_TYPE_RAW -- nie als Praefix setzen
            PmcCounterOutcome const oc = c.open(type, mit_pmu_typ(typ, config), event_name);
            if (oc == PmcCounterOutcome::Offen) {
                out_domaene = kCorePmuDomaenen[i];
                return oc;
            }
            letzte = oc; // die letzte SPRECHENDE Aussage gewinnt (z.B. ZugriffVerweigert statt ENOENT)
        }
        return letzte;
    }

    /// B-5: der AMD-ZWEITPFAD als reine DIAGNOSE-Sonde -- sie oeffnet den amd_l3-Uncore und schliesst ihn
    /// sofort wieder. Beantwortet genau eine Frage: existiert der echte L3-Zaehler auf dieser Maschine?
    ///   Rueckgabe Offen              -> ja, und er waere sogar zugaenglich (dann traegt das Log das)
    ///   Rueckgabe ZugriffVerweigert  -> ja, aber paranoid/CAP_PERFMON sperrt ihn (prod1 heute)
    ///   Rueckgabe EventNichtVorhanden-> nein: kein amd_l3-PMU (Intel, oder Treiber nicht geladen)
    /// Der PMU-TYP wird DYNAMISCH aus sysfs gelesen -- 17 ist auf prod1 der heutige Wert, keine Konstante.
    /// Die config-Kodierung ist am Objekt belegt (`perf list --details`): l3_lookup_state.l3_miss =
    /// event=0x4, umask=0x1; mit der Feldbelegung aus format/ (event=config:0-7, umask=config:8-15)
    /// ergibt das 0x104. Uncore-Zwaenge, beide am Objekt gemessen: pid MUSS -1 sein (per-Task -> EINVAL,
    /// ein Uncore kennt keinen Prozess) und exclude_kernel MUSS 0 sein (keine Ring-Trennung -> EINVAL).
    measurement::PmcCounterOutcome oeffne_amd_l3_uncore_() noexcept {
        using namespace detail_linux_perf;
        std::uint32_t typ = 0;
        int           cpu = 0;
        // Kein amd_l3 auf dieser Maschine -> der Zaehler existiert hier wirklich nicht. Das ist der
        // Intel-Fall und der Fall "Treiber fehlt"; beide sind EventNichtVorhanden, kein Rateversuch.
        if (!pmu_type_from_sysfs("amd_l3", typ)) return measurement::PmcCounterOutcome::EventNichtVorhanden;
        if (!pmu_first_cpu_from_sysfs("amd_l3", cpu)) cpu = 0;          // cpumask unlesbar -> erster Vertreter
        constexpr std::uint64_t kL3MissConfig = 0x4ULL | (0x1ULL << 8); // event=0x4, umask=0x1
        // Uncore-Zwaenge, beide am Objekt gemessen: pid MUSS -1 sein (per-Task -> EINVAL, ein Uncore
        // kennt keinen Prozess) und exclude_kernel MUSS 0 sein (keine Ring-Trennung -> EINVAL).
        return c_l3_uncore_.open(typ, kL3MissConfig, "amd_l3_l3_miss", /*pid*/ -1, cpu,
                                 /*exclude_kernel*/ false);
    }
    bool ready_  = false; ///< >=1 perf-Counter live geöffnet (Spiegel von WindowsPcmPmcSource::ready_)
    bool scaled_ = false; ///< >=1 gelesener Wert wurde multiplex-hochskaliert (Schätzung)

    bool          rapl_have_start_ = false;
    std::uint64_t rapl_start_uj_   = 0;

#if defined(COMDARE_ENABLE_PAPI)
    // ── Optionaler sekundärer PAPI-Pfad (additiv, NUR unter -DCOMDARE_ENABLE_PAPI). perf bleibt primär. ──
    // Wir nutzen ausschließlich die minimal-belegten PAPI-Aufrufe (Library-Init + 3 portable Preset-Events);
    // KEINE Annahmen über erweiterte/instabile PAPI-Signaturen.
    bool papi_ready_  = false;
    int  papi_evtset_ = PAPI_NULL;

    void init_papi_fallback_() noexcept {
        if (ready_) return; // perf hat geliefert → PAPI nicht nötig.
        if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) return;
        if (PAPI_create_eventset(&papi_evtset_) != PAPI_OK) return;
        // Portable Presets: L1-D-cache-miss, Total-cache-miss (LL), TLB-data-miss.
        (void)PAPI_add_event(papi_evtset_, PAPI_L1_DCM);
        (void)PAPI_add_event(papi_evtset_, PAPI_L3_TCM);
        (void)PAPI_add_event(papi_evtset_, PAPI_TLB_DM);
        papi_ready_ = (PAPI_num_events(papi_evtset_) > 0);
    }

    measurement::PmcCounters& end_papi_fallback_(measurement::PmcCounters& c) noexcept {
        if (!papi_ready_) return c; // available bleibt false.
        long long vals[3] = {0, 0, 0};
        if (PAPI_stop(papi_evtset_, vals) == PAPI_OK) {
            c.cache_misses_l1 = static_cast<std::uint64_t>(vals[0] < 0 ? 0 : vals[0]);
            c.cache_misses_l3 = static_cast<std::uint64_t>(vals[1] < 0 ? 0 : vals[1]);
            c.dtlb_misses     = static_cast<std::uint64_t>(vals[2] < 0 ? 0 : vals[2]);
            c.available       = true;
            // B5/M-2-KORREKTUR-2: PAPI_L3_TCM ist bei PAPI_OK ein reales Preset, kein Rateversuch.
            c.cache_misses_l3_source_available = true;
            // B-5: dieselbe Ehrlichkeit fuer die beiden Felder, die hier ebenfalls real gefuellt werden --
            // sonst truege ausgerechnet der Fallback-Pfad wieder nackte Zahlen ohne Quellen-Aussage.
            c.cache_misses_l1_source_available = true;
            c.dtlb_misses_source_available     = true;
        }
        return c;
    }
#endif // COMDARE_ENABLE_PAPI
};

} // namespace comdare::cache_engine::builder

#endif // COMDARE_ENABLE_PMC && __linux__
