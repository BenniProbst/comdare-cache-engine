// SPDX-License-Identifier: LicenseRef-Comdare-Research-1.0
// Concurrency-/Eigentumsmodell-Tests fuer comdare::rcu (Feature #25, honest-100 %)
//
// Deckt die beiden zuvor UNGETESTETEN Enden des Reader-Lifetimes ab, die der Ownership-Inversion-
// Fix (rcu.hpp) strukturell schliesst:
//   1. Thread-Ende: Reader-Slots muessen die Reader-Threads ueberleben, ohne dangling zu werden
//      (frueher: rohe Zeiger auf ein thread_local-Objekt in readers_ → Use-after-free in
//      synchronize()/reader_count() nach Thread-Join).
//   2. Stack-Domain-Zerstoerung + Adress-Wiederverwendung: eine neue Stack-RcuDomain an der Adresse
//      einer soeben zerstoerten muss einen FRISCHEN Slot bekommen (frueher: geteilter thread_local
//      "registered"-Flag → zweite Stack-Domain blieb faelschlich leer / Stale-Deref).
//
// Die Test-LOGIK ist deterministisch (feste Slot-Zahlen, feste reader_count()); zusaetzlich sind die
// Faelle als TSan-Ziel out-of-tree gedacht (g++-16 -fsanitize=thread). Der bestehende
// GracePeriodProtects…-Test in test_rcu.cpp bleibt unveraendert.

#include "rcu.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace rcu = comdare::rcu;

// §2.1 — Thread-Ende hinterlaesst keinen dangling Reader.
// N Reader-Threads registrieren sich (RcuReadGuard) und JOINEN. DANACH iteriert synchronize() /
// reader_count() die Domain-owned Slots. Vor dem Fix waren das N tote thread_local-Zeiger → UAF hier.
// Deterministisch: jeder Thread erzeugt genau EINEN Guard → genau EIN Slot; der Main-Thread
// registriert sich auf d NICHT → reader_count() == N und ueber viele synchronize()-Runden stabil.
TEST(RcuConcurrency, ThreadEndLeavesNoDanglingReader) {
    rcu::RcuDomain    d;
    constexpr int     kThreads = 8;
    std::atomic<long> reads{0};

    {
        std::vector<std::thread> workers;
        workers.reserve(kThreads);
        for (int i = 0; i < kThreads; ++i) {
            workers.emplace_back([&d, &reads]() {
                rcu::RcuReadGuard g{d};
                reads.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            });
        }
        for (auto& t : workers) { t.join(); }
    }

    // Alle Reader-Threads sind beendet; ihre Slots leben Domain-owned (inert, active=false) weiter.
    EXPECT_EQ(reads.load(), static_cast<long>(kThreads));
    EXPECT_EQ(d.reader_count(), static_cast<std::size_t>(kThreads));

    // Mehrfaches synchronize()/reader_count() ueber tote-Thread-Slots MUSS sicher sein (kein UAF).
    for (int round = 0; round < 128; ++round) {
        d.synchronize();
        EXPECT_EQ(d.reader_count(), static_cast<std::size_t>(kThreads));
    }
    SUCCEED();
}

// §2.2/§2.3 — Stack-Domain-Zerstoerung + Adress-Wiederverwendung liefert einen frischen Slot.
// In einer Schleife je Iteration eine Stack-RcuDomain bauen, EINEN Reader registrieren, Domain
// zerstoeren. Der Stack-Slot der Variablen 'd' wird typ. wiederverwendet (Adressgleichheit) — der
// monotone serial_ + generation-gekeyter Cache erzwingt dennoch Cache-Miss → frischer Slot.
// Vor dem Fix waere reader_count() ab i>=1 faelschlich 0 (geteilter registered-Flag) bzw. Stale-Deref.
TEST(RcuConcurrency, StackDomainAddressReuseGivesFreshSlot) {
    std::uintptr_t first_addr = 0;

    for (int i = 0; i < 64; ++i) {
        rcu::RcuDomain d;
        auto const     addr = reinterpret_cast<std::uintptr_t>(&d);
        if (i == 0) { first_addr = addr; }

        // Frische Domain → noch kein Reader registriert.
        EXPECT_EQ(d.reader_count(), 0u);
        {
            rcu::RcuReadGuard g{d};
            EXPECT_EQ(d.reader_count(), 1u); // genau EIN frischer Slot fuer diese (serial'd) Domain
            auto* s = d.current_thread_state();
            EXPECT_TRUE(s->active.load());
        }
        // synchronize() darf KEINEN Stale-Slot einer frueheren (zerstoerten) Domain anfassen.
        d.synchronize();
        EXPECT_EQ(d.reader_count(), 1u);
    }

    // Adress-Wiederverwendung ist wahrscheinlich, aber nicht standardgarantiert → nur informativ.
    (void)first_addr;
    SUCCEED();
}

// TSan-Ziel: Reader-Threads, die WAEHREND laufender synchronize()-Runden ENTSTEHEN UND ENDEN
// (Thread-Churn), gleichzeitig zum Writer, der ueber viele Grace-Periods alte Versionen freigibt.
// Uebt den "Slot ueberlebt Thread"-Pfad NEBENLAEUFIG zu synchronize() aus. Determiniert im Ergebnis
// (kein Crash, reads>0, Post-Join-synchronize() sicher), race-frei unter -fsanitize=thread.
// TSAN-GRENZE (2026-08-17, Lens-Uebergabe-Befund): -fsanitize=thread modelliert
// atomic_thread_fence NICHT (-Wtsan an rcu.hpp:125) -- genau der Store-Load-Fence des
// SB/Dekker-Musters in synchronize(), auf dem die Domaenen-Korrektheit beruht. Das gruene
// TSan-Ergebnis deckt also den REST, diese Fence-Ordnung deckt es strukturell nicht
// (werkzeug-bedingt, kein Lauf-Zufall). Vollstaendige Fence-Deckung = eigener Posten im
// TSan-Ausbau (HY-A2-Umfeld), nicht durch Wiederholungslaeufe erreichbar.
// FORTSCHREIBUNG A2.5/G4 M-2 (2026-08-19): die Grenze ist jetzt GEMESSEN
// (g4_m2_vorher_wtsan_warnung.log: GCC 15.3 warnt an BEIDEN Naht-Punkten, read_lock:105 und
// synchronize:125) und UEBERBRUECKT: die Naht faehrt unter TSan als seq_cst-RMW auf der
// geteilten Bruecke (rcu.hpp, store_load_fence_seq_cst + COMDARE_RCU_TSAN_BRUECKE) -- eine
// happens-before-Kante, die TSan VOLL modelliert; ausserhalb von TSan bleibt die Fence. Der
// Werkzeug-Satz "TSan modelliert atomic_thread_fence nicht" gilt unveraendert, trifft die Naht
// aber nicht mehr. Deckung seitdem STRUKTURELL statt nur dokumentiert: FenceNahtStruktur*
// (Quelltext-Wache, beisst auf jede Schwaechung) + FenceNahtHatHappensBeforeKante*
// (deterministischer HB-Biss ueber die echte Naht-Funktion, unten). Offen bleibt der
// TSan-VOLLAUSBAU (alle Concurrency-Targets im sanitize-Job) als eigener Posten.
// PRAEZISIERUNG 2026-08-17: die Teilzusage "reads>0" hing am Scheduling und fiel unter Last
// reproduzierbar. Sie ist jetzt GESCHAERFT zu "nach dem Anlauf wird WEITER gelesen" -- das ist
// die Nebenlaeufigkeit selbst statt einer Zahl, die auch ohne sie zustande kaeme. Beide Sperren
// und ihre Messung stehen unten.
TEST(RcuConcurrency, WriterSyncsWhileReaderThreadsChurnNoUseAfterFree) {
    rcu::RcuDomain    d;
    rcu::RcuDeferred  def;
    std::atomic<int*> slot{new int(0)};
    std::atomic<bool> stop{false};
    std::atomic<long> reads{0};

    // Je Reader EINMAL erhoeht, nach seinem ERSTEN Read. reads allein taugt als Anlauf-Mass
    // nicht: es ist EIN globaler Zaehler, ein einzelner schneller Reader erfuellte eine Schwelle
    // von kReaders im Alleingang -- die Zusage "jeder Reader hat gelesen" waere unbelegt.
    std::atomic<int> angelaufen{0};
    std::atomic<int> generation{0}; // Fortschritt des Writers, von den Readern gelesen

    constexpr int kReaders    = 4;
    constexpr int kWriterGen  = 500;
    constexpr int kReaderEnde = kWriterGen / 2; // Reader enden MITTEN in der Writer-Phase

    // DER READER HAENGT AM WRITER-FORTSCHRITT, NICHT AN EINER FESTEN RUNDENZAHL (2026-08-17,
    // zweiter Befund der Fix-Runde). Vorher lief er 300 Runden; unter Last war er damit LAENGST
    // DURCH, bevor der Writer seine erste Generation publiziert hatte -- 4x300 Reads brauchen
    // Mikrosekunden, eine einzige rcu_replace+flush laeuft durch eine ganze Grace-Period.
    // GEMESSEN in genau dieser Bauform: 43 von 50 Laeufen unter 32-Prozess-Last hatten NULL
    // Reads nebenlaeufig zum Writer. Der Test hat seinen Gegenstand -- Churn WAEHREND
    // synchronize() -- unter Last also nie geprueft; er war nur deshalb gruen, weil die alte
    // Zusage "es gab ueberhaupt Reads" davon nichts wissen wollte.
    // Jetzt endet der Reader, wenn der Writer die HAELFTE seiner Generationen hat. Damit ist
    // woertlich hergestellt, was der Kopfkommentar behauptet: Threads, die WAEHREND laufender
    // synchronize()-Runden entstehen UND enden.
    //
    // KEINE RUNDEN-OBERGRENZE. Zwei Fassungen mit einer solchen Notbremse sind GEMESSEN
    // gescheitert, weil jede feste Rundenzahl in derselben Groessenordnung liegt wie die
    // Writer-Phase und damit ein RENNEN wird statt einer Sicherung: mit 200000 spulten die Reader
    // in 3 von 50 Laeufen unter Last ihr Budget ab, BEVOR die erste Generation publiziert war --
    // dann ist der Waechter unten zu Recht rot, obwohl am Code nichts fehlt. Eine groessere Zahl
    // verschiebt das Rennen nur. Beendet wird der Reader deshalb ausschliesslich durch den
    // Writer: kReaderEnde mitten in der Phase, stop als Ausgang nach der letzten Generation.
    // Ein Writer, der wirklich haengt, faellt als ctest-Timeout auf -- das ist ehrlicher als ein
    // Zaehler, der gueltige Laeufe rot faerbt. Verklemmen kann sich dabei nichts: der Reader gibt
    // seinen Guard in JEDER Iteration frei, synchronize() wartet also nie auf ihn.
    //
    // Im Hot-Path steht bewusst NUR der eine Load der Schleifenbedingung. Ein zweiter Load zum
    // Mitzaehlen "war der Writer schon da" waere Messung IM Messgegenstand gewesen -- dieser
    // Test uebt gerade den wait-free Read-Pfad. Die Nebenlaeufigkeit wird statt dessen von aussen
    // ueber ein Schnappschuss-Paar bestimmt (unten).
    auto churn = [&]() {
        bool erster = true;
        while (generation.load(std::memory_order_relaxed) < kReaderEnde && !stop.load(std::memory_order_relaxed)) {
            rcu::RcuReadGuard g{d};
            int const*        p = slot.load(std::memory_order_acquire);
            volatile int      v = *p; // Deref waehrend der read-side critical section MUSS gueltig bleiben
            (void)v;
            reads.fetch_add(1, std::memory_order_relaxed);
            if (erster) {
                erster = false;
                angelaufen.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) { readers.emplace_back(churn); }

    // ANLAUF-SPERRE (2026-08-17): ohne sie war die Zusage im Kopfkommentar ("Determiniert im
    // Ergebnis ... reads>0") von Scheduling-GLUECK abhaengig. Unter Last kam der Main-Thread
    // durch alle 500 Generationen, BEVOR ein Reader seinen ersten Durchlauf schaffte; danach
    // stand stop=true, alle vier brachen bei k=0 ab, und EXPECT_GT(reads,0) fiel mit
    // "actual: 0 vs 0". REPRODUZIERT: 5 von 30 Laeufen unter 32-Prozess-Last, 0 von 50 ohne; in
    // der vollstaendig nachgestellten Ursprungs-Lage (32 Rechen-Prozesse UND zwei gleichzeitige
    // ctest-Serien) 8 von 50, beide Seiten mit genau dieser Signatur. Die Alternativ-Ursache ist
    // ausgeschlossen: ein EAGAIN der Thread-Erzeugung braucht ein knappes Prozess-Limit
    // (kuenstlich erzwungen: 20 von 20 rot), waehrend real 245493 erlaubt und ~1166 belegt sind.
    // KORREKTUR zur ersten Fassung dieses Kommentars: die gtest-Diagnose lag von Anfang an vor
    // (im clang-Protokoll direkt ueber der ctest-Zeile). Die Nachmessung belegte Reproduzierbar-
    // keit und Haeufigkeit, nicht die Ursache -- die stand schon da, ein zu enger Log-Filter
    // hatte sie verfehlt.
    // Der Assert war im RECHT -- er ist der Waechter dagegen, dass dieser Test seinen Gegenstand
    // (Reader-Churn NEBENLAEUFIG zu synchronize()) still verfehlt. Gefehlt hat das Warten.
    // TERMINIERUNG, seit dem Writer-Kopplungs-Umbau an ZWEI Bedingungen (2026-08-17): die Threads
    // sind erzeugt (sonst haette emplace_back geworfen), stop ist hier noch false UND generation
    // steht auf 0, liegt also unter kReaderEnde -- deshalb durchlaeuft jeder Reader mindestens
    // einmal den Rumpf und erhoeht angelaufen. Die zweite Bedingung ist neu und nicht kosmetisch:
    // ein Koeder-Lauf mit kReaderEnde=0 liess genau hier ENDLOS warten (ctest-Timeout 124, kein
    // Assert), weil dann kein Reader je in den Rumpf kommt. Wer kReaderEnde veraendert, muss
    // sicherstellen, dass es beim Start GROESSER 0 ist.
    while (angelaufen.load(std::memory_order_relaxed) < kReaders) { std::this_thread::yield(); }

    // ZWEITE STUFE (2026-08-17, Lens-Fund): die Anlauf-Sperre allein macht ein EXPECT_GT(reads,0)
    // TAUTOLOGISCH -- danach ist reads per Konstruktion >= kReaders, der Waechter kann nicht mehr
    // fallen. Ein Bezugspunkt-Vergleich (reads gegen den Anlauf-Stand, gemessen nach der ersten
    // Generation) war der erste Versuch und ist VERWORFEN: er stellte die richtige Frage an eine
    // Struktur, die sie nicht beantworten konnte, und faerbte den Test unter Last zu 43 von 50
    // Laeufen rot -- inhaltlich zu Recht, praktisch unlandbar. Geloest wird das jetzt eine Ebene
    // tiefer, in der Schleifenbedingung der Reader (oben): sie leben, solange der Writer arbeitet.
    // Gemessen wird ueber ein SCHNAPPSCHUSS-PAAR um die Writer-Phase herum: der Stand direkt nach
    // dem ERSTEN Publish gegen den Stand nach dem LETZTEN. Steigt reads dazwischen, hat
    // nachweislich ein Reader gelesen, WAEHREND der Writer Generationen publizierte und
    // synchronize() lief -- das ist die Nebenlaeufigkeit selbst, nicht ihr Nebenprodukt. Der
    // Gesamtzaehler taugt dafuer nicht (er ist nach der Anlauf-Sperre ohnehin > 0), die Differenz
    // schon: sie kann null werden und tut es, sobald die Reader den Writer verpassen.
    generation.store(1, std::memory_order_relaxed);
    rcu::rcu_replace(slot, new int(1), def); // publiziert neue Version, defer alte
    def.flush(d);                            // synchronize (Grace-Period) + delete alte NACH GP
    long const reads_bei_erster_gen = reads.load(std::memory_order_relaxed);

    for (int gen = 2; gen <= kWriterGen; ++gen) {
        // Erst den Fortschritt sichtbar machen, dann publizieren -- die Reader haengen mit ihrer
        // Lebensdauer an dieser Zahl.
        generation.store(gen, std::memory_order_relaxed);
        rcu::rcu_replace(slot, new int(gen), def);
        def.flush(d);
    }
    long const reads_nach_letzter_gen = reads.load(std::memory_order_relaxed);

    // stop bleibt die ZWEITE Sperre. Im Normalfall haben die Reader sich bei kReaderEnde laengst
    // selbst beendet -- genau das ist der Churn, den dieser Test uebt.
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : readers) { t.join(); }

    // Threads beendet → ihre Slots bleiben Domain-owned; Post-Join-synchronize() MUSS sicher sein.
    d.synchronize();
    // DER EIGENTLICHE WAECHTER: die DIFFERENZ ueber die Writer-Phase. Nicht tautologisch -- sie
    // faellt auf null, sobald die Reader den Writer verpassen, und genau das war vor dem Umbau
    // der Normalfall unter Last. Die Struktur oben stellt die Bedingung her, dieser Assert
    // prueft, dass sie eingetreten ist; Herstellen ohne Nachpruefen waere wieder nur ein
    // Versprechen.
    EXPECT_GT(reads_nach_letzter_gen, reads_bei_erster_gen)
        << "zwischen erster und letzter Writer-Generation wurde nicht ein einziges Mal gelesen: "
           "die Reader waren durch, bevor der Writer arbeitete -- der Test wuerde seinen "
           "Gegenstand (Churn WAEHREND synchronize()) verfehlen, statt ihn zu pruefen";
    EXPECT_GT(reads.load(), 0);
    delete slot.load();
    SUCCEED();
}

// ============================================================================================
// A2.5/G4 M-2 -- DIE FENCE-NAHT, MESSBAR: Struktur-Wache + deterministischer HB-Biss
// ============================================================================================

// (1) DIE STRUKTUR-WACHE: sie liest rcu.hpp (kommentarbereinigt, Muster der hy_a1-Zonen-Wache)
// und haelt die Naht-Form fest. KEIN Timing: wer die Fence schwaecht, die Bruecke relaxed macht
// oder einen der beiden Naht-Punkte am Ruf vorbeifuehrt, bricht GENAU EINE dieser Zaehlungen --
// deterministisch, in jedem Build, ohne TSan (V7: Abhaengigkeits-/Fence-Beleg statt Lauf-Zufall).
TEST(RcuConcurrency, FenceNahtStrukturFenceUndBrueckeSindSeqCstUndBeideNahtPunkteRufenSie) {
    std::filesystem::path const pfad{COMDARE_RCU_HEADER_PFAD};
    std::ifstream               ein{pfad};
    ASSERT_TRUE(ein.is_open()) << pfad;
    std::ostringstream puffer;
    puffer << ein.rdbuf();

    // Kommentar-Entferner -- ABSCHRIFT der kommentar-festen hy_a1-Bauform (Abschrift schlaegt
    // Import: ein Test-Helfer-Header ueber TU-Grenzen waere eine neue Kopplung nur fuers Zaehlen).
    auto ohne_kommentare = [](std::string const& roh) {
        std::string        raus;
        bool               in_block = false;
        std::istringstream zeilen{roh};
        std::string        z;
        while (std::getline(zeilen, z)) {
            std::string sauber;
            for (std::size_t i = 0; i < z.size(); ++i) {
                if (in_block) {
                    if (i + 1 < z.size() && z[i] == '*' && z[i + 1] == '/') {
                        in_block = false;
                        ++i;
                    }
                    continue;
                }
                if (i + 1 < z.size() && z[i] == '/' && z[i + 1] == '/') break;
                if (i + 1 < z.size() && z[i] == '/' && z[i + 1] == '*') {
                    in_block = true;
                    ++i;
                    continue;
                }
                sauber += z[i];
            }
            raus += sauber;
            raus += '\n';
        }
        return raus;
    };
    std::string const quelle = ohne_kommentare(puffer.str());

    auto zaehle = [&quelle](std::string_view muster) {
        std::size_t n = 0;
        for (std::size_t pos = quelle.find(muster); pos != std::string::npos;
             pos             = quelle.find(muster, pos + 1)) {
            ++n;
        }
        return n;
    };

    // (a) GENAU EINE Fence im ganzen Header, und sie ist seq_cst: der Fence-Zweig der Naht-
    //     Funktion. Eine zweite Fence waere eine Naht am Wachen-Radar vorbei; eine schwaechere
    //     Ordnung senkte die seq_cst-Zaehlung auf 0.
    EXPECT_EQ(zaehle("std::atomic_thread_fence"), std::size_t{1});
    EXPECT_EQ(zaehle("std::atomic_thread_fence(std::memory_order_seq_cst)"), std::size_t{1});
    // (b) GENAU EIN Bruecken-RMW, und er ist seq_cst: der TSan-Zweig derselben Funktion.
    EXPECT_EQ(zaehle("g_tsan_fence_bridge.fetch_add"), std::size_t{1});
    EXPECT_EQ(zaehle("g_tsan_fence_bridge.fetch_add(1, std::memory_order_seq_cst)"), std::size_t{1});
    // (c) BEIDE Naht-Punkte (read_lock, synchronize) rufen DIE EINE Funktion: 2 Rufe + 1
    //     Definition = 3 Vorkommen des Namens. Faellt ein Ruf weg (ein Naht-Punkt wieder auf
    //     eigene Faust), wird es 2; kommt eine zweite Definition dazu, wird es 4.
    EXPECT_EQ(zaehle("store_load_fence_seq_cst"), std::size_t{3});

    // Kommentar-Festigkeit des Entferners (Gegenprobe wie in der hy_a1-Wache: ohne sie koennte
    // er zur Identitaet degenerieren und (a)-(c) zaehlten Kommentar-Erwaehnungen mit).
    EXPECT_EQ(ohne_kommentare("// std::atomic_thread_fence im Kommentar\n"), std::string{"\n"});
    EXPECT_NE(ohne_kommentare("store_load_fence_seq_cst();\n").find("store_load_fence_seq_cst"),
              std::string::npos);
}

// (2) DER DETERMINISTISCHE HB-BISS ueber die ECHTE Naht-Funktion. Bauform Message-Passing:
//   Schreiber: payload (non-atomic) -> NAHT -> flag=1 (relaxed).
//   Leser:     spin auf flag==1 (relaxed) -> NAHT -> payload lesen.
// WARUM DAS DETERMINISTISCH IST UND KEIN TIMING-FLAKE: das relaxed-Flag traegt selbst KEINE
// happens-before-Kante -- die EINZIGE Kanten-Quelle ist die Naht. Unter TSan (Bruecken-Zweig)
// ist die Kette zwingend: der Leser-RMW laeuft in Echtzeit NACH dem Schreiber-RMW (der Leser
// startet seinen erst nach Sicht von flag==1, das der Schreiber erst NACH seiner Naht setzt);
// RMWs auf derselben Zelle sind echtzeit-total geordnet, der spaetere liest den Wert des
// frueheren aus dessen Release-Sequenz -> synchronizes-with -> der payload-Read ist geordnet.
// In JEDEM Lauf passieren beide Zugriffe; die Kante existiert oder fehlt STRUKTURELL:
//   Bruecke seq_cst  -> TSan-Modell traegt die Kante -> gruen.
//   Bruecke relaxed/entfernt (Wegwerf-Mutation) -> keine Kante -> TSan meldet das Race in
//   jedem Lauf (Rot-Beleg g4_m2_t1_rot_bruecke_relaxed_tsan.log).
// OHNE TSan prueft derselbe Fall die reale Sichtbarkeit ueber den Fence-Zweig (payload==42) --
// er ist dann kein Race-Detektor, aber auch nie falsch-rot.
TEST(RcuConcurrency, FenceNahtHatHappensBeforeKanteDeterministischUeberDieEchteNahtFunktion) {
    // WARMUP-THREAD, GEMESSEN NOTWENDIG (2026-08-19, build-tsan, GCC 15.3, Bruecke testweise auf
    // relaxed mutiert): TSan meldete das Probe-Race NUR, wenn vorher schon ein Kind-Thread eine
    // main-Stack-Variable beschrieben hatte und gejoint war -- Probe allein 0/20 rot, nach einem
    // solchen Vorgaengerfall 20/20 bzw. 5/5 rot, --gtest_repeat=2 5/5 rot (zweiter Durchlauf),
    // nach dem thread-freien Vorgaenger (StackDomain) 0/5, leerer join-Thread 0/20. Die Probe
    // stellt ihre Randbedingung deshalb SELBST her, statt von der Fall-Reihenfolge der Suite
    // abzuhaengen. Das ist eine dokumentierte BEOBACHTUNG der TSan-Laufzeit, keine Ursachen-
    // Behauptung ueber ihre Interna; die Biss-Messungen stehen im Rot-Log
    // g4_m2_t1_rot_bruecke_relaxed_tsan.log.
    int warm = 0;
    std::thread{[&warm] { warm = 1; }}.join();
    ASSERT_EQ(warm, 1);

    int              payload = 0; // non-atomic: der Konflikt-Gegenstand, den NUR die Naht ordnet
    std::atomic<int> flag{0};     // relaxed: Reihenfolge-Bote OHNE eigene HB-Kante

    std::thread schreiber([&] {
        payload = 42;                             // (1) der geschuetzte Wert
        rcu::store_load_fence_seq_cst();          // (2) DIE NAHT -- einzige Kanten-Quelle
        flag.store(1, std::memory_order_relaxed); // (3) Bote: Naht ist komplett
    });

    while (flag.load(std::memory_order_relaxed) == 0) { std::this_thread::yield(); }
    rcu::store_load_fence_seq_cst(); // Leser-Naht: schliesst die RMW-Kette (TSan) / Fence (real)
    EXPECT_EQ(payload, 42) << "Dekker-Sichtbarkeit ueber die Naht verletzt";

    schreiber.join();
}
