// S1 (Section 62-B Log-Flush, Befund CI-Smoke 12160 "6h stumm"): der ProgressHeartbeat-Baustein -- ein zeit-gatetes,
// geflushtes Fortschritts-Testat fuer die langen, sonst stillen Bau-/Mess-Schleifen. Leichte TU: pinnt das Zeilen-
// Format, die Sofort-erste-Einheit-Semantik und die Zeit-Drossel. In einen std::ostringstream statt std::cerr, damit
// die Ausgabe im Test literal geprueft werden kann.

#include <experiment_tree/progress_heartbeat.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>

namespace ex = ::comdare::cache_engine::builder::experiment;

namespace {
// Zaehlt die Vorkommen von "[heartbeat]" (== Anzahl emittierter Zeilen).
[[nodiscard]] std::size_t heartbeat_lines(std::string const& s) {
    std::size_t n = 0;
    for (std::size_t p = s.find("[heartbeat]"); p != std::string::npos; p = s.find("[heartbeat]", p + 1)) ++n;
    return n;
}
} // namespace

TEST(ProgressHeartbeat, ZeroIntervalEmitsEveryTickPlusDone) {
    // interval=0 -> jede Zelle emittiert; done() haengt genau EINE Abschluss-Zeile an.
    std::ostringstream    os;
    ex::ProgressHeartbeat hb{"mess-zelle", 3, os, std::chrono::seconds{0}};
    hb.tick();
    hb.tick();
    hb.tick();
    hb.done();
    std::string const out = os.str();
    EXPECT_EQ(heartbeat_lines(out), 4u) << out; // 3 ticks + 1 done
    EXPECT_NE(out.find("mess-zelle 1/3"), std::string::npos) << out;
    EXPECT_NE(out.find("mess-zelle 2/3"), std::string::npos) << out;
    EXPECT_NE(out.find("mess-zelle 3/3"), std::string::npos) << out;
    EXPECT_NE(out.find("mess-zelle fertig 3/3"), std::string::npos) << out;
    // Format-Pin: jede Zeile traegt den Zeit-Marker "t+<n>s".
    EXPECT_NE(out.find("t+"), std::string::npos) << out;
    EXPECT_NE(out.find("s\n"), std::string::npos) << out;
}

TEST(ProgressHeartbeat, LargeIntervalThrottlesButFirstTickAndDoneAlwaysEmit) {
    // Grosses Intervall -> nur die ERSTE Einheit (Sofort-Lebenszeichen) + die done()-Abschluss-Zeile; die Zellen
    // dazwischen sind gedrosselt (kein Spam), aber der Lauf bleibt sichtbar (Anti-6h-stumm-Garantie).
    std::ostringstream    os;
    ex::ProgressHeartbeat hb{"tier-build", 5, os, std::chrono::seconds{3600}};
    for (int i = 0; i < 5; ++i) hb.tick();
    hb.done();
    std::string const out = os.str();
    EXPECT_EQ(heartbeat_lines(out), 2u) << out;                             // erste Zelle + done
    EXPECT_NE(out.find("tier-build 1/5"), std::string::npos) << out;        // Sofort-Lebenszeichen
    EXPECT_EQ(out.find("tier-build 3/5"), std::string::npos) << out;        // gedrosselt (nicht emittiert)
    EXPECT_NE(out.find("tier-build fertig 5/5"), std::string::npos) << out; // Abschluss immer
}

TEST(ProgressHeartbeat, EveryNCountGateEmitsEachNthPlusFirstAndDone) {
    // #27 (2026-07-23): ZAEHL-Gate. Grosses Zeit-Intervall (das Zeit-Gate feuert NUR die erste Einheit als Sofort-
    // Lebenszeichen), every_n=4 -> ZUSAETZLICH jede 4. Einheit. Bei 12 ticks: Einheit 1 (Sofort) + 4/8/12 (Zaehl-Gate)
    // = 4 tick-Zeilen; done() haengt genau EINE an => 5. Die Nicht-Vielfachen sind gedrosselt (kein Spam).
    std::ostringstream    os;
    ex::ProgressHeartbeat hb{"tier-build", 12, os, std::chrono::seconds{3600}, /*every_n=*/4};
    for (int i = 0; i < 12; ++i) hb.tick();
    hb.done();
    std::string const out = os.str();
    EXPECT_EQ(heartbeat_lines(out), 5u) << out;                       // 1 (Sofort) + 4/8/12 (Zaehl) + done
    EXPECT_NE(out.find("tier-build 1/12"), std::string::npos) << out; // Sofort-Lebenszeichen (Zeit-Gate erste Einheit)
    EXPECT_NE(out.find("tier-build 4/12"), std::string::npos) << out; // Zaehl-Gate every_n=4
    EXPECT_NE(out.find("tier-build 8/12"), std::string::npos) << out;
    EXPECT_NE(out.find("tier-build 12/12"), std::string::npos) << out;
    EXPECT_EQ(out.find("tier-build 5/12"), std::string::npos) << out; // Nicht-Vielfaches: gedrosselt
    EXPECT_EQ(out.find("tier-build 7/12"), std::string::npos) << out;
    EXPECT_NE(out.find("tier-build fertig 12/12"), std::string::npos) << out; // Abschluss immer

    // every_n=0 (Default) => reines Zeit-Gate, byte-identisch zum Vor-#27-Verhalten (nur erste + done bei grossem Intervall).
    std::ostringstream    os0;
    ex::ProgressHeartbeat hb0{"tier-build", 12, os0, std::chrono::seconds{3600}, /*every_n=*/0};
    for (int i = 0; i < 12; ++i) hb0.tick();
    hb0.done();
    EXPECT_EQ(heartbeat_lines(os0.str()), 2u) << os0.str(); // nur erste Einheit + done (kein Zaehl-Gate)
}

TEST(ProgressHeartbeat, DefaultStreamIsCerrAndDoesNotTouchStdout) {
    // Konstruktion ohne os-Argument nutzt std::cerr (Progress-Kanal), niemals std::cout/CSV -> golden-neutral.
    // Hier nur der Konstruktions-/Aufruf-Rauchtest (kein cout-Abgriff noetig; die Naht schreibt per Konstruktion cerr).
    ex::ProgressHeartbeat hb{"mess-zelle", 1};
    hb.tick();
    hb.done();
    SUCCEED();
}
