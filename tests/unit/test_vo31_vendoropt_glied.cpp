// tests/unit/test_vo31_vendoropt_glied.cpp -- VO3-1(b) T-B: das NEUE Feld "vendoropt" des Toolchain-
// Glieds [5] + der tc-Format-Bump "1"->"2" (deklariertes Byte-Ereignis, Owner-GO X4 25.08.2026:
// "(b) global O2 ..." ist laut Ledger IDENTITAETSWIRKSAM -- Fingerprint-Neuberechnung erwartet).
// Bump-Doktrin toolchain_stamp_glied.hpp: die FELDAUSWAHL aendert sich -> Format-Kennung bumpen.
//
// T-1-KOEDER (rot zuerst, 26.08.2026, Beweisort backups-workflow/20260825-vo3-1-global-o2/bau/rot/):
// T-B1 baute am IST-Stand d3b5a393 und biss dort literal ("tc=1;..." statt "tc=2;...", RC=1).
// T-B2..T-B5 sind erst mit dem B4-Bau uebersetzbar; ihre Rot-Probe ist die Wegwerf-Mutation nach dem
// Bau (vendoropt-Append im Renderer auskommentiert -> T-B2 rot literal; Kopie bau/mutation/, #111).
#include <gtest/gtest.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <cache_engine/abi/anatomy_fingerprint.hpp>
#include <cache_engine/abi/toolchain_stamp_glied.hpp>
#include <toolchain_stamp_naht.hpp>

#ifndef COMDARE_VO31_EXPECT_VENDOROPT
#error "VO3-1 T-B5: COMDARE_VO31_EXPECT_VENDOROPT fehlt (Registrierung tests/unit/CMakeLists.txt)"
#endif

// Namespace-Aliases stehen JE TEST lokal (Vorbild test_m_w12): ein Datei-Scope-Alias 'abi' waere gegen
// den in der Include-Kette bereits sichtbaren Alias mehrdeutig.
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

// T-B2: das vendoropt-Segment steht an LETZTER Stelle -- und laeuft durch DIESELBEN Feld-Wachen wie
// jedes andere Feld (Struktur-/Transport-Diagnose nennt das Feld beim Namen, Wurf statt stiller
// Kollision).
TEST(Vo31VendoroptGlied, TB2SegmentAnLetzterStelleUndGewacht) {
    namespace abi = ::comdare::cache_engine::abi;
    abi::ToolchainStampParts p{};
    p.cxx_dialect       = "gcc";
    p.cxx_driver        = "g++-17";
    p.vendoropt         = "O2";
    std::string const g = abi::render_toolchain_stamp_glied(p);
    EXPECT_TRUE(g.starts_with("tc=2;")) << "glied='" << g << "'";
    EXPECT_TRUE(g.ends_with(";vendoropt=O2")) << "glied='" << g << "'";

    // Wachen-Deckung des NEUEN Feldes: ein Struktur-Zeichen im Wert wird FAIL-LOUD benannt.
    abi::ToolchainStampParts boese = p;
    boese.vendoropt                = "O;2";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(boese), std::string_view{"vendoropt"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(boese), std::invalid_argument);
}

// T-B3: LEER => KEIN Segment (Altform-Vertraeglichkeit: dieselbe Regel wie bei jedem Glied-Feld;
// die Alles-leer-IDENTITAET bleibt byte-gleich "").
TEST(Vo31VendoroptGlied, TB3LeeresVendoroptRendertKeinSegment) {
    namespace abi = ::comdare::cache_engine::abi;
    EXPECT_EQ(abi::render_toolchain_stamp_glied(abi::ToolchainStampParts{}), std::string{});
    abi::ToolchainStampParts p{};
    p.cxx_dialect       = "gcc";
    p.cxx_driver        = "g++-17";
    std::string const g = abi::render_toolchain_stamp_glied(p);
    EXPECT_EQ(g.find("vendoropt="), std::string::npos) << "glied='" << g << "'";
}

// T-B4: die Schluessel-Tafel traegt das Feld als Key [9] hinter atomic128 (Key [8]).
TEST(Vo31VendoroptGlied, TB4KeyTafelTraegtVendoroptAlsLetztes) {
    namespace abi = ::comdare::cache_engine::abi;
    EXPECT_EQ(abi::kToolchainGliedKeyCount, std::size_t{10});
    EXPECT_EQ(abi::kToolchainGliedKeys[8], std::string_view{"atomic128"});
    EXPECT_EQ(abi::kToolchainGliedKeys[9], std::string_view{"vendoropt"});
}

// T-B5: active_vendoropt() liefert die gehisste Configure-Quelle -- verglichen gegen den FREMDEN
// Nenner COMDARE_VO31_EXPECT_VENDOROPT (dieselbe COMDARE_OPT_O3-Fallunterscheidung, die auch die
// Release-Flags steuert; nie aus dem Pruefling abgeschrieben). Und das LIVE-Glied traegt das Segment.
TEST(Vo31VendoroptGlied, TB5ActiveVendoroptAusDerGehisstenQuelle) {
    namespace pfn            = ::comdare::cache_engine::profile_facade;
    std::string_view const v = pfn::active_vendoropt();
    EXPECT_TRUE(v == "O2" || v == "O3") << "vendoropt='" << v << "'";
    EXPECT_EQ(v, std::string_view{COMDARE_VO31_EXPECT_VENDOROPT});

    std::string const live = pfn::compose_live_toolchain_stamp_glied();
    EXPECT_TRUE(live.starts_with("tc=2;")) << "glied='" << live << "'";
    std::string erwartet{";vendoropt="};
    erwartet += v;
    EXPECT_TRUE(live.ends_with(erwartet)) << "glied='" << live << "'";
}

// T-B6: Laengen-Pruefpunkt (Bauplan B4(6)): die VOLL belegte Glied-[5]-Form -- alle 14 Felder, inklusive
// des NEUEN letzten Feldes vendoropt -- bleibt unter dem Preimage-Budget kAnatomyFingerprintToolchainMax.
// Die Budget-Reihe selbst bleibt UNANGETASTET (Bauplan: messen, Zahl nennen, nie die Reihe bewegen); die
// gemessene Laenge steht im Log ([VO3-1 T-B6] ...) und im Commit-Text. Rot-Probe: Wegwerf-Mutation des
// Budget-Vergleichs auf 100 B beisst literal (Kopie bau/mutation/).
TEST(Vo31VendoroptGlied, TB6VollFormBleibtUnterToolchainBudget) {
    namespace abi = ::comdare::cache_engine::abi;
    abi::ToolchainStampParts p{};
    p.cxx_dialect       = "gcc";
    p.cxx_realversion   = "15.3.0";
    p.opt               = "O3";
    p.opt_flags         = "-O3";
    p.simd              = "avx512";
    p.ceb               = "8.0";
    p.target_isa        = "x86_64";
    p.telemetry         = "profiler";
    p.build_type        = "Release";
    p.gate_contribution = "avx512";
    p.atomic128         = "cx16";
    p.atomic128_flags   = "-mcx16";
    p.cxx_driver        = "g++-15";
    p.vendoropt         = "O2";
    std::string const g = abi::render_toolchain_stamp_glied(p);
    EXPECT_TRUE(g.ends_with(";vendoropt=O2")) << "glied='" << g << "'";
    EXPECT_LE(g.size(), abi::kAnatomyFingerprintToolchainMax) << "glied='" << g << "'";
    std::cout << "[VO3-1 T-B6] glied_len=" << g.size() << " budget=" << abi::kAnatomyFingerprintToolchainMax
              << " glied='" << g << "'\n";
}
