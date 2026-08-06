// test_lb1_knoten_heuristik_log -- LB-1 (F9-Paketschnitt, A1-Lager-Rest-Welle).
//
// TRAGENDE Abnahme des Knoten-Log-Systems (knoten_heuristik_log.hpp): complete-heuristik.log je
// Knoten, SHA512-Anker auf dem v6-Blatt-Key, Alleinschreiber-Lock und -- als EIGENE Test-Klasse
// (Owner-Auflage A-1, "die Truncate-Zustandsmaschine ist die Bug-Quelle") -- die
// Truncate-Zustandsmaschine samt Crash-Semantik.
//
// OE-B-DUMMY-LAGER: FakeAblage (In-Memory, zaehlt die Verben) plus ein echtes Temp-Verzeichnis.
// Kein minio, kein Netz, kein mc; die Zeit kommt ueber eine SKRIPT-Uhr (NowFn), damit Frist- und
// Konfliktfaelle ohne Wall-Clock reproduzierbar sind.
//
// STEMPEL-HOMONYMIE (T2-A/F4-NB3, 2026-08-06): der SHA512-Anker dieses Logs sitzt auf dem v6-BLATT-Key,
// also am LAGER-/SIDECAR-Stempel (Blattinhalt + .fingerprint/.version/.algos/.variant). Er ist NICHT
// der Plan-Stempel `|bau=` aus bestandslog::slice_plan_stamp und nicht der Versionierungs-Stempel in
// der Binary. Drei Dinge, ein Wort -- die unqualifizierte Rede von "dem Stempel" hat den
// F4-NB2-Fehlschluss ausgeloest; hier steht deshalb, welcher gemeint ist.

#include "bestandslog/knoten_heuristik_log.hpp"
#include "bestandslog/lager_baum_writer.hpp"
#include "comdare_test_tmp.hpp"           // #278/#24 + Posten 69: per-User-/per-Build-Temp-Wurzel
#include "support/oeb_stempel_zeilen.hpp" // OE-B-Stempel-Fixture + split_lines + Ruecklese (LB-6)

#include <cache_engine/abi/anatomy_fingerprint.hpp> // OF-M3-2: das Overlay-Glied, heute ehrlich leer

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;
namespace ct = comdare::test;
namespace fs = std::filesystem;

namespace {

// Der CT-Nachweis der Zustandsmaschine: WER kann truncaten? Als Concept formuliert, damit die Frage
// ueberhaupt stellbar ist -- ein direkter requires-Ausdruck auf einem Nicht-Template-Typ waere ein
// harter Compile-Fehler statt einer Antwort.
template <class T>
concept KannTruncaten = requires(T zustand, bl::BaumAblage const& ablage, bl::AlleinschreiberNachweis const& n) {
    zustand.truncate_und_inventarisiere(ablage, n, std::string{});
};

// --- In-Memory-Ablage mit Verb-Zaehlung + Fehler-Injektion (auch fuer den Crash-Fall). ---
struct FakeAblage {
    std::map<std::string, std::string> dateien;
    std::size_t                        write_rufe = 0;
    // Der Crash: ab dem n-ten Schreibvorgang schlaegt jedes Schreiben fehl (0 == nie).
    std::size_t bricht_ab_schreibvorgang = 0;

    [[nodiscard]] bl::BaumAblage naht() {
        bl::BaumAblage a;
        a.verzeichnis_anlegen = [](std::string const&) { return true; };
        a.datei_schreiben     = [this](std::string const& p, std::string const& inhalt) {
            ++write_rufe;
            if (bricht_ab_schreibvorgang != 0 && write_rufe >= bricht_ab_schreibvorgang) return false;
            dateien[p] = inhalt;
            return true;
        };
        a.datei_lesen = [this](std::string const& p) -> std::optional<std::string> {
            auto it = dateien.find(p);
            if (it == dateien.end()) return std::nullopt;
            return it->second;
        };
        a.existiert       = [this](std::string const& p) { return dateien.count(p) != 0; };
        a.datei_entfernen = [this](std::string const& p) {
            dateien.erase(p);
            return true;
        };
        return a;
    }
};

// Skript-Uhr (dieselbe Bauform wie test_g3_bestandslog_lock).
struct SkriptUhr {
    std::int64_t            jetzt = 1'000'000;
    [[nodiscard]] bl::NowFn fn() {
        return [this]() { return jetzt; };
    }
};

std::string hex128(char c) { return std::string(128, c); }

bl::KnotenBlatt blatt(char c, bl::Genus g = bl::Genus::binary, std::string const& utc = "2026-08-03T12:00:00Z") {
    return bl::KnotenBlatt{hex128(c), g, utc};
}

bl::LockOwner owner(std::string const& uuid) { return bl::LockOwner{uuid, "prod1", 4242}; }

constexpr std::string_view kKnoten = "target_isa=amd64_v3/01_read_path=search_algo-prt_art";

bl::KnotenLog frisches_log() {
    bl::KnotenLog l;
    l.knoten    = std::string{kKnoten};
    l.stand_utc = "2026-08-03T12:00:00Z";
    l.erwartet  = 3;
    return l;
}

// Ein Log, das die 4-KB-Schwelle SICHER ueberschreitet (jedes Blatt ist ueber 150 Byte).
// Bis 2026-08-06 protected static in TruncateZustandsmaschine; seit LB-6 hier, weil der
// Nebenlaeufigkeits-Abschnitt (7) dasselbe Log braucht und kein Fixture-Erbe ist.
[[nodiscard]] bl::KnotenLog grosses_log() {
    bl::KnotenLog l = frisches_log();
    for (int i = 0; i < 40; ++i) {
        std::string key(128, 'a');
        key[0]       = static_cast<char>('0' + (i % 10));
        key[1]       = static_cast<char>('a' + (i / 10));
        auto const e = bl::beobachte_blatt(l, bl::KnotenBlatt{key, bl::Genus::binary, "2026-08-03T12:00:00Z"});
        (void)e;
    }
    return l;
}

class TempKnoten {
public:
    // POSTEN 69 / #278/#24: die Wurzel kommt aus comdare::test::user_tmp_dir(), nicht direkt aus
    // temp_directory_path(). Ein fester Name direkt unter /tmp ist je uid UND je Worktree
    // dieselbe Adresse -- der Konstruktor raeumt sie per remove_all leer, und zwei gleichzeitig
    // laufende ctest-Laeufe (zwei Worktrees desselben Users) loeschten einander mitten im Test die
    // Dateien. Fuer den Nebenlaeufigkeits-Abschnitt (7) ist das keine Theorie mehr: er faehrt zwei
    // echte Threads auf DIESEM Verzeichnis. user_tmp_dir() liegt weiterhin unter
    // temp_directory_path() -- die Golden-/Repo-Neutralitaet der TU aendert sich dadurch nicht.
    TempKnoten() {
        auto const basis = ::comdare::test::user_tmp_dir() / "comdare_lb1_knoten";
        fs::remove_all(basis);
        fs::create_directories(basis);
        pfad_ = basis.string();
    }
    ~TempKnoten() {
        std::error_code ec;
        fs::remove_all(fs::path{pfad_}, ec);
    }
    TempKnoten(TempKnoten const&)                                 = delete;
    TempKnoten&                      operator=(TempKnoten const&) = delete;
    [[nodiscard]] std::string const& pfad() const noexcept { return pfad_; }

private:
    std::string pfad_;
};

} // namespace

// ===========================================================================
// (1) FORMAT -- Roundtrip, Byte-Stabilitaet, Anker auf dem v6-Key.
// ===========================================================================
TEST(Lb1KnotenLog, RoundtripGateEmitParseEmit) {
    auto l = frisches_log();
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('a')).ok());
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('b', bl::Genus::measurement)).ok());
    auto const einmal = bl::emit_knoten_log(l);
    auto const wieder = bl::parse_knoten_log(einmal);
    ASSERT_TRUE(wieder.has_value());
    EXPECT_EQ(bl::emit_knoten_log(*wieder), einmal) << "emit(parse(emit(x))) == emit(x)";
    EXPECT_EQ(*wieder, l) << "Der geparste Stand ist FELDWEISE derselbe";
}

TEST(Lb1KnotenLog, EmitIstEingabeReihenfolgeUnabhaengig) {
    auto a = frisches_log();
    auto b = frisches_log();
    ASSERT_TRUE(bl::beobachte_blatt(a, blatt('a')).ok());
    ASSERT_TRUE(bl::beobachte_blatt(a, blatt('c')).ok());
    ASSERT_TRUE(bl::beobachte_blatt(b, blatt('c')).ok());
    ASSERT_TRUE(bl::beobachte_blatt(b, blatt('a')).ok());
    EXPECT_EQ(bl::emit_knoten_log(a), bl::emit_knoten_log(b));
}

TEST(Lb1KnotenLog, DerLogNameUndDerPfadHabenEineAbleitung) {
    EXPECT_EQ(bl::kKnotenLogName, "complete-heuristik.log");
    EXPECT_EQ(bl::knoten_log_pfad("a/b"), "a/b/complete-heuristik.log");
    EXPECT_EQ(bl::kKnotenLogTruncateSchwelleBytes, 4096u);
}

TEST(Lb1KnotenLogNegativ, KaputteGrammatikWirdNieStillGelesen) {
    EXPECT_FALSE(bl::parse_knoten_log("").has_value());
    EXPECT_FALSE(bl::parse_knoten_log("irgendwas\n").has_value());
    EXPECT_FALSE(bl::parse_knoten_log("complete-heuristik v1\nunbekannt=1\n").has_value())
        << "Ein unbekannter Schluessel ist ein Grammatik-Bruch, kein zu ignorierendes Feld";
    EXPECT_FALSE(bl::parse_knoten_log("complete-heuristik v1\nabgeschlossen=ja\n").has_value());
    // Ein Blatt-Key, der kein 128-hex ist, wuerde die ZAEHLUNG des Astes verfaelschen.
    EXPECT_FALSE(bl::parse_knoten_log("complete-heuristik v1\nblatt=kurz;genus=binary;done_utc=x\n").has_value());
    EXPECT_FALSE(
        bl::parse_knoten_log("complete-heuristik v1\nblatt=" + hex128('a') + ";genus=fremd;done_utc=x\n").has_value());
}

TEST(Lb1KnotenLogNegativ, HoehereFormatVersionWirdAbgelehnt) {
    auto const fremd = bl::parse_knoten_log("complete-heuristik v2\nknoten=x\n");
    ASSERT_TRUE(fremd.has_value()) << "v2 ist grammatikalisch lesbar ...";
    EXPECT_FALSE(bl::knoten_log_format_supported(*fremd)) << "... aber NICHT vertraeglich";
    FakeAblage f;
    f.dateien[bl::knoten_log_pfad(std::string{kKnoten})] = "complete-heuristik v2\nknoten=x\n";
    auto const ladung                                    = bl::lade_knoten_log(f.naht(), std::string{kKnoten});
    EXPECT_EQ(ladung.fehler, bl::KnotenLogFehler::format_version_fremd);
}

TEST(Lb1KnotenLogNegativ, BlattKeyFormWirdBeimEintragenGeprueft) {
    auto       l = frisches_log();
    auto const e = bl::beobachte_blatt(l, bl::KnotenBlatt{"zu_kurz", bl::Genus::binary, "x"});
    EXPECT_EQ(e.fehler, bl::KnotenLogFehler::blatt_key_form);
    EXPECT_EQ(l.binaries, 0u) << "Ein abgewiesenes Blatt darf die Zaehlung nicht anfassen";
    EXPECT_TRUE(l.blaetter.empty());
}

// ===========================================================================
// (2) INLINE-UPDATE + Ast-Heuristik (ABNAHME-1/2).
// ===========================================================================
TEST(Lb1AstHeuristik, InlineUpdateIstIdempotent) {
    auto l = frisches_log();
    EXPECT_TRUE(bl::beobachte_blatt(l, blatt('a')).neu);
    EXPECT_FALSE(bl::beobachte_blatt(l, blatt('a')).neu) << "Dasselbe (key,genus) zaehlt genau einmal";
    EXPECT_EQ(l.binaries, 1u);
    EXPECT_EQ(l.blaetter.size(), 1u);
    // DERSELBE Key in einem ANDEREN Genus ist ein anderes Blatt (Binary vs. Messwert).
    EXPECT_TRUE(bl::beobachte_blatt(l, blatt('a', bl::Genus::measurement)).neu);
    EXPECT_EQ(l.binaries, 1u);
    EXPECT_EQ(l.messwerte, 1u);
}

TEST(Lb1AstHeuristik, ZaehlungIstDieBinaereGrundlageDerVollstaendigkeit) {
    auto l = frisches_log(); // erwartet = 3
    EXPECT_FALSE(l.ast_vollstaendig());
    EXPECT_EQ(l.rest_offen(), 3u);
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('a')).ok());
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('b')).ok());
    EXPECT_EQ(l.rest_offen(), 1u);
    EXPECT_FALSE(l.ast_vollstaendig()) << "Ohne abgeschlossen=1 gilt ein Ast nie als vollstaendig";
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('c')).ok());
    l.abgeschlossen = true;
    EXPECT_TRUE(l.ast_vollstaendig());
    EXPECT_EQ(l.rest_offen(), 0u);
    // erwartet==0 == unbekannt -> konservativ NICHT vollstaendig (lieber einmal zu viel absteigen).
    l.erwartet = 0;
    EXPECT_FALSE(l.ast_vollstaendig());
}

TEST(Lb1AstHeuristik, FehlendeBlaetterWerdenDepthFirstErmittelt) {
    auto l = frisches_log();
    ASSERT_TRUE(bl::beobachte_blatt(l, blatt('b')).ok());
    std::vector<bl::KnotenBlatt> const erwartet{blatt('a'), blatt('b'), blatt('c')};
    auto const                         fehlt = bl::fehlende_blaetter(l, erwartet);
    ASSERT_EQ(fehlt.size(), 2u);
    EXPECT_EQ(fehlt[0].key_sha512, hex128('a'));
    EXPECT_EQ(fehlt[1].key_sha512, hex128('c')) << "Reihenfolge == die der Erwartungs-Liste (deterministisch)";
    EXPECT_TRUE(bl::fehlende_blaetter(l, {}).empty());
}

TEST(Lb1AstHeuristik, ErstLaufLiefertEinFrischesLogOhneFehler) {
    FakeAblage f;
    auto const ladung = bl::lade_knoten_log(f.naht(), std::string{kKnoten});
    EXPECT_TRUE(ladung.ok());
    EXPECT_TRUE(ladung.frisch);
    EXPECT_EQ(ladung.log.knoten, kKnoten);
    EXPECT_EQ(ladung.log.binaries + ladung.log.messwerte, 0u);
    EXPECT_EQ(bl::lade_knoten_log(f.naht(), "").fehler, bl::KnotenLogFehler::knoten_leer);
}

// ===========================================================================
// (3) ALLEINSCHREIBER-LOCK -- Wiederverwendung der bestandslog_lock-Mechanik.
// ===========================================================================
TEST(Lb1KnotenLock, GenauEinSchreiberBekommtDenKnoten) {
    FakeAblage f;
    SkriptUhr  uhr;
    auto const ablage = f.naht();

    bool        a_lief = false;
    bool        b_lief = false;
    std::string a_knoten;
    // A haelt den Lock und versucht WAEHRENDDESSEN, B denselben Knoten nehmen zu lassen.
    auto const o_a = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            a_lief         = true;
            a_knoten       = n.knoten();
            auto const o_b = bl::mit_knoten_lock(ablage, std::string{kKnoten}, owner("uuid-b"), uhr.fn(),
                                                 [&](bl::AlleinschreiberNachweis const&) {
                                                     b_lief = true;
                                                     return true;
                                                 });
            EXPECT_EQ(o_b, bl::LockOutcome::lock_unavailable);
            return true;
        });
    EXPECT_EQ(o_a, bl::LockOutcome::ok);
    EXPECT_TRUE(a_lief);
    EXPECT_FALSE(b_lief) << "Ein zweiter Schreiber betritt den Knoten NICHT";
    EXPECT_EQ(a_knoten, kKnoten);
    // Nach dem Zyklus ist der Lock wieder frei (RAII-Freigabe der bestehenden Mechanik).
    EXPECT_EQ(f.dateien.count(bl::lock_key_for(bl::knoten_log_pfad(std::string{kKnoten}))), 0u);
}

TEST(Lb1KnotenLock, VerschiedeneKnotenKollidierenNie) {
    // ABNAHME-2: Multithreading entsteht durch BRANCHING auf der System-Achse, nicht durch feineres
    // Locking -- zwei System-Zellen beruehren nie denselben Knoten.
    FakeAblage f;
    SkriptUhr  uhr;
    auto const ablage = f.naht();
    bool       innen  = false;
    auto const o      = bl::mit_knoten_lock(
        ablage, "target_isa=amd64_v3/x", owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const&) {
            auto const o2 = bl::mit_knoten_lock(ablage, "target_isa=arm64/x", owner("uuid-b"), uhr.fn(),
                                                [&](bl::AlleinschreiberNachweis const&) {
                                                    innen = true;
                                                    return true;
                                                });
            EXPECT_EQ(o2, bl::LockOutcome::ok);
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_TRUE(innen);
}

TEST(Lb1KnotenLock, SektionsDauerBleibtUnterDemBudget) {
    // Beweis-4-Vorgriff (Muster test_g3_bestandslog_lock): die Frist-Wache benutzt kSectionBudgetSeconds.
    FakeAblage f;
    SkriptUhr  uhr;
    auto const ablage  = f.naht();
    auto const vorher  = uhr.jetzt;
    auto const o       = bl::mit_knoten_lock(ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(),
                                             [&](bl::AlleinschreiberNachweis const&) { return true; });
    auto const nachher = uhr.jetzt;
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_LT(nachher - vorher, static_cast<std::int64_t>(bl::kSectionBudgetSeconds))
        << "literal: Sektions-Dauer " << (nachher - vorher) << "s < budget=" << bl::kSectionBudgetSeconds << "s";
    EXPECT_EQ(bl::kSectionBudgetSeconds, 30) << "Die EINE Budget-Zahl (bestandslog_lock.hpp), nicht nachgebaut";
    EXPECT_EQ(bl::kLockTtlSeconds, 1800) << "Die EINE ttl-Obergrenze (30 min), nicht nachgebaut";
}

TEST(Lb1KnotenLock, OhneOwnerTokenWirdNichtGeschrieben) {
    FakeAblage f;
    SkriptUhr  uhr;
    bool       lief = false;
    auto const o    = bl::mit_knoten_lock(f.naht(), std::string{kKnoten}, bl::LockOwner{"", "prod1", 1}, uhr.fn(),
                                          [&](bl::AlleinschreiberNachweis const&) {
                                           lief = true;
                                           return true;
                                          });
    EXPECT_EQ(o, bl::LockOutcome::lock_unavailable);
    EXPECT_FALSE(lief) << "fail-closed: ohne Owner-Token gibt es keinen Alleinschreiber";
}

// ===========================================================================
// (4) DIE TRUNCATE-ZUSTANDSMASCHINE -- die EIGENE Test-Klasse (Owner-Auflage A-1).
//
// Der Owner hat den Truncate als BUG-QUELLE benannt. Deshalb steht hier jede Zusage einzeln:
// wer darf truncaten, wann, mit welchem Nachweis, und was bleibt nach einem Abriss uebrig.
// ===========================================================================
class TruncateZustandsmaschine : public ::testing::Test {
protected:
    FakeAblage f;
    SkriptUhr  uhr;
    // grosses_log() stand bis 2026-08-06 HIER als protected static. Seit LB-6 braucht ihn auch der
    // Nebenlaeufigkeits-Abschnitt (7), der KEIN Fixture-Erbe ist -- er ist deshalb in den anonymen
    // Namensraum oben gewandert statt ein zweites Mal hingeschrieben zu werden. Die Aufrufe in den
    // TEST_F-Rumpfen darunter sind unveraendert (unqualifizierte Namenssuche findet ihn dort).
};

TEST_F(TruncateZustandsmaschine, BuildEndeIstDerRegelfallUndErhaeltDieZaehlung) {
    auto const ablage = f.naht();
    auto const o      = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLogOffen offen{grosses_log()};
            auto const         vorher_binaries = offen.log().binaries;
            auto               schluss         = std::move(offen).schluss_build_ende(n);
            if (!schluss) return false;
            EXPECT_EQ(schluss->grund(), "build_ende");
            auto const erg = schluss->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z");
            EXPECT_TRUE(erg.ok()) << bl::to_string(erg.fehler);
            EXPECT_EQ(erg.blaetter_fort, 40u);
            EXPECT_EQ(erg.binaries, vorher_binaries) << "Die ZAEHLUNG (binaere Grundlage) ueberlebt den Truncate";
            EXPECT_LT(erg.nachher_bytes, erg.vorher_bytes);
            EXPECT_TRUE(schluss->log().abgeschlossen);
            EXPECT_FALSE(schluss->log().inventur_laeuft);
            EXPECT_TRUE(schluss->log().blaetter.empty());
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    // Auf der Platte steht die inventarisierte Form.
    auto const gelesen = bl::parse_knoten_log(f.dateien.at(bl::knoten_log_pfad(std::string{kKnoten})));
    ASSERT_TRUE(gelesen.has_value());
    EXPECT_TRUE(gelesen->abgeschlossen);
    EXPECT_EQ(gelesen->binaries, 40u);
    EXPECT_TRUE(gelesen->blaetter.empty());
}

TEST_F(TruncateZustandsmaschine, AdHocTruncateNurUeberVierKilobyte) {
    auto const ablage = f.naht();
    // (a) KLEINES Log -> der Schluss-Zustand wird gar nicht erst betreten.
    auto const o1 = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLogOffen offen{frisches_log()};
            EXPECT_FALSE(offen.schwelle_erreicht());
            EXPECT_LE(offen.bytes(), bl::kKnotenLogTruncateSchwelleBytes);
            auto schluss = std::move(offen).schluss_ad_hoc(n);
            EXPECT_FALSE(schluss.has_value()) << "4 KB sind eine BEDINGUNG, keine Empfehlung";
            return true;
        });
    EXPECT_EQ(o1, bl::LockOutcome::ok);
    // (b) GROSSES Log -> zulaessig.
    auto const o2 = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLogOffen offen{grosses_log()};
            EXPECT_TRUE(offen.schwelle_erreicht());
            EXPECT_GT(offen.bytes(), bl::kKnotenLogTruncateSchwelleBytes);
            auto schluss = std::move(offen).schluss_ad_hoc(n);
            if (!schluss) return false;
            EXPECT_EQ(schluss->grund(), "groesse_ueber_4kb");
            auto const erg = schluss->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z");
            EXPECT_TRUE(erg.ok());
            EXPECT_LE(erg.nachher_bytes, bl::kKnotenLogTruncateSchwelleBytes);
            return true;
        });
    EXPECT_EQ(o2, bl::LockOutcome::ok);
}

TEST_F(TruncateZustandsmaschine, EinNachweisFuerEinenFREMDENKnotenIstKeinNachweis) {
    auto const ablage = f.naht();
    auto const o      = bl::mit_knoten_lock(
        ablage, "ein/ganz/anderer/knoten", owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLogOffen offen{grosses_log()}; // Log gehoert kKnoten, der Lock einem anderen Knoten
            EXPECT_EQ(offen.schreibe(ablage, n), bl::KnotenLogFehler::kein_alleinschreiber_lock);
            auto schluss = std::move(offen).schluss_build_ende(n);
            EXPECT_FALSE(schluss.has_value()) << "Sonst genuegte irgendein Lock, um irgendwo zu truncaten";
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_EQ(f.dateien.count(bl::knoten_log_pfad(std::string{kKnoten})), 0u);
}

TEST_F(TruncateZustandsmaschine, ZweiSchreiberKonkurrenzGenauEinerTruncatetDieZaehlungBleibtKonsistent) {
    auto const ablage    = f.naht();
    int        truncates = 0;
    auto const o         = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            // Der ZWEITE Schreiber versucht es waehrenddessen -- er bekommt den Knoten nicht und
            // kommt damit nie in die Naehe eines Nachweises.
            auto const o_b = bl::mit_knoten_lock(
                ablage, std::string{kKnoten}, owner("uuid-b"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n2) {
                    bl::KnotenLogOffen zweit{grosses_log()};
                    auto               s2 = std::move(zweit).schluss_build_ende(n2);
                    if (s2 && s2->truncate_und_inventarisiere(ablage, n2, "2026-08-03T13:00:00Z").ok()) ++truncates;
                    return true;
                });
            EXPECT_EQ(o_b, bl::LockOutcome::lock_unavailable);
            bl::KnotenLogOffen offen{grosses_log()};
            auto               s = std::move(offen).schluss_build_ende(n);
            if (!s) return false;
            if (s->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z").ok()) ++truncates;
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_EQ(truncates, 1) << "GENAU EINER truncatet";
    auto const gelesen = bl::parse_knoten_log(f.dateien.at(bl::knoten_log_pfad(std::string{kKnoten})));
    ASSERT_TRUE(gelesen.has_value());
    EXPECT_EQ(gelesen->binaries, 40u) << "Die Zaehlung bleibt konsistent (kein Doppel-Zaehlen)";
}

TEST_F(TruncateZustandsmaschine, CrashMittenInDerInventurHinterlaesstDieMarkeUndHeiltNurPerNeuAufnahme) {
    auto const ablage = f.naht();
    // Der Abriss: SCHRITT 1 (Crash-Marke) gelingt, SCHRITT 2 (die inventarisierte Form) nicht.
    // Der Lock-Zyklus schreibt vorher sein Lock-Objekt -> der 2. Schreibvorgang ist Schritt 1.
    auto const vor_crash = f.write_rufe;
    (void)vor_crash;
    f.bricht_ab_schreibvorgang = 3;
    auto const o               = bl::mit_knoten_lock(
        ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLogOffen offen{grosses_log()};
            auto               s = std::move(offen).schluss_build_ende(n);
            if (!s) return false;
            auto const erg = s->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z");
            EXPECT_EQ(erg.fehler, bl::KnotenLogFehler::ablage_schreiben);
            return false;
        });
    EXPECT_EQ(o, bl::LockOutcome::work_failed);
    f.bricht_ab_schreibvorgang = 0;

    // Auf der Platte steht die CRASH-MARKE -- und das Laden meldet sie als unvollstaendig.
    auto const ladung = bl::lade_knoten_log(ablage, std::string{kKnoten});
    EXPECT_EQ(ladung.fehler, bl::KnotenLogFehler::unvollstaendig);
    EXPECT_TRUE(ladung.log.inventur_laeuft);
    EXPECT_FALSE(ladung.log.abgeschlossen);

    // A-2: geheilt wird NUR per Ast-NEU-Inventarisierung -- und die liefert exakt denselben Endstand
    // wie ein ununterbrochener Lauf.
    std::vector<bl::KnotenBlatt> const ast_ist = grosses_log().blaetter;
    auto const neu = bl::inventarisiere_ast_neu(std::string{kKnoten}, ast_ist, 3, "2026-08-03T12:00:00Z");
    EXPECT_FALSE(neu.inventur_laeuft) << "Die Neuaufnahme LOESCHT die Marke";
    EXPECT_EQ(neu.binaries, grosses_log().binaries);
    EXPECT_EQ(bl::emit_knoten_log(neu), bl::emit_knoten_log(grosses_log()))
        << "Ast-Neu-Inventarisierung == identischer Endstand (byte-genau)";
}

TEST_F(TruncateZustandsmaschine, NormalerBauStandIstKEINAbriss) {
    // Die Gegenprobe zur Crash-Marke: abgeschlossen=0 MIT Blaettern ist der voellig normale Zustand
    // eines Astes, an dem gerade gebaut wird -- ihn als Abriss zu melden waere ein Fehlalarm, der
    // jeden Lauf in eine unnoetige Neu-Inventarisierung triebe.
    auto const ablage = f.naht();
    auto const o      = bl::mit_knoten_lock(ablage, std::string{kKnoten}, owner("uuid-a"), uhr.fn(),
                                            [&](bl::AlleinschreiberNachweis const& n) {
                                           bl::KnotenLogOffen offen{grosses_log()};
                                           EXPECT_EQ(offen.schreibe(ablage, n), bl::KnotenLogFehler::ok);
                                           return true;
                                            });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    auto const ladung = bl::lade_knoten_log(ablage, std::string{kKnoten});
    EXPECT_TRUE(ladung.ok()) << bl::to_string(ladung.fehler);
    EXPECT_FALSE(ladung.log.abgeschlossen);
    EXPECT_EQ(ladung.log.blaetter.size(), 40u);
}

TEST_F(TruncateZustandsmaschine, DieZweiSchlussGruendeSindTypenKeinFlag) {
    // State-Pattern (GoF) mit TYP-Zustaenden: die Gruende sind verschiedene TYPEN, kein enum-Feld --
    // ein dritter Grund kann nicht versehentlich entstehen, er muss als Typ eingefuehrt werden.
    static_assert(
        !std::is_same_v<bl::KnotenLogSchluss<bl::BuildEnde>, bl::KnotenLogSchluss<bl::GroesseUeberschritten>>);
    static_assert(bl::SchlussGrund<bl::BuildEnde>);
    static_assert(bl::SchlussGrund<bl::GroesseUeberschritten>);
    EXPECT_EQ(bl::BuildEnde::name(), "build_ende");
    EXPECT_EQ(bl::GroesseUeberschritten::name(), "groesse_ueber_4kb");
    // Und: der offene Zustand hat die Truncate-Methode GAR NICHT (compile-hart, hier als Nachweis,
    // dass es nicht bloss eine Laufzeit-Bitte ist).
    static_assert(!KannTruncaten<bl::KnotenLogOffen>,
                  "Im offenen Zustand darf ein Truncate nicht einmal AUSDRUECKBAR sein (State-Pattern mit "
                  "Typ-Zustaenden, A-1).");
    static_assert(KannTruncaten<bl::KnotenLogSchluss<bl::BuildEnde>>);
    static_assert(KannTruncaten<bl::KnotenLogSchluss<bl::GroesseUeberschritten>>);
}

// ===========================================================================
// (5) OE-B-DUMMY-LAGER auf echtem Dateisystem (der Knoten liegt im S2-Baum).
// ===========================================================================
TEST(Lb1DummyLager, KnotenLogLebtImRealenBaumNebenDenBlaettern) {
    TempKnoten        temp;
    auto const        ablage = bl::make_filesystem_ablage();
    SkriptUhr         uhr;
    std::string const knoten = temp.pfad() + "/" + std::string{kKnoten};
    ASSERT_TRUE(ablage.verzeichnis_anlegen(knoten));
    // "Binary" = Textdatei mit Stempel-String (OE-B) -- das Log liegt daneben. Der Stempel ist seit
    // LB-6 MEHRZEILIG (Form des echten g1_binary_version_block, support/oeb_stempel_zeilen.hpp);
    // die zeilenweise Ruecklese steht im Test darunter.
    ASSERT_TRUE(ablage.datei_schreiben(knoten + "/perm.dll", ct::oeb_stempel_block()));

    auto const o =
        bl::mit_knoten_lock(ablage, knoten, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLog l;
            l.knoten    = knoten;
            l.stand_utc = "2026-08-03T12:00:00Z";
            l.erwartet  = 1;
            bl::KnotenLogOffen offen{std::move(l)};
            EXPECT_TRUE(offen.beobachte(blatt('a')).neu);
            auto s = std::move(offen).schluss_build_ende(n);
            if (!s) return false;
            return s->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z").ok();
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_TRUE(fs::exists(fs::path{bl::knoten_log_pfad(knoten)}));
    EXPECT_FALSE(fs::exists(fs::path{bl::lock_key_for(bl::knoten_log_pfad(knoten))})) << "Lock ist freigegeben";
    auto const ladung = bl::lade_knoten_log(ablage, knoten);
    ASSERT_TRUE(ladung.ok()) << bl::to_string(ladung.fehler);
    EXPECT_TRUE(ladung.log.ast_vollstaendig());
    EXPECT_EQ(ladung.log.binaries, 1u);
}

TEST(Lb1DummyLager, StempelBlattWirdNebenDemKnotenLogZeilenweiseVerbatimZurueckgelesen) {
    // LB-6 / Owner-Verschaerfung 06.08.2026: "jede Zeile verbatim". Der Test darueber belegt, dass
    // das Knoten-Log NEBEN der Dummy-Binary lebt -- er sagt aber nichts darueber, ob die Binary
    // ihren Stempel unbeschadet behaelt, waehrend der gelockte Zyklus im selben Verzeichnis
    // schreibt und truncatet. Genau das prueft dieser Test, und zwar zeilenweise.
    TempKnoten        temp;
    auto const        ablage = bl::make_filesystem_ablage();
    SkriptUhr         uhr;
    std::string const knoten     = temp.pfad() + "/" + std::string{kKnoten};
    std::string const blatt_pfad = knoten + "/perm.dll";
    std::string const stempel    = ct::oeb_stempel_block();
    ASSERT_TRUE(ablage.verzeichnis_anlegen(knoten));
    ASSERT_TRUE(ablage.datei_schreiben(blatt_pfad, stempel));

    // Der volle gelockte Zyklus inklusive Truncate laeuft NEBEN dem Blatt.
    auto const o =
        bl::mit_knoten_lock(ablage, knoten, owner("uuid-a"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            bl::KnotenLog l;
            l.knoten    = knoten;
            l.stand_utc = "2026-08-03T12:00:00Z";
            l.erwartet  = 1;
            bl::KnotenLogOffen offen{std::move(l)};
            EXPECT_TRUE(offen.beobachte(blatt('a')).neu);
            auto s = std::move(offen).schluss_build_ende(n);
            if (!s) return false;
            return s->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z").ok();
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    ASSERT_TRUE(fs::exists(fs::path{bl::knoten_log_pfad(knoten)}));

    // RUECKLESE des Stempel-Blattes ueber std::ifstream -- ein ZWEITER Weg, nicht derselbe
    // BaumAblage-Baustein, mit dem geschrieben wurde (ein symmetrischer Fehler hoebe sich sonst auf).
    auto const roh = ct::lies_blatt_datei(fs::path{blatt_pfad});
    ASSERT_TRUE(roh.has_value()) << "Blatt nicht oeffenbar: " << blatt_pfad;
    EXPECT_EQ(*roh, stempel);

    // Zeilenzahl zuerst (sonst faellt eine verlorene Zeile nicht auf), dann jede Zeile einzeln.
    EXPECT_EQ(std::count(roh->begin(), roh->end(), '\n'), static_cast<std::ptrdiff_t>(ct::kOeBStempelZeilenZahl));
    auto const zeilen = ct::split_lines(*roh);
    ASSERT_EQ(zeilen.size(), ct::kOeBStempelZeilenZahl);
    EXPECT_EQ(zeilen[0], ct::kOeBStempelZeile0);
    EXPECT_EQ(zeilen[1], ct::kOeBStempelZeile1);
    EXPECT_EQ(zeilen[2], ct::kOeBStempelZeile2);
    EXPECT_EQ(zeilen[3], ct::kOeBStempelZeile3);
    for (std::size_t i = 0; i < zeilen.size(); ++i)
        EXPECT_TRUE(zeilen[i].starts_with(ct::kOeBStempelLabels[i])) << "Zeile " << i << " = '" << zeilen[i] << "'";

    // Und das Knoten-Log daneben ist ein EIGENES Blatt: sein Kopf ist nicht der Stempel.
    auto const log_roh = ct::lies_blatt_datei(fs::path{bl::knoten_log_pfad(knoten)});
    ASSERT_TRUE(log_roh.has_value());
    auto const log_zeilen = ct::split_lines(*log_roh);
    ASSERT_FALSE(log_zeilen.empty());
    EXPECT_EQ(log_zeilen[0], "complete-heuristik v1");
    EXPECT_NE(*log_roh, *roh) << "Stempel und Knoten-Log duerfen sich nie zu EINER Datei verschmelzen";
}

// ===========================================================================
// (6) SHA512-OVERLAY -- die DEKLARIERTE Luecke (L14-Muster), nicht die stille.
// ===========================================================================
TEST(Lb1OverlayAbgrenzung, DasOverlayGliedIstStrukturellDaUndHeuteLEER) {
    // OF-M3-2 = Fallback B: das Glied existiert seit A13-M3/C3, traegt aber bis zur Owner-Definition
    // nur seinen Separator. Diese Scheibe konsumiert den Fingerprint AS-IS und eicht NICHTS.
    EXPECT_TRUE(comdare::cache_engine::abi::kOverlaySourceHash.empty())
        << "Sobald der Overlay-Codegen das Define setzt, ist das Voll-Soll der ABNAHME-3/4 erreichbar "
           "-- bis dahin ist die Luecke DEKLARIERT, nicht still.";
    // O-2/C-2 (Format 2 -> 3): 6 -> 8 Glieder (Toolchain [5], bvset [6]); das Overlay-Glied ist ans ENDE
    // gewandert und heisst jetzt [7]. Die AUSSAGE dieses Tests ist unveraendert -- das Overlay-Glied ist
    // strukturell da und heute leer -- nur seine Position und die Glied-Zahl sind nachgezogen. Die
    // POSITION wird ab hier ueber die BENANNTE Konstante geprueft, nicht ueber eine nackte Zahl: eine
    // weitere Umsortierung soll diesen Test nicht erneut faelschlich rot faerben.
    EXPECT_EQ(comdare::cache_engine::abi::kAnatomyFingerprintGliedCount, 8u);
    EXPECT_EQ(comdare::cache_engine::abi::kAnatomyFingerprintOverlayGlied,
              comdare::cache_engine::abi::kAnatomyFingerprintGliedCount - 1u)
        << "das Overlay-Glied ist per O-2/C-2 das Schwanz-Glied";
    // Und der Anker des Knoten-Logs ist genau die Form dieses Fingerprints -- keine Zweitrechnung.
    EXPECT_TRUE(bl::ist_fingerprint_hex(hex128('a')));
    EXPECT_FALSE(bl::ist_fingerprint_hex(std::string(127, 'a')));
}

// ===========================================================================
// (7) ECHTE DATEISYSTEM-KONKURRENZ -- zwei bzw. vier OS-Threads auf EINEM Knoten (LB-6).
//
// BEFUND, DER DIESEN ABSCHNITT ERZWINGT: der Bestands-Konkurrenz-Test
// TruncateZustandsmaschine.ZweiSchreiberKonkurrenzGenauEinerTruncatet... laeuft auf FakeAblage
// (In-Memory) und ist SEQUENZIELL VERSCHACHTELT -- der "zweite Schreiber" ist ein Aufruf INNERHALB
// des ersten, auf demselben Thread. Er belegt die Zustandslogik, aber kein einziges Byte davon
// beruehrt je ein Dateisystem und kein einziger Befehl laeuft je gleichzeitig. LB-6 verlangt einen
// Spin-Lock-Konkurrenz-Test; hier fahren echte std::thread gegen make_filesystem_ablage().
//
// WAS HIER BEWUSST NICHT BEHAUPTET WIRD: wechselseitiger Ausschluss. bestandslog_lock.hpp sagt in
// seinem Kopf ausdruecklich, dass mc/S3 mit den Bordmitteln des Hauses KEIN CAS bieten und ein
// Rest-Race-Fenster bleibt (store -> Zweit-Verify ist kein atomarer Vergleich-und-Setze). Ein Test,
// der Mutual Exclusion behauptet, behauptete also etwas, das das Design gar nicht verspricht -- er
// waere ein Flake-Erzeuger, kein Beweis. Gemessen wird stattdessen die Zusage, die das Design
// WIRKLICH gibt: Kollisionen sind UNSCHAEDLICH (idempotente, byte-gleiche Schreibvorgaenge). Der
// beobachtete Ueberlappungsgrad wird als Test-Property protokolliert, nicht assertiert.
// ===========================================================================
namespace {

/// Ein Log fuer einen konkreten (Temp-)Knoten -- dieselben 40 Blaetter, nur mit anderem Knoten-Pfad.
[[nodiscard]] bl::KnotenLog grosses_log_fuer(std::string const& knoten) {
    bl::KnotenLog l = grosses_log();
    l.knoten        = knoten;
    return l;
}

/// Der Endstand, den ein VOLLZOGENER Truncate hinterlaesst: Blatt-Liste fort, Zaehlung erhalten,
/// abgeschlossen=1, Crash-Marke geloescht (knoten_heuristik_log.hpp, Schritt 2).
[[nodiscard]] std::string erwartete_inventarisierte_bytes(std::string const& knoten, std::string const& stand_utc) {
    bl::KnotenLog l   = grosses_log_fuer(knoten);
    l.blaetter        = {};
    l.stand_utc       = stand_utc;
    l.abgeschlossen   = true;
    l.inventur_laeuft = false;
    return bl::emit_knoten_log(l);
}

/// Ein Truncate-Zyklus im gelockten Abschnitt. Liefert true == Arbeit gelungen (Vertrag von fn).
bool truncate_im_abschnitt(bl::BaumAblage const& ablage, std::string const& knoten,
                           bl::AlleinschreiberNachweis const& n, std::atomic<int>& truncates) {
    bl::KnotenLogOffen offen{grosses_log_fuer(knoten)};
    auto               s = std::move(offen).schluss_build_ende(n);
    if (!s) return false;
    if (s->truncate_und_inventarisiere(ablage, n, "2026-08-03T13:00:00Z").ok())
        truncates.fetch_add(1, std::memory_order_relaxed);
    return true;
}

/// Warten mit FRIST: ein Test darf nie haengen. Liefert false, wenn die Frist verstrich.
[[nodiscard]] bool warte_auf(std::atomic<bool> const& flagge, std::chrono::seconds budget) {
    auto const frist = std::chrono::steady_clock::now() + budget;
    while (!flagge.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() > frist) return false;
        std::this_thread::yield();
    }
    return true;
}

} // namespace

TEST(Lb1DateisystemKonkurrenz, ZweiEchteThreadsGenauEinerTruncatetDieZaehlungBleibtKonsistent) {
    TempKnoten        temp;
    auto const        ablage = bl::make_filesystem_ablage();
    std::string const knoten = temp.pfad() + "/" + std::string{kKnoten};
    ASSERT_TRUE(ablage.verzeichnis_anlegen(knoten));

    // KEINE Skript-Uhr hier: zwei Threads teilten sich sonst ein und dasselbe beschreibbare int64
    // (SkriptUhr::jetzt) -- das waere ein Datenrennen im TEST selbst. system_now_epoch_s ist
    // zustandslos. Die Frist-Wache misst damit echte Sekunden gegen budget=30 (kSectionBudgetSeconds).
    bl::NowFn const uhr = [] { return bl::system_now_epoch_s(); };

    std::atomic<int>  truncates{0};
    std::atomic<bool> a_haelt{false};  // A ist IM Abschnitt
    std::atomic<bool> b_fertig{false}; // B hat seinen Versuch hinter sich
    std::atomic<bool> b_wartezeit_ok{true};
    auto              o_a = bl::LockOutcome::deadline_exceeded; // bewusst KEINE der Erwartungen
    auto              o_b = bl::LockOutcome::deadline_exceeded;

    std::thread ta([&] {
        o_a = bl::mit_knoten_lock(ablage, knoten, owner("uuid-a"), uhr, [&](bl::AlleinschreiberNachweis const& n) {
            a_haelt.store(true, std::memory_order_release);
            // A HAELT den Lock, bis B seinen Versuch wirklich hinter sich hat. Ohne dieses Warten
            // waere die Konkurrenz nur behauptet: B koennte vollstaendig NACH A's Freigabe laufen,
            // und der Test faerbte sich gruen, ohne je eine Ueberschneidung gesehen zu haben.
            if (!warte_auf(b_fertig, std::chrono::seconds{20})) b_wartezeit_ok.store(false);
            return truncate_im_abschnitt(ablage, knoten, n, truncates);
        });
    });
    std::thread tb([&] {
        if (!warte_auf(a_haelt, std::chrono::seconds{20})) b_wartezeit_ok.store(false);
        o_b = bl::mit_knoten_lock(ablage, knoten, owner("uuid-b"), uhr, [&](bl::AlleinschreiberNachweis const& n) {
            return truncate_im_abschnitt(ablage, knoten, n, truncates);
        });
        b_fertig.store(true, std::memory_order_release);
    });
    ta.join();
    tb.join();

    ASSERT_TRUE(b_wartezeit_ok.load()) << "Die Ueberschneidung kam nicht zustande -- ohne sie ist der Test wertlos";
    EXPECT_EQ(o_a, bl::LockOutcome::ok) << bl::to_string(o_a);
    EXPECT_EQ(o_b, bl::LockOutcome::lock_unavailable)
        << "Ein zweiter Schreiber betritt den Knoten NICHT, solange A ihn haelt -- gemeldet: " << bl::to_string(o_b);
    EXPECT_EQ(truncates.load(), 1) << "GENAU EINER truncatet";

    // Der Lock ist auf beiden Wegen freigegeben (RAII bzw. nie erworben).
    EXPECT_FALSE(fs::exists(fs::path{bl::lock_key_for(bl::knoten_log_pfad(knoten))}));
    // Und auf der Platte steht die inventarisierte Form -- byte-genau die eines ungestoerten Laufs.
    auto const roh = ct::lies_blatt_datei(fs::path{bl::knoten_log_pfad(knoten)});
    ASSERT_TRUE(roh.has_value());
    EXPECT_EQ(*roh, erwartete_inventarisierte_bytes(knoten, "2026-08-03T13:00:00Z"));
    auto const ladung = bl::lade_knoten_log(ablage, knoten);
    ASSERT_TRUE(ladung.ok()) << bl::to_string(ladung.fehler);
    EXPECT_TRUE(ladung.log.abgeschlossen);
    EXPECT_FALSE(ladung.log.inventur_laeuft) << "keine haengengebliebene Crash-Marke";
    EXPECT_EQ(ladung.log.binaries, 40u) << "Die Zaehlung bleibt konsistent (kein Doppel-Zaehlen)";
    EXPECT_TRUE(ladung.log.blaetter.empty());
}

TEST(Lb1DateisystemKonkurrenz, SpinKonkurrenzVierThreadsKonvergiertAufEinByteIdentischesLog) {
    TempKnoten        temp;
    auto const        ablage = bl::make_filesystem_ablage();
    std::string const knoten = temp.pfad() + "/" + std::string{kKnoten};
    ASSERT_TRUE(ablage.verzeichnis_anlegen(knoten));
    bl::NowFn const uhr = [] { return bl::system_now_epoch_s(); };

    constexpr int    kThreads = 4;
    std::atomic<int> gewonnen{0}, abgewiesen{0}, dritte_klasse{0}, truncates{0};
    std::atomic<int> drin{0}, drin_max{0};

    std::vector<std::thread> mannschaft;
    mannschaft.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        mannschaft.emplace_back([&, i] {
            auto const frist = std::chrono::steady_clock::now() + std::chrono::seconds{20};
            for (;;) {
                auto const o = bl::mit_knoten_lock(
                    ablage, knoten, owner("uuid-" + std::to_string(i)), uhr, [&](bl::AlleinschreiberNachweis const& n) {
                        // NUR BEOBACHTUNG, keine Zusicherung (s. Abschnitts-Kopf): wie viele Threads
                        // waren zugleich im Abschnitt? Das Rest-Race-Fenster des Locks ist bekannt
                        // und dokumentiert; hier wird es gemessen statt weggeschaut.
                        int const jetzt = drin.fetch_add(1, std::memory_order_acq_rel) + 1;
                        for (int alt = drin_max.load(std::memory_order_relaxed); jetzt > alt;)
                            if (drin_max.compare_exchange_weak(alt, jetzt, std::memory_order_relaxed)) break;
                        bool const erg = truncate_im_abschnitt(ablage, knoten, n, truncates);
                        drin.fetch_sub(1, std::memory_order_acq_rel);
                        return erg;
                    });
                if (o == bl::LockOutcome::ok) {
                    gewonnen.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
                if (o == bl::LockOutcome::lock_unavailable) {
                    abgewiesen.fetch_add(1, std::memory_order_relaxed);
                } else {
                    dritte_klasse.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
                if (std::chrono::steady_clock::now() > frist) break; // Frist: ein Test haengt nie
                // Der Wiederhol-Jitter gehoert laut try_acquire_lock-Kopf dem AUFRUFER -- hier ist
                // der Aufrufer dieser Thread.
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        });
    }
    for (auto& t : mannschaft) t.join();

    RecordProperty("beobachtete_maximale_ueberlappung", drin_max.load());
    RecordProperty("abgewiesene_versuche", abgewiesen.load());

    EXPECT_EQ(gewonnen.load(), kThreads) << "jeder Spin-Schreiber bekommt den Knoten irgendwann";
    EXPECT_EQ(dritte_klasse.load(), 0) << "kein Zyklus endet in work_failed oder deadline_exceeded";
    EXPECT_EQ(truncates.load(), kThreads) << "jeder gewonnene Abschnitt vollzieht seinen Truncate";
    EXPECT_GE(drin_max.load(), 1);

    // DIE ZUSAGE, DIE DAS DESIGN GIBT: der Endstand ist BYTE-IDENTISCH zu dem eines einzelnen,
    // ungestoerten Laufs. Das ist Punkt (b) des bestandslog_lock-Kopfes -- Kollisionen kosten
    // doppelte Arbeit, nie Datenverlust. Der jeweils LETZTE Schreibvorgang eines Threads ist die
    // inventarisierte Form (Schritt 2 folgt immer auf dessen Schritt 1), und alle vier Threads
    // schreiben dieselben Bytes; ein haengengebliebener Zwischenstand ist damit ausgeschlossen.
    EXPECT_FALSE(fs::exists(fs::path{bl::lock_key_for(bl::knoten_log_pfad(knoten))})) << "kein Lock-Leichnam";
    auto const roh = ct::lies_blatt_datei(fs::path{bl::knoten_log_pfad(knoten)});
    ASSERT_TRUE(roh.has_value());
    EXPECT_EQ(*roh, erwartete_inventarisierte_bytes(knoten, "2026-08-03T13:00:00Z"));
    auto const ladung = bl::lade_knoten_log(ablage, knoten);
    ASSERT_TRUE(ladung.ok()) << bl::to_string(ladung.fehler);
    EXPECT_FALSE(ladung.log.inventur_laeuft) << "keine haengengebliebene Crash-Marke nach vier Konkurrenten";
    EXPECT_TRUE(ladung.log.abgeschlossen);
    EXPECT_EQ(ladung.log.binaries, 40u);
}
