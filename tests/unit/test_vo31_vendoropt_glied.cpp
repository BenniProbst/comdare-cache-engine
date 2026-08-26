// tests/unit/test_vo31_vendoropt_glied.cpp -- VO3-1(b) T-B: das NEUE Feld "vendoropt" des Toolchain-
// Glieds [5] + der tc-Format-Bump "1"->"2" (deklariertes Byte-Ereignis, Owner-GO X4 25.08.2026:
// "(b) global O2 ..." ist laut Ledger IDENTITAETSWIRKSAM -- Fingerprint-Neuberechnung erwartet).
// Bump-Doktrin toolchain_stamp_glied.hpp: die FELDAUSWAHL aendert sich -> Format-Kennung bumpen.
//
// T-1-KOEDER (rot zuerst, 26.08.2026, Beweisort backups-workflow/20260825-vo3-1-global-o2/bau/rot/):
// T-B1 baut am IST-Stand d3b5a393 und beisst dort literal ("tc=1;..." statt "tc=2;...").
// Die nicht vorab uebersetzbaren Haelften T-B2..T-B5 kommen mit dem B4-Bau in DIESE Datei; ihre
// Rot-Probe ist die Wegwerf-Mutation nach dem Bau (bau/mutation/, Praezedenz #111).
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <cache_engine/abi/toolchain_stamp_glied.hpp>

TEST(Vo31VendoroptGlied, TB1GliedKopfTraegtFormat2) {
    namespace abi = ::comdare::cache_engine::abi;
    abi::ToolchainStampParts p{};
    p.cxx_dialect       = "gcc";
    p.cxx_driver        = "g++-17";
    std::string const g = abi::render_toolchain_stamp_glied(p);
    // Der Kopf traegt die Glied-Format-Version; seit VO3-1(b) ist das "2" (neues letztes Feld
    // vendoropt, Key [9]) -- s. Bump-Doktrin am Format-Anker (toolchain_stamp_glied.hpp).
    EXPECT_TRUE(g.starts_with("tc=2;")) << "glied='" << g << "'";
}
