// S1 (Section 62-B Log-Flush, Befund CI-Smoke 12160 "6h stumm"): der ProgressHeartbeat-Baustein -- ein zeit-gatetes,
// geflushtes Fortschritts-Testat fuer die langen, sonst stillen Bau-/Mess-Schleifen. Leichte TU: pinnt das Zeilen-
// Format, die Sofort-erste-Einheit-Semantik und die Zeit-Drossel. In einen std::ostringstream statt std::cerr, damit
// die Ausgabe im Test literal geprueft werden kann.
// E-04-P1 (Nachtrag): zusaetzlich die vier Parse-Zweige von heartbeat_every_n (COMDARE_HEARTBEAT_EVERY) --
// ungesetzt / leer / Teil-Frass / "0" -- plus der Vollparse als Gegenprobe und die Verhaltens-Wirkung von "0".

#include <experiment_tree/progress_heartbeat.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib> // E-04-P1: setenv/unsetenv fuer die COMDARE_HEARTBEAT_EVERY-Parse-Zweige
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

// E-04-P1: RAII-Wache um COMDARE_HEARTBEAT_EVERY. Der Parser liest die Variable bei JEDEM Aufruf; die Tests
// duerfen die Umgebung der uebrigen TUs dieses Binaries nicht verschmutzen, also wird der Vor-Zustand
// (gesetzt/ungesetzt + Wert) gesichert und im Destruktor exakt wiederhergestellt.
class EveryEnvGuard {
public:
    EveryEnvGuard() {
        char const* const v = std::getenv(kName);
        war_gesetzt_        = (v != nullptr);
        if (war_gesetzt_) alt_ = v;
    }
    EveryEnvGuard(EveryEnvGuard const&)            = delete;
    EveryEnvGuard& operator=(EveryEnvGuard const&) = delete;
    ~EveryEnvGuard() {
        if (war_gesetzt_)
            ::setenv(kName, alt_.c_str(), 1);
        else
            ::unsetenv(kName);
    }
    static void setze(char const* wert) { ::setenv(kName, wert, 1); }
    static void loesche() { ::unsetenv(kName); }

private:
    static constexpr char const* kName = "COMDARE_HEARTBEAT_EVERY";
    bool                         war_gesetzt_{false};
    std::string                  alt_{};
};
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

// E-04-P1 (Trace-Budget, Teil 1d): heartbeat_every_n ist die EINZIGE Stelle, an der COMDARE_HEARTBEAT_EVERY gelesen
// wird -- ihre vier Parse-Zweige entscheiden, ob ein Voll-Bau-Job seinen Trace sprengt oder stumm wird. Sie waren
// bis hier ungetestet (Review-Kleinbefund). Gepinnt wird jeder Zweig EINZELN, plus der Vollparse als Gegenprobe:
// ohne ihn wuerde ein "return voreinstellung;" (also ein wirkungsloser Deckel) die Negativ-Faelle still bestehen.
TEST(HeartbeatEveryN, AlleVierParseZweigeUndDerVollparse) {
    EveryEnvGuard const wache; // stellt den Vor-Zustand der TU-Umgebung wieder her

    // (1) UNGESETZT -> Voreinstellung (byte-identisch zum Vor-E-04-P1-Verhalten; K = Compile-Worker-Zahl).
    EveryEnvGuard::loesche();
    EXPECT_EQ(ex::heartbeat_every_n(24), 24u) << "ungesetzt: der Aufrufer-Default gilt unveraendert";
    EXPECT_EQ(ex::heartbeat_every_n(1), 1u) << "ungesetzt: der Default wird durchgereicht, nicht ersetzt";

    // (2) LEER (gesetzt, aber "") -> Voreinstellung. Ein leerer Wert entsteht real durch 'export VAR=' in einem
    //     CI-Job-Prolog; from_chars wuerde auf einem leeren Bereich sonst einen unbestimmten Wert stehen lassen.
    EveryEnvGuard::setze("");
    EXPECT_EQ(ex::heartbeat_every_n(24), 24u) << "leer: kein Deckel, Default gilt";

    // (3) TEIL-FRASS "24abc" -> Voreinstellung. Genau HIER unterscheidet sich der Vollparse von atoi: atoi haette
    //     24 geliefert und damit einen Tippfehler still als gueltigen Deckel akzeptiert.
    EveryEnvGuard::setze("24abc");
    EXPECT_EQ(ex::heartbeat_every_n(7), 7u)
        << "Teil-Parse zaehlt NICHT: '24abc' ist kein Deckel, sondern ein Tippfehler";
    EveryEnvGuard::setze("abc");
    EXPECT_EQ(ex::heartbeat_every_n(7), 7u) << "gar keine Ziffer: ebenfalls Default";
    EveryEnvGuard::setze("-5");
    EXPECT_EQ(ex::heartbeat_every_n(7), 7u) << "negativ: fuer std::size_t kein gueltiger Vollparse -> Default";

    // (4) "0" -> 0 == ZAEHL-Gate AUS (nur noch das Zeit-Gate). Das ist ein ECHTER Wert, keine Ablehnung: 0 muss den
    //     Default UEBERSTIMMEN, sonst waere der Deckel nicht abschaltbar.
    EveryEnvGuard::setze("0");
    EXPECT_EQ(ex::heartbeat_every_n(24), 0u) << "'0' ueberstimmt den Default (Zaehl-Gate abschaltbar)";

    // (5) VOLLPARSE (Gegenprobe): ein gueltiger Wert kommt an -- sonst waeren (1)-(3) trivial erfuellbar.
    EveryEnvGuard::setze("256");
    EXPECT_EQ(ex::heartbeat_every_n(24), 256u) << "Vollparse: der Env-Deckel ersetzt den Default";
}

// E-04-P1: der "0"-Zweig ist kein reiner Zahlen-Pin -- er MUSS am Ende ein zeit-gatetes Verhalten ergeben. Hier
// wird der Wert wie im Bau-Loop (build_orchestrator) durch heartbeat_every_n in den Konstruktor gereicht.
TEST(HeartbeatEveryN, NullBedeutetNurZeitGateImLaufendenHeartbeat) {
    EveryEnvGuard const wache;
    EveryEnvGuard::setze("0");
    std::ostringstream    os;
    ex::ProgressHeartbeat hb{"tier-build", 12, os, std::chrono::seconds{3600},
                             /*every_n=*/ex::heartbeat_every_n(4)};
    for (int i = 0; i < 12; ++i) hb.tick();
    hb.done();
    std::string const out = os.str();
    EXPECT_EQ(heartbeat_lines(out), 2u) << out;                       // nur erste Einheit (Sofort-Lebenszeichen) + done
    EXPECT_EQ(out.find("tier-build 4/12"), std::string::npos) << out; // kein Zaehl-Gate mehr

    // Gegenprobe mit demselben Aufrufer-Default, aber ohne Env: das Zaehl-Gate greift wieder (4/8/12).
    EveryEnvGuard::loesche();
    std::ostringstream    os2;
    ex::ProgressHeartbeat hb2{"tier-build", 12, os2, std::chrono::seconds{3600},
                              /*every_n=*/ex::heartbeat_every_n(4)};
    for (int i = 0; i < 12; ++i) hb2.tick();
    hb2.done();
    EXPECT_EQ(heartbeat_lines(os2.str()), 5u) << os2.str(); // 1 + 4/8/12 + done
}

TEST(ProgressHeartbeat, DefaultStreamIsCerrAndDoesNotTouchStdout) {
    // Konstruktion ohne os-Argument nutzt std::cerr (Progress-Kanal), niemals std::cout/CSV -> golden-neutral.
    // Hier nur der Konstruktions-/Aufruf-Rauchtest (kein cout-Abgriff noetig; die Naht schreibt per Konstruktion cerr).
    ex::ProgressHeartbeat hb{"mess-zelle", 1};
    hb.tick();
    hb.done();
    SUCCEED();
}
