#pragma once
// =====================================================================================
// Dataset-Akte (AP-10): host-seitiges Dataset-Manifest
// -------------------------------------------------------------------------------------
// Erfasst reproduzierbare Metadaten fuer externe Datensaetze, ohne die Datensaetze selbst
// zu registrieren oder das Operation[uint64]-Modell zu veraendern.
// =====================================================================================

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace comdare::measurement::dataset_loader {

struct DatasetAkte {
    std::string   id;
    std::string   source_path;
    std::string   preprocessing;
    std::uint64_t checksum{};
    std::uint64_t line_count{};
};

namespace dataset_akte_detail {

inline constexpr std::uint64_t kFnv1a64OffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnv1a64Prime       = 1099511628211ull;

inline void fnv1a64_update(std::uint64_t& hash, unsigned char byte) noexcept {
    hash ^= byte;
    hash *= kFnv1a64Prime;
}

[[nodiscard]] inline std::string hex_u64(std::uint64_t value) {
    std::ostringstream os;
    os << "0x" << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << value;
    return os.str();
}

} // namespace dataset_akte_detail

[[nodiscard]] inline DatasetAkte compute_dataset_akte(std::string_view id, std::filesystem::path const& source_path,
                                                      std::string_view preprocessing = "none") {
    std::ifstream in{source_path, std::ios::binary};
    if (!in) { throw std::runtime_error{"cannot open dataset source for akte: " + source_path.string()}; }

    std::uint64_t checksum   = dataset_akte_detail::kFnv1a64OffsetBasis;
    std::uint64_t line_count = 0;

    char byte{};
    while (in.get(byte)) {
        auto const value = static_cast<unsigned char>(byte);
        dataset_akte_detail::fnv1a64_update(checksum, value);
        if (byte == '\n') { ++line_count; }
    }
    if (in.bad()) { throw std::runtime_error{"error while reading dataset source for akte: " + source_path.string()}; }

    return DatasetAkte{std::string{id}, source_path.string(), std::string{preprocessing}, checksum, line_count};
}

[[nodiscard]] inline std::string serialize_dataset_akte(DatasetAkte const& akte) {
    std::ostringstream os;
    os << "id=" << akte.id << '\n';
    os << "source_path=" << akte.source_path << '\n';
    os << "checksum=" << dataset_akte_detail::hex_u64(akte.checksum) << '\n';
    os << "line_count=" << akte.line_count << '\n';
    os << "preprocessing=" << akte.preprocessing << '\n';
    return os.str();
}

[[nodiscard]] inline bool write_dataset_akte(std::filesystem::path const& path, DatasetAkte const& akte) {
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out) { return false; }
    out << serialize_dataset_akte(akte);
    return static_cast<bool>(out);
}

} // namespace comdare::measurement::dataset_loader