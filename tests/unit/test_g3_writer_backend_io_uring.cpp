// test_g3_writer_backend_io_uring -- G3 / #46b Lagerhaltung, Scheibe B7.
//
// Dieses Test-Target wird MIT -DCOMDARE_WRITER_BACKEND_IO_URING kompiliert -> die CT-Wahl aktiviert
// das io_uring-Backend (spool_writer.hpp ActiveWriterBackend). Geprueft: (a) die CT-Wahl greift,
// (b) io_uring baut im Gate, (c) Laufzeit-Probe -> ENOSYS/seccomp => GTEST_SKIP, (d) BYTE-IDENTITAET
// gegen das portable Referenz-Backend (B21). Ohne minio.

#include "artifact_transport/spool_writer.hpp" // ActiveWriterBackend (io_uring unter dem Define)
#include "artifact_transport/writer_backend_io_uring.hpp"
#include "artifact_transport/writer_backend_portable.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace at = comdare::cache_engine::builder::artifact_transport;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("g3_iou_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::string read_file(std::filesystem::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

// Binaerer Testinhalt mit NUL-, High- und Steuerbytes.
std::string make_binary_payload(std::size_t n) {
    std::string s(n, '\0');
    for (std::size_t i = 0; i < n; ++i) s[i] = static_cast<char>((i * 37 + 11) & 0xff);
    return s;
}

} // namespace

// -----------------------------------------------------------------------------
// CT-Wahl + Name (baut immer, unabhaengig von der Laufzeit-Verfuegbarkeit).
// -----------------------------------------------------------------------------
TEST(G3IoUring, CompileTimeSelectionUnderDefine) {
    // Dieses Target ist mit COMDARE_WRITER_BACKEND_IO_URING kompiliert -> Active == io_uring.
    EXPECT_EQ(at::IoUringWriterBackend::name(), "io_uring");
    EXPECT_EQ(at::ActiveWriterBackend::name(), "io_uring");
    EXPECT_EQ(at::ActiveSpoolWriter::backend_name(), "io_uring");
}

// -----------------------------------------------------------------------------
// Byte-Identitaet gegen portable (B21) -- oder SKIP wenn io_uring zur Laufzeit fehlt.
// -----------------------------------------------------------------------------
TEST(G3IoUring, ByteIdentityVsPortableOrSkip) {
    if (!at::IoUringWriterBackend::available()) GTEST_SKIP() << "io_uring nicht verfuegbar (ENOSYS/seccomp)";
    TempDir           tmp;
    std::string const payload = make_binary_payload(30);

    at::SpoolEntry via_io{tmp.path / "io.bin", payload};
    at::SpoolEntry via_port{tmp.path / "port.bin", payload};
    ASSERT_TRUE(at::IoUringWriterBackend::write(via_io));
    ASSERT_TRUE(at::PortableWriterBackend::write(via_port));

    EXPECT_EQ(read_file(via_io.dest), payload);
    EXPECT_EQ(read_file(via_io.dest), read_file(via_port.dest)); // byte-identisch
}

TEST(G3IoUring, EmptyFileOrSkip) {
    if (!at::IoUringWriterBackend::available()) GTEST_SKIP() << "io_uring nicht verfuegbar";
    TempDir        tmp;
    at::SpoolEntry e{tmp.path / "empty.bin", ""};
    ASSERT_TRUE(at::IoUringWriterBackend::write(e));
    EXPECT_TRUE(std::filesystem::exists(e.dest));
    EXPECT_EQ(read_file(e.dest), "");
}

TEST(G3IoUring, LargePayloadByteIdentityOrSkip) {
    if (!at::IoUringWriterBackend::available()) GTEST_SKIP() << "io_uring nicht verfuegbar";
    TempDir           tmp;
    std::string const payload = make_binary_payload(512 * 1024); // ~ eine reale Binary-Groessenordnung

    at::SpoolEntry via_io{tmp.path / "big_io.bin", payload};
    at::SpoolEntry via_port{tmp.path / "big_port.bin", payload};
    ASSERT_TRUE(at::IoUringWriterBackend::write(via_io));
    ASSERT_TRUE(at::PortableWriterBackend::write(via_port));
    EXPECT_EQ(read_file(via_io.dest), read_file(via_port.dest));
    EXPECT_EQ(read_file(via_io.dest).size(), payload.size());
}

TEST(G3IoUring, CreatesParentDirsOrSkip) {
    if (!at::IoUringWriterBackend::available()) GTEST_SKIP() << "io_uring nicht verfuegbar";
    TempDir        tmp;
    at::SpoolEntry e{tmp.path / "sub" / "dir" / "x.bin", make_binary_payload(64)};
    ASSERT_TRUE(at::IoUringWriterBackend::write(e));
    EXPECT_TRUE(std::filesystem::exists(e.dest));
}

// -----------------------------------------------------------------------------
// SpoolWriter<io_uring> gegen Direktschreiben -- der Writer-Thread nutzt seinen eigenen Ring.
// -----------------------------------------------------------------------------
TEST(G3IoUring, SpoolWriterWithIoUringByteIdentityOrSkip) {
    if (!at::IoUringWriterBackend::available()) GTEST_SKIP() << "io_uring nicht verfuegbar";
    TempDir                  tmp;
    std::vector<std::string> payloads;
    for (int i = 0; i < 5; ++i) payloads.push_back(make_binary_payload(1000 + i * 111));

    {
        at::SpoolWriter<at::IoUringWriterBackend> w;
        for (int i = 0; i < 5; ++i) w.submit(tmp.path / (std::to_string(i) + ".bin"), payloads[i]);
        w.close();
    }
    for (int i = 0; i < 5; ++i) {
        auto          ref = tmp.path / (std::to_string(i) + ".ref");
        std::ofstream os(ref, std::ios::binary | std::ios::trunc);
        os.write(payloads[i].data(), static_cast<std::streamsize>(payloads[i].size()));
        os.close();
        EXPECT_EQ(read_file(tmp.path / (std::to_string(i) + ".bin")), payloads[i]);
        EXPECT_EQ(read_file(tmp.path / (std::to_string(i) + ".bin")), read_file(ref));
    }
}
