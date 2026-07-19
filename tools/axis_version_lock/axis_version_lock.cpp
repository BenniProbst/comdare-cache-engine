// PAKET W3-C (Ledger Sec.32-F4/GN-8/O-4): axis_version_lock -- Content-Digest-Tripwire fuer die
// Heuristik-Strategie-Header. Eigenstaendiges C++23-Executable (kein Python, keine Fremdbibliothek).
//
// ZWECK (GN-8/O-4, verbatim-treu Sec.32-F4): "codegen-Minor = CI-Tripwire hart-rot (Lock
// tools/axis_version_lock/axis_version.lock)". Wird der INHALT eines Strategie-Headers geaendert, OHNE
// seine deklarierte algo_version hochzuzaehlen, faellt der Check auf ROT (Exit 1). So kann kein Algorithmus
// still driften: entweder der Digest ist unveraendert, oder die Version wurde bewusst gebumpt.
//
// SHA-256: WIEDERVERWENDUNG der vorhandenen consteval/constexpr-Implementation
// (libs/cache_engine/src/sha256/ctsha.hpp, RFC 6234) -- die constexpr-Funktion sha256(span) ist auch zur
// LAUFZEIT aufrufbar. KEINE zweite SHA-Implementation (vendor->faithful->self-contained-Doktrin).
//
// algo_version-Marker: die Strategie-Header tragen eine ASCII-Zeile "// AXIS_ALGO_VERSION: <N>". Der Tool
// liest N; fehlt der Marker, gilt N=0 + eine sichtbare Diagnose (kein stiller Default).
//
// LOCK-FORMAT (v1, erstes Format):
//   # axis_version.lock -- GN-8/O-4 content-digest tripwire (PAKET W3-C, Ledger Sec.32-F4)
//   # format: v1
//   # fields per line: <algo_version> <sha256-hex> <relative-path>
//   1 <64-hex> libs/cache_engine/heuristik/axis_spline.hpp
//
// AUFRUF:
//   axis_version_lock --write <lockfile> <header> [<header> ...]
//   axis_version_lock --check <lockfile> <header> [<header> ...]
// Exit: 0 = gruen (alle OK), 1 = ROT (Tripwire: Digest geaendert ohne Version-Bump / fehlend), 2 = Usage.
//
// INERT-by-default (Anti-Phantom): EXCLUDE_FROM_ALL, laeuft nur, wenn explizit im CI-Job aufgerufen.

#include <sha256/ctsha.hpp> // comdare::cache_engine::sha256::{sha256, to_hex, Digest}

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace sha = ::comdare::cache_engine::sha256;

struct HeaderFacts {
    bool          exists     = false;
    std::uint64_t version    = 0; // aus dem AXIS_ALGO_VERSION-Marker (0 = Marker fehlt)
    bool          has_marker = false;
    std::string   digest_hex; // SHA-256 der ROHEN Datei-Bytes
};

/// Liest die ROHEN Bytes einer Datei. exists=false wenn nicht lesbar.
[[nodiscard]] bool read_file_bytes(std::string const& path, std::vector<std::uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string const s = ss.str();
    out.assign(s.begin(), s.end());
    return true;
}

/// Parst die deklarierte algo_version aus dem Marker "AXIS_ALGO_VERSION:". Erste Fundstelle gewinnt.
[[nodiscard]] bool parse_algo_version(std::vector<std::uint8_t> const& bytes, std::uint64_t& out) {
    static constexpr std::string_view kMarker = "AXIS_ALGO_VERSION:";
    std::string_view const            text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
    std::size_t const                 pos = text.find(kMarker);
    if (pos == std::string_view::npos) return false;
    std::size_t i = pos + kMarker.size();
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    std::uint64_t v     = 0;
    bool          digit = false;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
        v     = v * 10 + static_cast<std::uint64_t>(text[i] - '0');
        digit = true;
        ++i;
    }
    if (!digit) return false;
    out = v;
    return true;
}

[[nodiscard]] HeaderFacts inspect_header(std::string const& path) {
    HeaderFacts               hf;
    std::vector<std::uint8_t> bytes;
    if (!read_file_bytes(path, bytes)) return hf; // exists=false
    hf.exists           = true;
    hf.has_marker       = parse_algo_version(bytes, hf.version);
    sha::Digest const d = sha::sha256(std::span<const std::uint8_t>{bytes.data(), bytes.size()});
    auto const        h = sha::to_hex(d);
    hf.digest_hex.assign(h.begin(), h.end());
    return hf;
}

struct LockEntry {
    std::uint64_t version = 0;
    std::string   digest_hex;
};

/// Parst eine Lock-Datei (v1). Kommentar-/Leerzeilen werden ignoriert. Rueckgabe map<path,entry>.
[[nodiscard]] std::map<std::string, LockEntry> parse_lock(std::string const& path, bool& ok) {
    std::map<std::string, LockEntry> out;
    std::ifstream                    f(path);
    ok = static_cast<bool>(f);
    if (!ok) return out;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        LockEntry          e;
        std::string        rel;
        if (ls >> e.version >> e.digest_hex >> rel) out[rel] = e;
    }
    return out;
}

int do_write(std::string const& lockfile, std::vector<std::string> const& headers) {
    std::ofstream out(lockfile, std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "axis_version_lock: FEHLER kann Lock-Datei nicht schreiben: %s\n", lockfile.c_str());
        return 2;
    }
    out << "# axis_version.lock -- GN-8/O-4 content-digest tripwire (PAKET W3-C, Ledger Sec.32-F4)\n";
    out << "# format: v1\n";
    out << "# fields per line: <algo_version> <sha256-hex> <relative-path>\n";
    int rc = 0;
    for (std::string const& h : headers) {
        HeaderFacts const hf = inspect_header(h);
        if (!hf.exists) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Header nicht lesbar: %s\n", h.c_str());
            rc = 2;
            continue;
        }
        if (!hf.has_marker) {
            std::fprintf(stderr, "axis_version_lock: WARNUNG kein AXIS_ALGO_VERSION-Marker in %s -> version=0\n",
                         h.c_str());
        }
        out << hf.version << ' ' << hf.digest_hex << ' ' << h << '\n';
        std::fprintf(stdout, "axis_version_lock: LOCK v%llu %s %s\n", static_cast<unsigned long long>(hf.version),
                     hf.digest_hex.c_str(), h.c_str());
    }
    return rc;
}

int do_check(std::string const& lockfile, std::vector<std::string> const& headers) {
    bool                                   lock_ok = false;
    std::map<std::string, LockEntry> const lock    = parse_lock(lockfile, lock_ok);
    if (!lock_ok) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Datei nicht lesbar: %s\n", lockfile.c_str());
        return 1;
    }
    int red = 0;
    for (std::string const& h : headers) {
        HeaderFacts const hf = inspect_header(h);
        if (!hf.exists) {
            std::fprintf(stderr, "axis_version_lock: ROT Header fehlt: %s\n", h.c_str());
            red = 1;
            continue;
        }
        auto const it = lock.find(h);
        if (it == lock.end()) {
            std::fprintf(stderr, "axis_version_lock: ROT Header nicht im Lock verzeichnet (unlocked): %s\n", h.c_str());
            red = 1;
            continue;
        }
        LockEntry const& e = it->second;
        if (hf.digest_hex == e.digest_hex) {
            std::fprintf(stdout, "axis_version_lock: OK unveraendert v%llu %s\n",
                         static_cast<unsigned long long>(e.version), h.c_str());
            continue;
        }
        // Digest geaendert -> nur legitim, wenn die Version echt hochgezaehlt wurde.
        if (hf.version > e.version) {
            std::fprintf(stdout, "axis_version_lock: OK Digest geaendert MIT Version-Bump v%llu->v%llu %s\n",
                         static_cast<unsigned long long>(e.version), static_cast<unsigned long long>(hf.version),
                         h.c_str());
            std::fprintf(stdout, "axis_version_lock: HINWEIS Lock erneuern (--write) fuer %s\n", h.c_str());
            continue;
        }
        std::fprintf(
            stderr, "axis_version_lock: ROT Header-Digest geaendert OHNE algo_version-Bump (v%llu==v%llu) %s\n",
            static_cast<unsigned long long>(hf.version), static_cast<unsigned long long>(e.version), h.c_str());
        std::fprintf(stderr, "axis_version_lock:     erwartet %s\n", e.digest_hex.c_str());
        std::fprintf(stderr, "axis_version_lock:     ist      %s\n", hf.digest_hex.c_str());
        red = 1;
    }
    if (red == 0)
        std::fprintf(stdout, "axis_version_lock: GRUEN alle %zu Strategie-Header konsistent\n", headers.size());
    return red;
}

void usage() {
    std::fprintf(stderr, "usage: axis_version_lock (--write|--check) <lockfile> <header> [<header> ...]\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        usage();
        return 2;
    }
    std::string const              mode     = argv[1];
    std::string const              lockfile = argv[2];
    std::vector<std::string> const headers(argv + 3, argv + argc);
    if (mode == "--write") return do_write(lockfile, headers);
    if (mode == "--check") return do_check(lockfile, headers);
    usage();
    return 2;
}
