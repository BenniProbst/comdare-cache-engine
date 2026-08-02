// test_g3_reservation_lifecycle -- G3 / #46b Lagerhaltung, Scheibe B4 (Lebenszyklus).
//
// pro-forma-Registrierung -> ETA-Kalibrierung -> Done | Released, +50%-Takeover-Praedikat,
// PromiseGuard (Release bei Abbruch, commit() bei Fertigstellung). Literal, zeit-frei parametrisiert.

#include "bestandslog/reservation_lifecycle.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace bl = comdare::cache_engine::builder::bestandslog;

// ---------------------------------------------------------------------------
// pro-forma-Registrierung + Frist.
// ---------------------------------------------------------------------------
TEST(G3ReservationLifecycle, MakeProForma) {
    auto r = bl::make_pro_forma_reservation("uuid-A/0", bl::BatchTyp::tier, "prod1", 32, 0, 4096,
                                            "2026-07-23T12:00:00Z", "2026-07-23T12:30:00Z");
    EXPECT_EQ(r.id, "uuid-A/0");
    EXPECT_EQ(r.typ, bl::BatchTyp::tier);
    EXPECT_EQ(r.threads, 32u);
    EXPECT_EQ(r.slice_count, 4096u);
    EXPECT_EQ(r.status, bl::BatchStatus::offen);
    EXPECT_TRUE(r.eta_s.empty()); // pro-forma: noch keine ETA
    EXPECT_TRUE(r.avg_size_bytes.empty());
    EXPECT_EQ(r.pro_forma_bis_utc, "2026-07-23T12:30:00Z");
}

TEST(G3ReservationLifecycle, ProFormaDeadline) {
    EXPECT_EQ(bl::pro_forma_deadline_epoch_s(1000, 30), 2800); // 1000 + 30*60
    EXPECT_EQ(bl::pro_forma_deadline_epoch_s(1000), 2800);     // Default 30min
    EXPECT_TRUE(bl::is_pro_forma_expired(2800, 2801));
    EXPECT_FALSE(bl::is_pro_forma_expired(2800, 2800));
    EXPECT_FALSE(bl::is_pro_forma_expired(2800, 2799));
}

// ---------------------------------------------------------------------------
// ETA-Kalibrierung + Zustandsuebergaenge.
// ---------------------------------------------------------------------------
TEST(G3ReservationLifecycle, ApplyCalibrationFillsEtaAndAvg) {
    auto r = bl::make_pro_forma_reservation("uuid-A/0", bl::BatchTyp::tier, "prod1", 32, 0, 4096,
                                            "2026-07-23T12:00:00Z", "2026-07-23T12:30:00Z");
    bl::apply_calibration(r, bl::EtaResult{912.5, 428032});
    EXPECT_EQ(r.eta_s, "912.500");
    EXPECT_EQ(r.avg_size_bytes, "428032");
    ASSERT_TRUE(bl::parse_seconds(r.eta_s).has_value());
    EXPECT_DOUBLE_EQ(*bl::parse_seconds(r.eta_s), 912.5);
}

TEST(G3ReservationLifecycle, FormatParseSecondsRoundtrip) {
    EXPECT_EQ(bl::format_seconds(256.0), "256.000");
    ASSERT_TRUE(bl::parse_seconds("256.000").has_value());
    EXPECT_DOUBLE_EQ(*bl::parse_seconds("256.000"), 256.0);
    // TP1FK1-B3: LEER ist keine 0.0 mehr, sondern "nicht kalibriert" -- der Unterschied entscheidet
    // im Takeover, ob der ETA- oder der pro-forma-Zweig urteilt.
    EXPECT_FALSE(bl::parse_seconds("").has_value());
}

// TP1FK1-B3 (NEGATIVTEST, Codex-Befund): parse_seconds prueft Fehlercode UND Endzeiger. Vor der
// Haertung lieferte "1junk" die 1.0 -- eine erfundene ETA, aus der das Takeover-Praedikat nach
// 1,5 Sekunden "Maschine gestorben" ableitete.
TEST(G3ReservationLifecycle, B3ParseSecondsIstFailClosed) {
    EXPECT_FALSE(bl::parse_seconds("1junk").has_value()) << "Nachgestellter Muell -> nullopt, nie 1.0.";
    EXPECT_FALSE(bl::parse_seconds("12.5s").has_value());
    EXPECT_FALSE(bl::parse_seconds("murks").has_value());
    EXPECT_FALSE(bl::parse_seconds(" 12.5").has_value()) << "Fuehrender Whitespace ist nicht unsere Wire-Form.";
    EXPECT_FALSE(bl::parse_seconds("12.5 ").has_value());
    // Die von format_seconds erzeugte Form bleibt gueltig (der Roundtrip ist der Vertrag).
    ASSERT_TRUE(bl::parse_seconds(bl::format_seconds(0.5)).has_value());
    EXPECT_DOUBLE_EQ(*bl::parse_seconds(bl::format_seconds(0.5)), 0.5);
    // has_usable_eta = strikt parsbar UND positiv (die EINE Auslegung beider Konsumenten).
    EXPECT_TRUE(bl::has_usable_eta("100.000"));
    EXPECT_FALSE(bl::has_usable_eta("1junk"));
    EXPECT_FALSE(bl::has_usable_eta("0.000"));
    EXPECT_FALSE(bl::has_usable_eta(""));
}

// TP1FK1-B3: die Wirkung am Praedikat -- ein Record mit "1junk" faellt auf den pro-forma-Zweig
// (und ist VOR der Frist damit nicht uebernehmbar), statt nach 1,5s als "tot" zu gelten.
TEST(G3ReservationLifecycle, B3KaputteEtaFaelltAufProFormaZweig) {
    bl::BatchReservierung r;
    r.status = bl::BatchStatus::offen;
    r.eta_s  = "1junk"; // wuerde vor dem Fix als ETA 1s gelesen
    EXPECT_FALSE(bl::is_reservation_takeable(r, 1000, 2800, 1002)) << "1,5s nach dem Update ist die Maschine LEBEND.";
    EXPECT_TRUE(bl::is_reservation_takeable(r, 1000, 2800, 2801)) << "Erst die pro-forma-Frist gibt sie frei.";
}

// TP1FK1-B3 (NEGATIVTEST, CX-W8): std::from_chars deutet "inf"/"infinity" fuer double VOLLSTAENDIG
// (Endzeiger am Ende, ec == errc{}). VOR der isfinite-Wache lieferte has_usable_eta("inf") == true,
// die Reservierung galt als kalibriert, und is_takeable_by_eta rechnete "elapsed > 1.5 * inf" ==
// IMMER false: der Claim war NIE uebernehmbar (permanente Verklemmung). Mit der Wache faellt "inf"
// auf den pro-forma-Zweig -- exakt wie "nan", das schon immer korrekt fiel (nan > 0.0 == false).
TEST(G3ReservationLifecycle, B3InfIstKeineBrauchbareEta) {
    // (1) parse_seconds deutet inf/infinity WEITERHIN (das ist die strtod-Wahrheit) ...
    EXPECT_TRUE(bl::parse_seconds("inf").has_value());
    EXPECT_TRUE(bl::parse_seconds("infinity").has_value());
    // (2) ... aber has_usable_eta verwirft sie: nicht endlich == keine Kalibrierung.
    EXPECT_FALSE(bl::has_usable_eta("inf")) << "inf war vor dem Fix >0 und damit faelschlich 'usable'.";
    EXPECT_FALSE(bl::has_usable_eta("infinity"));
    EXPECT_FALSE(bl::has_usable_eta("nan")) << "nan war schon immer korrekt (nan > 0.0 == false).";
    EXPECT_TRUE(bl::has_usable_eta("100.000")) << "eine echte, endliche ETA bleibt brauchbar.";
    // (3) die Wirkung am Praedikat: ein "inf"-Record faellt auf pro-forma -> nach der Frist uebernehmbar
    //     (statt nie). VOR dem Fix waehlte er den ETA-Zweig und war fuer JEDES now nicht uebernehmbar.
    bl::BatchReservierung r;
    r.status = bl::BatchStatus::offen;
    r.eta_s  = "inf";
    EXPECT_TRUE(bl::is_reservation_takeable(r, 1000, 2800, 2801))
        << "Nach der pro-forma-Frist ist der inf-Claim uebernehmbar -- keine permanente Verklemmung.";
    EXPECT_FALSE(bl::is_reservation_takeable(r, 1000, 2800, 2500)) << "Vor der Frist bleibt er stehen (konservativ).";
}

TEST(G3ReservationLifecycle, MarkDoneAndReleased) {
    bl::BatchReservierung r;
    r.status = bl::BatchStatus::offen;
    bl::mark_done(r);
    EXPECT_EQ(r.status, bl::BatchStatus::done);
    bl::mark_released(r);
    EXPECT_EQ(r.status, bl::BatchStatus::released);
}

// ---------------------------------------------------------------------------
// +50%-Takeover-Praedikat.
// ---------------------------------------------------------------------------
TEST(G3ReservationLifecycle, TakeoverByEtaFiftyPercent) {
    // eta=100, letztes Update bei 1000. Uebernehmbar erst wenn elapsed > 1.5*100 = 150.
    EXPECT_FALSE(bl::is_takeable_by_eta(100.0, 1000, 1150)); // exakt 150 -> noch nicht
    EXPECT_TRUE(bl::is_takeable_by_eta(100.0, 1000, 1151));  // 151 > 150 -> frei
    EXPECT_FALSE(bl::is_takeable_by_eta(100.0, 1000, 1000)); // frisch
}

TEST(G3ReservationLifecycle, ReservationTakeablePredicate) {
    // done -> nie uebernehmbar.
    bl::BatchReservierung done_r;
    done_r.status = bl::BatchStatus::done;
    EXPECT_FALSE(bl::is_reservation_takeable(done_r, 1000, 2800, 999999));

    // offen ohne ETA -> pro-forma-Frist entscheidet.
    bl::BatchReservierung pf;
    pf.status = bl::BatchStatus::offen;                              // eta_s leer
    EXPECT_FALSE(bl::is_reservation_takeable(pf, 1000, 2800, 2500)); // vor Frist
    EXPECT_TRUE(bl::is_reservation_takeable(pf, 1000, 2800, 2801));  // nach Frist

    // offen mit ETA -> +50%-Praedikat entscheidet (last_update=1000, eta=100).
    bl::BatchReservierung cal;
    cal.status = bl::BatchStatus::offen;
    cal.eta_s  = "100.000";
    EXPECT_FALSE(bl::is_reservation_takeable(cal, 1000, 2800, 1150));
    EXPECT_TRUE(bl::is_reservation_takeable(cal, 1000, 2800, 1151));
}

// ---------------------------------------------------------------------------
// PromiseGuard: Release bei Abbruch, kein Release nach commit(), Move gibt das Versprechen weiter.
// ---------------------------------------------------------------------------
TEST(G3ReservationLifecycle, PromiseGuardReleasesOnAbort) {
    int calls = 0;
    {
        bl::PromiseGuard g([&] { ++calls; });
        // kein commit -> Abbruch
    }
    EXPECT_EQ(calls, 1);
}

TEST(G3ReservationLifecycle, PromiseGuardCommitSuppressesRelease) {
    int calls = 0;
    {
        bl::PromiseGuard g([&] { ++calls; });
        g.commit();
        EXPECT_TRUE(g.committed());
    }
    EXPECT_EQ(calls, 0);
}

TEST(G3ReservationLifecycle, PromiseGuardMoveFiresOnce) {
    int calls = 0;
    {
        bl::PromiseGuard g1([&] { ++calls; });
        bl::PromiseGuard g2(std::move(g1)); // g1 verschoben -> feuert nicht mehr
        // g2 nicht committed -> feuert genau einmal
    }
    EXPECT_EQ(calls, 1);
}
