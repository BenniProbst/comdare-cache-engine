// test_ge3_messwert_genus_und_ziel -- S4 der A1-Lager-Rest-Welle: G-E3 + G-E6 + G-E7.
//
// TRAGENDE Abnahme der drei Dossier-Gaps:
//   G-E3  der PRODUKTIVE Schreiber des zweiten Bestandslog-Genus (measurement),
//   G-E6  der Versions-Tag in der Lager-Identitaet + der Wire-Bruch syntax_version 3 -> 4,
//   G-E7  die Dual-ccache-Konfigurationsnaht (Default minio=Binaries / NAS=Mess-CSV, beidseitig
//         umschaltbar) als CT-Strategy samt mc-Alias-Preflight (L1).
//
// OE-B: FakeStore (In-Memory-Map) als BestandTransport, Skript-Uhr, keine mc-/minio-/Netz-Beruehrung.

#include "bestandslog/bestandslog_document.hpp"
#include "bestandslog/bestandslog_factory.hpp"
#include "bestandslog/bestandslog_index.hpp"
#include "bestandslog/lager_ziel_strategie.hpp"
#include "bestandslog/messwert_registrierung.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// FakeStore: zaehlt die Verben, damit ein Testat "kein fetch im Schreib-Pfad" belegen kann.
struct FakeStore {
    std::map<std::string, std::string> objekte;
    std::size_t                        fetches = 0;
    std::size_t                        stores  = 0;

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
            objekte.erase(k);
            return true;
        };
        t.stat = [](std::string const&) -> std::optional<bl::ObjectStat> { return std::nullopt; };
        return t;
    }
};

bl::LockOwner owner(std::string const& uuid) { return bl::LockOwner{uuid, "prod1", 99}; }

bl::NowFn uhr() {
    return []() { return std::int64_t{1'000'000}; };
}

bl::ZellKoordinaten zelle_avx2() { return {.combo = "vereint", .opt = "O2", .simd = "avx2"}; }
bl::ZellKoordinaten zelle_avx512() { return {.combo = "vereint", .opt = "O2", .simd = "avx512"}; }

// Die Mess-Schluessel-Komponenten: voll-permutative Mess-Zeilen + Hardware-Identitaet. NICHT der
// Binary-Fingerprint -- die Trennung der Genera ist genau der Punkt (Section 66-N3, keine Fusion).
std::array<std::string_view, 3> mess_komponenten() {
    return {"measurement_tooling=wallclock@1.0.0.c", "[load_framework=ycsb@1.0.0.c]", "hw=prod1/amd64_v3"};
}

std::array<std::string_view, 3> mess_komponenten_zweit() {
    return {"measurement_tooling=pmc@1.0.0.c", "[load_framework=ycsb@1.0.0.c]", "hw=prod1/amd64_v3"};
}

} // namespace

// ===========================================================================
// (1) G-E3 -- der Messwert-Schluessel und das zweite Genus.
// ===========================================================================
TEST(Ge3MesswertGenus, DerSchluesselKommtAusDerMesswertPolicyUeberDIEEINESha512Wahrheit) {
    auto const k = bl::messwert_key_hex(mess_komponenten());
    EXPECT_EQ(k.size(), 128u);
    // EINE Ableitung, kein Zweitweg: messwert_key_hex geht ueber die B3-Factory-Policy.
    EXPECT_EQ(k, bl::to_hex(bl::Bestand<bl::MesswertKeyPolicy>::key_of(mess_komponenten())));
    // Verschiedene Mess-Konfigurationen ergeben verschiedene Schluessel (Injektivitaet des
    // '\n'-getrennten Preimage, A13-M3/OF-M3-1).
    EXPECT_NE(k, bl::messwert_key_hex(mess_komponenten_zweit()));

    // EHRLICHE PRAEZISIERUNG (am Ist erhoben, nicht behauptet): die DIGEST-MECHANIK ist zwischen den
    // Genera BEWUSST dieselbe (beide Policies rufen derive_key_from_lines -- die EINE SHA512-Wahrheit,
    // Section 62-B). Getrennt sind die Genera durch (a) die KOMPONENTEN, die der Aufrufer reicht
    // (Binary: Anatomie-Stempel-Glieder; Messwert: Mess-Zeilen + Hardware-Identitaet), (b) das
    // genus()-Feld der Policy und (c) das EIGENE Dokument mit eigenem doc_key (D-05). Es waere falsch,
    // hier eine Digest-Verschiedenheit zu behaupten, die die Implementierung nicht leistet.
    EXPECT_EQ(k, bl::to_hex(bl::Bestand<bl::BinaryKeyPolicy>::key_of(mess_komponenten())))
        << "Dieselbe Glied-Folge ergibt denselben Digest -- die Trennung liegt in Komponenten + Genus + Dokument";
    EXPECT_EQ(bl::MesswertKeyPolicy::genus(), bl::Genus::measurement);
    EXPECT_EQ(bl::BinaryKeyPolicy::genus(), bl::Genus::binary);
    EXPECT_NE(bl::MesswertKeyPolicy::genus(), bl::BinaryKeyPolicy::genus());
}

TEST(Ge3MesswertGenus, E2ERueckschriebSchreibtEinMeasurementDokumentMitKorrektemGenus) {
    FakeStore            store;
    auto const           t = store.naht();
    bl::MesswertRunState mess;
    mess.load(t, "lager/messwerte.xml");
    EXPECT_EQ(mess.lager_size(), 0u);

    auto const key = bl::messwert_key_hex(mess_komponenten());
    EXPECT_EQ(mess.observe(key, zelle_avx2(), "search_algo=k_ary/blatt", 4096, "[vereint,O2,avx2]",
                           "2026-08-03T12:00:00Z", "search_algo@1.0.0.c"),
              bl::MesswertOutcome::fresh_register);
    EXPECT_EQ(mess.pending_fresh(), 1u);

    auto const n = mess.flush(t, "lager/messwerte.xml", "2026-08-03T12:00:00Z", owner("uuid-a"), 1800, uhr());
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 1u);
    EXPECT_EQ(mess.pending_fresh(), 0u);

    auto const doc = bl::parse_bestandslog(store.objekte.at("lager/messwerte.xml"));
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->genus, bl::Genus::measurement) << "Das Genus ist FEST -- ein Schreiber, der es vom Aufrufer "
                                                     "bekaeme, koennte die zwei Realms vermischen";
    ASSERT_EQ(doc->bestand.size(), 1u);
    EXPECT_EQ(doc->bestand[0].key_sha512, key);
    EXPECT_EQ(doc->bestand[0].zelle, zelle_avx2());
    EXPECT_EQ(doc->bestand[0].versions, "search_algo@1.0.0.c");
    // Roundtrip-Gate: emit(parse(emit)) == emit.
    EXPECT_EQ(bl::emit_document(*doc), store.objekte.at("lager/messwerte.xml"));
}

TEST(Ge3MesswertGenus, DedupUeberDasTupelAusKeyUndZelle) {
    FakeStore  store;
    auto const t   = store.naht();
    auto const key = bl::messwert_key_hex(mess_komponenten());
    {
        bl::MesswertRunState mess;
        mess.load(t, "lager/messwerte.xml");
        (void)mess.observe(key, zelle_avx2(), "p", 1, "s", "2026-08-03T12:00:00Z");
        // DIESELBE Mess-Konfiguration in einer ANDEREN Zelle ist ein ANDERER Bestand.
        (void)mess.observe(key, zelle_avx512(), "p", 1, "s", "2026-08-03T12:00:00Z");
        ASSERT_TRUE(mess.flush(t, "lager/messwerte.xml", "u", owner("uuid-a"), 1800, uhr()).has_value());
    }
    {
        bl::MesswertRunState zweit;
        zweit.load(t, "lager/messwerte.xml");
        EXPECT_EQ(zweit.lager_size(), 2u);
        EXPECT_TRUE(zweit.lager_contains(key, zelle_avx2()));
        EXPECT_TRUE(zweit.lager_contains(key, zelle_avx512()));
        EXPECT_FALSE(zweit.lager_contains(key, bl::ZellKoordinaten{}));
        EXPECT_EQ(zweit.observe(key, zelle_avx2(), "p", 1, "s", "u"), bl::MesswertOutcome::lager_hit);
        EXPECT_EQ(zweit.lager_hits(), 1u);
        EXPECT_EQ(zweit.pending_fresh(), 0u) << "Ein Hit merkt nichts vor";
    }
}

TEST(Ge3MesswertGenusNegativ, OhneGueltigenSchluesselWirdNichtsRegistriert) {
    FakeStore            store;
    auto const           t = store.naht();
    bl::MesswertRunState mess;
    mess.load(t, "d");
    EXPECT_EQ(mess.observe("", zelle_avx2(), "p", 1, "s", "u"), bl::MesswertOutcome::no_key);
    EXPECT_EQ(mess.observe(std::string(127, 'a'), zelle_avx2(), "p", 1, "s", "u"), bl::MesswertOutcome::no_key);
    EXPECT_EQ(mess.observe(std::string(128, 'Z'), zelle_avx2(), "p", 1, "s", "u"), bl::MesswertOutcome::no_key);
    EXPECT_EQ(mess.pending_fresh(), 0u);
    EXPECT_EQ(bl::to_string(bl::MesswertOutcome::no_key), "no_key");
}

TEST(Ge3MesswertGenusNegativ, ReQueueUndGezielterVerwurfMitTrenner) {
    FakeStore store;
    // Ein Store, dessen Schreiben fehlschlaegt -> der Batch bleibt VORGEMERKT (B13-Re-Queue).
    auto t  = store.naht();
    t.store = [](std::string const&, std::string const&) { return false; };
    bl::MesswertRunState mess;
    (void)mess.observe(bl::messwert_key_hex(mess_komponenten()), zelle_avx2(), "stem=1/result.csv", 1, "s", "u");
    (void)mess.observe(bl::messwert_key_hex(mess_komponenten_zweit()), zelle_avx2(), "stem=16/result.csv", 1, "s", "u");
    EXPECT_FALSE(mess.flush(t, "d", "u", owner("uuid-a"), 1800, uhr()).has_value());
    EXPECT_EQ(mess.pending_fresh(), 2u) << "Ein gescheiterter Store verliert den Batch nicht";

    // TP1-F1 auf dem MESS-Pfad: der Praefix "stem=1" darf "stem=16" NICHT mitreissen.
    EXPECT_EQ(mess.discard_fresh_with_pfad_prefix({"stem=1"}), 1u);
    EXPECT_EQ(mess.pending_fresh(), 1u);
    EXPECT_EQ(mess.discard_fresh_with_pfad_prefix({}), 0u);
}

// ===========================================================================
// (2) G-E6 -- der Versions-Tag und der Wire-Bruch 3 -> 4.
// ===========================================================================
TEST(Ge6VersionsTag, V4RoundtrippedByteStabilUndTraegtDenTag) {
    bl::BestandslogDocument d;
    d.genus       = bl::Genus::measurement;
    d.created_utc = "2026-08-03T12:00:00Z";
    bl::BestandEintrag e;
    e.key_sha512 = std::string(128, 'a');
    e.zelle      = zelle_avx2();
    e.pfad       = "p";
    e.bytes      = 7;
    e.stempel    = "[vereint,O2,avx2]";
    e.done_utc   = "2026-08-03T12:00:00Z";
    e.versions   = "search_algo@1.0.0.c;target_isa@1.0.0.c";
    d.bestand.push_back(e);

    auto const einmal = bl::emit_document(d);
    EXPECT_NE(einmal.find("syntax_version=\"4\""), std::string::npos);
    EXPECT_NE(einmal.find("versions=\"search_algo@1.0.0.c;target_isa@1.0.0.c\""), std::string::npos);
    auto const wieder = bl::parse_bestandslog(einmal);
    ASSERT_TRUE(wieder.has_value());
    EXPECT_EQ(bl::emit_document(*wieder), einmal);
    EXPECT_EQ(wieder->bestand[0].versions, e.versions);
}

TEST(Ge6VersionsTag, OhneTagBleibtDieAusgabeByteGleichZurV3Form) {
    // Der Grund, weshalb das Feld ANS ENDE gehoert und der Emitter es nur GEFUELLT schreibt.
    bl::BestandslogDocument d;
    d.created_utc = "2026-08-03T12:00:00Z";
    bl::BestandEintrag e;
    e.key_sha512 = std::string(128, 'b');
    e.pfad       = "p";
    e.done_utc   = "u";
    d.bestand.push_back(e);
    auto const text = bl::emit_document(d);
    EXPECT_EQ(text.find("versions="), std::string::npos) << "Ein leeres Tag erscheint NICHT in der Ausgabe";
    EXPECT_NE(text.find("done_utc=\"u\"/>"), std::string::npos);
}

TEST(Ge6VersionsTag, V3WirdTREUGelesenUndV4VonEinemV3LeserABGELEHNT) {
    // (a) Ein echtes v3-Dokument (ohne versions-Attribut) bleibt lesbar, das Tag ist leer.
    std::string const v3 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<bestandslog syntax_version=\"3\" semantics_version=\"1\" genus=\"measurement\" "
                           "doc_revision=\"5\" created_utc=\"u\">\n"
                           "  <bestand>\n"
                           "    <eintrag key_sha512=\"" +
                           std::string(128, 'c') +
                           "\" combo=\"\" opt=\"\" simd=\"\" pfad=\"p\" bytes=\"3\" stempel=\"s\" done_utc=\"u\"/>\n"
                           "  </bestand>\n  <reservierungen>\n  </reservierungen>\n</bestandslog>\n";
    auto const        p  = bl::parse_bestandslog(v3);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->syntax_version, 3);
    EXPECT_TRUE(bl::document_syntax_supported(*p));
    ASSERT_EQ(p->bestand.size(), 1u);
    EXPECT_TRUE(p->bestand[0].versions.empty());

    // (b) Die NEGATIV-PROBE: der v3-LESER lehnt v4 ab. Der heutige Leser kann nicht so tun, als
    // waere er alt -- also wird die Wache dort geprueft, wo sie sitzt: document_syntax_supported
    // vergleicht gegen kSyntaxVersion. Ein v4-Dokument gegen einen Leser mit kSyntaxVersion==3
    // entspricht hier einem v5-Dokument gegen den heutigen Leser (dieselbe Relation, +1).
    bl::BestandslogDocument zukunft;
    zukunft.syntax_version = bl::kSyntaxVersion + 1;
    EXPECT_FALSE(bl::document_syntax_supported(zukunft))
        << "Ein v3-Leser haette das versions-Attribut still geschluckt und einen versions-FREMDEN "
           "Bestand fuer den eigenen gehalten -- genau das verhindert der Bump";
    EXPECT_EQ(bl::kSyntaxVersion, 4);
}

TEST(Ge6VersionsTag, EinAbweichenderTagAendertDieIDENTITAETNICHT) {
    // Section 66-N3: keine Schluessel-Fusion. Sonst ergaebe derselbe Bestand mit und ohne Tag zwei
    // Eintraege -- ein Dedup-Drift, der Doppel-Bauten erzeugte.
    bl::BestandEintrag a;
    a.key_sha512         = std::string(128, 'd');
    a.zelle              = zelle_avx2();
    a.versions           = "search_algo@1.0.0.c";
    bl::BestandEintrag b = a;
    b.versions           = "search_algo@2.0.0.c";
    EXPECT_TRUE(bl::same_eintrag_identity(a, b));
    EXPECT_FALSE(bl::eintrag_identity_less(a, b));
    EXPECT_FALSE(bl::eintrag_identity_less(b, a));
    // ... und der Index schluesselt ueber dasselbe Tupel.
    auto const ka = bl::lager_key_from_entry(a);
    auto const kb = bl::lager_key_from_entry(b);
    ASSERT_TRUE(ka.has_value() && kb.has_value());
    EXPECT_EQ(*ka, *kb);
}

TEST(Ge6VersionsTag, MergeVerschmilztDieBeidenEintraegeZuEinem) {
    FakeStore               store;
    auto const              t = store.naht();
    bl::BestandslogDocument erst;
    erst.genus = bl::Genus::measurement;
    bl::BestandEintrag e;
    e.key_sha512 = std::string(128, 'e');
    e.zelle      = zelle_avx2();
    e.done_utc   = "2026-08-03T10:00:00Z";
    erst.bestand.push_back(e);
    ASSERT_TRUE(bl::store_document_merged(t, "d", erst).has_value());

    bl::BestandslogDocument zweit = erst;
    zweit.bestand[0].versions     = "search_algo@1.0.0.c";
    zweit.bestand[0].done_utc     = "2026-08-03T11:00:00Z";
    auto const geschrieben        = bl::store_document_merged(t, "d", zweit);
    ASSERT_TRUE(geschrieben.has_value());
    ASSERT_EQ(geschrieben->bestand.size(), 1u) << "Gleiche (key, zelle) -> EIN Eintrag, kein Dedup-Drift";
    EXPECT_EQ(geschrieben->bestand[0].versions, "search_algo@1.0.0.c") << "Der SPAETERE Eintrag gewinnt";
    EXPECT_GT(geschrieben->doc_revision, 0u);
}

// ===========================================================================
// (3) G-E7 -- die Dual-ccache-Konfigurationsnaht (CT-Strategy + Preflight).
// ===========================================================================
namespace {

bl::ZielKonfiguration konfig() {
    bl::ZielKonfiguration k;
    k.minio_alias   = "comdare";
    k.minio_praefix = "lager/binaries";
    k.nas_wurzel    = "/mnt/measure-drop";
    return k;
}

bl::AliasBekanntFn nur_comdare() {
    return [](std::string const& a) { return a == "comdare"; };
}

} // namespace

TEST(Ge7DualCcache, DefaultMatrixBinaryNachMinioMesswertAufsNas) {
    EXPECT_EQ(bl::BinaryStandardZiel::backend(), bl::LagerBackend::objekt_store);
    EXPECT_EQ(bl::MesswertStandardZiel::backend(), bl::LagerBackend::nas_ablage);
    EXPECT_EQ(bl::BinaryStandardZiel::backend_name(), "minio");
    EXPECT_EQ(bl::MesswertStandardZiel::backend_name(), "nas");

    auto const b = bl::BinaryStandardZiel::aufloesen(konfig(), "target_isa=amd64_v3/blatt", "perm.dll", nur_comdare());
    ASSERT_TRUE(b.ok()) << bl::to_string(b.fehler);
    EXPECT_EQ(b.ziel, "lager/binaries/target_isa=amd64_v3/blatt/perm.dll");

    auto const m = bl::MesswertStandardZiel::aufloesen(konfig(), "mess=vereint/blatt", "20260812-093011.xlsx");
    ASSERT_TRUE(m.ok()) << bl::to_string(m.fehler);
    EXPECT_EQ(m.ziel, "/mnt/measure-drop/mess=vereint/blatt/20260812-093011.xlsx");
}

TEST(Ge7DualCcache, DieMatrixIstGEGENTEILIGKonfigurierbar) {
    // Section 65 / KONSOLIDIERT:16 -- "beide gegenteilig konfigurierbar". Die Umschaltung ist ein
    // TEMPLATE-Argument, kein Laufzeit-Schalter: die Wahl steht im TYP.
    EXPECT_EQ(bl::BinaryAufNasZiel::backend(), bl::LagerBackend::nas_ablage);
    EXPECT_EQ(bl::MesswertAufMinioZiel::backend(), bl::LagerBackend::objekt_store);

    auto const b = bl::BinaryAufNasZiel::aufloesen(konfig(), "target_isa=amd64_v3", "perm.dll");
    ASSERT_TRUE(b.ok());
    EXPECT_EQ(b.ziel, "/mnt/measure-drop/target_isa=amd64_v3/perm.dll");

    auto const m = bl::MesswertAufMinioZiel::aufloesen(konfig(), "mess=vereint", "r.csv", nur_comdare());
    ASSERT_TRUE(m.ok());
    EXPECT_EQ(m.ziel, "lager/binaries/mess=vereint/r.csv");
    // Und die Genus-Zuordnung bleibt dabei erhalten (das Genus wandert nicht mit dem Backend).
    EXPECT_EQ(bl::BinaryAufNasZiel::genus(), bl::Genus::binary);
    EXPECT_EQ(bl::MesswertAufMinioZiel::genus(), bl::Genus::measurement);
}

TEST(Ge7DualCcacheNegativ, McAliasPreflightFaengtDieLokalerPfadFalle) {
    // L1: mc behandelt ein unbekanntes erstes Argument als LOKALEN PFAD -> ein Tippfehler im Alias
    // erzeugt klaglos ein Verzeichnis. Deshalb eine FEHLERKLASSE, nie ein Fallback.
    auto k               = konfig();
    k.minio_alias        = "tippfehler";
    auto const unbekannt = bl::BinaryStandardZiel::aufloesen(k, "p", "b", nur_comdare());
    EXPECT_EQ(unbekannt.fehler, bl::ZielFehler::alias_unbekannt);
    EXPECT_TRUE(unbekannt.ziel.empty()) << "Kein stiller lokaler Pfad";

    // Ohne Pruefer wird NICHT geraten.
    EXPECT_EQ(bl::BinaryStandardZiel::aufloesen(konfig(), "p", "b").fehler, bl::ZielFehler::alias_unbekannt);

    // Eine S3-/HTTP-URL ist KEIN Alias (mc-Alias-Doktrin).
    k.minio_alias = "https://minio.comdare.local";
    EXPECT_EQ(bl::BinaryStandardZiel::aufloesen(k, "p", "b", nur_comdare()).fehler, bl::ZielFehler::alias_ist_url);
    k.minio_alias = "s3://bucket";
    EXPECT_EQ(bl::BinaryStandardZiel::aufloesen(k, "p", "b", nur_comdare()).fehler, bl::ZielFehler::alias_ist_url);
    k.minio_alias = "";
    EXPECT_EQ(bl::BinaryStandardZiel::aufloesen(k, "p", "b", nur_comdare()).fehler, bl::ZielFehler::alias_leer);
}

TEST(Ge7DualCcacheNegativ, NasWurzelMussGesetztUndABSOLUTSein) {
    auto k       = konfig();
    k.nas_wurzel = "";
    EXPECT_EQ(bl::MesswertStandardZiel::aufloesen(k, "p", "b").fehler, bl::ZielFehler::nas_wurzel_leer);
    k.nas_wurzel = "relativ/drop";
    EXPECT_EQ(bl::MesswertStandardZiel::aufloesen(k, "p", "b").fehler, bl::ZielFehler::nas_wurzel_relativ)
        << "Eine relative Wurzel laege je nach CWD woanders -- im Zwei-Maschinen-Betrieb stille Divergenz";
    EXPECT_EQ(bl::MesswertStandardZiel::aufloesen(konfig(), "p", "").fehler, bl::ZielFehler::blatt_leer);
}

TEST(Ge7DualCcache, DieBackendWahlIstKeinLaufzeitSchalter) {
    // CT-Beweis: die zwei Ziele sind verschiedene TYPEN, und die Default-Matrix ist eine CT-Tabelle.
    static_assert(!std::is_same_v<bl::BinaryStandardZiel, bl::BinaryAufNasZiel>);
    static_assert(std::is_same_v<bl::StandardZiel<bl::Genus::binary>::type, bl::ObjektStoreBackend>);
    static_assert(std::is_same_v<bl::StandardZiel<bl::Genus::measurement>::type, bl::NasAblageBackend>);
    static_assert(bl::LagerZielBackend<bl::ObjektStoreBackend>);
    static_assert(bl::LagerZielBackend<bl::NasAblageBackend>);
    static_assert(bl::BinaryStandardZiel::backend() == bl::LagerBackend::objekt_store);
    EXPECT_EQ(bl::to_string(bl::LagerBackend::objekt_store), "objekt_store");
    EXPECT_EQ(bl::to_string(bl::LagerBackend::nas_ablage), "nas_ablage");
    EXPECT_EQ(bl::to_string(bl::ZielFehler::ok), "ok");
}
