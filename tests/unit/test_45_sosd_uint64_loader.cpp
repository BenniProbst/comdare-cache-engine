// test_45_sosd_uint64_loader — #45 (2026-07-08): SOSD-Binaer-Loader (LE 8-Byte-Count + N*uint64 LE).
// Verifiziert: (1) Round-Trip binaere LE-Keys -> Read-Ops mit exakten key_ids; (2) seed-invariant;
// (3) Rejection von truncated/missing/empty. Synthetische SOSD-Blobs (kein echter 1.6-GB-Korpus noetig).
#include <comdare/measurement/dataset_loader/dataset_loader.hpp>
#include <comdare/measurement/dataset_loader/loaders/sosd_uint64_loader.hpp>

#include "comdare_test_tmp.hpp" // ::comdare::test::user_tmp_dir()

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace dl         = ::comdare::measurement::dataset_loader;
namespace dl_loaders = ::comdare::measurement::dataset_loader::loaders;
namespace wg         = ::comdare::workload_generator;

/// 8 Bytes little-endian (Byte-Reihenfolge unabhaengig von Host-Endianness).
[[nodiscard]] std::string le_u64(std::uint64_t v) {
    std::string s(8, '\0');
    for (int i = 0; i < 8; ++i) { s[static_cast<std::size_t>(i)] = static_cast<char>((v >> (8 * i)) & 0xFFu); }
    return s;
}

/// SOSD-Blob: LE 8-Byte-Count-Header + N*uint64 LE.
[[nodiscard]] std::string make_sosd_blob(std::vector<std::uint64_t> const& keys) {
    std::string blob = le_u64(keys.size());
    for (auto k : keys) { blob += le_u64(k); }
    return blob;
}

[[nodiscard]] std::string write_bin(std::string_view name, std::string const& bytes) {
    auto const    path = ::comdare::test::user_tmp_dir() / std::string{name};
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    EXPECT_TRUE(static_cast<bool>(out));
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    EXPECT_TRUE(static_cast<bool>(out));
    return path.string();
}

[[nodiscard]] std::vector<std::uint64_t> key_ids(std::vector<wg::Operation> const& ops) {
    std::vector<std::uint64_t> keys;
    keys.reserve(ops.size());
    for (auto const& op : ops) { keys.push_back(op.key_id); }
    return keys;
}

} // namespace

TEST(Sosd45Loader, ParsesBinaryLeUint64ToReadOpsRoundTrip) {
    std::vector<std::uint64_t> const keys{1u, 42u, 0xFFFFFFFFFFFFFFFFull, 1'000'000u, 7u, 0u, 0x0102030405060708ull};
    auto const                       path = write_bin("comdare_45_sosd.bin", make_sosd_blob(keys));

    EXPECT_TRUE(dl_loaders::kSosdUint64LoaderRegistered);
    ASSERT_TRUE(dl::DatasetLoaderRegistry::instance().has("sosd_uint64"));

    auto const first  = dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", path, 1);
    auto const second = dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", path, 99);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(first->size(), keys.size());

    for (std::size_t i = 0; i < keys.size(); ++i) {
        EXPECT_EQ((*first)[i].op, wg::OperationKind::Read);
        EXPECT_EQ((*first)[i].key_id, keys[i]); // LE-Bytes -> exakter Schluessel (Round-Trip)
        EXPECT_EQ((*first)[i].scan_length, 0u);
    }
    EXPECT_EQ(key_ids(*first), key_ids(*second)); // seed-invariant
}

TEST(Sosd45Loader, RejectsTruncatedMissingAndEmpty) {
    // Header sagt 3, aber nur 1 Key -> unvollstaendig -> nullopt.
    auto const trunc = write_bin("comdare_45_sosd_trunc.bin", le_u64(3) + le_u64(11));
    EXPECT_FALSE(dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", trunc, 0).has_value());

    // Fehlende Datei -> nullopt.
    EXPECT_FALSE(
        dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", "/nonexistent/comdare_45.bin", 0).has_value());

    // count==0 -> keine Ops -> nullopt.
    auto const zero = write_bin("comdare_45_sosd_zero.bin", le_u64(0));
    EXPECT_FALSE(dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", zero, 0).has_value());

    // Header allein unvollstaendig (nur 4 Bytes) -> nullopt.
    auto const short_header = write_bin("comdare_45_sosd_shorthdr.bin", std::string(4, '\0'));
    EXPECT_FALSE(dl::DatasetLoaderRegistry::instance().try_load("sosd_uint64", short_header, 0).has_value());
}
