#pragma once
// writer_backend_io_uring.hpp -- G3 / #46b Lagerhaltung, Scheibe B7 (Ledger Section 62-B PLATTFORM-AUFLAGE).
//
// Das io_uring-Writer-Backend (Linux-Beschleuniger der Writer-CT-Strategy), gebaut gegen das VENDORED
// liburing (Stufe-1-Vendor, ext/io/liburing/, Tag liburing-2.6, Commit f7dcc1ea, Tag-Objekt 96141c98;
// siehe ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md). liburing macht die SQ/CQ-Ring-Barriers korrekt
// (io_uring_smp_load_acquire/store_release intern) -- keine rohe Syscall-/mmap-Handarbeit hier.
//
// Single-Ring-Disziplin: EIN Ring pro Thread (thread_local), dem Design "zweiter Writer-Thread
// besitzt den Ring". MINIMAL-Scope (Auflage): NUR IORING_OP_WRITE mit synchroner Completion (ein SQE
// -> submit_and_wait -> ein CQE je submit); kein SQPOLL, keine registered buffers, kein async-Reap --
// die Asynchronitaet traegt der Writer-THREAD, der Ring bleibt simpel.
//
// FEHLER-EHRLICHKEIT (Auflage): available() ist die harte Start-Probe (io_uring_queue_init). Schlaegt
// sie im io_uring-gebauten Binary fehl (ENOSYS/EPERM/seccomp), MUSS der Backend-Start hart+laut
// abbrechen -- die CT-Wahl heisst: wer io_uring baut, bekommt io_uring oder eine klare Diagnose, KEIN
// stilles Degradieren (kein Runtime-Fallback). write() liefert bei nicht initialisiertem Ring false;
// der Unit-Test macht aus !available() ein GTEST_SKIP. CQE-res wird je Write geprueft (Teil-Writes
// werden nachgeschoben; res<=0 bricht hart ab). Das portable Backend bleibt die Korrektheits-Referenz.
//
// UAPI-/liburing-Provenienz: liburing-2.6 (io_uring_queue_init/get_sqe/prep_write/submit_and_wait/
// wait_cqe/cqe_seen/queue_exit), gebaut fuer Kernel-6.17-Klasse, x86_64 __NR_io_uring_setup/enter=
// 425/426 (in liburings syscall.c gekapselt).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), Linux + vendored liburing + stdlib.
// __linux__-geschuetzt; auf anderen OS nur der available()=false / write()=false-Stub.

#include "artifact_transport/ram_spool.hpp" // SpoolEntry

#include <string_view>

#if defined(__linux__)
#include <liburing.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <system_error>
#endif

namespace comdare::cache_engine::builder::artifact_transport {

#if defined(__linux__)
namespace io_uring_detail {

// EIN io_uring-Ring, im Besitz EINES Threads (Single-Ring-Disziplin). RAII: queue_init im ctor,
// queue_exit im dtor. Nicht kopier-/verschiebbar.
class Ring {
public:
    Ring() { ok_ = (io_uring_queue_init(kDepth, &ring_, 0) == 0); }
    ~Ring() {
        if (ok_) io_uring_queue_exit(&ring_);
    }
    Ring(Ring const&)            = delete;
    Ring& operator=(Ring const&) = delete;

    [[nodiscard]] bool ok() const noexcept { return ok_; }

    // Schreibt buf[0..len) an file-offset in fd ueber EINE SQE und wartet synchron auf die Completion.
    // Rueckgabe: geschriebene Bytes (>0) oder <0 (Fehler). Die Ring-Barriers macht liburing.
    long write_at(int fd, void const* buf, unsigned len, std::uint64_t offset) noexcept {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (sqe == nullptr) return -1; // SQ voll (bei einem in-flight SQE nicht zu erwarten)
        io_uring_prep_write(sqe, fd, buf, len, offset);
        int const submitted = io_uring_submit_and_wait(&ring_, 1);
        if (submitted < 0) return -1;
        io_uring_cqe* cqe = nullptr;
        int const     wr  = io_uring_wait_cqe(&ring_, &cqe);
        if (wr < 0 || cqe == nullptr) return -1;
        long const res = cqe->res; // <0 = -errno; >=0 = geschriebene Bytes
        io_uring_cqe_seen(&ring_, cqe);
        return res;
    }

private:
    static constexpr unsigned kDepth = 8;
    io_uring                  ring_{};
    bool                      ok_ = false;
};

// Der thread-eigene Ring (Single-Ring-Disziplin: EIN Ring gehoert EINEM Thread).
inline Ring& thread_ring() {
    thread_local Ring r;
    return r;
}

} // namespace io_uring_detail
#endif // __linux__

struct IoUringWriterBackend {
    // Harte Start-Probe: laesst sich ein Ring einrichten? ENOSYS/EPERM/seccomp -> false.
    [[nodiscard]] static bool available() noexcept {
#if defined(__linux__)
        io_uring  ring{};
        int const r = io_uring_queue_init(1, &ring, 0);
        if (r < 0) return false;
        io_uring_queue_exit(&ring);
        return true;
#else
        return false;
#endif
    }

    // Persistiert einen Spool-Eintrag via io_uring (true = ok). Legt Elterndirs an; prueft CQE-res je
    // Write und schiebt Teil-Writes nach; leere Datei = leg via open(O_CREAT|O_TRUNC) an.
    [[nodiscard]] static bool write(SpoolEntry const& e) {
#if defined(__linux__)
        auto& ring = io_uring_detail::thread_ring();
        if (!ring.ok()) return false; // Ring nicht initialisiert -> harter Fehler (kein Fallback)

        std::error_code ec;
        if (e.dest.has_parent_path()) std::filesystem::create_directories(e.dest.parent_path(), ec);

        int const fd = ::open(e.dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return false;

        bool          ok        = true;
        std::uint64_t offset    = 0;
        char const*   data      = e.bytes.data();
        std::size_t   remaining = e.bytes.size();
        while (remaining > 0) {
            unsigned const chunk = static_cast<unsigned>(remaining > (1u << 30) ? (1u << 30) : remaining);
            long const     res   = ring.write_at(fd, data + offset, chunk, offset);
            if (res <= 0) { // res<0 = Fehler; res==0 = kein Fortschritt -> hart abbrechen
                ok = false;
                break;
            }
            offset += static_cast<std::uint64_t>(res);
            remaining -= static_cast<std::size_t>(res);
        }
        ::close(fd);
        return ok;
#else
        (void)e;
        return false;
#endif
    }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "io_uring"; }
};

} // namespace comdare::cache_engine::builder::artifact_transport
