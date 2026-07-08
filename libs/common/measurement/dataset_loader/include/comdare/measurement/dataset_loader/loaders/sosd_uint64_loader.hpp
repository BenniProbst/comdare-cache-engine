#pragma once
// =====================================================================================
// SOSD-Binär-Loader (#45, 2026-07-08): sosd_uint64
// -------------------------------------------------------------------------------------
// SOSD-Datensaetze (books_200M, osm_cellids, fb, wiki_ts) liegen als BINAERES Format vor:
// Little-Endian 8-Byte-Count-Header (uint64 N) + N * uint64 (LE) sortierte Schluessel.
// Dieser Loader parst genau dieses Format und bildet jeden Schluessel auf eine Read-
// Operation ab (analog example_uint64_file, aber BINAER statt ASCII — s. dessen Kommentar
// "echte Framework-Loader ersetzen nur die Parse-/Mapping-Logik in load()").
//
// Der 6er-Kanon (url/dna/protein/xml/tpcds-id/trec-terms) braucht diesen Loader NICHT
// (alle String-Korpora ueber string_corpus); sosd ist ein Bestands-Extra ausserhalb des
// Kanons. Keine Zeit/Randomness/externe Hash-Lib; deterministisch.
//
// Aktivierung: Header in einer gelinkten TU einbinden (Selbst-Registrierung via inline-
// Variable, ODR-sicher). loader_id == "sosd_uint64".
// =====================================================================================

#include <comdare/measurement/dataset_loader/dataset_loader.hpp>

#include <cstdint>
#include <fstream>
#include <ios>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::measurement::dataset_loader::loaders {

/// Liest das SOSD-Binaerformat (LE 8-Byte-Count + N*uint64 LE) aus @p dataset_id -> Read-Ops.
class SosdUint64Loader final : public DatasetLoaderStrategy {
public:
    [[nodiscard]] std::optional<std::vector<wg::Operation>> load(std::string_view dataset_id,
                                                                 std::uint64_t /*seed*/) override {
        std::ifstream in{std::string{dataset_id}, std::ios::binary};
        if (!in) { return std::nullopt; }

        std::uint64_t count{};
        if (!read_le_u64(in, count)) { return std::nullopt; } // 8-Byte-Count-Header fehlt

        std::vector<wg::Operation> ops;
        ops.reserve(static_cast<std::size_t>(count));
        for (std::uint64_t i = 0; i < count; ++i) {
            std::uint64_t key{};
            if (!read_le_u64(in, key)) { return std::nullopt; } // vorzeitiges EOF = unvollstaendig
            ops.push_back(wg::Operation{wg::OperationKind::Read, key, 0});
        }
        if (ops.empty()) { return std::nullopt; }
        return ops;
    }

    [[nodiscard]] std::string metadata() const override {
        return "sosd_uint64: SOSD binary (LE 8-byte count header + N*uint64 LE) -> Read ops (deterministic)";
    }

private:
    /// Liest 8 Bytes als little-endian uint64 — byteweise, KEINE Annahme ueber Host-Endianness.
    [[nodiscard]] static bool read_le_u64(std::istream& in, std::uint64_t& out) noexcept {
        unsigned char bytes[8];
        in.read(reinterpret_cast<char*>(bytes), 8);
        if (in.gcount() != 8) { return false; }
        out = 0;
        for (int i = 0; i < 8; ++i) { out |= static_cast<std::uint64_t>(bytes[i]) << (8 * i); }
        return true;
    }
};

/// Selbst-Registrierung beim Einbinden dieses Headers (inline-Variable -> ODR-gemerged, einmalig).
inline const bool kSosdUint64LoaderRegistered = [] {
    DatasetLoaderRegistry::instance().register_loader("sosd_uint64", std::make_unique<SosdUint64Loader>());
    return true;
}();

} // namespace comdare::measurement::dataset_loader::loaders
