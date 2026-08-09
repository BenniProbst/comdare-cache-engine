// test_lb0_lager_pfad_grammatik -- LB-0/LB-2/LB-3-Kern (F9-Paketschnitt, A1-Lager-Rest-Welle).
//
// TRAGENDE Abnahme der Pfad-Grammatik-Single-Source (lager_pfad_grammatik.hpp) UND des Baum-Writers
// beider Realm-Wurzeln (lager_baum_writer.hpp). Kein Smoke: jede Zusage des Designs bekommt hier
// entweder einen GOLDEN-STRING oder eine NEGATIV-PROBE.
//
// Die Anker sind 1:1 die Beispiele des A9-Design-Dossiers
// (docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md, Abschnitt 5 = drei Namens-Beispiele,
// Abschnitt 6.1/6.2 = Pfad-Kaskaden). Das ist die L5-Abnahme-Naht: A9 muss diese Datei OHNE Aenderung
// konsumieren koennen -- ein Fork der Grammatik faellt hier auf, nicht erst im Feld.
//
// K1 (Owner-Entscheid 09.08.2026) -- GATTUNG UND GENUS SIND DIE WURZELEBENEN. Beide Kaskaden tragen
// seither drei gemeinsame Ebenen vorne: gattung=<token>/genus=<token>/realm=<binaries|messdaten>.
// Fuer diese TU heisst das dreierlei, und alle drei sind Absicht:
//   * jeder Ebenen-INDEX unten ist um 3 verschoben (die Golden-Strings selbst sind unveraendert --
//     Owner: "REST wie gehabt"),
//   * die zwei L3-Hybrid-Tests sind NEU GESCHRIEBEN statt geloescht: eine aufgehobene Regel braucht
//     den Beleg fuer das Gegenteil, sonst heisst "aufgehoben" nur "ungeprueft",
//   * Abschnitt (4b) ist neu und traegt die GEGENEINGAENGE zu den neuen Ordnungsregeln.
//
// OE-B-DUMMY-LAGER: die Realm-Writer werden gegen ein echtes Temp-Verzeichnis gefahren; die
// "Binaries" sind Textdateien mit Stempel-String. Kein minio, kein Netz, kein mc.
//
// STEMPEL-HOMONYMIE (T2-A/F4-NB3, 2026-08-06): "Stempel" heisst hier der LAGER-/SIDECAR-Stempel --
// Blattinhalt plus .fingerprint/.version/.algos/.variant. Er ist NICHT der Plan-Stempel `|bau=` aus
// bestandslog::slice_plan_stamp (der Bau-Identitaets-Anker des Batch-Plans) und auch nicht der
// Versionierungs-Stempel, den eine Tier-Binary einkompiliert traegt. Drei Dinge, ein Wort: die
// unqualifizierte Rede von "dem Stempel" hat den F4-NB2-Fehlschluss ausgeloest, deshalb steht die
// Unterscheidung hier im Kopf und nicht im Gedaechtnis des Lesers.

#include "bestandslog/lager_baum_writer.hpp"
#include "bestandslog/lager_pfad_grammatik.hpp"
#include "comdare_test_tmp.hpp"           // #278/#24 + Posten 69: per-User-/per-Build-Temp-Wurzel
#include "support/oeb_stempel_zeilen.hpp" // OE-B-Stempel-Fixture + split_lines + Ruecklese (LB-6)

#include <anatomy/anatomy_base.hpp> // K1: AnatomyGattung/AnatomyGenus + gattung_of (FREMDES Orakel)
#include <cache_engine/abi/system_axis_order.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace bl  = comdare::cache_engine::builder::bestandslog;
namespace cea = comdare::cache_engine::anatomy;
namespace ct  = comdare::test;
namespace fs  = std::filesystem;

namespace {

// --- Ein Fake-Ablage-Backend: In-Memory-Baum, zaehlt die Verben (kein Dateisystem noetig). ---
struct FakeAblage {
    std::map<std::string, std::string> dateien;
    std::vector<std::string>           verzeichnisse;
    std::size_t                        mkdir_rufe   = 0;
    std::size_t                        write_rufe   = 0;
    bool                               mkdir_bricht = false;
    bool                               write_bricht = false;

    [[nodiscard]] bl::BaumAblage naht() {
        bl::BaumAblage a;
        a.verzeichnis_anlegen = [this](std::string const& p) {
            ++mkdir_rufe;
            if (mkdir_bricht) return false;
            verzeichnisse.push_back(p);
            return true;
        };
        a.datei_schreiben = [this](std::string const& p, std::string const& inhalt) {
            ++write_rufe;
            if (write_bricht) return false;
            dateien[p] = inhalt;
            return true;
        };
        a.datei_lesen = [this](std::string const& p) -> std::optional<std::string> {
            auto it = dateien.find(p);
            if (it == dateien.end()) return std::nullopt;
            return it->second;
        };
        a.existiert = [this](std::string const& p) {
            if (dateien.count(p) != 0) return true;
            for (auto const& d : verzeichnisse)
                if (d == p) return true;
            return false;
        };
        return a;
    }
};

// Die 18 Organ-Haupt-Achsen mit Beispiel-Werten. BEWUSST in einer anderen Reihenfolge geliefert als
// die Gruppen-Tabelle -- der Writer muss sie selbst ordnen (sonst haengt der Ordnername an der
// Aufruf-Reihenfolge des Hosts und ist nicht reproduzierbar).
std::vector<bl::AchsenWert> organ_18_gemischt() {
    return {
        {"persistence_target", "none"},
        {"search_algo", "prt_art"},
        {"queuing_q2", "none"},
        {"cache_traversal", "bfs"},
        {"node_type", "b_tree"},
        {"memory_layout", "soa"},
        {"path_compression", "none"},
        {"filter", "bloom"},
        {"serialization", "flat"},
        {"mapping", "direct"},
        {"allocator", "arena"},
        {"value_handle", "inline_v"},
        {"index_organization", "clustered"},
        {"migration_policy", "lru"},
        {"concurrency", "single"},
        {"prefetch", "off"},
        {"io_dispatch", "sync"},
        {"queuing_q1", "none"},
    };
}

std::vector<bl::AchsenWert> system_drei() {
    return {{"target_isa", "amd64_v3"}, {"operating_system", "linux"}, {"external_utils", "avx2"}};
}

// Der 128-hex-Anker: NICHT auf einen konkreten Digest geeicht (Risiko 1 des Bauplans: W10/E-24
// verschieben jeden kuenftigen v6-Key). Geprueft wird nur die FORM.
std::string hex128(char c) { return std::string(128, c); }

// Ein Temp-Verzeichnis je Test (OE-B-Dummy-Lager).
//
// POSTEN 69 / #278/#24 (nachgezogen 2026-08-06): die Wurzel kommt aus comdare::test::user_tmp_dir(),
// nicht direkt aus temp_directory_path(). Ein fester Name direkt unter /tmp ist fuer JEDEN User und
// JEDEN Worktree dieselbe Adresse; der Konstruktor raeumt sie per remove_all leer, also loeschten
// zwei gleichzeitig laufende ctest-Laeufe desselben Users einander mitten im Test die Dateien
// (genau die Flake-Klasse, wegen der comdare_test_tmp.hpp ueberhaupt existiert -- diese TU hatte die
// Haus-Regel bisher nicht befolgt). user_tmp_dir() liegt weiterhin UNTER temp_directory_path(): die
// Repo-/Golden-Neutralitaet der TU aendert sich dadurch nicht.
class TempLager {
public:
    TempLager() {
        auto const basis = ::comdare::test::user_tmp_dir() / "comdare_lb0_lager";
        fs::remove_all(basis);
        fs::create_directories(basis);
        pfad_ = basis.string();
    }
    ~TempLager() {
        std::error_code ec;
        fs::remove_all(fs::path{pfad_}, ec);
    }
    TempLager(TempLager const&)            = delete;
    TempLager& operator=(TempLager const&) = delete;

    [[nodiscard]] std::string const& pfad() const noexcept { return pfad_; }

private:
    std::string pfad_;
};

} // namespace

// ===========================================================================
// (1) GOLDEN-STRINGS der Grammatik -- die drei A9-Namens-Beispiele (Abschnitt 5).
//     Sie sind zusaetzlich als consteval-Anker im Header verdrahtet; hier stehen sie als LAUFZEIT-
//     Beleg, damit ein Leser die Zeichenkette sieht, die im Baum landet.
// ===========================================================================
TEST(Lb0Grammatik, A9NamensBeispiel1WallclockSweep) {
    std::array<bl::KvPaar, 3> const p{bl::KvPaar{"measurement_category", "wallclock"}, bl::KvPaar{"workload", "ycsb_a"},
                                      bl::KvPaar{"working_set_n", "sweep"}};
    auto const                      e = bl::blatt_dateiname("20260812", "093011", p, "xlsx");
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    EXPECT_FALSE(e.gekuerzt);
    EXPECT_EQ(e.text, "20260812-093011_measurement_category=wallclock+workload=ycsb_a+working_set_n=sweep.xlsx");
}

TEST(Lb0Grammatik, A9NamensBeispiel2OptLevelSweep) {
    std::array<bl::KvPaar, 2> const p{bl::KvPaar{"opt_level", "sweep"}, bl::KvPaar{"atomic128", "on"}};
    auto const                      e = bl::blatt_dateiname("20260812", "101500", p, "xlsx");
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    EXPECT_EQ(e.text, "20260812-101500_opt_level=sweep+atomic128=on.xlsx");
}

TEST(Lb0Grammatik, A9NamensBeispiel3OrganUnterPlusCsvFallback) {
    std::array<bl::KvPaar, 2> const p{bl::KvPaar{"node_fanout", "sweep"}, bl::KvPaar{"prefetch_distance", "4"}};
    auto const                      x = bl::blatt_dateiname("20260812", "114205", p, "xlsx");
    ASSERT_TRUE(x.ok());
    EXPECT_EQ(x.text, "20260812-114205_node_fanout=sweep+prefetch_distance=4.xlsx");
    // CSV-Fallback DERSELBEN Factory (A9-Doc Abschnitt 5, Beispiel 3): nur die Endung wechselt.
    auto const c = bl::blatt_dateiname("20260812", "114205", p, "csv");
    ASSERT_TRUE(c.ok());
    EXPECT_EQ(c.text, "20260812-114205_node_fanout=sweep+prefetch_distance=4.csv");
}

TEST(Lb0Grammatik, KeineDynamischeVariableErgibtNurDatumZeit) {
    // KERN 26.07. Section 6: konstante Variablen werden WEGGELASSEN. Bleibt keine uebrig, ist der
    // Name nur Datum+Uhrzeit -- das ist kein Fehler, sondern der Normalfall eines fixen Blattes.
    auto const e = bl::blatt_dateiname("20260812", "093011", {}, "xlsx");
    ASSERT_TRUE(e.ok());
    EXPECT_EQ(e.text, "20260812-093011.xlsx");
}

// ===========================================================================
// (2) GOLDEN-STRINGS der Ordner-Ebenen -- A9-Doc 6.1.
// ===========================================================================
TEST(Lb0Grammatik, A9PfadEbene1MessKombinatorik) {
    std::array<bl::KvPaar, 2> const p{bl::KvPaar{"mess", "vereint"}, bl::KvPaar{"load_framework", "on"}};
    auto const                      e = bl::kv_kette(p);
    ASSERT_TRUE(e.ok());
    EXPECT_EQ(e.text, "mess=vereint+load_framework=on");
}

TEST(Lb0Grammatik, A9PfadEbene2SystemRekombination) {
    std::array<bl::KvPaar, 3> const p{bl::KvPaar{"target_isa", "amd64_v3"}, bl::KvPaar{"operating_system", "linux"},
                                      bl::KvPaar{"external_utils", "avx2"}};
    auto const                      e = bl::kv_kette(p);
    ASSERT_TRUE(e.ok());
    EXPECT_EQ(e.text, "target_isa=amd64_v3+operating_system=linux+external_utils=avx2");
}

TEST(Lb0Grammatik, A9PfadGruppenOrdner01ReadPath) {
    std::array<bl::KvPaar, 2> const p{bl::KvPaar{"search_algo", "prt_art"}, bl::KvPaar{"cache_traversal", "bfs"}};
    auto const                      e = bl::gruppen_segment("01_read_path", p);
    ASSERT_TRUE(e.ok());
    EXPECT_EQ(e.text, "01_read_path=search_algo-prt_art+cache_traversal-bfs");
}

// ===========================================================================
// (3) NEGATIV-PROBEN der Grammatik (jede Zusage des Bauplans, literal).
// ===========================================================================
TEST(Lb0GrammatikNegativ, ZeichenklassenVerstossWirdKlassifiziertAbgewiesen) {
    std::array<bl::KvPaar, 1> const gross{bl::KvPaar{"workload", "YCSB_A"}};
    auto const                      a = bl::kv_kette(gross);
    EXPECT_FALSE(a.ok());
    EXPECT_EQ(a.fehler, bl::LagerPfadFehler::wert_zeichenklasse);
    EXPECT_TRUE(a.text.empty()) << "Ein abgewiesenes Segment darf keinen halben Text zurueckgeben";

    std::array<bl::KvPaar, 1> const slash{bl::KvPaar{"workload", "a/b"}};
    EXPECT_EQ(bl::kv_kette(slash).fehler, bl::LagerPfadFehler::wert_zeichenklasse);

    std::array<bl::KvPaar, 1> const plus{bl::KvPaar{"workload", "a+b"}};
    EXPECT_EQ(bl::kv_kette(plus).fehler, bl::LagerPfadFehler::wert_zeichenklasse);

    std::array<bl::KvPaar, 1> const gleich{bl::KvPaar{"workload", "a=b"}};
    EXPECT_EQ(bl::kv_kette(gleich).fehler, bl::LagerPfadFehler::wert_zeichenklasse);

    std::array<bl::KvPaar, 1> const achse{bl::KvPaar{"Work-Load", "a"}};
    EXPECT_EQ(bl::kv_kette(achse).fehler, bl::LagerPfadFehler::achse_zeichenklasse);
}

TEST(Lb0GrammatikNegativ, SanitisierungIstExplizitUndNiemalsImplizit) {
    // Die Bau-Funktion weist ab; erst die SICHTBARE Sanitisierung macht den Wert schreibbar.
    EXPECT_EQ(bl::sanitisiere_wert("YCSB_A"), "ycsb_a");
    EXPECT_EQ(bl::sanitisiere_wert("6.17.0-35"), "6.17.0-35");
    EXPECT_EQ(bl::sanitisiere_wert("gcc 15.3 (x)"), "gcc_15.3__x_");
    EXPECT_EQ(bl::sanitisiere_wert("a=b+c/d"), "a_b_c_d");
    // Laengen-stabil: ein Zeichen je Zeichen, damit zwei verschiedene Rohwerte nicht kollabieren.
    EXPECT_EQ(bl::sanitisiere_wert("a=b").size(), std::string_view{"a=b"}.size());
    EXPECT_NE(bl::sanitisiere_wert("a=b"), bl::sanitisiere_wert("a==b"));
}

TEST(Lb0GrammatikNegativ, ZweihundertByteUeberlaufErgibtKurznamenUndErhaeltDieVollKette) {
    // Eine Kette, die das Komponenten-Limit sicher sprengt.
    std::vector<std::string> namen;
    std::vector<bl::KvPaar>  paare;
    for (int i = 0; i < 20; ++i) namen.push_back("unterachse_nummer_" + std::to_string(i));
    for (auto const& n : namen) paare.push_back(bl::KvPaar{n, "ein_hinreichend_langer_wert"});
    auto const e = bl::kv_kette(paare);
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    EXPECT_TRUE(e.gekuerzt);
    EXPECT_LE(e.text.size(), bl::kMaxKomponenteBytes);
    EXPECT_TRUE(e.text.starts_with("H="));
    EXPECT_EQ(e.text.size(), std::string_view{"H="}.size() + bl::kKurznameHexLen);
    // Voll-Kette ERHALTEN (nie stilles Kuerzen) und der Kurzname DETERMINISTISCH.
    EXPECT_GT(e.voll_kette.size(), bl::kMaxKomponenteBytes);
    auto const zweiter = bl::kv_kette(paare);
    EXPECT_EQ(e.text, zweiter.text) << "Der Kurzname muss deterministisch sein (Hash, nicht Zaehler)";
    // Zwei VERSCHIEDENE Voll-Ketten ergeben zwei verschiedene Kurznamen.
    paare.back().wert = "ein_hinreichend_langer_wert2";
    EXPECT_NE(e.text, bl::kv_kette(paare).text);
}

TEST(Lb0GrammatikNegativ, DateinameUeberlaufZiehtDenKurznamenMitDatumUndZeit) {
    std::vector<std::string> namen;
    std::vector<bl::KvPaar>  paare;
    for (int i = 0; i < 20; ++i) namen.push_back("unterachse_nummer_" + std::to_string(i));
    for (auto const& n : namen) paare.push_back(bl::KvPaar{n, "ein_hinreichend_langer_wert"});
    auto const e = bl::blatt_dateiname("20260812", "093011", paare, "xlsx");
    ASSERT_TRUE(e.ok());
    EXPECT_TRUE(e.gekuerzt);
    EXPECT_EQ(e.text, "20260812-093011_H=" + bl::kurzname_hex(e.voll_kette) + ".xlsx");
    EXPECT_LE(e.text.size(), bl::kMaxKomponenteBytes);
}

TEST(Lb0GrammatikNegativ, DatumUndZeitFormWerdenGeprueft) {
    EXPECT_EQ(bl::blatt_dateiname("2026812", "093011", {}, "xlsx").fehler, bl::LagerPfadFehler::datum_form);
    EXPECT_EQ(bl::blatt_dateiname("2026081x", "093011", {}, "xlsx").fehler, bl::LagerPfadFehler::datum_form);
    EXPECT_EQ(bl::blatt_dateiname("20260812", "9301", {}, "xlsx").fehler, bl::LagerPfadFehler::zeit_form);
    EXPECT_EQ(bl::blatt_dateiname("20260812", "093011", {}, "").fehler, bl::LagerPfadFehler::endung_leer);
}

TEST(Lb0GrammatikNegativ, PraefixKollisionEins16WirdMitTrennerAufgeloest) {
    // TP1-F1-Klasse auf ORDNER-Ebene: "...=1" darf niemals "...=16" treffen.
    EXPECT_TRUE(bl::pfad_praefix_passt("node_fanout=1/blatt=x", "node_fanout=1"));
    EXPECT_FALSE(bl::pfad_praefix_passt("node_fanout=16/blatt=x", "node_fanout=1"));
    EXPECT_TRUE(bl::pfad_praefix_passt("node_fanout=16/blatt=x", "node_fanout=16"));
    EXPECT_TRUE(bl::pfad_praefix_passt("node_fanout=1", "node_fanout=1")) << "Ein Knoten ist sein eigenes Praefix";
    EXPECT_FALSE(bl::pfad_praefix_passt("node_fanout=1", "")) << "Ein leeres Praefix darf nie alles treffen";
}

TEST(Lb0Grammatik, K1HybridIstEinWertWieJederAndere) {
    // ERSETZT Lb0GrammatikNegativ.L3HybridStringBrichtHart (bis 09.08.2026). Die Aussage ist um 180
    // Grad gedreht, deshalb ist der Test NEU GESCHRIEBEN und nicht angepasst: L3 verbot den Token
    // "hybrid" auf jeder Ebene, K1 sortiert die Hybrid-Gattung als regulaere Gattung+Genus ein.
    // Der alte Test hier nur zu LOESCHEN waere die halbe Arbeit -- eine aufgehobene Regel braucht den
    // Beleg, dass jetzt das GEGENTEIL gilt, sonst ist "aufgehoben" bloss "ungeprueft".
    std::array<bl::KvPaar, 1> const wert{bl::KvPaar{"tier", "hybrid"}};
    EXPECT_TRUE(bl::kv_kette(wert).ok());
    EXPECT_EQ(bl::kv_kette(wert).text, "tier=hybrid");

    std::array<bl::KvPaar, 1> const achse{bl::KvPaar{"hybrid_tier", "on"}};
    EXPECT_TRUE(bl::kv_kette(achse).ok());
    EXPECT_EQ(bl::kv_kette(achse).text, "hybrid_tier=on");

    std::array<bl::KvPaar, 1> const gruppe{bl::KvPaar{"tier", "plain"}};
    auto const                      g = bl::gruppen_segment("hybrid_gruppe", gruppe);
    ASSERT_TRUE(g.ok()) << bl::to_string(g.fehler);
    EXPECT_EQ(g.text, "hybrid_gruppe=tier-plain");

    EXPECT_TRUE(bl::blatt_dateiname("20260812", "093011", wert, "xlsx").ok());

    // GEGENEINGANG (T-4): die Aufhebung ist ENG. Sie betrifft NUR das Namens-Verbot -- die
    // Zeichenklasse gilt unveraendert, ein "Hybrid" mit Grossbuchstaben faellt weiterhin durch.
    std::array<bl::KvPaar, 1> const gross{bl::KvPaar{"tier", "Hybrid"}};
    EXPECT_EQ(bl::kv_kette(gross).fehler, bl::LagerPfadFehler::wert_zeichenklasse);
    // Und der Paar-Trenner bleibt im Gruppen-Segment mehrdeutig, auch fuer Hybrid-Werte.
    std::array<bl::KvPaar, 1> const suffix{bl::KvPaar{"tier", "hybrid-so"}};
    EXPECT_EQ(bl::gruppen_segment("01_read_path", suffix).fehler, bl::LagerPfadFehler::gruppen_wert_trenner);
}

TEST(Lb0GrammatikNegativ, GruppenSegmentWeistDenPaarTrennerImWertAb) {
    // '-' ist in der Werte-Klasse legal, im GRUPPEN-Segment aber der Paar-Trenner -> mehrdeutig.
    std::array<bl::KvPaar, 1> const p{bl::KvPaar{"search_algo", "prt-art"}};
    EXPECT_EQ(bl::gruppen_segment("01_read_path", p).fehler, bl::LagerPfadFehler::gruppen_wert_trenner);
    // In der normalen kv-Kette (Ordner-Ebene 1/2, Dateiname) bleibt '-' erlaubt (Kernel-Versionen).
    std::array<bl::KvPaar, 1> const k{bl::KvPaar{"kernel", "6.17.0-35"}};
    EXPECT_TRUE(bl::kv_kette(k).ok());
}

TEST(Lb0Grammatik, PfadJoinIstZugleichDerObjektPraefix) {
    // LB-0: Baum-Pfad == minio-Objekt-Praefix. EINE Funktion, keine zweite Uebersetzung.
    std::vector<std::string> const ebenen{"mess=vereint+load_framework=on", "target_isa=amd64_v3", "blatt=x"};
    auto const                     p = bl::pfad_join(ebenen);
    EXPECT_EQ(p, "mess=vereint+load_framework=on/target_isa=amd64_v3/blatt=x");
    EXPECT_EQ(bl::objekt_key(p, "20260812-093011.xlsx"), p + "/20260812-093011.xlsx");
    EXPECT_EQ(bl::objekt_key("", "datei"), "datei") << "Ohne Praefix entsteht nie ein fuehrender Trenner";
    // Leere Ebenen erzeugen nie "//".
    std::vector<std::string> const mit_luecke{"a", "", "b"};
    EXPECT_EQ(bl::pfad_join(mit_luecke), "a/b");
}

// ===========================================================================
// (4) BAUM-WRITER -- Messdaten-Realm-Kaskade (A9-Doc 6.1, 11 Ebenen).
// ===========================================================================
namespace {

// K1: BEIDE Fixtures tragen DASSELBE Wurzel-Paar. Absicht -- so ist der einzige Unterschied zwischen
// den zwei Realm-Pfaden die realm-Ebene selbst. Verschiedene Gattungen hier waeren bequem, machten den
// Disjunktheits-Test unten aber wertlos: er wuerde dann die Gattung messen, nicht die Realm-Trennung.
inline constexpr bl::LagerWurzelPaar kWurzelMapSa{cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm};

bl::MessdatenBaumSpec messdaten_spec() {
    // 09.08.2026 (Warnungs-Runde 2, clang -Wmissing-field-initializers; RAUSCHEN, aber ehrlich
    // gemacht): die Spec steht jetzt als VOLLE designierte Liste da -- auch die drei absichtlich
    // leeren Unter-Ebenen sind benannt, statt still per Aggregat-Rest zu entstehen.
    bl::MessdatenBaumSpec s{.wurzel       = kWurzelMapSa,
                            .mess         = {{"mess", "vereint"}, {"load_framework", "on"}},
                            .system       = system_drei(),
                            .meta_metas   = {{"simd", "avx2"}},
                            .organ        = organ_18_gemischt(),
                            .haupt_blatt  = {{"blatt", hex128('a')}},
                            .mess_unter   = {},
                            .system_unter = {},
                            .organ_unter  = {}};
    return s;
}

} // namespace

TEST(Lb2MessdatenRealm, KaskadeTraegtDieElfEbenenInDerBindendenOrdnung) {
    auto const k = bl::MessdatenRealmPolicy::kaskade(messdaten_spec());
    ASSERT_TRUE(k.ok()) << bl::to_string(k.fehler) << "/" << bl::to_string(k.pfad_fehler);
    ASSERT_EQ(k.ebenen.size(), 11u) << "K1: 2 Wurzel (gattung, genus) + 1 realm + 1 Mess + 1 System + "
                                       "5 Organ-Gruppen + 1 Haupt-Blatt (Unter-Ebenen leer)";
    // K1 -- die drei gemeinsamen Wurzelebenen, VOR allem anderen.
    EXPECT_EQ(k.ebenen[0], "gattung=map");
    EXPECT_EQ(k.ebenen[1], "genus=search_algorithm");
    EXPECT_EQ(k.ebenen[2], "realm=messdaten");
    // ... und ab hier die Messdaten-Kaskade "wie gehabt" (Owner-Praezisierung), nur um 3 verschoben.
    EXPECT_EQ(k.ebenen[3], "mess=vereint+load_framework=on");
    // Meta-Metas HINTEN, hinter load_framework on/off (D-09).
    EXPECT_EQ(k.ebenen[4], "target_isa=amd64_v3+operating_system=linux+external_utils=avx2+simd=avx2");
    EXPECT_EQ(k.ebenen[5], "01_read_path=search_algo-prt_art+cache_traversal-bfs");
    EXPECT_EQ(k.ebenen[6], "02_layout=node_type-b_tree+memory_layout-soa+path_compression-none+filter-bloom+"
                           "serialization-flat");
    EXPECT_EQ(k.ebenen[7], "03_placement=mapping-direct+allocator-arena+value_handle-inline_v+"
                           "index_organization-clustered+migration_policy-lru");
    EXPECT_EQ(k.ebenen[8], "04_execution=concurrency-single+prefetch-off");
    EXPECT_EQ(k.ebenen[9], "05_write_path_io=queuing_q1-none+queuing_q2-none+io_dispatch-sync+"
                           "persistence_target-none");
    EXPECT_EQ(k.ebenen[10], "blatt=" + hex128('a'));
}

TEST(Lb2MessdatenRealm, DreiUnterEbenenKommenNurWennSieGebrauchtWerden) {
    auto s        = messdaten_spec();
    s.mess_unter  = {{"measurement_category", "wallclock"}};
    s.organ_unter = {{"node_fanout", "16"}};
    auto const k  = bl::MessdatenRealmPolicy::kaskade(s);
    ASSERT_TRUE(k.ok());
    ASSERT_EQ(k.ebenen.size(), 13u) << "system_unter bleibt leer -> die Ebene entfaellt";
    EXPECT_EQ(k.ebenen[11], "mess_unter=measurement_category-wallclock");
    EXPECT_EQ(k.ebenen[12], "organ_unter=node_fanout-16");
}

TEST(Lb2MessdatenRealm, OrganAchsenWerdenVonDerTabelleGeordnetNichtVomAufrufer) {
    auto  s1 = messdaten_spec();
    auto  s2 = messdaten_spec();
    auto& v  = s2.organ;
    std::reverse(v.begin(), v.end());
    auto const k1 = bl::MessdatenRealmPolicy::kaskade(s1);
    auto const k2 = bl::MessdatenRealmPolicy::kaskade(s2);
    ASSERT_TRUE(k1.ok() && k2.ok());
    EXPECT_EQ(k1.pfad(), k2.pfad()) << "Derselbe Werte-Satz MUSS denselben Ordner ergeben, egal in welcher "
                                       "Reihenfolge der Host ihn reicht";
}

TEST(Lb2MessdatenRealmNegativ, SystemAchsenAusserDerBindendenOrdnungBrechen) {
    auto s   = messdaten_spec();
    s.system = {{"operating_system", "linux"}, {"target_isa", "amd64_v3"}, {"external_utils", "avx2"}};
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::system_achsen_ordnung);

    s.system = {{"compiler", "gcc"}};
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::system_achse_unbekannt)
        << "compiler ist seit A3/O-8 Schritt 4 KEINE System-Haupt-Achse mehr";
}

TEST(Lb2MessdatenRealmNegativ, UnvollstaendigeOderFremdeOrganAchsenBrechen) {
    auto s = messdaten_spec();
    s.organ.pop_back();
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::organ_achse_fehlt);

    s = messdaten_spec();
    s.organ.push_back({"telemetry", "on"});
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::organ_achse_unbekannt)
        << "telemetry hat die binary_id-permutierende Komposition mit Bau-INC-2c verlassen";

    s              = messdaten_spec();
    s.organ.back() = {"search_algo", "zweitwert"};
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::organ_achse_doppelt);
}

TEST(Lb2MessdatenRealmNegativ, PflichtEbenenSindPflicht) {
    auto s = messdaten_spec();
    s.mess.clear();
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::ebene_fehlt);
    s = messdaten_spec();
    s.system.clear();
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::ebene_fehlt);
    s = messdaten_spec();
    s.haupt_blatt.clear();
    EXPECT_EQ(bl::MessdatenRealmPolicy::kaskade(s).fehler, bl::LagerBaumFehler::ebene_fehlt);
}

TEST(Lb2MessdatenRealm, K1HybridWertLaeuftAuchUeberDenWriterDurch) {
    // ERSETZT Lb2MessdatenRealmNegativ.HybridBrichtAuchUeberDenWriter (bis 09.08.2026): dieselbe
    // Eingabe, umgekehrte Erwartung. Der Writer hat keine eigene Hybrid-Meinung mehr.
    auto s       = messdaten_spec();
    s.mess_unter = {{"tier", "hybrid"}};
    auto const k = bl::MessdatenRealmPolicy::kaskade(s);
    ASSERT_TRUE(k.ok()) << bl::to_string(k.fehler) << "/" << bl::to_string(k.pfad_fehler);
    EXPECT_EQ(k.ebenen.back(), "mess_unter=tier-hybrid");
}

// ===========================================================================
// (4b) K1-GEGENEINGAENGE -- zu JEDER neuen Ordnungsregel ein Pfad, der sie VERLETZT (T-4).
//      Ohne diesen Abschnitt bewiese der Rest der Datei nur, dass gueltige Pfade durchlaufen.
// ===========================================================================
TEST(Lb2WurzelNegativ, GenusAusEinerFREMDENGattungWirdKlassifiziertAbgewiesen) {
    // DIE zentrale neue Ordnungsregel: die zwei Wurzelebenen sind eine HIERARCHIE, kein Nebeneinander.
    // Set gehoert zur Gattung Container -- unter gattung=map hat es nichts zu suchen. Ohne diese Wache
    // entstuende der Pfad "gattung=map/genus=set/..." klaglos: wohlgeformt, schreibbar, und eine LUEGE
    // ueber die Abstammung. Genau die Klasse Fehler, die nie klappert.
    bl::MessdatenBaumSpec s{.wurzel       = bl::LagerWurzelPaar{cea::AnatomyGattung::Map, cea::AnatomyGenus::Set},
                            .mess         = {{"mess", "vereint"}},
                            .system       = system_drei(),
                            .meta_metas   = {},
                            .organ        = organ_18_gemischt(),
                            .haupt_blatt  = {{"blatt", hex128('a')}},
                            .mess_unter   = {},
                            .system_unter = {},
                            .organ_unter  = {}};
    auto const            k = bl::MessdatenRealmPolicy::kaskade(s);
    EXPECT_EQ(k.fehler, bl::LagerBaumFehler::gattung_genus_unvereinbar);
    EXPECT_TRUE(k.pfad().empty()) << "Ein abgewiesenes Paar darf keinen halben Pfad zurueckgeben";

    // ORAKEL AUS FREMDER QUELLE (T-5): die Zuordnung wird NICHT aus dem Writer abgeschrieben, sondern
    // bei der Anatomie erfragt -- sie ist die Einzigquelle der Ebene-2->Ebene-1-Bindung.
    ASSERT_EQ(cea::gattung_of(cea::AnatomyGenus::Set), cea::AnatomyGattung::Container);
    ASSERT_NE(cea::gattung_of(cea::AnatomyGenus::Set), cea::AnatomyGattung::Map);

    // GEGENPROBE: dasselbe Genus unter SEINER Gattung laeuft durch -- die Wache weist das PAAR ab,
    // nicht das Genus. Ohne diese Haelfte waere "lehnt ab" auch mit einer kaputten Wache erfuellt.
    bl::MessdatenBaumSpec gut{.wurzel     = bl::LagerWurzelPaar{cea::AnatomyGattung::Container, cea::AnatomyGenus::Set},
                              .mess       = {{"mess", "vereint"}},
                              .system     = system_drei(),
                              .meta_metas = {},
                              .organ      = organ_18_gemischt(),
                              .haupt_blatt  = {{"blatt", hex128('a')}},
                              .mess_unter   = {},
                              .system_unter = {},
                              .organ_unter  = {}};
    auto const            ok = bl::MessdatenRealmPolicy::kaskade(gut);
    ASSERT_TRUE(ok.ok()) << bl::to_string(ok.fehler);
    EXPECT_EQ(ok.ebenen[0], "gattung=container");
    EXPECT_EQ(ok.ebenen[1], "genus=set");
}

TEST(Lb2WurzelNegativ, EinEnumWertOhneLagerTokenBrichtStattStillUnterUnknownZuLanden) {
    // Der zweite Gegeneingang: ein Wert, den die Anatomie gar nicht kennt (kuenftiger/fremder
    // Enumerator). Er hat KEINEN Lager-Token -- und bekommt deshalb auch keinen Ersatz-Ordner.
    // Genau das ist der Unterschied zu gattung_name(), das hier "Unknown" liefern wuerde: ein
    // schreibbarer Ordner, unter dem sich alles Unbekannte still sammelt.
    auto const fremd = static_cast<cea::AnatomyGenus>(200);
    ASSERT_EQ(cea::genus_name(fremd), "Unknown") << "Vorbedingung: 200 ist kein Enumerator der Anatomie";
    ASSERT_TRUE(bl::lager_genus_token(fremd).empty()) << "und hat folgerichtig keinen Lager-Token";

    bl::BinariesBaumSpec s{.wurzel     = bl::LagerWurzelPaar{cea::AnatomyGattung::Map, fremd},
                           .system     = system_drei(),
                           .meta_metas = {},
                           .organ      = organ_18_gemischt(),
                           .mess_typ   = {{"mess", "vereint"}}};
    auto const           k = bl::BinariesRealmPolicy::kaskade(s);
    EXPECT_EQ(k.fehler, bl::LagerBaumFehler::wurzel_token_fehlt)
        << "ANWESENHEIT vor BEZIEHUNG: der fehlende Token ist die erste Ursache, nicht die Unvereinbarkeit";
    EXPECT_TRUE(k.pfad().empty());
}

TEST(Lb2Wurzel, DieLagerTokenSindEINGEFROREN) {
    // Diese Token sind ab dem ersten eingelagerten Blatt Adressen auf der Platte. Sie stehen deshalb
    // hier EINZELN und WOERTLICH -- nicht als Schleife ueber die Tabelle, denn eine Schleife ueber die
    // geprueften Werte belegt nur, dass die Tabelle sich selbst gleicht (T-5: das Orakel darf nicht
    // aus dem Prueflung stammen). Wer einen Token aendert, aendert diese Zeilen sichtbar mit.
    EXPECT_EQ(bl::lager_gattung_token(cea::AnatomyGattung::Map), "map");
    EXPECT_EQ(bl::lager_gattung_token(cea::AnatomyGattung::Container), "container");
    EXPECT_EQ(bl::lager_gattung_token(cea::AnatomyGattung::Graph), "graph");
    EXPECT_EQ(bl::lager_genus_token(cea::AnatomyGenus::SearchAlgorithm), "search_algorithm");
    EXPECT_EQ(bl::lager_genus_token(cea::AnatomyGenus::Set), "set");
    EXPECT_EQ(bl::lager_genus_token(cea::AnatomyGenus::Sequence), "sequence");
    EXPECT_EQ(bl::lager_genus_token(cea::AnatomyGenus::Adapter), "adapter");
    EXPECT_EQ(bl::lager_genus_token(cea::AnatomyGenus::View), "view");

    // Der Token ist BEWUSST nicht der C++-Name: E-24 C7-1 hat den Enumerator SearchAlgorithm nach Map
    // umbenannt, weil damals nichts eingefroren war. Waere der Ordnername an gattung_name() gekoppelt,
    // haette diese Umbenennung jeden bestehenden Ast verwaisen lassen.
    constexpr auto kSa = cea::AnatomyGenus::SearchAlgorithm;
    EXPECT_NE(bl::lager_genus_token(kSa), cea::genus_name(kSa))
        << "search_algorithm != SearchAlgorithm -- Lager-Token und C++-Etikett sind entkoppelt";
    EXPECT_EQ(bl::sanitisiere_wert(cea::genus_name(kSa)), "searchalgorithm")
        << "und die bequeme Abkuerzung sanitisiere_wert(genus_name()) haette den Unterstrich verschluckt";
}

TEST(Lb2Wurzel, NENNER_JedeGattungUndJedesGenusDerAnatomieIstEinsortierbar) {
    // NENNER mit Grundgesamtheit IN DER AUSGABE: die Zahl kommt aus der Anatomie (Scan ueber den
    // gesamten uint8_t-Bereich mit gattung_name()/genus_name() als Zugehoerigkeits-Orakel), nicht aus
    // der Token-Tabelle. Waere die Tabelle die Quelle, waere die Quote definitionsgemaess 100 Prozent.
    std::size_t gattungen = 0, mit_gattung_token = 0, genera = 0, mit_genus_token = 0;
    for (int v = 0; v <= 255; ++v) {
        auto const ga = static_cast<cea::AnatomyGattung>(static_cast<std::uint8_t>(v));
        if (cea::gattung_name(ga) != std::string_view{"Unknown"}) {
            ++gattungen;
            if (!bl::lager_gattung_token(ga).empty()) ++mit_gattung_token;
        }
        auto const ge = static_cast<cea::AnatomyGenus>(static_cast<std::uint8_t>(v));
        if (cea::genus_name(ge) != std::string_view{"Unknown"}) {
            ++genera;
            if (!bl::lager_genus_token(ge).empty()) ++mit_genus_token;
        }
    }
    std::cout << "[K1-NENNER] Gattungen mit Lager-Token: " << mit_gattung_token << "/" << gattungen
              << " -- Genera mit Lager-Token: " << mit_genus_token << "/" << genera
              << " (Grundgesamtheit: Scan ueber alle 256 uint8_t-Werte, Zugehoerigkeit per "
                 "anatomy::gattung_name/genus_name)\n";
    // NACHGEZOGEN 09.08.2026 (Warnungs-Runde 1, Klasse REGRESSION): hier standen 3 und 5. Die
    // Anatomie fuehrt seit HY-A1 VIER Gattungen und SECHS Genera (HeuristikAdapter bzw.
    // FunctionInterfaceReroute). Diese beiden Zeilen sind eine ABSICHTLICHE Stolperkante: sie sollen
    // fallen, wenn die Anatomie waechst -- genau das ist passiert, und genau deshalb werden sie
    // NACHGEZOGEN und nicht etwa gegen die laufende Zahl aufgeweicht. Eine Zusicherung, die sich die
    // Grundgesamtheit selbst aus der Tabelle holt, koennte nie mehr fallen.
    EXPECT_EQ(gattungen, 4u) << "Stand 09.08.2026: Map, Container, Graph, HeuristikAdapter";
    EXPECT_EQ(genera, 6u)
        << "Stand 09.08.2026: SearchAlgorithm, Set, Sequence, Adapter, View, FunctionInterfaceReroute";
    EXPECT_EQ(mit_gattung_token, gattungen) << "K1: keine Gattung ohne Lager-Token";
    EXPECT_EQ(mit_genus_token, genera) << "K1: kein Genus ohne Lager-Token";
}

// ===========================================================================
// (5) BAUM-WRITER -- Binaries-Realm-Kaskade (A9-Doc 6.2, D-12, ABNAHME-5, LED-68b).
// ===========================================================================
namespace {

bl::BinariesBaumSpec binaries_spec() {
    // Volle designierte Liste (Warnungs-Runde 2, wie messdaten_spec oben): alle fuenf Ebenen benannt.
    bl::BinariesBaumSpec s{.wurzel     = kWurzelMapSa,
                           .system     = system_drei(),
                           .meta_metas = {{"simd", "avx2"}},
                           .organ      = organ_18_gemischt(),
                           .mess_typ   = {{"mess", "vereint"}}};
    return s;
}

} // namespace

TEST(Lb3BinariesRealm, WurzelIstDieSystemAchseUndMessTypLiegtZutiefst) {
    auto const k = bl::BinariesRealmPolicy::kaskade(binaries_spec());
    ASSERT_TRUE(k.ok()) << bl::to_string(k.fehler);
    ASSERT_EQ(k.ebenen.size(), 10u) << "K1: 2 Wurzel + 1 realm + 1 System + 5 Organ-Gruppen + 1 Mess-Typ";
    // K1 -- dieselben drei Wurzelebenen wie im Messdaten-Realm, nur der realm-Token unterscheidet sich.
    EXPECT_EQ(k.ebenen[0], "gattung=map");
    EXPECT_EQ(k.ebenen[1], "genus=search_algorithm");
    EXPECT_EQ(k.ebenen[2], "realm=binaries");
    // Owner: "Binary-Ordner ... branchen unter Gattung->Genus->Binary/Messung->REST wie gehabt."
    EXPECT_EQ(k.ebenen[3], "target_isa=amd64_v3+operating_system=linux+external_utils=avx2+simd=avx2");
    EXPECT_EQ(k.ebenen[4], "01_read_path=search_algo-prt_art+cache_traversal-bfs");
    EXPECT_EQ(k.ebenen.back(), "mess=vereint") << "D-12: der Mess-Typ ist der TIEFSTE Haupt-Achsen-Typ";
}

TEST(Lb3BinariesRealm, Abnahme5CebBinariesLiegenImSystemAchsenBlatt) {
    auto const spec = binaries_spec();
    auto const voll = bl::BinariesRealmPolicy::kaskade(spec);
    auto const ceb  = bl::BinariesRealmPolicy::ceb_blatt_ebenen(spec);
    ASSERT_TRUE(ceb.ok()) << bl::to_string(ceb.fehler);
    // K1-SCHWESTERPFLICHT (T-6): ceb_blatt_ebenen() baut seinen Satz eigenstaendig. Zoege man die drei
    // Wurzelebenen nur in kaskade() ein, waeren es hier weiterhin 1 Ebene -- und die Praefix-Zusage
    // von ABNAHME-5 waere LEISE gebrochen. Deshalb die Ebenen-ZAHL, nicht nur das Praefix-Praedikat.
    ASSERT_EQ(ceb.ebenen.size(), 4u) << "gattung, genus, realm, system";
    EXPECT_EQ(ceb.ebenen[0], "gattung=map");
    EXPECT_EQ(ceb.ebenen[2], "realm=binaries");
    EXPECT_EQ(ceb.ebenen[3], voll.ebenen[3]);
    // Und: der CEB-Praefix ist ein echtes Praefix des Tier-Pfades (mit Trenner-Semantik).
    EXPECT_TRUE(bl::pfad_praefix_passt(voll.pfad(), ceb.pfad()));
}

TEST(Lb3BinariesRealm, Led68bTestLogLiegtNebenDerBinary) {
    EXPECT_EQ(bl::test_log_neben("perm.dll"), "perm.dll.test.log");
    EXPECT_EQ(bl::test_log_neben("libperm.so"), "libperm.so.test.log");
}

TEST(Lb3BinariesRealm, BlattIdentitaetIstDerV6FingerprintNurFormGeprueft) {
    // Risiko-1-Auflage: NICHTS wird auf einen konkreten Digest geeicht (W10/E-24 verschieben sie).
    auto const gut = bl::blatt_segment_aus_fingerprint(hex128('a'));
    ASSERT_TRUE(gut.has_value());
    EXPECT_EQ(gut->achse, "blatt");
    EXPECT_EQ(gut->wert.size(), 128u);
    EXPECT_FALSE(bl::blatt_segment_aus_fingerprint(std::string(127, 'a')).has_value());
    EXPECT_FALSE(bl::blatt_segment_aus_fingerprint(std::string(128, 'A')).has_value()) << "Klein-Hex, kein Gross-Hex";
    EXPECT_FALSE(bl::blatt_segment_aus_fingerprint(std::string(128, 'g')).has_value());
    EXPECT_TRUE(bl::ist_fingerprint_hex(std::string(128, '0')));
}

TEST(Lb3BinariesRealm, DieZweiRealmsSindDisjunkt) {
    EXPECT_NE(bl::BinariesRealmPolicy::realm(), bl::MessdatenRealmPolicy::realm());
    EXPECT_EQ(bl::to_string(bl::LagerRealm::binaries), "binaries");
    EXPECT_EQ(bl::to_string(bl::LagerRealm::messdaten), "messdaten");
    // Derselbe Achsen-Satz ergibt in beiden Realms VERSCHIEDENE Pfade (keine stille Verschmelzung).
    auto const m = bl::MessdatenRealmPolicy::kaskade(messdaten_spec());
    auto const b = bl::BinariesRealmPolicy::kaskade(binaries_spec());
    EXPECT_NE(m.pfad(), b.pfad());
    // K1: die Trennung sitzt jetzt IN der Kaskade (Ebene 3) und nicht mehr im wurzel_-String, den der
    // Aufrufer reicht. Vorher war "die Realms sind disjunkt" eine Aussage ueber zwei Konstruktor-
    // Argumente des Tests -- ein Stellvertreter. Jetzt ist sie eine Aussage ueber den Baum selbst.
    ASSERT_GE(m.ebenen.size(), 3u);
    ASSERT_GE(b.ebenen.size(), 3u);
    EXPECT_EQ(m.ebenen[2], "realm=messdaten");
    EXPECT_EQ(b.ebenen[2], "realm=binaries");
    // Die zwei Wurzelebenen darueber sind identisch -- der Unterschied ist AUSSCHLIESSLICH der Realm.
    EXPECT_EQ(m.ebenen[0], b.ebenen[0]);
    EXPECT_EQ(m.ebenen[1], b.ebenen[1]);
    EXPECT_FALSE(bl::pfad_praefix_passt(m.pfad(), b.pfad()));
    EXPECT_FALSE(bl::pfad_praefix_passt(b.pfad(), m.pfad()));
}

// ===========================================================================
// (6) EINLAGERUNG (depth-first) -- FakeAblage zaehlt die Verben, OE-B-Dummy-Lager schreibt real.
// ===========================================================================
TEST(Lb2Einlagerung, DepthFirstLegtJedenKnotenAnBevorDasBlattGeschriebenWird) {
    FakeAblage f;
    auto const w = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
    auto const e = w.einlagern(messdaten_spec(), "20260812-093011.xlsx", "inhalt");
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    ASSERT_EQ(e.knoten.size(), 11u) << "K1: drei Ebenen mehr als vor dem Wurzel-Umbau";
    // Von der Wurzel abwaerts, jeder Knoten ein echtes Praefix des naechsten.
    for (std::size_t i = 1; i < e.knoten.size(); ++i)
        EXPECT_TRUE(bl::pfad_praefix_passt(e.knoten[i], e.knoten[i - 1])) << e.knoten[i];
    EXPECT_EQ(e.knoten.front(), "wurzel/gattung=map") << "K1: der ERSTE angelegte Knoten unter der Lager-Wurzel "
                                                         "ist die Gattung -- nicht der Realm und nicht die Mess-Ebene";
    EXPECT_EQ(e.knoten[1], "wurzel/gattung=map/genus=search_algorithm");
    EXPECT_EQ(e.knoten[2], "wurzel/gattung=map/genus=search_algorithm/realm=messdaten");
    EXPECT_EQ(e.knoten_pfad, e.knoten.back());
    EXPECT_EQ(e.blatt_pfad, e.knoten.back() + "/20260812-093011.xlsx");
    EXPECT_EQ(f.dateien.at(e.blatt_pfad), "inhalt");
    EXPECT_EQ(f.write_rufe, 1u) << "Genau EIN Blatt geschrieben";
    EXPECT_EQ(f.mkdir_rufe, 12u) << "Wurzel + 11 Ebenen";
}

TEST(Lb2EinlagerungNegativ, AblageFehlerWerdenKlassifiziertNichtVerschluckt) {
    {
        FakeAblage f;
        f.mkdir_bricht = true;
        auto const w   = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
        auto const e   = w.einlagern(messdaten_spec(), "x.xlsx", "i");
        EXPECT_EQ(e.fehler, bl::LagerBaumFehler::ablage_verzeichnis);
        EXPECT_EQ(f.write_rufe, 0u) << "Ohne Ast wird nie ein Blatt geschrieben";
    }
    {
        FakeAblage f;
        f.write_bricht = true;
        auto const w   = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
        auto const e   = w.einlagern(messdaten_spec(), "x.xlsx", "i");
        EXPECT_EQ(e.fehler, bl::LagerBaumFehler::ablage_datei);
    }
    {
        FakeAblage f;
        auto const w = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
        auto const e = w.einlagern(messdaten_spec(), "", "i");
        EXPECT_EQ(e.fehler, bl::LagerBaumFehler::ebene_fehlt);
        EXPECT_EQ(f.mkdir_rufe, 0u) << "Ein Blatt ohne Namen legt gar nichts erst an";
    }
}

TEST(Lb3Einlagerung, OeBDummyLagerBeideRealmsAufEchtemDateisystem) {
    // K1: BEIDE Writer bekommen DIESELBE Wurzel -- naemlich die des LAGERS. Vor K1 stand hier
    // temp.pfad()+"/binaries" bzw. +"/messdaten"; die Realm-Trennung kam damit aus zwei verschiedenen
    // Konstruktor-Argumenten DIESES TESTS und nicht aus dem Baum. Gleiche Wurzel zu reichen ist der
    // schaerfere Aufbau: trennen muss jetzt die Kaskade.
    TempLager  temp;
    auto const bin  = bl::make_binaries_baum_writer(bl::make_filesystem_ablage(), temp.pfad());
    auto const mess = bl::make_messdaten_baum_writer(bl::make_filesystem_ablage(), temp.pfad());

    // "Binary" = Textdatei mit Stempel-String (OE-B). Der Stempel ist seit LB-6 MEHRZEILIG (die
    // Form des echten g1_binary_version_block, support/oeb_stempel_zeilen.hpp) -- der frueher hier
    // stehende einzeilige String "[vereint,O2,avx2][a,b,c]+bt=Release" konnte die Owner-Forderung
    // "jede Zeile verbatim" nicht tragen. Die ZEILENWEISE Ruecklese steht im Test darunter.
    std::string const stempel = ct::oeb_stempel_block();
    auto const        b       = bin.einlagern(binaries_spec(), "perm.dll", stempel);
    ASSERT_TRUE(b.ok()) << bl::to_string(b.fehler);
    EXPECT_TRUE(fs::exists(fs::path{b.blatt_pfad}));
    // LED-68b: das Test-Log liegt NEBEN der Binary, im selben Knoten.
    auto const log = bin.einlagern(binaries_spec(), bl::test_log_neben("perm.dll"), "ctest: 0 failed\n");
    ASSERT_TRUE(log.ok());
    EXPECT_EQ(fs::path{log.blatt_pfad}.parent_path(), fs::path{b.blatt_pfad}.parent_path());

    auto const m = mess.einlagern(messdaten_spec(), "20260812-093011.xlsx", "PK-platzhalter");
    ASSERT_TRUE(m.ok()) << bl::to_string(m.fehler);
    EXPECT_TRUE(fs::exists(fs::path{m.blatt_pfad}));

    // Die zwei Realm-Aeste sind auch auf der Platte disjunkt -- und zwar UNTERHALB der gemeinsamen
    // Gattung/Genus-Wurzel, genau wie der Owner es angeordnet hat.
    std::string const gemeinsam = temp.pfad() + "/gattung=map/genus=search_algorithm";
    EXPECT_TRUE(bl::pfad_praefix_passt(b.blatt_pfad, gemeinsam)) << b.blatt_pfad;
    EXPECT_TRUE(bl::pfad_praefix_passt(m.blatt_pfad, gemeinsam)) << m.blatt_pfad;
    EXPECT_FALSE(bl::pfad_praefix_passt(m.blatt_pfad, gemeinsam + "/realm=binaries"));
    EXPECT_FALSE(bl::pfad_praefix_passt(b.blatt_pfad, gemeinsam + "/realm=messdaten"));
    // Und die gemeinsame Wurzel existiert wirklich als EIN Ordner (nicht zweimal nebeneinander).
    EXPECT_TRUE(fs::is_directory(fs::path{gemeinsam}));

    // Idempotenz: derselbe Spec ergibt denselben Knoten (kein zweiter Ast).
    auto const b2 = bin.einlagern(binaries_spec(), "perm.dll", stempel);
    ASSERT_TRUE(b2.ok());
    EXPECT_EQ(b2.knoten_pfad, b.knoten_pfad);
}

TEST(Lb3Einlagerung, OeBStempelWirdVomDateisystemZeilenweiseVerbatimZurueckgelesen) {
    // LB-6 / Owner-Verschaerfung 06.08.2026: "aus simulierten Textdokumenten die Stempel auszulesen
    // und JEDE ZEILE VERBATIM auszuwerten". Der Test darueber belegte bisher nur fs::exists -- dass
    // eine Datei DA ist, ist keine Aussage ueber ihren INHALT. Hier wird das Blatt geoeffnet,
    // gelesen, zerlegt und Zeile fuer Zeile einzeln geprueft.
    TempLager  temp;
    auto const bin = bl::make_binaries_baum_writer(bl::make_filesystem_ablage(), temp.pfad()); // K1: Lager-Wurzel

    std::string const stempel = ct::oeb_stempel_block();
    auto const        b       = bin.einlagern(binaries_spec(), "perm.dll", stempel);
    ASSERT_TRUE(b.ok()) << bl::to_string(b.fehler);
    ASSERT_TRUE(fs::exists(fs::path{b.blatt_pfad}));

    // RUECKLESE ueber einen ZWEITEN, vom Schreib-Weg unabhaengigen Pfad (std::ifstream statt
    // BaumAblage::datei_lesen): kaemen Schreiben und Lesen aus demselben Baustein, hoebe sich ein
    // symmetrischer Fehler darin im Vergleich selbst auf.
    auto const roh = ct::lies_blatt_datei(fs::path{b.blatt_pfad});
    ASSERT_TRUE(roh.has_value()) << "Blatt nicht oeffenbar: " << b.blatt_pfad;
    EXPECT_EQ(*roh, stempel) << "Der Blattinhalt ist byte-identisch zum eingelagerten Stempel";

    // ZEILENZAHL ZUERST: ohne sie faellt eine VERLORENE Zeile nicht auf -- die verbleibenden Zeilen
    // passen dann einzeln alle, und der Test bliebe gruen.
    EXPECT_EQ(std::count(roh->begin(), roh->end(), '\n'), static_cast<std::ptrdiff_t>(ct::kOeBStempelZeilenZahl));
    auto const zeilen = ct::split_lines(*roh);
    ASSERT_EQ(zeilen.size(), ct::kOeBStempelZeilenZahl);

    // JEDE Zeile EINZELN und VERBATIM -- vier benannte Erwartungen, keine Sammel-Behauptung.
    EXPECT_EQ(zeilen[0], ct::kOeBStempelZeile0);
    EXPECT_EQ(zeilen[1], ct::kOeBStempelZeile1);
    EXPECT_EQ(zeilen[2], ct::kOeBStempelZeile2);
    EXPECT_EQ(zeilen[3], ct::kOeBStempelZeile3);
    // ... und jede traegt ihr Label an Position 0: eine VERTAUSCHTE Reihenfolge faellt damit auch
    // dann auf, wenn alle vier Zeilen einzeln vorhanden sind.
    for (std::size_t i = 0; i < zeilen.size(); ++i)
        EXPECT_TRUE(zeilen[i].starts_with(ct::kOeBStempelLabels[i])) << "Zeile " << i << " = '" << zeilen[i] << "'";

    // LED-68b: das Test-Log NEBEN der Binary ist ebenfalls Text und wird ebenfalls zurueckgelesen.
    auto const log = bin.einlagern(binaries_spec(), bl::test_log_neben("perm.dll"), "ctest: 0 failed\n");
    ASSERT_TRUE(log.ok()) << bl::to_string(log.fehler);
    auto const log_roh = ct::lies_blatt_datei(fs::path{log.blatt_pfad});
    ASSERT_TRUE(log_roh.has_value());
    auto const log_zeilen = ct::split_lines(*log_roh);
    ASSERT_EQ(log_zeilen.size(), 1u);
    EXPECT_EQ(log_zeilen[0], "ctest: 0 failed");
    // Und das Log hat den Stempel NICHT ueberschrieben (zwei Blaetter, ein Knoten).
    auto const stempel_nochmal = ct::lies_blatt_datei(fs::path{b.blatt_pfad});
    ASSERT_TRUE(stempel_nochmal.has_value());
    EXPECT_EQ(*stempel_nochmal, stempel);
}

TEST(Lb2Einlagerung, GekuerzteEbenenReisenMitVollKetteZumAufrufer) {
    // Nie stilles Kuerzen: der Aufrufer bekommt (Kurzname, Voll-Kette) und legt sie ins Knoten-Log.
    auto s = messdaten_spec();
    for (int i = 0; i < 20; ++i) s.mess_unter.push_back({"unterachse_nummer_" + std::to_string(i), "langer_wert"});
    FakeAblage f;
    auto const w = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
    auto const e = w.einlagern(s, "x.xlsx", "i");
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    ASSERT_EQ(e.gekuerzt.size(), 1u);
    EXPECT_TRUE(e.gekuerzt.front().first.find("H=") != std::string::npos);
    EXPECT_GT(e.gekuerzt.front().second.size(), bl::kMaxKomponenteBytes);
}

TEST(Lb2Resolver, KaskadeUndKnotenPfadOhneJedesAnlegen) {
    // A9-Doc 6.1: "A9 legt NIE selbst Knoten an, sondern bezieht das Blatt-Verzeichnis vom
    // Baum-Writer/Resolver". Der Resolver-Weg fasst die Ablage nicht an.
    FakeAblage f;
    auto const w = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
    auto const p = w.knoten_pfad(messdaten_spec());
    EXPECT_FALSE(p.empty());
    EXPECT_TRUE(p.starts_with("wurzel/"));
    EXPECT_EQ(f.mkdir_rufe, 0u);
    EXPECT_EQ(f.write_rufe, 0u);
    // Und er stimmt mit dem Einlagerungs-Weg ueberein.
    auto const e = w.einlagern(messdaten_spec(), "x.xlsx", "i");
    ASSERT_TRUE(e.ok());
    EXPECT_EQ(p, e.knoten_pfad);
}
