// test_g3_ram_spool_writer -- G3 / #46b Lagerhaltung, Scheibe B6.
//
// RAM-Spool-Trigger (Groesse ODER Dutzend), CT-Strategy SpoolWriter<PortableWriterBackend>, Byte-
// Identitaet portable-Backend vs. Direktschreiben (B21-Korrektheitsreferenz), Crash-Fenster
// (pending vs. persisted), synchroner flush-Barrier.

#include "artifact_transport/ram_spool.hpp"
#include "artifact_transport/spool_writer.hpp"
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

// Eindeutiges Temp-Verzeichnis je Testfall (RAII-Aufraeumung).
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("g3_spool_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// Liest eine Datei binaer komplett ein.
std::string read_file(std::filesystem::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

} // namespace

// ---------------------------------------------------------------------------
// RAM-Spool-Trigger.
// ---------------------------------------------------------------------------
TEST(G3RamSpool, TriggersByCount) {
    at::RamSpool spool(/*max_bytes*/ 1ull << 40, /*count_trigger*/ 3);
    spool.add(at::SpoolEntry{"a", "xx"});
    spool.add(at::SpoolEntry{"b", "yy"});
    EXPECT_FALSE(spool.should_flush());
    spool.add(at::SpoolEntry{"c", "zz"});
    EXPECT_TRUE(spool.should_flush()); // Anzahl 3 erreicht
    auto drained = spool.drain();
    EXPECT_EQ(drained.size(), 3u);
    EXPECT_TRUE(spool.empty());
    EXPECT_EQ(spool.total_bytes(), 0u);
}

TEST(G3RamSpool, TriggersBySize) {
    at::RamSpool spool(/*max_bytes*/ 10, /*count_trigger*/ 1000);
    spool.add(at::SpoolEntry{"a", std::string(6, 'x')});
    EXPECT_FALSE(spool.should_flush()); // 6 < 10
    spool.add(at::SpoolEntry{"b", std::string(5, 'y')});
    EXPECT_TRUE(spool.should_flush()); // 11 >= 10
}

// ---------------------------------------------------------------------------
// Portable-Backend: Byte-Identitaet gegen Direktschreiben (die Korrektheits-Referenz).
// ---------------------------------------------------------------------------
TEST(G3RamSpool, PortableBackendByteIdentity) {
    TempDir     tmp;
    std::string payload = std::string("\x00\x01\x02binary\xff\xfe payload", 20);

    at::SpoolEntry e{tmp.path / "via_backend.bin", payload};
    ASSERT_TRUE(at::PortableWriterBackend::write(e));

    // Referenz: dieselben Bytes direkt schreiben.
    auto          ref = tmp.path / "via_direct.bin";
    std::ofstream os(ref, std::ios::binary | std::ios::trunc);
    os.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    os.close();

    EXPECT_EQ(read_file(e.dest), payload);
    EXPECT_EQ(read_file(e.dest), read_file(ref)); // byte-identisch
    EXPECT_EQ(at::PortableWriterBackend::name(), "portable");
}

TEST(G3RamSpool, PortableBackendCreatesParentDirs) {
    TempDir        tmp;
    at::SpoolEntry e{tmp.path / "sub" / "dir" / "x.bin", "hi"};
    ASSERT_TRUE(at::PortableWriterBackend::write(e));
    EXPECT_TRUE(std::filesystem::exists(e.dest));
    EXPECT_EQ(read_file(e.dest), "hi");
}

// ---------------------------------------------------------------------------
// SpoolWriter: Persistenz bei flush/close, Byte-Identitaet, Crash-Fenster, Trigger.
// ---------------------------------------------------------------------------
TEST(G3RamSpool, SpoolWriterPersistsOnFlush) {
    TempDir tmp;
    {
        at::SpoolWriter<at::PortableWriterBackend> w(/*max_bytes*/ 1ull << 40, /*count_trigger*/ 100);
        w.submit(tmp.path / "0.bin", "aaa");
        w.submit(tmp.path / "1.bin", "bbbb");
        // Unter Trigger -> noch im RAM (Crash-Fenster).
        EXPECT_EQ(w.pending_count(), 2u);
        EXPECT_FALSE(std::filesystem::exists(tmp.path / "0.bin"));
        w.flush(); // synchroner Barrier
        EXPECT_EQ(w.pending_count(), 0u);
        EXPECT_EQ(w.persisted_count(), 2u);
    }
    EXPECT_EQ(read_file(tmp.path / "0.bin"), "aaa");
    EXPECT_EQ(read_file(tmp.path / "1.bin"), "bbbb");
}

TEST(G3RamSpool, SpoolWriterTriggerPersistsBatch) {
    TempDir                                    tmp;
    at::SpoolWriter<at::PortableWriterBackend> w(/*max_bytes*/ 1ull << 40, /*count_trigger*/ 3);
    w.submit(tmp.path / "0.bin", "a");
    w.submit(tmp.path / "1.bin", "b");
    w.submit(tmp.path / "2.bin", "c"); // 3 -> Trigger, Batch geht an den Writer-Thread
    w.flush();                         // wartet auf den in-flight Batch
    EXPECT_EQ(w.persisted_count(), 3u);
    EXPECT_EQ(w.pending_count(), 0u);
}

TEST(G3RamSpool, SpoolWriterByteIdentityVsDirect) {
    TempDir                  tmp;
    std::vector<std::string> payloads;
    for (int i = 0; i < 5; ++i) payloads.push_back("binary-payload-" + std::to_string(i) + std::string(3, '\0'));

    {
        at::SpoolWriter<at::PortableWriterBackend> w;
        for (int i = 0; i < 5; ++i) w.submit(tmp.path / (std::to_string(i) + ".bin"), payloads[i]);
        w.close(); // flush + join
    }
    for (int i = 0; i < 5; ++i) {
        // Direktschreib-Referenz.
        auto          ref = tmp.path / (std::to_string(i) + ".ref");
        std::ofstream os(ref, std::ios::binary | std::ios::trunc);
        os.write(payloads[i].data(), static_cast<std::streamsize>(payloads[i].size()));
        os.close();
        EXPECT_EQ(read_file(tmp.path / (std::to_string(i) + ".bin")), payloads[i]);
        EXPECT_EQ(read_file(tmp.path / (std::to_string(i) + ".bin")), read_file(ref));
    }
}

TEST(G3RamSpool, CrashWindowPendingNotOnDisk) {
    TempDir                                    tmp;
    at::SpoolWriter<at::PortableWriterBackend> w(/*max_bytes*/ 1ull << 40, /*count_trigger*/ 100);
    w.submit(tmp.path / "0.bin", "data");
    // Vor jedem Trigger/flush: die Bytes existieren NUR im RAM -> ein Absturz hier verliert sie.
    EXPECT_EQ(w.pending_count(), 1u);
    EXPECT_EQ(w.persisted_count(), 0u);
    EXPECT_FALSE(std::filesystem::exists(tmp.path / "0.bin"));
    w.close(); // sauberer Abbau persistiert doch noch
    EXPECT_TRUE(std::filesystem::exists(tmp.path / "0.bin"));
}

TEST(G3RamSpool, ActiveBackendIsPortableByDefault) {
    // Ohne COMDARE_WRITER_BACKEND_* Define ist die CT-Wahl das portable Backend.
    EXPECT_EQ(at::ActiveWriterBackend::name(), "portable");
    EXPECT_EQ(at::ActiveSpoolWriter::backend_name(), "portable");
}
