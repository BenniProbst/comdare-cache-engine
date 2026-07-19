#pragma once
// V5-#26 / Task #152 (CE-DL2) — LinuxPerfPmcSource: REALE CPU-Cache-Miss-Quelle via perf_event_open(2) (Linux).
//
// Linux-Spiegel der WindowsPcmPmcSource: Drop-in-Implementierung derselben `IPmcSource`-Naht (pmc_source.hpp),
// liefert die +6 HW-Counter (cache_misses L1/LL, dtlb, best-effort L2/coherence, RAPL-energy) aus echten
// Performance-Monitoring-Countern statt `NullPmcSource`-0. KEINE Änderung an POD/Pipeline/PDF/CSV-Schema
// (pmc_source.hpp:6-9) — eine weitere IPmcSource-Implementierung, die die bereits existierenden 7 pmc_*-Spalten
// befüllt (cache_engine_builder_iterator.hpp lazy_csv_header/format_csv_row, UNVERÄNDERT).
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

#if defined(COMDARE_ENABLE_PMC) && defined(__linux__)

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

    /// Öffnet den Counter (self-monitoring: pid=0, cpu=-1). false = EACCES/EPERM/ENOENT/EINVAL/ENODEV →
    /// dieses Feld bleibt available=false (KEIN erfundener Wert). attr.disabled=1 → Start erst per begin().
    bool open(std::uint32_t type, std::uint64_t config) noexcept {
        struct ::perf_event_attr attr;
        std::memset(&attr, 0, sizeof(attr)); // Muellbits in Reserve-Feldern → EINVAL; immer memset.
        attr.type           = type;
        attr.size           = sizeof(attr); // PFLICHT (Forward-Compat / ENTER-FIELD-Check) → sonst EINVAL.
        attr.config         = config;
        attr.disabled       = 1; // gestartet wird per ioctl ENABLE in begin().
        attr.exclude_kernel = 1; // unter Default-paranoid(2) nötig (sonst EACCES).
        attr.exclude_hv     = 1; // Hypervisor ausschließen.
        attr.inherit        = 0; // keine Kind-Vererbung.
        attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        long const r        = perf_event_open(&attr, /*pid*/ 0, /*cpu*/ -1, /*group_fd*/ -1, /*flags*/ 0);
        if (r < 0) {
            fd_ = -1;
            return false;
        }
        fd_ = static_cast<int>(r);
        return true;
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
///   cache_misses_l2         <- KEIN portabler generischer Counter → bleibt 0 (kein RAW-Rateversuch)
///   coherence_invalidations <- KEIN portabler generischer Counter → bleibt 0 (kein RAW-Rateversuch)
///   energy_micro_joules     <- best-effort RAPL (powercap energy_uj, Delta) → 0 ohne Zone/Leserecht
class LinuxPerfPmcSource final : public measurement::IPmcSource {
public:
    LinuxPerfPmcSource() noexcept {
        using namespace detail_linux_perf;
        // Jeder Counter wird INDIVIDUELL geöffnet; ein Fehlschlag deaktiviert NUR dieses Feld (nicht die Source).
        l1d_ok_  = c_l1d_.open(PERF_TYPE_HW_CACHE, cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ,
                                                             PERF_COUNT_HW_CACHE_RESULT_MISS));
        ll_ok_   = c_ll_.open(PERF_TYPE_HW_CACHE, cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ,
                                                            PERF_COUNT_HW_CACHE_RESULT_MISS));
        dtlb_ok_ = c_dtlb_.open(PERF_TYPE_HW_CACHE, cache_cfg(PERF_COUNT_HW_CACHE_DTLB, PERF_COUNT_HW_CACHE_OP_READ,
                                                              PERF_COUNT_HW_CACHE_RESULT_MISS));
        // L2 + coherence_invalidations: KEIN portabler generischer Counter → bewusst NICHT geöffnet, Feld 0.
        // RAPL: best-effort sysfs-Snapshot; Verfügbarkeit erst bei begin() (Lesbarkeit kann variieren).
        ready_ = l1d_ok_ || ll_ok_ || dtlb_ok_;
#if defined(COMDARE_ENABLE_PAPI)
        init_papi_fallback_(); // additiv: nur falls perf-Counter ausfielen UND PAPI verfügbar.
#endif
    }

    void begin() noexcept override {
        // Counter-Gruppe auf 0 + zählen an, UNMITTELBAR vor t0 (run_observable_perm Z.165).
        if (l1d_ok_) {
            c_l1d_.reset();
            c_l1d_.enable();
        }
        if (ll_ok_) {
            c_ll_.reset();
            c_ll_.enable();
        }
        if (dtlb_ok_) {
            c_dtlb_.reset();
            c_dtlb_.enable();
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
        if (l1d_ok_) c_l1d_.disable();
        if (ll_ok_) c_ll_.disable();
        if (dtlb_ok_) c_dtlb_.disable();

        bool ok = false, scaled = false;
        bool any = false;
        if (l1d_ok_) {
            std::uint64_t const v = c_l1d_.read_scaled(ok, scaled);
            if (ok) {
                c.cache_misses_l1 = v;
                any               = true;
                if (scaled) scaled_ = true;
            }
        }
        if (ll_ok_) {
            // LL = Last-Level (CPU-abhängig L2 ODER L3) → ehrlich in das L3-Feld (Last-Level), NICHT L2 erfinden.
            std::uint64_t const v = c_ll_.read_scaled(ok, scaled);
            if (ok) {
                c.cache_misses_l3 = v;
                any               = true;
                if (scaled) scaled_ = true;
            }
        }
        if (dtlb_ok_) {
            std::uint64_t const v = c_dtlb_.read_scaled(ok, scaled);
            if (ok) {
                c.dtlb_misses = v;
                any           = true;
                if (scaled) scaled_ = true;
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
    detail_linux_perf::PerfCounter c_l1d_{};  ///< L1-D read miss
    detail_linux_perf::PerfCounter c_ll_{};   ///< Last-Level (L3) read miss
    detail_linux_perf::PerfCounter c_dtlb_{}; ///< dTLB read miss
    bool                           l1d_ok_  = false;
    bool                           ll_ok_   = false;
    bool                           dtlb_ok_ = false;
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
        }
        return c;
    }
#endif // COMDARE_ENABLE_PAPI
};

} // namespace comdare::cache_engine::builder

#endif // COMDARE_ENABLE_PMC && __linux__
