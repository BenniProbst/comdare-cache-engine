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
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits> // TP1FK1-B1: Ueberlauf-Kante des Deckungs-Praedikats
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
    // TP1FK1-B4 (Interleaving-Naht): laeuft NACH jedem fetch DES DOKUMENT-Keys und darf den Store
    // veraendern. Damit ist der Wettlauf "der totgeglaubte Owner schreibt zwischen Klassifikation
    // und Store doch noch Done" deterministisch skriptbar -- ohne Threads, ohne Wall-Clock.
    std::string           doc_key_hook; // welcher Key den Hook ausloest (leer = keiner)
    std::function<void()> nach_doc_fetch;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto                       it = objs.find(k);
            std::optional<std::string> out;
            if (it != objs.end()) out = it->second;
            if (nach_doc_fetch && !doc_key_hook.empty() && k == doc_key_hook) nach_doc_fetch();
            return out;
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

// TP1FK1-B1: der Sweep braucht den Scope dieses Laufs (Batch-Typ + Selektions-Menge). Die
// Bestandsfaelle dieses Tests bauen alle auf fremde_pro_forma(...) mit slice_begin=0/slice_count=4096
// und typ=tier -> `voll_scope()` ist der Scope eines Laufs, der GENAU dieses Fenster baut. Die Menge
// wird ABGELEITET (0..4095), nicht als Literal-Liste gepflegt.
std::vector<std::size_t> fenster_indices(std::size_t begin, std::size_t count) {
    std::vector<std::size_t> v;
    v.reserve(count);
    for (std::size_t i = 0; i < count; ++i) v.push_back(begin + i);
    return v;
}
bl::SweepScope voll_scope() {
    static std::vector<std::size_t> const alle = fenster_indices(0, 4096);
    return bl::make_sweep_scope(bl::BatchTyp::tier, alle);
}

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
// 1b. TP1FK1-B3 (NEGATIVTEST, Codex-Befund): unmoegliche ZIVILDATEN werden abgelehnt, nicht
//     normalisiert. Vor der Haertung rechnete die days-from-civil-Formel den 31.04. still auf den
//     01.05. um -- ein erfundener Zeit-Anker unter einer Enteignungs-Entscheidung.
// =================================================================================================
TEST(G3TakeoverSweep, B3EpochIsoLehntUnmoeglicheZivildatenAb) {
    // Tage je Monat (die frueher pauschale Wache d<=31 liess alle vier durch).
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-04-31T07:00:00Z").has_value()) << "April hat 30 Tage.";
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-06-31T07:00:00Z").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-09-31T07:00:00Z").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-11-31T07:00:00Z").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-02-30T07:00:00Z").has_value());
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-01-00T07:00:00Z").has_value()) << "Tag 0 existiert nicht.";
    // Schaltjahr-Regel (durch 4, aber nicht durch 100, ausser durch 400).
    EXPECT_FALSE(bl::epoch_from_utc_iso("2027-02-29T07:00:00Z").has_value()) << "2027 ist kein Schaltjahr.";
    EXPECT_TRUE(bl::epoch_from_utc_iso("2028-02-29T07:00:00Z").has_value()) << "2028 ist ein Schaltjahr.";
    EXPECT_FALSE(bl::epoch_from_utc_iso("1900-02-29T07:00:00Z").has_value()) << "1900: durch 100, nicht durch 400.";
    EXPECT_TRUE(bl::epoch_from_utc_iso("2000-02-29T07:00:00Z").has_value()) << "2000: durch 400.";
    // Sekunde: unser Formatter erzeugt nie 60 -- eine 60 kommt aus einer fremden Quelle.
    EXPECT_FALSE(bl::epoch_from_utc_iso("2026-08-02T07:00:60Z").has_value());
    EXPECT_TRUE(bl::epoch_from_utc_iso("2026-08-02T07:00:59Z").has_value());
    // Und die Wirkung im Sweep: ein Record mit unmoeglichem Anker wird NIE uebernommen.
    FakeStore s;
    auto      krumm         = fremde_pro_forma(kT0);
    krumm.pro_forma_bis_utc = "2026-04-31T07:00:00Z"; // unmoegliches Datum
    lege_dokument(s, {krumm});
    std::string const vorher = s.objs[kDocKey];
    auto const        t      = s.transport();
    auto const        erg =
        bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 100 * 3600), voll_scope());
    EXPECT_EQ(erg.uebernommen, 0U) << "Ein unmoegliches Zivildatum ist ein kaputter Anker, kein Freibrief.";
    EXPECT_EQ(s.objs[kDocKey], vorher);
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
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

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
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist - 1), voll_scope());

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
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 10 * 3600), voll_scope());

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
    auto const frueh = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 149), voll_scope());
    EXPECT_EQ(frueh.uebernommen, 0U);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::offen);

    // 151s ohne Update: ETA um 50% ueberschritten -> "Maschine GESTORBEN", Arbeit uebernehmen.
    auto const spaet = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 151), voll_scope());
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

    auto const t = s.transport();
    auto const erg =
        bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 100 * 3600), voll_scope());

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
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());
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
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 24 * 3600), voll_scope());
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
    auto const frueh = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 60), voll_scope());
    EXPECT_EQ(frueh.uebernommen, 0U) << "Eine kaputte eta_s darf NIE als 'sofort uebernehmbar' wirken.";
    EXPECT_EQ(status_von(s, murks.id), bl::BatchStatus::offen);

    // NACH der pro-forma-Frist greift der Frist-Zweig fuer beide.
    auto const spaet = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());
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

    auto const t = s.transport();
    auto const erg =
        bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(kT0 + 100 * 3600), voll_scope());
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
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

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

// =================================================================================================
// 7. TP1FK1-B1 (Codex-Befund): der Sweep ist an DIESEN Lauf gekoppelt -- Batch-Typ + Selektion.
//    Ohne die Kopplung released der Sweep Claims, deren Arbeit dieser Lauf gar nicht aufnehmen kann;
//    die Arbeit bleibt dann liegen (Claim weg, niemand baut).
// =================================================================================================
TEST(G3TakeoverSweep, B1FremderBatchTypWirdNieUebernommen) {
    // Ein verfallener FREMDER planer_block (CEB-Vorreservierung). Der TIER-Sweep kann seine Arbeit
    // weder erkennen (sie ist keine Tier-Binary) noch bauen -> Haende weg, obwohl die Frist um ist.
    FakeStore s;
    auto      block = fremde_pro_forma(kT0);
    block.typ       = bl::BatchTyp::planer_block;
    lege_dokument(s, {block});
    std::string const vorher = s.objs[kDocKey];
    auto const        frist  = bl::epoch_from_utc_iso(block.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());

    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

    EXPECT_EQ(erg.offene_fremde, 1U);
    EXPECT_EQ(erg.uebernommen, 0U) << "Ein planer_block ist nicht die Arbeit des Tier-Sweeps.";
    EXPECT_EQ(erg.typ_fremd, 1U);
    EXPECT_EQ(erg.nicht_gedeckt, 0U);
    EXPECT_FALSE(erg.geschrieben);
    EXPECT_EQ(s.objs[kDocKey], vorher) << "Kein Schreibvorgang -- der fremde Claim bleibt byte-identisch stehen.";
    EXPECT_EQ(status_von(s, block.id), bl::BatchStatus::offen);
    EXPECT_NE(cerr_fang.text().find("takeover-scope:"), std::string::npos)
        << "Das bewusste Nicht-Handeln ist benannt (Nie-stumm). Ausgabe war:\n"
        << cerr_fang.text();
}

TEST(G3TakeoverSweep, B1EigenerTypBleibtUebernehmbar) {
    // Gegenprobe zur Typ-Wache: derselbe Fall mit typ=tier wird sehr wohl uebernommen -- die Wache
    // darf den Normalfall nicht mit-abschalten.
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0); // make_slice_reservation -> BatchTyp::tier
    ASSERT_EQ(fremde.typ, bl::BatchTyp::tier);
    lege_dokument(s, {fremde});
    auto const frist = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    auto const t   = s.transport();
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());
    EXPECT_EQ(erg.uebernommen, 1U);
    EXPECT_EQ(erg.typ_fremd, 0U);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::released);
}

TEST(G3TakeoverSweep, B1NichtGedeckteFensterBleibenStehen) {
    // Drei verfallene fremde tier-Claims, dieselbe Frist -- unterschiedliche Fenster:
    //   (a) [0,4096)      voll in der Selektion dieses Laufs        -> uebernehmbar
    //   (b) [8192,12288)  komplett ausserhalb                       -> stehen lassen
    //   (c) [4000,4196)   nur TEILWEISE gedeckt (der Rest > 4095)   -> stehen lassen
    // (c) ist der eigentliche Kern: eine Teil-Deckung hiesse, dass ein Rest der freigegebenen
    // Arbeit von niemandem gebaut wird. Volle Deckung oder gar nichts.
    FakeStore s;
    auto      innen = bl::make_slice_reservation("uuid-prod2-77", 0, "prod2", 24, 0, 4096, kT0);
    auto      weit  = bl::make_slice_reservation("uuid-prod2-77", 1, "prod2", 24, 8192, 4096, kT0);
    auto      halb  = bl::make_slice_reservation("uuid-prod2-77", 2, "prod2", 24, 4000, 196, kT0);
    lege_dokument(s, {innen, weit, halb});
    auto const frist = bl::epoch_from_utc_iso(innen.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());

    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

    EXPECT_EQ(erg.offene_fremde, 3U);
    EXPECT_EQ(erg.uebernommen, 1U) << "NUR das voll gedeckte Fenster.";
    EXPECT_EQ(erg.nicht_gedeckt, 2U);
    ASSERT_EQ(erg.ids.size(), 1U);
    EXPECT_EQ(erg.ids[0], innen.id);
    EXPECT_EQ(status_von(s, innen.id), bl::BatchStatus::released);
    EXPECT_EQ(status_von(s, weit.id), bl::BatchStatus::offen)
        << "Wer die Arbeit nicht uebernehmen kann, enteignet nicht.";
    EXPECT_EQ(status_von(s, halb.id), bl::BatchStatus::offen) << "Teil-Deckung ist keine Deckung.";
    EXPECT_NE(cerr_fang.text().find("takeover-scope:"), std::string::npos) << cerr_fang.text();
}

TEST(G3TakeoverSweep, B1LeereSelektionUebernimmtNichts) {
    // Ein Lauf ohne Selektion baut nichts -> er kann nichts uebernehmen (fail-closed by construction).
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});
    std::string const vorher = s.objs[kDocKey];
    auto const        frist  = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());
    auto const                     t = s.transport();
    std::vector<std::size_t> const leer;
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1),
                                                       bl::make_sweep_scope(bl::BatchTyp::tier, leer));
    EXPECT_EQ(erg.uebernommen, 0U);
    EXPECT_EQ(erg.nicht_gedeckt, 1U);
    EXPECT_EQ(s.objs[kDocKey], vorher);
}

TEST(G3TakeoverSweep, B1ScopeDeckungsPraedikatLiteral) {
    // Das Deckungs-Praedikat selbst, literal (die Bausteine-Ebene unter den Sweep-Faellen oben).
    auto const sc = bl::make_sweep_scope(bl::BatchTyp::tier, fenster_indices(10, 5)); // {10,11,12,13,14}
    EXPECT_TRUE(bl::scope_covers_slice(sc, 10, 5));
    EXPECT_TRUE(bl::scope_covers_slice(sc, 11, 3));
    EXPECT_FALSE(bl::scope_covers_slice(sc, 9, 5)) << "9 fehlt in der Selektion.";
    EXPECT_FALSE(bl::scope_covers_slice(sc, 13, 5)) << "15/16/17 fehlen.";
    EXPECT_FALSE(bl::scope_covers_slice(sc, 10, 0)) << "Ein leeres Fenster ist nichts zum Uebernehmen.";
    EXPECT_FALSE(bl::scope_covers_slice(sc, 100, 1));
    // Ueberlauf-Wache: ein krummes Feld darf nie in ein wahres Urteil kippen.
    EXPECT_FALSE(bl::scope_covers_slice(sc, (std::numeric_limits<std::uint64_t>::max)(), 2));
    // Luecke MITTEN in der Selektion -> keine volle Deckung.
    std::vector<std::size_t> const loechrig{10, 11, 13, 14};
    auto const                     sc2 = bl::make_sweep_scope(bl::BatchTyp::tier, loechrig);
    EXPECT_FALSE(bl::scope_covers_slice(sc2, 10, 5));
    EXPECT_TRUE(bl::scope_covers_slice(sc2, 10, 2));
}

// =================================================================================================
// 8. TP1FK1-B4 (Codex-Befund): 'uebernommen' wird AM STORE-RESULTAT revalidiert. Der Merge kann eine
//    Uebernahme richtigerweise verhindern (done schlaegt released) -- dann hat dieser Lauf NICHTS
//    uebernommen und darf es auch nicht behaupten.
// =================================================================================================
TEST(G3TakeoverSweep, B4InterleavingDoneGewinntZaehltNichtAlsUebernahme) {
    FakeStore  s;
    auto const fremde = fremde_pro_forma(kT0);
    lege_dokument(s, {fremde});
    auto const frist = bl::epoch_from_utc_iso(fremde.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());

    // Der Wettlauf: NACH dem Klassifikations-fetch (erster fetch des Dokument-Keys) schreibt der
    // totgeglaubte Owner sein Done. Der Store-interne Merge sieht es dann und laesst done gewinnen.
    int fetches      = 0;
    s.doc_key_hook   = kDocKey;
    s.nach_doc_fetch = [&] {
        if (++fetches != 1) return; // genau EINMAL, direkt nach der Klassifikation
        auto fertig = fremde;
        bl::mark_done(fertig);
        lege_dokument(s, {fertig});
    };

    CerrCapture cerr_fang;
    auto const  t   = s.transport();
    auto const  erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

    EXPECT_TRUE(erg.geschrieben) << "Der gelockte Schreib-Zyklus lief -- das ist nicht die Frage.";
    EXPECT_TRUE(erg.revalidiert);
    EXPECT_EQ(erg.uebernommen, 0U) << "done gewinnt im Merge -> dieser Lauf hat NICHTS uebernommen.";
    EXPECT_TRUE(erg.ids.empty()) << "Und er nennt auch keine id als uebernommen.";
    EXPECT_EQ(erg.vom_owner_beendet, 1U);
    EXPECT_EQ(erg.nicht_bestaetigt, 0U);
    EXPECT_EQ(status_von(s, fremde.id), bl::BatchStatus::done) << "Fortschritt geht nie verloren (status_rank).";
    EXPECT_NE(cerr_fang.text().find("VOM OWNER BEENDET"), std::string::npos)
        << "Der Fall ist benannt, nicht als Uebernahme verbucht. Ausgabe war:\n"
        << cerr_fang.text();
    EXPECT_EQ(cerr_fang.text().find("uebernommen ("), std::string::npos)
        << "KEINE Uebernahme-Erfolgszeile. Ausgabe war:\n"
        << cerr_fang.text();
}

TEST(G3TakeoverSweep, B4GemischtNurBestaetigteZaehlen) {
    // Zwei Kandidaten, einer wird zwischendurch fertig -> genau EINER ist uebernommen.
    FakeStore s;
    auto      a = bl::make_slice_reservation("uuid-prod2-77", 0, "prod2", 24, 0, 2048, kT0);
    auto      b = bl::make_slice_reservation("uuid-prod2-77", 1, "prod2", 24, 2048, 2048, kT0);
    lege_dokument(s, {a, b});
    auto const frist = bl::epoch_from_utc_iso(a.pro_forma_bis_utc);
    ASSERT_TRUE(frist.has_value());

    int fetches      = 0;
    s.doc_key_hook   = kDocKey;
    s.nach_doc_fetch = [&] {
        if (++fetches != 1) return;
        auto b_fertig = b;
        bl::mark_done(b_fertig);
        lege_dokument(s, {a, b_fertig});
    };

    auto const t   = s.transport();
    auto const erg = bl::takeover_expired_reservations(t, kDocKey, ich(), 90, feste_uhr(*frist + 1), voll_scope());

    EXPECT_TRUE(erg.revalidiert);
    EXPECT_EQ(erg.uebernommen, 1U);
    ASSERT_EQ(erg.ids.size(), 1U);
    EXPECT_EQ(erg.ids[0], a.id);
    EXPECT_EQ(erg.vom_owner_beendet, 1U);
    EXPECT_EQ(status_von(s, a.id), bl::BatchStatus::released);
    EXPECT_EQ(status_von(s, b.id), bl::BatchStatus::done);
}
