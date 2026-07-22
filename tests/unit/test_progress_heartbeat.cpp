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

TEST(ProgressHeartbeat, DefaultStreamIsCerrAndDoesNotTouchStdout) {
    // Konstruktion ohne os-Argument nutzt std::cerr (Progress-Kanal), niemals std::cout/CSV -> golden-neutral.
    // Hier nur der Konstruktions-/Aufruf-Rauchtest (kein cout-Abgriff noetig; die Naht schreibt per Konstruktion cerr).
    ex::ProgressHeartbeat hb{"mess-zelle", 1};
    hb.tick();
    hb.done();
    SUCCEED();
}
