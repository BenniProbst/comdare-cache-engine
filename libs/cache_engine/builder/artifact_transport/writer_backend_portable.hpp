#pragma once
// writer_backend_portable.hpp -- G3 / #46b Lagerhaltung, Scheibe B6 (Ledger §62-B PLATTFORM-AUFLAGE).
//
// Das PORTABLE Referenz-Backend der Writer-CT-Strategy: schreibt die Spool-Bytes 1:1 via ofstream
// (binary), legt fehlende Elternverzeichnisse an. Es ist Pflicht-Basis auf ALLEN dokumentierten OS
// (8er-Docker-Matrix) und die KORREKTHEITS-REFERENZ (B21), gegen die die beschleunigten Backends
// (io_uring/IoRing) auf Byte-Identitaet geprueft werden.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib.

#include "artifact_transport/ram_spool.hpp" // SpoolEntry

#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>
#include <system_error>

namespace comdare::cache_engine::builder::artifact_transport {

struct PortableWriterBackend {
    // Persistiert einen Spool-Eintrag (true = ok). Legt fehlende Elternverzeichnisse an.
    [[nodiscard]] static bool write(SpoolEntry const& e) {
        std::error_code ec;
        if (e.dest.has_parent_path()) std::filesystem::create_directories(e.dest.parent_path(), ec);
        std::ofstream os(e.dest, std::ios::binary | std::ios::trunc);
        if (!os) return false;
        os.write(e.bytes.data(), static_cast<std::streamsize>(e.bytes.size()));
        return static_cast<bool>(os);
    }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "portable"; }
};

} // namespace comdare::cache_engine::builder::artifact_transport
