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
    EXPECT_DOUBLE_EQ(bl::parse_seconds(r.eta_s), 912.5);
}

TEST(G3ReservationLifecycle, FormatParseSecondsRoundtrip) {
    EXPECT_EQ(bl::format_seconds(256.0), "256.000");
    EXPECT_DOUBLE_EQ(bl::parse_seconds("256.000"), 256.0);
    EXPECT_DOUBLE_EQ(bl::parse_seconds(""), 0.0);
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
