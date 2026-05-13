// SPDX-License-Identifier: Apache-2.0
// Tests fuer ModuleLoader (Phase 7.2.D, 2026-05-13)

#include "../../cache_engine/builder/module_loader/module_loader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace ld = comdare::builder::loader;

namespace {

// Pfad zur Mock-DLL — von CMake via Compile-Definition gesetzt
#ifndef COMDARE_MOCK_DLL_PATH
  #error "COMDARE_MOCK_DLL_PATH must be defined by CMake"
#endif
std::filesystem::path const kMockDllPath{COMDARE_MOCK_DLL_PATH};
std::uint64_t const         kExpectedFingerprint = 0xABCDEF0123456789ULL;

}  // namespace

TEST(ModuleLoader, PlatformSuffixIsValid) {
    auto s = ld::ModuleLoader::platform_suffix();
    EXPECT_TRUE(s == ".dll" || s == ".so" || s == ".dylib");
}

TEST(ModuleLoader, LoadNonexistentReturnsFileNotFound) {
    ld::ModuleHandle h;
    auto status = ld::ModuleLoader::load(
        std::filesystem::path{"/nonexistent/path/foo.dll"}, h);
    EXPECT_EQ(status, ld::status_file_not_found);
    EXPECT_FALSE(h.valid());
}

TEST(ModuleLoader, LoadInvalidBinaryReturnsLoadFailed) {
    auto tmp = std::filesystem::temp_directory_path() / "not_a_dll.dll";
    {
        std::ofstream out{tmp, std::ios::binary};
        out << "this is not a valid PE/ELF file";
    }
    ld::ModuleHandle h;
    auto status = ld::ModuleLoader::load(tmp, h);
    EXPECT_EQ(status, ld::status_load_failed);
    EXPECT_FALSE(h.valid());
    std::filesystem::remove(tmp);
}

TEST(ModuleLoader, LoadMockDllSucceeds) {
    ASSERT_TRUE(std::filesystem::exists(kMockDllPath))
        << "Mock-DLL not found at " << kMockDllPath.string();

    ld::ModuleHandle h;
    auto status = ld::ModuleLoader::load(kMockDllPath, h);
    EXPECT_EQ(status, ld::status_ok);
    ASSERT_TRUE(h.valid());

    auto const* m = h.get();
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->abi_version,             COMDARE_ABI_VERSION);
    EXPECT_EQ(m->permutation_fingerprint, kExpectedFingerprint);
    EXPECT_NE(m->create_instance,         nullptr);
    EXPECT_NE(m->destroy_instance,        nullptr);
    EXPECT_NE(m->run_workload,            nullptr);
    EXPECT_NE(m->pull_live_counters,      nullptr);
}

TEST(ModuleLoader, MockDllRunWorkloadProducesExpectedRecord) {
    ld::ModuleHandle h;
    ASSERT_EQ(ld::ModuleLoader::load(kMockDllPath, h), ld::status_ok);
    auto const* m = h.get();

    void* inst = m->create_instance(nullptr);
    ASSERT_NE(inst, nullptr);

    comdare_workload_descriptor_v1 wl{};
    comdare_measurement_record_v1   rec{};
    m->run_workload(inst, &wl, &rec);
    EXPECT_EQ(rec.version,      COMDARE_ABI_VERSION);
    EXPECT_EQ(rec.op_count,     12345u);
    EXPECT_EQ(rec.total_cycles, 67890u);

    comdare_hw_counters_v1 hw{};
    m->pull_live_counters(inst, &hw);
    EXPECT_EQ(hw.cycles, 99u);

    m->destroy_instance(inst);
}

TEST(ModuleLoader, FingerprintAccessorMatches) {
    ld::ModuleHandle h;
    ASSERT_EQ(ld::ModuleLoader::load(kMockDllPath, h), ld::status_ok);
    EXPECT_EQ(h.fingerprint(), kExpectedFingerprint);
}

TEST(ModuleLoader, MoveSemanticsTransferOwnership) {
    ld::ModuleHandle h1;
    ASSERT_EQ(ld::ModuleLoader::load(kMockDllPath, h1), ld::status_ok);
    EXPECT_TRUE(h1.valid());

    ld::ModuleHandle h2 = std::move(h1);
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
    EXPECT_EQ(h2.fingerprint(), kExpectedFingerprint);
}

TEST(ModuleLoader, LoadAllFromDirectoryFindsMockDll) {
    std::vector<ld::ModuleHandle> handles;
    auto status = ld::ModuleLoader::load_all(kMockDllPath.parent_path(), handles);
    EXPECT_EQ(status, ld::status_ok);
    // Mindestens unsere Mock-DLL muss gefunden + geladen sein
    bool found = false;
    for (auto& h : handles) {
        if (h.fingerprint() == kExpectedFingerprint) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Mock-DLL not in handles[] (got " << handles.size() << " handles)";
}
