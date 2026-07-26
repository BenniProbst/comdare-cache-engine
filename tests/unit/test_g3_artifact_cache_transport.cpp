// test_g3_artifact_cache_transport -- G3 / #46b Lagerhaltung, Folge-A (T1).
//
// Der Objekt-per-Key-Weg des ArtifactCache + der eine Binder auf die BestandTransport-Naht.
//
// DETERMINISTISCH OHNE MINIO: getestet wird die INERTE Instanz -- ein default-konstruierter
// ArtifactCache hat weder Endpoint noch Bucket, minio_enabled() ist false. Das ist bewusst KEIN
// from_env(): der Test darf nicht davon abhaengen, dass die Umgebung zufaellig keine
// COMDARE_MINIO_*-Variablen traegt. Kein mc wird gestartet, kein Netz, keine TEMP-Datei entsteht.
//
// Was das beweist, ist genau das Sicherheitsversprechen der Naht: ohne konfigurierte Ebene B melden
// alle vier Verben ehrlich "nichts geschehen" (nullopt/false) statt eines falschen Erfolgs, und der
// daraus gebundene Transport verhaelt sich fuer das Bestandslog wie ein leerer Store. Der scharfe
// mc-Pfad ist Infrastruktur (Shellout) und gehoert nicht in einen Unit-Test.
//
// ---------------------------------------------------------------------------------------------
// G4b-1 / AUF-C2 (2026-07-26) -- ERWEITERUNG, drei neue Bloecke am Dateiende:
//   (1) make_fingerprint_key_fn: Sidecar vorhanden / fehlt / leer / mit '\n' / falsche Laenge /
//       Nicht-Hex. Dieser Block schreibt als EINZIGER in dieser TU echte Dateien -- unter
//       std::filesystem::temp_directory_path in ein pro-Test eindeutiges Verzeichnis, das im
//       Destruktor wieder verschwindet. Die Kopf-Aussage "keine TEMP-Datei entsteht" gilt damit
//       weiterhin fuer die vier ArtifactCache-/Binder-Tests, aber NICHT mehr fuer die ganze Datei.
//   (2) Eine Fake-Transport-Runde (In-Memory-Map, kein mc/minio/Netz): Union-per-id ueber
//       store_document_merged und die MERGE-Aufloesung ueber den Status-Rang (offen<released<done).
//   (3) pro_forma_deadline_epoch_s: die 30-Minuten-Frist als Wache.
//   (4) Die Ebenen-Praedikate minio_enabled/drop_enabled/inert, auf denen das Host-Gate steht
//       (AUF-B3 korrigiert): NUR-Ebene-C ist nicht inert und hat dennoch keinen Objekt-Store.
//       Dieser Block setzt Umgebungsvariablen und stellt sie per RAII wieder her.
//
// WAS DIESE TU AUSDRUECKLICH NICHT BEWEIST (AUF-C1, damit es niemand dafuer haelt): dass ohne
// COMDARE_BESTANDSLOG kein Transport gebunden und kein Schreibpfad betreten wird. Diese
// Entscheidung faellt in profile_run_facade / profile_run_entry / messung_driver -- keine davon
// liegt im Include-Satz dieses Test-Targets (builder | include | src | common). Der Nachweis ist
// ein literaler Lauf-Log ohne Env plus Byte-Diff der Emissionen, nicht dieser Test.
// ---------------------------------------------------------------------------------------------

#include "artifact_transport/artifact_cache.hpp"
#include "bestandslog/artifact_cache_transport.hpp"
#include "bestandslog/builder_registration.hpp"   // LagerRunState (der Konsument der gebundenen Naht)
#include "bestandslog/fingerprint_key_source.hpp" // G4b-1/AUF-A3: make_fingerprint_key_fn (der reale key_of)
#include "bestandslog/reservation_lifecycle.hpp"  // G4b-1/AUF-C2: pro_forma_deadline_epoch_s + mark_done

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib> // AUF-B3-Wache: setenv/unsetenv/getenv fuer die Ebenen-Praedikate
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace at = comdare::cache_engine::builder::artifact_transport;
namespace bl = comdare::cache_engine::builder::bestandslog;
namespace ex = comdare::cache_engine::builder::experiment; // fingerprint_sidecar_path (die Suffix-Wahrheit)

namespace {
constexpr char const* kDocKey = "bestandslog/binary_bestand.xml";

// Ein 128-hex-Fingerprint in genau der Form, die write_fingerprint_sidecar schreibt (Kleinbuchstaben,
// OHNE abschliessenden Newline).
[[nodiscard]] std::string gueltiger_fingerprint() { return std::string(128, 'a'); }

// Ein eindeutiges TEMP-Verzeichnis, das sich selbst wieder abraeumt. Kein mktemp-Shellout: der Name
// wird aus dem gtest-Testnamen gebildet, der innerhalb eines Laufs eindeutig ist.
class TempDir {
public:
    explicit TempDir(std::string const& name) : pfad_(std::filesystem::temp_directory_path() / ("g3_fpkey_" + name)) {
        std::error_code ec;
        std::filesystem::remove_all(pfad_, ec); // Rest eines abgebrochenen Vorlaufs
        std::filesystem::create_directories(pfad_, ec);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(pfad_, ec);
    }
    TempDir(TempDir const&)            = delete;
    TempDir& operator=(TempDir const&) = delete;

    [[nodiscard]] std::filesystem::path const& pfad() const noexcept { return pfad_; }
    // Der "Binary"-Pfad, den der Orchestrator gebaut haette -- die Datei selbst braucht nicht zu
    // existieren, key_of liest ausschliesslich das Sidecar daneben.
    [[nodiscard]] std::filesystem::path binary() const { return pfad_ / "perm.dll"; }

    // Schreibt <binary>.fingerprint mit EXAKT diesem Inhalt (keine Zusaetze, kein Newline).
    void schreibe_sidecar(std::string const& inhalt) const {
        std::ofstream f{ex::fingerprint_sidecar_path(binary()), std::ios::binary | std::ios::trunc};
        f << inhalt;
    }

private:
    std::filesystem::path pfad_;
};

// Ein Fake-Transport auf einer In-Memory-Map: deterministisch, kein mc, kein Netz, keine Datei.
// Genau das Muster, das der Kopf von bestandslog_lock.hpp fuer Tests vorsieht.
struct FakeStore {
    std::map<std::string, std::string> objekte;

    [[nodiscard]] bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& key) -> std::optional<std::string> {
            auto const it = objekte.find(key);
            if (it == objekte.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& key, std::string const& content) -> bool {
            objekte[key] = content;
            return true;
        };
        t.remove = [this](std::string const& key) -> bool {
            objekte.erase(key);
            return true;
        };
        t.stat = [this](std::string const& key) -> std::optional<bl::ObjectStat> {
            auto const it = objekte.find(key);
            if (it == objekte.end()) return std::nullopt;
            return bl::ObjectStat{.size = it->second.size(), .mtime_epoch_s = 0};
        };
        return t;
    }
};

// Eine Reservierung mit gesetzter id/status -- die uebrigen Felder sind fuer den Merge unerheblich.
[[nodiscard]] bl::BatchReservierung reservierung(std::string id, bl::BatchStatus status, std::string eta_s = {}) {
    bl::BatchReservierung r;
    r.id             = std::move(id);
    r.typ            = bl::BatchTyp::tier;
    r.maschine       = "prod1";
    r.status         = status;
    r.eta_s          = std::move(eta_s);
    r.reserviert_utc = "2026-07-26T00:00:00Z";
    return r;
}

// Setzt die vier Ebenen-Variablen deterministisch und stellt den Vorzustand im Destruktor wieder her
// (Muster wie test_cache_mc_timeout.cpp:65-92). Deterministisch UNABHAENGIG davon, was die Umgebung
// des Test-Laufs zufaellig traegt: jede der vier Variablen wird entweder gesetzt oder explizit
// entfernt, nie "gelassen wie sie ist".
class EnvGuard {
public:
    EnvGuard() {
        for (auto const* name : kNames) {
            char const* const alt = std::getenv(name);
            vorher_.emplace_back(name, alt != nullptr ? std::optional<std::string>{alt} : std::nullopt);
            ::unsetenv(name);
        }
    }
    ~EnvGuard() {
        for (auto const& [name, wert] : vorher_) {
            if (wert)
                ::setenv(name, wert->c_str(), 1);
            else
                ::unsetenv(name);
        }
    }
    EnvGuard(EnvGuard const&)            = delete;
    EnvGuard& operator=(EnvGuard const&) = delete;

    static void setze(char const* name, char const* wert) { ::setenv(name, wert, 1); }

private:
    static constexpr char const* kNames[] = {"COMDARE_MINIO_ENDPOINT", "COMDARE_MINIO_BUCKET", "COMDARE_MINIO_PREFIX",
                                             "COMDARE_MEASUREMENT_DROP_URL"};
    std::vector<std::pair<char const*, std::optional<std::string>>> vorher_;
};

// Ein Dokument mit genau diesen Reservierungen.
[[nodiscard]] bl::BestandslogDocument dokument(std::vector<bl::BatchReservierung> res, std::uint64_t revision = 1) {
    bl::BestandslogDocument d;
    d.genus          = bl::Genus::binary;
    d.created_utc    = "2026-07-26T00:00:00Z";
    d.doc_revision   = revision;
    d.reservierungen = std::move(res);
    return d;
}
} // namespace

// ---------------------------------------------------------------------------
// Die vier Verben auf einer inerten Instanz: kein Effekt, kein falscher Erfolg.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, ObjectVerbsAreInertWithoutMinio) {
    at::ArtifactCache const cache; // default == weder Ebene B noch Ebene C konfiguriert
    ASSERT_FALSE(cache.minio_enabled());
    ASSERT_TRUE(cache.inert());

    EXPECT_FALSE(cache.object_fetch(kDocKey).has_value());
    EXPECT_FALSE(cache.object_store(kDocKey, "<bestandslog/>"));
    EXPECT_FALSE(cache.object_remove(kDocKey));
    EXPECT_FALSE(cache.object_stat(kDocKey).has_value());
}

// ---------------------------------------------------------------------------
// Der Binder setzt alle vier Verben (die Naht ist vollstaendig belegt, nicht halb).
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, BinderPopulatesAllFourVerbs) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    EXPECT_TRUE(static_cast<bool>(t.fetch));
    EXPECT_TRUE(static_cast<bool>(t.store));
    EXPECT_TRUE(static_cast<bool>(t.remove));
    EXPECT_TRUE(static_cast<bool>(t.stat));
}

// Der gebundene Transport reicht die inerte Antwort unveraendert durch -- er erfindet nichts.
TEST(G3ArtifactCacheTransport, BoundTransportForwardsInertAnswers) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    EXPECT_FALSE(t.fetch(kDocKey).has_value());
    EXPECT_FALSE(t.store(kDocKey, "<bestandslog/>"));
    EXPECT_FALSE(t.remove(kDocKey));
    EXPECT_FALSE(t.stat(kDocKey).has_value());
}

// ---------------------------------------------------------------------------
// Das Bestandslog auf einem inerten Transport: leeres Lager, kein Phantom-Bestand, und der
// fehlgeschlagene Schreibweg wird als Store-Fehler SICHTBAR (nullopt) statt still geschluckt.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, LagerRunStateOnInertTransport) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.lager_size(), 0u); // nichts zu laden -> leeres Lager
    EXPECT_EQ(st.lager_hits(), 0u);

    // Der vierte flush-Parameter (LockOwner) kam mit dem N7-Schreib-Lock der Lager-Ertuechtigung hinzu;
    // gebildet wird er mit dem Haus-Helfer, genau wie an der Produktions-Aufrufstelle im Iterator
    // (make_lock_owner(bestand_owner_uuid, bestand_maschine)). Die Werte sind fuer diesen Test
    // gleichgueltig -- auf einem inerten Transport kommt es nie zu einem Lock-Erwerb.
    bl::LockOwner const ich = bl::make_lock_owner("test-owner", "testhost");

    // Ohne Beobachtung gibt es nichts zu schreiben -> 0 (kein Fehler, es wurde nichts versucht).
    auto const nichts = st.flush(t, kDocKey, "2026-07-26T00:00:00Z", ich);
    ASSERT_TRUE(nichts.has_value());
    EXPECT_EQ(*nichts, 0u);

    // Mit Beobachtung schlaegt der Schreibweg fehl und das MELDET der flush (nullopt), statt einen
    // Erfolg vorzutaeuschen.
    bl::ZellKoordinaten const zelle{.combo = "default", .opt = "O2", .simd = "avx2"};
    EXPECT_EQ(st.observe(std::string(128, 'a'), zelle, "tier/0.dll", 1, "", "utc"), bl::DedupOutcome::fresh_register);
    EXPECT_FALSE(st.flush(t, kDocKey, "2026-07-26T00:00:01Z", ich).has_value());
}

// ---------------------------------------------------------------------------
// ObjectMeta -> ObjectStat wird feldweise uebersetzt; mtime bleibt ehrlich 0 == unbekannt.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, ObjectMetaDefaultsAreHonest) {
    at::ObjectMeta const m;
    EXPECT_EQ(m.size, 0u);
    EXPECT_EQ(m.mtime_epoch_s, 0); // 0 == unbekannt, kein erfundener Zeitstempel

    bl::ObjectStat const s{.size = m.size, .mtime_epoch_s = m.mtime_epoch_s};
    EXPECT_EQ(s.size, m.size);
    EXPECT_EQ(s.mtime_epoch_s, m.mtime_epoch_s);
}

// ===========================================================================
// G4b-1 / AUF-A3 + AUF-C2 (1): make_fingerprint_key_fn -- der reale bestand_key_of.
//
// Die vier vom Verdikt namentlich verlangten Faelle plus die beiden Verstoss-Faelle. Jeder
// Fehlschlag ist nullopt und wird beim Konsumenten zu DedupOutcome::no_key -- die Binary ist fuer
// das Lager unsichtbar. Genau deshalb ist der '\n'-Fall der wichtigste: write_fingerprint_sidecar
// schreibt OHNE Newline, jede fremd erzeugte Datei hat einen, und ohne Trim waere das ein
// STILLER Totalausfall des Lagers.
// ===========================================================================
TEST(G3FingerprintKeySource, SidecarVorhandenLiefertDenSchluessel) {
    TempDir const t{"vorhanden"};
    auto const    erwartet = gueltiger_fingerprint();
    t.schreibe_sidecar(erwartet);

    auto const key_of = bl::make_fingerprint_key_fn();
    auto const key    = key_of(t.binary());
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, erwartet);
    EXPECT_EQ(key->size(), 128u);
}

TEST(G3FingerprintKeySource, SidecarFehltLiefertNullopt) {
    TempDir const t{"fehlt"}; // absichtlich KEIN schreibe_sidecar
    ASSERT_FALSE(std::filesystem::exists(ex::fingerprint_sidecar_path(t.binary())));

    auto const key_of = bl::make_fingerprint_key_fn();
    EXPECT_FALSE(key_of(t.binary()).has_value()); // fehlerklasse=sidecar_fehlt
}

TEST(G3FingerprintKeySource, SidecarLeerLiefertNullopt) {
    TempDir const t{"leer"};
    t.schreibe_sidecar(""); // Datei existiert, Inhalt leer (abgebrochener Schreibvorgang)
    ASSERT_TRUE(std::filesystem::exists(ex::fingerprint_sidecar_path(t.binary())));

    auto const key_of = bl::make_fingerprint_key_fn();
    EXPECT_FALSE(key_of(t.binary()).has_value()); // fehlerklasse=sidecar_leer
}

// Der Kern von AUF-A3: 129 Zeichen auf der Platte, 128 nach dem Trim -> GUELTIG.
TEST(G3FingerprintKeySource, SidecarMitNewlineWirdGetrimmtUndAkzeptiert) {
    TempDir const t{"newline"};
    auto const    erwartet = gueltiger_fingerprint();
    t.schreibe_sidecar(erwartet + "\n");
    ASSERT_EQ(std::filesystem::file_size(ex::fingerprint_sidecar_path(t.binary())), 129u);

    auto const key_of = bl::make_fingerprint_key_fn();
    auto const key    = key_of(t.binary());
    ASSERT_TRUE(key.has_value()); // ohne Trim waere das hier nullopt gewesen
    EXPECT_EQ(*key, erwartet);    // der Newline reist NICHT in den Lager-Schluessel
}

// CRLF + fuehrende Leerzeichen: derselbe Trim, beidseitig.
TEST(G3FingerprintKeySource, SidecarMitCrlfUndLeerraumWirdGetrimmt) {
    TempDir const t{"crlf"};
    auto const    erwartet = gueltiger_fingerprint();
    t.schreibe_sidecar("  " + erwartet + "\r\n");

    auto const key_of = bl::make_fingerprint_key_fn();
    auto const key    = key_of(t.binary());
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, erwartet);
}

TEST(G3FingerprintKeySource, FalscheLaengeUndNichtHexWerdenAbgelehnt) {
    auto const key_of = bl::make_fingerprint_key_fn();
    {
        TempDir const t{"zu_kurz"};
        t.schreibe_sidecar(std::string(127, 'a'));
        EXPECT_FALSE(key_of(t.binary()).has_value()); // fehlerklasse=laenge_verstoss
    }
    {
        TempDir const t{"zu_lang"};
        t.schreibe_sidecar(std::string(129, 'a'));    // 129 ECHTE Zeichen, kein Whitespace zum Trimmen
        EXPECT_FALSE(key_of(t.binary()).has_value()); // fehlerklasse=laenge_verstoss
    }
    {
        TempDir const t{"nicht_hex"};
        t.schreibe_sidecar(std::string(127, 'a') + "z");
        EXPECT_FALSE(key_of(t.binary()).has_value()); // fehlerklasse=zeichen_verstoss
    }
}

// Der Schluessel ist an DIE EINE Suffix-Wahrheit gebunden (AUF-A2): key_of liest genau den Pfad, den
// fingerprint_sidecar_path bildet -- ein danebenliegendes .version/.algos interessiert ihn nicht.
TEST(G3FingerprintKeySource, LiestNurDasFingerprintSidecar) {
    TempDir const t{"nur_fingerprint"};
    {
        std::ofstream f{t.binary().string() + ".version", std::ios::binary | std::ios::trunc};
        f << gueltiger_fingerprint(); // ein PERFEKT gueltiger 128-hex-Wert, aber in der falschen Datei
    }
    auto const key_of = bl::make_fingerprint_key_fn();
    EXPECT_FALSE(key_of(t.binary()).has_value());

    t.schreibe_sidecar(gueltiger_fingerprint());
    EXPECT_TRUE(key_of(t.binary()).has_value());
}

// ===========================================================================
// G4b-1 / AUF-C2 (2): eine Fake-Transport-Runde -- Union-per-id und Status-Rang-Merge.
//
// store_document_merged ist der EINE Schreibweg (fetch -> merge -> store, nie blinde Ersetzung).
// Was hier geprueft wird, ist der Grund, warum eine id-Wahl im planer_block folgenreich ist: der
// Merge ist MONOTON. Ein einmal terminales 'done' verwirft jedes spaetere 'offen' unter derselben
// id -- die Reservierung ist unter einer stabilen id also genau EINMAL belegbar.
// ===========================================================================
TEST(G3BestandslogFakeRound, UnionPerIdVereintZweiMaschinen) {
    FakeStore  store;
    auto const t = store.transport();

    // prod1 schreibt zuerst (Remote leer -> lokaler Stand mit gebumpter Revision).
    auto const nach_prod1 =
        bl::store_document_merged(t, kDocKey, dokument({reservierung("prod1/0", bl::BatchStatus::offen)}));
    ASSERT_TRUE(nach_prod1.has_value());
    ASSERT_EQ(nach_prod1->reservierungen.size(), 1u);
    EXPECT_EQ(nach_prod1->doc_revision, 2u); // 1 (lokal) + 1

    // prod2 schreibt danach mit EIGENER id -> Union, beide Records ueberleben.
    auto const nach_prod2 =
        bl::store_document_merged(t, kDocKey, dokument({reservierung("prod2/0", bl::BatchStatus::offen)}));
    ASSERT_TRUE(nach_prod2.has_value());
    ASSERT_EQ(nach_prod2->reservierungen.size(), 2u);
    EXPECT_EQ(nach_prod2->reservierungen[0].id, "prod1/0"); // nach id sortiert -> byte-stabiler Emit
    EXPECT_EQ(nach_prod2->reservierungen[1].id, "prod2/0");
}

TEST(G3BestandslogFakeRound, MergeLoestIdKonfliktUeberStatusRangAuf) {
    // offen(0) < released(1) < done(2). Der hoehere Rang gewinnt, in BEIDEN Argument-Reihenfolgen --
    // der Merge ist damit unabhaengig davon, welche Maschine zuerst schreibt.
    auto const a_offen = dokument({reservierung("prod1/0", bl::BatchStatus::offen)});
    auto const b_done  = dokument({reservierung("prod1/0", bl::BatchStatus::done)});

    auto const vorwaerts = bl::merge_documents(a_offen, b_done);
    ASSERT_EQ(vorwaerts.reservierungen.size(), 1u);
    EXPECT_EQ(vorwaerts.reservierungen[0].status, bl::BatchStatus::done);

    auto const rueckwaerts = bl::merge_documents(b_done, a_offen);
    ASSERT_EQ(rueckwaerts.reservierungen.size(), 1u);
    EXPECT_EQ(rueckwaerts.reservierungen[0].status, bl::BatchStatus::done); // Fortschritt geht NIE verloren

    // released liegt dazwischen.
    auto const c_released = dokument({reservierung("prod1/0", bl::BatchStatus::released)});
    EXPECT_EQ(bl::merge_documents(a_offen, c_released).reservierungen[0].status, bl::BatchStatus::released);
    EXPECT_EQ(bl::merge_documents(b_done, c_released).reservierungen[0].status, bl::BatchStatus::done);
}

// Bei GLEICHEM Rang gewinnt die kalibrierte (gefuellte eta_s) -- sonst stabil das erste Argument.
TEST(G3BestandslogFakeRound, GleicherRangBevorzugtDieKalibrierte) {
    auto const ohne_eta = dokument({reservierung("prod1/0", bl::BatchStatus::offen)});
    auto const mit_eta  = dokument({reservierung("prod1/0", bl::BatchStatus::offen, "1800")});

    EXPECT_EQ(bl::merge_documents(ohne_eta, mit_eta).reservierungen[0].eta_s, "1800");
    EXPECT_EQ(bl::merge_documents(mit_eta, ohne_eta).reservierungen[0].eta_s, "1800"); // stabil a
}

// Die Monotonie-Falle ausdruecklich als Wache: ein terminales 'done' laesst sich unter derselben id
// nicht wieder auf 'offen' zuruecksetzen. Das ist KEIN Defekt des Merges, sondern seine Zusage --
// und der Grund, warum eine wiederverwendete id kein zweites Mal reserviert werden kann.
TEST(G3BestandslogFakeRound, TerminalesDoneBleibtTerminal) {
    FakeStore  store;
    auto const t = store.transport();

    ASSERT_TRUE(bl::store_document_merged(t, kDocKey, dokument({reservierung("plan/0", bl::BatchStatus::done)})));
    auto const zweiter =
        bl::store_document_merged(t, kDocKey, dokument({reservierung("plan/0", bl::BatchStatus::offen)}));
    ASSERT_TRUE(zweiter.has_value());
    ASSERT_EQ(zweiter->reservierungen.size(), 1u);
    EXPECT_EQ(zweiter->reservierungen[0].status, bl::BatchStatus::done); // das 'offen' ist verworfen
}

// mark_done/mark_released sind die Zustands-Uebergaenge, die den Rang erzeugen.
TEST(G3BestandslogFakeRound, MarkDoneErzeugtDenTerminalenRang) {
    auto r = reservierung("prod1/0", bl::BatchStatus::offen);
    bl::mark_released(r);
    EXPECT_EQ(r.status, bl::BatchStatus::released);
    bl::mark_done(r);
    EXPECT_EQ(r.status, bl::BatchStatus::done);
}

// ===========================================================================
// G4b-1 / AUF-C2 (3): die 30-Minuten-Frist.
//
// pro_forma_deadline_epoch_s ist heute ohne Produktions-Aufrufer -- der Tier-Pfad uebergibt fuer
// reserviert_utc und pro_forma_bis_utc denselben now-Wert, die Frist ist dort also eine Sekunde
// spaeter abgelaufen. Diese Wache haelt die Rechenregel fest, damit der Fix daran andocken kann.
// ===========================================================================
TEST(G3ProFormaDeadline, DefaultSind30MinutenNachDerReservierung) {
    constexpr std::int64_t reserviert = 1'800'000'000;
    EXPECT_EQ(bl::pro_forma_deadline_epoch_s(reserviert), reserviert + 30 * 60);
    EXPECT_EQ(bl::pro_forma_deadline_epoch_s(reserviert), reserviert + 1800);
    EXPECT_EQ(bl::kProFormaMinutes, 30);
}

// ===========================================================================
// G4b-1 / AUF-B3 (korrigiert 2026-07-26): die EBENEN-PRAEDIKATE, auf denen das Host-Gate steht.
//
// Das Gate im messung_driver lautet COMDARE_BESTANDSLOG=="true" UND minio_enabled() UND owner-
// Identitaet gesetzt. Der Grund fuer minio_enabled() statt !inert() ist genau der Fall
// NUR-EBENE-C unten: dort ist inert() bereits false, obwohl KEIN Objekt-Store existiert. Diese
// Tests halten den Unterschied fest, damit die Gate-Bedingung nicht versehentlich zurueckgedreht
// wird. Die Gate-ENTSCHEIDUNG selbst liegt in main.cpp und ist in dieser TU nicht fuehrbar
// (AUF-C1) -- hier wird nur die Tatsache gesichert, auf die sie sich stuetzt.
// ===========================================================================
TEST(G3CacheEbenenPraedikate, NurEbeneCIstNichtInertAberOhneObjektStore) {
    EnvGuard const guard; // alle vier Ebenen-Vars entfernt
    EnvGuard::setze("COMDARE_MEASUREMENT_DROP_URL", "https://example.invalid/drop");
    auto const cache = at::ArtifactCache::from_env();

    EXPECT_TRUE(cache.drop_enabled());   // Ebene C konfiguriert
    EXPECT_FALSE(cache.minio_enabled()); // Ebene B NICHT
    EXPECT_FALSE(cache.inert());         // DIE FALLE: inert() ist hier schon false

    // Und trotzdem sind alle drei Objekt-Verben tot -- ein daraus gebundener Transport waere
    // vollstaendig belegt und vollstaendig wirkungslos.
    EXPECT_FALSE(cache.object_fetch(kDocKey).has_value());
    EXPECT_FALSE(cache.object_store(kDocKey, "<bestandslog/>"));
    EXPECT_FALSE(cache.object_remove(kDocKey));

    auto const t = bl::make_bestand_transport(cache);
    EXPECT_TRUE(static_cast<bool>(t.store));          // belegt ...
    EXPECT_FALSE(t.store(kDocKey, "<bestandslog/>")); // ... aber ohne Wirkung
}

TEST(G3CacheEbenenPraedikate, NurEbeneBErfuelltDieGateBedingung) {
    EnvGuard const guard;
    EnvGuard::setze("COMDARE_MINIO_ENDPOINT", "fakealias");
    EnvGuard::setze("COMDARE_MINIO_BUCKET", "fakebucket");
    auto const cache = at::ArtifactCache::from_env();

    EXPECT_TRUE(cache.minio_enabled()); // die Gate-Bedingung
    EXPECT_FALSE(cache.drop_enabled());
    EXPECT_FALSE(cache.inert());
}

// Endpoint ODER Bucket allein genuegt NICHT -- minio_enabled() verlangt beide (artifact_cache.hpp:221).
TEST(G3CacheEbenenPraedikate, HalbeMinioKonfigurationErfuelltDasGateNicht) {
    {
        EnvGuard const guard;
        EnvGuard::setze("COMDARE_MINIO_ENDPOINT", "fakealias"); // ohne Bucket
        EXPECT_FALSE(at::ArtifactCache::from_env().minio_enabled());
    }
    {
        EnvGuard const guard;
        EnvGuard::setze("COMDARE_MINIO_BUCKET", "fakebucket"); // ohne Endpoint
        EXPECT_FALSE(at::ArtifactCache::from_env().minio_enabled());
    }
}

TEST(G3CacheEbenenPraedikate, KeineEbeneIstInertUndErfuelltDasGateNicht) {
    EnvGuard const guard; // alle vier entfernt
    auto const     cache = at::ArtifactCache::from_env();

    EXPECT_FALSE(cache.minio_enabled());
    EXPECT_FALSE(cache.drop_enabled());
    EXPECT_TRUE(cache.inert());
}

TEST(G3ProFormaDeadline, FristLiegtEchtInDerZukunft) {
    // Der eigentliche Punkt: die Frist darf NIE gleich dem Reservierungs-Zeitpunkt sein, sonst ist
    // sie eine Sekunde spaeter abgelaufen und der Takeover-Schutz wirkungslos.
    constexpr std::int64_t reserviert = 1'800'000'000;
    EXPECT_GT(bl::pro_forma_deadline_epoch_s(reserviert), reserviert);
    EXPECT_GT(bl::pro_forma_deadline_epoch_s(reserviert, 1), reserviert);
    EXPECT_EQ(bl::pro_forma_deadline_epoch_s(reserviert, 0), reserviert); // 0 Minuten explizit = sofort faellig
}
