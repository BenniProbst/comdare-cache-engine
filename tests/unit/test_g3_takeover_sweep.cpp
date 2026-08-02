// test_g3_takeover_sweep -- Lager-Strecke Teilpaket 1 (A): der Takeover-Konsument (G-E1, ABNAHME-6).
//
// WAS HIER BEWIESEN WIRD: dass die bisher konsumentenlosen Takeover-Praedikate (is_reservation_
// takeable, reservation_lifecycle.hpp) jetzt PRODUKTIV urteilen -- beim Lauf-Start werden fremde
// offene Reservierungen geprueft, verfallene uebernommen (released) und die nicht als Bestand
// verzeichnete Arbeit landet ueber den normalen Registrierungs-Weg als Bestand (LEDGER:3291-Soll).
// Deterministisch, ohne minio: FakeStore + Skript-Uhr (Muster test_g3_builder_registration).
//
// Die Zeit-Anker-ERWARTUNGEN sind ABGELEITET, nie handkopiert (A9): jede Frist entsteht ueber
// make_slice_reservation/utc_iso_from_epoch aus einer Skript-Uhr, und die Roundtrip-Wahrheit
// epoch_from_utc_iso(utc_iso_from_epoch(t)) == t wird eigens belegt.

#include "bestandslog/builder_registration.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// In-Memory-Objekt-Store (Fake-Transport; Teilmenge des test_g3_builder_registration-Musters).
// fail_store_key laesst genau EINEN Schluessel scheitern -- der echte Schreibfehler-Pfad.
struct FakeStore {
    std::map<std::string, std::string> objs;
    std::string                        fail_store_key;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            if (!fail_store_key.empty() && k == fail_store_key) return false;
            objs[k] = c;
            return true;
        };
        t.remove = [this](std::string const& k) -> bool {
            objs.erase(k);
            return true;
        };
        t.stat = [this](std::string const& k) -> std::optional<bl::ObjectStat> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return bl::ObjectStat{.size = it->second.size(), .mtime_epoch_s = 0};
        };
        return t;
    }
};

constexpr char const* kDocKey = "bestandslog/binary_bestand.xml";

bl::LockOwner ich() { return bl::LockOwner{"uuid-prod1-42", "prod1", 4242}; }

bl::NowFn feste_uhr(std::int64_t ts) {
    return [ts]() { return ts; };
}

// cerr-Fang fuer pruefbare Testat-Zeilen (Muster test_g3_builder_registration).
class CerrCapture {
public:
    CerrCapture() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrCapture(CerrCapture const&)            = delete;
    CerrCapture& operator=(CerrCapture const&) = delete;
    ~CerrCapture() { std::cerr.rdbuf(alt_); }

    [[nodiscard]] std::string text() const { return puffer_.str(); }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

// Basis-Uhr aller Faelle. Der Wert selbst ist beliebig; alle Fristen werden RELATIV abgeleitet.
constexpr std::int64_t kT0 = 1'770'000'000;

// Eine FREMDE pro-forma-Reservierung, angelegt zur Zeit `reserviert_epoch` (Frist = +30min daraus).
bl::BatchReservierung fremde_pro_forma(std::int64_t reserviert_epoch, std::size_t seq = 0) {
    return bl::make_slice_reservation("uuid-prod2-77", seq, "prod2", 24, 0, 4096, reserviert_epoch);
}

// Dokument mit gegebenen Reservierungen in den FakeStore legen (ueber den normalen Emitter).
void lege_dokument(FakeStore& s, std::vector<bl::BatchReservierung> res) {
    bl::BestandslogDocument d;
    d.genus          = bl::Genus::binary;
    d.created_utc    = bl::utc_iso_from_epoch(kT0);
    d.reservierungen = std::move(res);
    s.objs[kDocKey]  = bl::emit_document(d);
}

// Das Dokument aus dem Store zuruecklesen (ASSERT-fest im Aufrufer).
std::optional<bl::BestandslogDocument> lies_dokument(FakeStore& s) {
    auto it = s.objs.find(kDocKey);
    if (it == s.objs.end()) return std::nullopt;
    return bl::parse_bestandslog(it->second);
}

// Den Status einer Reservierungs-id im Store nachschlagen.
std::optional<bl::BatchStatus> status_von(FakeStore& s, std::string const& id) {
    auto const d = lies_dokument(s);
    if (!d) return std::nullopt;
    for (auto const& r : d->reservierungen)
        if (r.id == id) return r.status;
    return std::nullopt;
}

std::string hexkey(char c) { return std::string(128, c); }

} // namespace

// =================================================================================================
// 1. DIE ZEIT-BRUECKE: epoch_from_utc_iso ist die Inverse des einen Formatters
// =================================================================================================
TEST(G3TakeoverSweep, EpochIsoRoundtripUndStrengeAblehnung) {
    // Die Erwartung ist der ROUNDTRIP selbst -- kein handgerechneter Kalenderwert (A9).
    for (std::int64_t const t : {std::int64_t{0}, kT0, kT0 + 1799, std::int64_t{4'102'444'800}}) {
        auto const iso = bl::utc_iso_from_epoch(t);
        auto const rt  = bl::epoch_from_utc_iso(iso);
        ASSERT_TRUE(rt.has_value()) << iso;
        EXPECT_EQ(*rt, t) << iso;
    }
    // Strenge: alles ausser dem exakten Format faellt als nullopt heraus (die Takeover-Entscheidung
    // darf auf einem kaputten Anker nie gruenden).
    EXPECT_FALSE(bl::epoch_from_utc_iso("").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("kaputt").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-08-02 07:00:00Z").has_value()); // Leerzeichen statt T
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-08-02T07:00:00").has_value());  // Z fehlt
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-13-02T07:00:00Z").has_value()); // Monat 13
}

// =================================================================================================
// 2. OWNER-TRENNUNG: fremd ist der ANDERE Owner-Anteil, nie ein Praefix-Zufall
// =================================================================================================
TEST(G3TakeoverSweep, OwnerAnteilTrenntEigenVonFremd) {
    auto const eigene = bl::make_slice_reservation("uuid-prod1-42", 3, "prod1", 32, 0, 4096, kT0);
    auto const fremde = fremde_pro_forma(kT0);
    EXPECT_FALSE(bl::is_foreign_reservation(eigene, "uuid-prod1-42"));
    EXPECT_TRUE(bl::is_foreign_reservation(fremde, "uuid-prod1-42"));
    // Kein Praefix-Zufall: ein Owner, der ein Praefix des anderen ist, bleibt fremd.
    auto kurz = eigene;
    kurz.id   = "uuid-prod1/9";
    EXPECT_TRUE(bl::is_foreign_reservation(kurz, "uuid-prod1-42"));
    EXPECT_FALSE(bl::is_foreign_reservation(kurz, "uuid-prod1"));
}

// =================================================================================================
// 3. DIE VIER PFLICHT-FAELLE DES SWEEPS
// =================================================================================================
TEST(G3TakeoverSweep, VerfalleneProFormaWirdUebernommen) {
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});

    // Skript-Uhr NACH der abgeleiteten 30min-Frist (Frist + 1s) -- die Zahl 1800 steht hier nicht.
    auto const frist = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1));

    EXPECT_EQ(erg.offene_fremde, 1U);
    EXPECT_EQ(erg.uebernommen, 1U);
    EXPECT_TRUE(erg.geschrieben);
    ASSERT_EQ(erg.ids.size(), 1U);
    EXPECT_EQ(erg.ids[0], fremde.id);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::released) << "Der Claim ist im STORE released.";
    EXPECT_NE(cerr_fang.text().find("takeover: 1 verfallene fremde Reservierung"), std::string::npos)
        << "Eine Uebernahme ist ein benanntes Ereignis (Nie-stumm), Testat war:\n"
        << cerr_fang.text();
}

TEST(G3TakeoverSweep, FrischeFremdeBleibtUnangetastet) {
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});
    std::string const vorher = s.objs[kDocKey];

    // Skript-Uhr VOR der Frist (Frist - 1s): nichts passiert, das Dokument bleibt BYTE-identisch
    // (kein Schreibvorgang, keine doc_revision-Bewegung, kein Testat-Rauschen).
    auto const frist = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist - 1));

    EXPECT_EQ(erg.offene_fremde, 1U);
    EXPECT_EQ(erg.uebernommen, 0U);
    EXPECT_FALSE(erg.geschrieben);
    EXPECT_EQ(s.objs[kDocKey], vorher) << "Eine LEBENDE fremde Reservierung darf keinen Schreibvorgang ausloesen.";
    EXPECT_TRUE(cerr_fang.text().empty()) << cerr_fang.text();
}

TEST(G3TakeoverSweep, EigeneWirdNieUebernommen) {
    FakeStore  s;
    auto const eigene = bl::make_slice_reservation("uuid-prod1-42", 0, "prod1", 32, 0, 4096, kT0);
    lege_dokument(s, {eigene});
    std::string const vorher = s.objs[kDocKey];

    // WEIT nach der Frist -- und trotzdem tabu: die eigene offene Reservierung ist der eigene
    // (Wieder-)Anlauf, kein toter Fremd-Claim.
    auto const t   = s.transport();
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 10 * 3600));

    EXPECT_EQ(erg.offene_fremde, 0U);
    EXPECT_EQ(erg.uebernommen, 0U);
    EXPECT_EQ(s.objs[kDocKey], vorher);
    EXPECT_EQ(status_von(s, eigene.id), bl::BatchStatus::offen);
}

TEST(G3TakeoverSweep, EtaFallMaschineGestorbenUndGegenprobe) {
    // Nach der Kalibrierung urteilt die 1,5xETA-Regel (Anker = reserviert_utc, s. Kopf des
    // Konsumenten): ETA 100s -> uebernehmbar erst NACH 150s ohne Update.
    FakeStore s;
    auto      fremde = fremde_pro_forma(kT0);
    bl::apply_calibration(fremde, bl::EtaResult{100.0, 1024});
    lege_dokument(s, {fremde});
    auto const t = s.transport();

    // Gegenprobe zuerst: 149s nach der Reservierung ist die Maschine nicht tot.
    auto const frueh = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 149));
    EXPECT_EQ(frueh.uebernommen, 0U);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::offen);

    // 151s ohne Update: ETA um 50% ueberschritten -> "Maschine GESTORBEN", Arbeit uebernehmen.
    auto const spaet = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 151));
    EXPECT_EQ(spaet.uebernommen, 1U);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::released);
}

TEST(G3TakeoverSweep, KaputteZeitankerWerdenNieUebernommen) {
    FakeStore s;
    auto      kaputt         = fremde_pro_forma(kT0);
    kaputt.pro_forma_bis_utc = "irgendwann"; // unparsebarer Anker
    auto kaputt_eta          = fremde_pro_forma(kT0, 1);
    bl::apply_calibration(kaputt_eta, bl::EtaResult{10.0, 512});
    kaputt_eta.reserviert_utc = ""; // ETA-Fall ohne Uhr-Anker
    lege_dokument(s, {kaputt, kaputt_eta});
    std::string const vorher = s.objs[kDocKey];

    auto const t   = s.transport();
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 100 * 3600));

    EXPECT_EQ(erg.offene_fremde, 2U);
    EXPECT_EQ(erg.uebernommen, 0U) << "Ohne parsbaren Zeit-Anker wird NIE uebernommen -- lieber eine "
                                      "tote Reservierung stehen lassen als eine lebende stehlen.";
    EXPECT_EQ(s.objs[kDocKey], vorher);
}

// =================================================================================================
// 4. DIE UEBERNOMMENE ARBEIT LANDET ALS BESTAND (der LEDGER:3291-Schluss-Satz)
// =================================================================================================
TEST(G3TakeoverSweep, UebernommeneArbeitLandetAlsBestand) {
    // Lage: prod2 hat reserviert und ist gestorben; der Bestand ist LEER (nichts verzeichnet).
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});
    auto const frist = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    auto const t = s.transport();

    // Schritt 1 (Lauf-Start): Sweep uebernimmt den toten Claim.
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1));
    ASSERT_EQ(erg.uebernommen, 1U);

    // Schritt 2 (der Lauf baut die unverzeichnete Arbeit und registriert sie -- der normale
    // I1-Weg: load -> observe(frisch) -> flush). Kein echter Compile noetig: registriert wird die
    // Buchhaltung, nicht die Bytes.
    bl::LagerRunState lager;
    lager.load(t, kDocKey);
    EXPECT_EQ(lager.lager_size(), 0U) << "Vorher ist NICHTS als Bestand verzeichnet.";
    auto const outcome = lager.observe(hexkey('a'), bl::ZellKoordinaten{"", "O2", "avx2"}, "tier/0/perm.dll", 4096,
                                       "[d,e,f][g,h,i]", bl::utc_iso_from_epoch(*frist + 5));
    EXPECT_EQ(outcome, bl::DedupOutcome::fresh_register);
    auto const n = lager.flush(t, kDocKey, bl::utc_iso_from_epoch(*frist + 6), ich(), 90, feste_uhr(*frist + 6));
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 1U);

    // Endzustand: der fremde Claim ist released UND die Arbeit steht als Bestand im selben Dokument.
    auto const d = lies_dokument(s);
    ASSERT_TRUE(d.has_value());
    ASSERT_EQ(d->bestand.size(), 1U);
    EXPECT_EQ(d->bestand[0].key_sha512, hexkey('a'));
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::released);
}

// =================================================================================================
// 5. PROMISEGUARD-AUFLAGE: der Abbruchpfad hinterlaesst weiterhin released
// =================================================================================================
TEST(G3TakeoverSweep, PromiseGuardAbbruchpfadHinterlaesstReleased) {
    // Der Iterator-Abbruchpfad in Miniatur (dieselben Bausteine wie run_planer_driven_provision):
    // pro-forma-Reservierung schreiben, PromiseGuard OHNE commit zerstoeren -> released im Store.
    // Der Takeover-Sweep dieses Pakets darf diese Semantik nicht verschieben: released ist danach
    // NICHT uebernehmbar (kein Doppel-Takeover auf sauber freigegebener Arbeit).
    FakeStore  s;
    auto const t   = s.transport();
    auto       res = bl::make_slice_reservation("uuid-prod2-77", 0, "prod2", 24, 0, 4096, kT0);
    ASSERT_TRUE(bl::store_reservation_locked(t, kDocKey, bl::LockOwner{"uuid-prod2-77", "prod2", 7}, 90, feste_uhr(kT0),
                                             res, "pro-forma-Reservierung"));
    {
        bl::PromiseGuard guard([&] {
            bl::mark_released(res);
            (void)bl::store_reservation_locked(t, kDocKey, bl::LockOwner{"uuid-prod2-77", "prod2", 7}, 90,
                                               feste_uhr(kT0 + 1), res, "Release-Reservierung");
        });
        // kein commit() -> Abbruch
    }
    EXPECT_EQ(status_von(s, res.id), bl::BatchStatus::released);

    // Und der Sweep laesst die sauber freigegebene Reservierung in Ruhe -- selbst Stunden spaeter.
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 24 * 3600));
    EXPECT_EQ(erg.offene_fremde, 0U);
    EXPECT_EQ(erg.uebernommen, 0U);
    EXPECT_EQ(status_von(s, res.id), bl::BatchStatus::released);
}

// =================================================================================================
// 6. NACHBESSERUNGS-HAERTUNG (Truth-Check A-1/A-2/A-3 + Schreibfehler-Pfad)
// =================================================================================================
TEST(G3TakeoverSweep, A1KaputteOderNullEtaFaelltAufDenProFormaZweig) {
    // parse_seconds("murks") == 0.0 -- ohne die A-1-Wache waere is_takeable_by_eta(0, ...) SOFORT
    // frei und eine lebende Maschine enteignet. Mit Wache urteilt der pro-forma-Zweig.
    FakeStore s;
    auto      murks = fremde_pro_forma(kT0, 0);
    murks.eta_s     = "murks"; // nicht parsbar
    auto null_eta   = fremde_pro_forma(kT0, 1);
    null_eta.eta_s  = "0.000"; // nicht positiv
    lege_dokument(s, {murks, null_eta});
    auto const frist = bl::epoch_from_utc_iso(murks.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    auto const t = s.transport();

    // VOR der pro-forma-Frist: NICHTS wird uebernommen (die kaputte eta macht nicht sofort-frei).
    auto const frueh = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 60));
    EXPECT_EQ(frueh.uebernommen, 0U) << "Eine kaputte eta_s darf NIE als 'sofort uebernehmbar' wirken.";
    EXPECT_EQ(status_von(s, murks.id), bl::BatchStatus::offen);

    // NACH der pro-forma-Frist greift der Frist-Zweig fuer beide.
    auto const spaet = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1));
    EXPECT_EQ(spaet.uebernommen, 2U);
    EXPECT_EQ(status_von(s, murks.id), bl::BatchStatus::released);
    EXPECT_EQ(status_von(s, null_eta.id), bl::BatchStatus::released);
}

TEST(G3TakeoverSweep, A2IdOhneSchraegstrichWirdNieUebernommen) {
    FakeStore s;
    auto      fremdform = fremde_pro_forma(kT0);
    fremdform.id        = "ganz-ohne-schraegstrich"; // keine der beiden gebauten id-Formen
    lege_dokument(s, {fremdform});
    std::string const vorher = s.objs[kDocKey];

    auto const t   = s.transport();
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 100 * 3600));
    EXPECT_EQ(erg.uebernommen, 0U) << "Unbekanntes id-Format -> konservativ stehen lassen (Befund, kein Freibrief).";
    EXPECT_EQ(s.objs[kDocKey], vorher);
}

TEST(G3TakeoverSweep, A3StoreFehlerNenntKeineIdsUndLaesstAllesUnveraendert) {
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});
    s.fail_store_key         = kDocKey; // der echte Schreibfehler-Pfad
    std::string const vorher = s.objs[kDocKey];
    auto const        frist  = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());

    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1));

    EXPECT_FALSE(erg.geschrieben);
    EXPECT_EQ(erg.uebernommen, 0U);
    EXPECT_TRUE(erg.ids.empty()) << "A-3: ein gescheiterter Store hat nichts uebernommen, also nennt er nichts.";
    EXPECT_EQ(erg.offene_fremde, 1U);
    EXPECT_EQ(s.objs[kDocKey], vorher) << "Das Dokument bleibt beim Schreibfehler byte-unveraendert.";
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::offen);
    EXPECT_NE(cerr_fang.text().find("nicht persistiert"), std::string::npos)
        << "Der Fehlschlag ist eine benannte Testat-Zeile, kein Schweigen. Ausgabe war:\n"
        << cerr_fang.text();
    EXPECT_EQ(cerr_fang.text().find("takeover:"), std::string::npos)
        << "KEINE Erfolgs-Testat-Zeile bei gescheitertem Store.";
}
