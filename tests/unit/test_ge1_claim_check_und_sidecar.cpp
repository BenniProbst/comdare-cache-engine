// test_ge1_claim_check_und_sidecar -- S5 der A1-Lager-Rest-Welle.
//
// TRAGENDE Abnahme von:
//   G-E1 / 2.4-(8) / ABNAHME-6  der LAUF-START-CLAIM-CHECK: fremde verfallene Reservierungen werden
//                               UEBERNOMMEN, lebende STEHEN GELASSEN, planer_block nie angefasst,
//                               und die B4-Revalidierung am Store-Resultat bleibt.
//   N8-Variante-B               die per-Owner-SIDECAR-Registrierung: Registrieren ist ein reiner
//                               Klein-Store (FakeStore ZAEHLT die Verben -- kein fetch des
//                               Hauptdokuments), Konsolidierung NUR im Schluss-Zustand,
//                               Zwei-Owner-Parallel-Registrierung ohne Lost-Update.
//
// Muster test_g3_takeover_sweep: FakeStore (In-Memory) + Skript-Uhr, kein minio, kein Netz.

#include "bestandslog/builder_registration.hpp"
#include "bestandslog/registrierungs_sidecar.hpp"
#include "bestandslog/reservation_lifecycle.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// CT-Nachweis der Zustandsmaschine: WER kann konsolidieren? Als Concept formuliert, damit die Frage
// stellbar ist -- ein direkter requires-Ausdruck auf einem Nicht-Template-Typ waere ein harter
// Compile-Fehler statt einer Antwort (dieselbe Bauform wie in test_lb1_knoten_heuristik_log).
template <class T>
concept KannKonsolidieren = requires(T zustand, bl::BestandTransport const& t, bl::AlleinschreiberNachweis const& n) {
    zustand.konsolidiere(t, n, std::string{});
};

struct FakeStore {
    std::map<std::string, std::string> objekte;
    std::size_t                        fetches     = 0;
    std::size_t                        stores      = 0;
    std::size_t                        removes     = 0;
    bool                               mit_listing = true;

    [[nodiscard]] bl::BestandTransport naht() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            ++fetches;
            auto it = objekte.find(k);
            if (it == objekte.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& v) {
            ++stores;
            objekte[k] = v;
            return true;
        };
        t.remove = [this](std::string const& k) {
            ++removes;
            objekte.erase(k);
            return true;
        };
        t.stat = [](std::string const&) -> std::optional<bl::ObjectStat> { return std::nullopt; };
        if (mit_listing)
            t.list = [this](std::string const& praefix) {
                std::vector<std::string> out;
                for (auto const& [k, v] : objekte) {
                    (void)v;
                    if (k.size() > praefix.size() && k.compare(0, praefix.size(), praefix) == 0 &&
                        k[praefix.size()] == '/')
                        out.push_back(k);
                }
                return out;
            };
        return t;
    }
};

// Skript-Uhr: der Test setzt "jetzt" von Hand -- keine Wall-Clock.
struct SkriptUhr {
    std::int64_t            jetzt = 1'700'000'000;
    [[nodiscard]] bl::NowFn fn() {
        return [this]() { return jetzt; };
    }
};

bl::LockOwner owner(std::string const& uuid) { return bl::LockOwner{uuid, "prod1", 7}; }

// Eine FREMDE, laengst verfallene tier-Reservierung ueber ein Fenster, das dieser Lauf voll deckt.
bl::BatchReservierung fremde_verfallene(std::string const& id, std::uint64_t begin, std::uint64_t count,
                                        std::int64_t reserviert_epoch) {
    return bl::make_pro_forma_reservation(id, bl::BatchTyp::tier, "prod2", 24, begin, count,
                                          bl::utc_iso_from_epoch(reserviert_epoch),
                                          bl::utc_iso_from_epoch(bl::pro_forma_deadline_epoch_s(reserviert_epoch)));
}

std::string dokument_mit(std::vector<bl::BatchReservierung> const& res) {
    bl::BestandslogDocument d;
    d.genus          = bl::Genus::binary;
    d.created_utc    = "2026-08-03T12:00:00Z";
    d.reservierungen = res;
    return bl::emit_document(d);
}

constexpr std::string_view kDoc = "lager/binaries.xml";

} // namespace

// ===========================================================================
// (1) G-E1 -- der LAUF-START-CLAIM-CHECK.
// ===========================================================================
TEST(Ge1ClaimCheck, VerfalleneFremdeReservierungWirdUebernommenUndDasFensterLiegtInDerSelektion) {
    FakeStore store;
    SkriptUhr uhr;
    // vor 2 Stunden reserviert -> die 30-min-pro-forma-Frist ist laengst um.
    store.objekte[std::string{kDoc}] = dokument_mit({fremde_verfallene("uuid-fremd/0", 0, 4, uhr.jetzt - 7200)});

    std::vector<std::size_t> const indices{0, 1, 2, 3}; // deckt [0,4) VOLL
    auto const                     scope = bl::make_sweep_scope(bl::BatchTyp::tier, indices);
    auto const erg = bl::takeover_expired_reservations(store.naht(), std::string{kDoc}, owner("uuid-ich"),
                                                       bl::default_lock_ttl_s(), uhr.fn(), scope);
    EXPECT_EQ(erg.offene_fremde, 1u);
    EXPECT_EQ(erg.uebernommen, 1u);
    ASSERT_EQ(erg.ids.size(), 1u);
    EXPECT_EQ(erg.ids[0], "uuid-fremd/0");
    EXPECT_TRUE(erg.geschrieben);
    EXPECT_TRUE(erg.revalidiert) << "B4: gezaehlt wird am ZURUECKGELESENEN Store-Resultat";
    EXPECT_EQ(erg.typ_fremd, 0u);
    EXPECT_EQ(erg.nicht_gedeckt, 0u);

    // Der Claim ist weg -> nichts haelt die Arbeit mehr zurueck; der per-Binary-Miss-Weg baut nach.
    auto const nachher = bl::parse_bestandslog(store.objekte.at(std::string{kDoc}));
    ASSERT_TRUE(nachher.has_value());
    ASSERT_EQ(nachher->reservierungen.size(), 1u);
    EXPECT_EQ(nachher->reservierungen[0].status, bl::BatchStatus::released);
}

TEST(Ge1ClaimCheck, LebendeFremdeReservierungWirdSTEHENGELASSEN) {
    FakeStore store;
    SkriptUhr uhr;
    store.objekte[std::string{kDoc}] = dokument_mit({fremde_verfallene("uuid-fremd/0", 0, 4, uhr.jetzt - 60)});
    auto const erg                   = bl::takeover_expired_reservations(
        store.naht(), std::string{kDoc}, owner("uuid-ich"), bl::default_lock_ttl_s(), uhr.fn(),
        bl::make_sweep_scope(bl::BatchTyp::tier, std::vector<std::size_t>{0, 1, 2, 3}));
    EXPECT_EQ(erg.offene_fremde, 1u);
    EXPECT_EQ(erg.uebernommen, 0u);
    EXPECT_FALSE(erg.geschrieben) << "Ohne Kandidat wird gar nicht erst geschrieben";
    auto const nachher = bl::parse_bestandslog(store.objekte.at(std::string{kDoc}));
    ASSERT_TRUE(nachher.has_value());
    EXPECT_EQ(nachher->reservierungen[0].status, bl::BatchStatus::offen);
}

TEST(Ge1ClaimCheck, PlanerBlockWirdNIEAngefasstUndNichtGedeckteFensterBleibenStehen) {
    FakeStore store;
    SkriptUhr uhr;
    auto      pb                     = fremde_verfallene("uuid-fremd/pb", 0, 4, uhr.jetzt - 7200);
    pb.typ                           = bl::BatchTyp::planer_block;
    auto const ausserhalb            = fremde_verfallene("uuid-fremd/weit", 900, 4, uhr.jetzt - 7200);
    store.objekte[std::string{kDoc}] = dokument_mit({pb, ausserhalb});

    auto const erg = bl::takeover_expired_reservations(
        store.naht(), std::string{kDoc}, owner("uuid-ich"), bl::default_lock_ttl_s(), uhr.fn(),
        bl::make_sweep_scope(bl::BatchTyp::tier, std::vector<std::size_t>{0, 1, 2, 3}));
    EXPECT_EQ(erg.offene_fremde, 2u);
    EXPECT_EQ(erg.uebernommen, 0u);
    EXPECT_EQ(erg.typ_fremd, 1u) << "planer_block: der Tier-Sweep kann seine Arbeit nicht uebernehmen";
    EXPECT_EQ(erg.nicht_gedeckt, 1u) << "Fenster ausserhalb der Selektion: wer nicht uebernehmen kann, enteignet nicht";
    auto const nachher = bl::parse_bestandslog(store.objekte.at(std::string{kDoc}));
    ASSERT_TRUE(nachher.has_value());
    for (auto const& r : nachher->reservierungen) EXPECT_EQ(r.status, bl::BatchStatus::offen);
}

TEST(Ge1ClaimCheck, DerNullFallSchweigtUndSchreibtNichts) {
    FakeStore store;
    SkriptUhr uhr;
    store.objekte[std::string{kDoc}] = dokument_mit({});
    auto const vor_stores            = store.stores;
    auto const erg                   = bl::takeover_expired_reservations(
        store.naht(), std::string{kDoc}, owner("uuid-ich"), bl::default_lock_ttl_s(), uhr.fn(),
        bl::make_sweep_scope(bl::BatchTyp::tier, std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(erg.offene_fremde, 0u);
    EXPECT_EQ(erg.uebernommen, 0u);
    EXPECT_EQ(store.stores, vor_stores) << "Kein Kandidat -> kein Lock, kein Schreibvorgang";
}

// ===========================================================================
// (2) N8-VARIANTE B -- die per-Owner-Sidecar-Registrierung.
// ===========================================================================
TEST(N8SidecarRegistrierung, RegistrierenSchreibtNURDasSidecarObjekt) {
    // Der eigentliche N7-D4-Punkt: KEIN fetch-merge-store des (39-65 MB grossen) Hauptdokuments.
    FakeStore store;
    SkriptUhr uhr;
    // Ein grosses Hauptdokument liegt vor -- es darf im Registrier-Pfad NICHT angefasst werden.
    store.objekte[std::string{kDoc}] = dokument_mit({fremde_verfallene("uuid-x/0", 0, 4, uhr.jetzt)});
    auto const t                     = store.naht();

    bl::RegistrierungOffen const reg{std::string{kDoc}};
    auto const                   vor_fetches = store.fetches;
    auto const                   vor_stores  = store.stores;
    EXPECT_EQ(reg.registriere(t, "uuid-a", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-a", 0, "prod1", 32, 0, 4096, uhr.jetzt)},
                              "2026-08-03T12:00:00Z"),
              bl::SidecarFehler::ok);
    EXPECT_EQ(store.fetches, vor_fetches) << "NULL fetches -- genau das ist Variante B";
    EXPECT_EQ(store.stores, vor_stores + 1) << "GENAU EIN Store: das Sidecar-Objekt";
    EXPECT_EQ(store.objekte.count(bl::sidecar_key_for(kDoc, "uuid-a")), 1u);
    EXPECT_EQ(bl::sidecar_key_for(kDoc, "uuid-a"), "lager/binaries.xml.reg/uuid-a.xml");
    EXPECT_EQ(bl::registrierungs_praefix(kDoc), "lager/binaries.xml.reg");
}

TEST(N8SidecarRegistrierung, ZweiOwnerParallelOhneLostUpdate) {
    FakeStore                    store;
    SkriptUhr                    uhr;
    auto const                   t = store.naht();
    bl::RegistrierungOffen const reg{std::string{kDoc}};
    ASSERT_EQ(reg.registriere(t, "uuid-a", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-a", 0, "prod1", 32, 0, 4096, uhr.jetzt)}, "u"),
              bl::SidecarFehler::ok);
    ASSERT_EQ(reg.registriere(t, "uuid-b", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-b", 0, "prod2", 24, 4096, 4096, uhr.jetzt)}, "u"),
              bl::SidecarFehler::ok);
    // Getrennte Objekte -> der eine kann den anderen gar nicht ueberschreiben (der N7-D4-Defekt).
    EXPECT_EQ(store.objekte.count(bl::sidecar_key_for(kDoc, "uuid-a")), 1u);
    EXPECT_EQ(store.objekte.count(bl::sidecar_key_for(kDoc, "uuid-b")), 1u);

    // Und der LESER sieht beide vereinigt, ohne dass konsolidiert wurde.
    auto const ladung = bl::lade_mit_sidecars(t, std::string{kDoc});
    ASSERT_TRUE(ladung.ok()) << bl::to_string(ladung.fehler);
    ASSERT_TRUE(ladung.doc.has_value());
    EXPECT_EQ(ladung.sidecars_gesehen, 2u);
    ASSERT_EQ(ladung.doc->reservierungen.size(), 2u);
    EXPECT_EQ(ladung.doc->reservierungen[0].id, "uuid-a/0");
    EXPECT_EQ(ladung.doc->reservierungen[1].id, "uuid-b/0");
}

TEST(N8SidecarRegistrierung, KonsolidierungNURImSchlussZustandUndUnterLock) {
    FakeStore                    store;
    SkriptUhr                    uhr;
    auto const                   t = store.naht();
    bl::RegistrierungOffen const reg{std::string{kDoc}};
    ASSERT_EQ(reg.registriere(t, "uuid-a", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-a", 0, "prod1", 32, 0, 4096, uhr.jetzt)}, "u"),
              bl::SidecarFehler::ok);
    ASSERT_EQ(reg.registriere(t, "uuid-b", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-b", 0, "prod2", 24, 4096, 4096, uhr.jetzt)}, "u"),
              bl::SidecarFehler::ok);

    std::uint64_t revision = 0;
    auto const    o        = bl::mit_dokument_alleinschreiber(t, std::string{kDoc}, owner("uuid-ich"), uhr.fn(),
                                                              [&](bl::AlleinschreiberNachweis const& n) {
                                                        auto s = reg.schluss_build_ende(n);
                                                        if (!s) return false;
                                                        EXPECT_EQ(s->grund(), "build_ende");
                                                        auto const e = s->konsolidiere(t, n, "2026-08-03T13:00:00Z");
                                                        EXPECT_TRUE(e.ok()) << bl::to_string(e.fehler);
                                                        EXPECT_EQ(e.sidecars_vereint, 2u);
                                                        EXPECT_EQ(e.sidecars_entfernt, 2u);
                                                        revision = e.doc_revision;
                                                        return true;
                                                              });
    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_GT(revision, 0u) << "doc_revision bleibt monoton";
    // Die Sidecars sind fort, das Hauptdokument traegt beide Records.
    EXPECT_EQ(store.objekte.count(bl::sidecar_key_for(kDoc, "uuid-a")), 0u);
    EXPECT_EQ(store.objekte.count(bl::sidecar_key_for(kDoc, "uuid-b")), 0u);
    auto const haupt = bl::parse_bestandslog(store.objekte.at(std::string{kDoc}));
    ASSERT_TRUE(haupt.has_value());
    ASSERT_EQ(haupt->reservierungen.size(), 2u);
    // Roundtrip-Gate.
    EXPECT_EQ(bl::emit_document(*haupt), store.objekte.at(std::string{kDoc}));
    // Und die Konsolidierung ist DETERMINISTISCH: ein zweiter Lauf aendert nichts mehr.
    auto const zweit = bl::lade_mit_sidecars(t, std::string{kDoc});
    ASSERT_TRUE(zweit.ok());
    EXPECT_EQ(zweit.sidecars_gesehen, 0u);
    ASSERT_EQ(zweit.doc->reservierungen.size(), 2u);
}

TEST(N8SidecarRegistrierungNegativ, EinFremderNachweisKonsolidiertNICHT) {
    FakeStore                    store;
    SkriptUhr                    uhr;
    auto const                   t = store.naht();
    bl::RegistrierungOffen const reg{std::string{kDoc}};
    auto const                   o = bl::mit_dokument_alleinschreiber(
        t, "ein/anderes/dokument.xml", owner("uuid-ich"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            auto s = reg.schluss_build_ende(n);
            EXPECT_FALSE(s.has_value()) << "Sonst konsolidierte ein beliebiger Lock ein beliebiges Dokument";
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
}

TEST(N8SidecarRegistrierungNegativ, FehlendesListingIstEINEFEHLERKLASSEKeinStillesNichtstun) {
    // DEKLARIERTE Luecke (L14): ArtifactCache hat heute kein Listing-Verb. Ohne BestandTransport::list
    // sieht der Leser NUR das Hauptdokument -- und das wird GEMELDET.
    FakeStore store;
    SkriptUhr uhr;
    store.mit_listing                = false;
    store.objekte[std::string{kDoc}] = dokument_mit({});
    auto const t                     = store.naht();
    EXPECT_FALSE(static_cast<bool>(t.list));

    bl::RegistrierungOffen const reg{std::string{kDoc}};
    ASSERT_EQ(reg.registriere(t, "uuid-a", bl::Genus::binary,
                              {bl::make_slice_reservation("uuid-a", 0, "prod1", 32, 0, 4096, uhr.jetzt)}, "u"),
              bl::SidecarFehler::ok)
        << "Registrieren geht auch ohne Listing -- nur das Vereinigen braucht es";

    auto const ladung = bl::lade_mit_sidecars(t, std::string{kDoc});
    EXPECT_EQ(ladung.fehler, bl::SidecarFehler::auflistung_nicht_verfuegbar);
    EXPECT_TRUE(ladung.doc.has_value()) << "Das Hauptdokument kommt trotzdem -- nur eben unvollstaendig";

    auto const o = bl::mit_dokument_alleinschreiber(
        t, std::string{kDoc}, owner("uuid-ich"), uhr.fn(), [&](bl::AlleinschreiberNachweis const& n) {
            auto s = reg.schluss_build_ende(n);
            if (!s) return false;
            EXPECT_EQ(s->konsolidiere(t, n, "u").fehler, bl::SidecarFehler::auflistung_nicht_verfuegbar);
            return true;
        });
    EXPECT_EQ(o, bl::LockOutcome::ok);
}

TEST(N8SidecarRegistrierungNegativ, FailClosedOhneOwnerUndOhneTransport) {
    FakeStore                    store;
    auto const                   t = store.naht();
    bl::RegistrierungOffen const reg{std::string{kDoc}};
    EXPECT_EQ(reg.registriere(t, "", bl::Genus::binary, {}, "u"), bl::SidecarFehler::owner_leer);
    bl::RegistrierungOffen const leer{""};
    EXPECT_EQ(leer.registriere(t, "uuid-a", bl::Genus::binary, {}, "u"), bl::SidecarFehler::doc_key_leer);
    bl::BestandTransport const ohne;
    EXPECT_EQ(reg.registriere(ohne, "uuid-a", bl::Genus::binary, {}, "u"), bl::SidecarFehler::kein_transport);
    EXPECT_EQ(bl::lade_mit_sidecars(ohne, std::string{kDoc}).fehler, bl::SidecarFehler::kein_transport);
    EXPECT_EQ(bl::to_string(bl::SidecarFehler::owner_leer), "owner_leer");
}

TEST(N8SidecarRegistrierungNegativ, EinUnlesbaresHauptdokumentWirdNIEUeberschrieben) {
    // Dieselbe Doktrin wie die Voll-Wipe-Wache von store_document_merged: Haende weg.
    FakeStore store;
    store.objekte[std::string{kDoc}] = "<kein bestandslog/>";
    auto const t                     = store.naht();
    auto const ladung                = bl::lade_mit_sidecars(t, std::string{kDoc});
    EXPECT_EQ(ladung.fehler, bl::SidecarFehler::hauptdokument_unlesbar);
    EXPECT_FALSE(ladung.doc.has_value());
    EXPECT_EQ(store.objekte.at(std::string{kDoc}), "<kein bestandslog/>") << "unberuehrt";
}

TEST(N8SidecarRegistrierung, DerOffeneZustandKannNICHTKonsolidieren) {
    // State-Pattern mit Typ-Zustaenden (dieselbe Doktrin wie die Truncate-Maschine, S3): die
    // Konsolidierung ist im offenen Zustand nicht AUSDRUECKBAR.
    static_assert(!KannKonsolidieren<bl::RegistrierungOffen>,
                  "Im offenen Zustand darf die Konsolidierung nicht einmal AUSDRUECKBAR sein.");
    static_assert(KannKonsolidieren<bl::RegistrierungSchluss<bl::BuildEnde>>);
    static_assert(bl::SchlussGrund<bl::BuildEnde>);
    EXPECT_EQ(bl::RegistrierungSchluss<bl::BuildEnde>::grund(), "build_ende");
}
