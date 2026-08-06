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
#include "support/oeb_stempel_zeilen.hpp" // OE-B-Stempel-Fixture + split_lines + Ruecklese (LB-6)

#include <cache_engine/abi/system_axis_order.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;
namespace ct = comdare::test;
namespace fs = std::filesystem;

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
class TempLager {
public:
    TempLager() {
        auto const basis = fs::temp_directory_path() / "comdare_lb0_lager";
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

TEST(Lb0GrammatikNegativ, L3HybridStringBrichtHart) {
    // Der L3-Entscheid ist nicht nur ein Kommentar: JEDE Ebene laeuft durch dieselbe Wache.
    std::array<bl::KvPaar, 1> const wert{bl::KvPaar{"tier", "hybrid"}};
    EXPECT_EQ(bl::kv_kette(wert).fehler, bl::LagerPfadFehler::hybrid_segment_verboten);

    std::array<bl::KvPaar, 1> const achse{bl::KvPaar{"hybrid_tier", "on"}};
    EXPECT_EQ(bl::kv_kette(achse).fehler, bl::LagerPfadFehler::hybrid_segment_verboten);

    std::array<bl::KvPaar, 1> const suffix{bl::KvPaar{"tier", "hybrid-so"}};
    EXPECT_EQ(bl::gruppen_segment("01_read_path", suffix).fehler, bl::LagerPfadFehler::hybrid_segment_verboten);

    std::array<bl::KvPaar, 1> const gruppe{bl::KvPaar{"tier", "plain"}};
    EXPECT_EQ(bl::gruppen_segment("hybrid_gruppe", gruppe).fehler, bl::LagerPfadFehler::hybrid_segment_verboten);

    EXPECT_EQ(bl::blatt_dateiname("20260812", "093011", wert, "xlsx").fehler,
              bl::LagerPfadFehler::hybrid_segment_verboten);

    // GEGENPROBE: die Wache darf nichts Fachfremdes mitreissen.
    std::array<bl::KvPaar, 1> const harmlos{bl::KvPaar{"tier", "hybridization"}};
    EXPECT_TRUE(bl::kv_kette(harmlos).ok());
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

bl::MessdatenBaumSpec messdaten_spec() {
    bl::MessdatenBaumSpec s;
    s.mess        = {{"mess", "vereint"}, {"load_framework", "on"}};
    s.system      = system_drei();
    s.meta_metas  = {{"simd", "avx2"}};
    s.organ       = organ_18_gemischt();
    s.haupt_blatt = {{"blatt", hex128('a')}};
    return s;
}

} // namespace

TEST(Lb2MessdatenRealm, KaskadeTraegtDieElfEbenenInDerBindendenOrdnung) {
    auto const k = bl::MessdatenRealmPolicy::kaskade(messdaten_spec());
    ASSERT_TRUE(k.ok()) << bl::to_string(k.fehler) << "/" << bl::to_string(k.pfad_fehler);
    ASSERT_EQ(k.ebenen.size(), 8u) << "1 Mess + 1 System + 5 Organ-Gruppen + 1 Haupt-Blatt (Unter-Ebenen leer)";
    EXPECT_EQ(k.ebenen[0], "mess=vereint+load_framework=on");
    // Meta-Metas HINTEN, hinter load_framework on/off (D-09).
    EXPECT_EQ(k.ebenen[1], "target_isa=amd64_v3+operating_system=linux+external_utils=avx2+simd=avx2");
    EXPECT_EQ(k.ebenen[2], "01_read_path=search_algo-prt_art+cache_traversal-bfs");
    EXPECT_EQ(k.ebenen[3], "02_layout=node_type-b_tree+memory_layout-soa+path_compression-none+filter-bloom+"
                           "serialization-flat");
    EXPECT_EQ(k.ebenen[4], "03_placement=mapping-direct+allocator-arena+value_handle-inline_v+"
                           "index_organization-clustered+migration_policy-lru");
    EXPECT_EQ(k.ebenen[5], "04_execution=concurrency-single+prefetch-off");
    EXPECT_EQ(k.ebenen[6], "05_write_path_io=queuing_q1-none+queuing_q2-none+io_dispatch-sync+"
                           "persistence_target-none");
    EXPECT_EQ(k.ebenen[7], "blatt=" + hex128('a'));
}

TEST(Lb2MessdatenRealm, DreiUnterEbenenKommenNurWennSieGebrauchtWerden) {
    auto s        = messdaten_spec();
    s.mess_unter  = {{"measurement_category", "wallclock"}};
    s.organ_unter = {{"node_fanout", "16"}};
    auto const k  = bl::MessdatenRealmPolicy::kaskade(s);
    ASSERT_TRUE(k.ok());
    ASSERT_EQ(k.ebenen.size(), 10u) << "system_unter bleibt leer -> die Ebene entfaellt";
    EXPECT_EQ(k.ebenen[8], "mess_unter=measurement_category-wallclock");
    EXPECT_EQ(k.ebenen[9], "organ_unter=node_fanout-16");
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

TEST(Lb2MessdatenRealmNegativ, HybridBrichtAuchUeberDenWriter) {
    auto s       = messdaten_spec();
    s.mess_unter = {{"tier", "hybrid"}};
    auto const k = bl::MessdatenRealmPolicy::kaskade(s);
    EXPECT_EQ(k.fehler, bl::LagerBaumFehler::grammatik);
    EXPECT_EQ(k.pfad_fehler, bl::LagerPfadFehler::hybrid_segment_verboten);
}

// ===========================================================================
// (5) BAUM-WRITER -- Binaries-Realm-Kaskade (A9-Doc 6.2, D-12, ABNAHME-5, LED-68b).
// ===========================================================================
namespace {

bl::BinariesBaumSpec binaries_spec() {
    bl::BinariesBaumSpec s;
    s.system     = system_drei();
    s.meta_metas = {{"simd", "avx2"}};
    s.organ      = organ_18_gemischt();
    s.mess_typ   = {{"mess", "vereint"}};
    return s;
}

} // namespace

TEST(Lb3BinariesRealm, WurzelIstDieSystemAchseUndMessTypLiegtZutiefst) {
    auto const k = bl::BinariesRealmPolicy::kaskade(binaries_spec());
    ASSERT_TRUE(k.ok()) << bl::to_string(k.fehler);
    ASSERT_EQ(k.ebenen.size(), 7u) << "1 System + 5 Organ-Gruppen + 1 Mess-Typ";
    EXPECT_EQ(k.ebenen.front(), "target_isa=amd64_v3+operating_system=linux+external_utils=avx2+simd=avx2");
    EXPECT_EQ(k.ebenen[1], "01_read_path=search_algo-prt_art+cache_traversal-bfs");
    EXPECT_EQ(k.ebenen.back(), "mess=vereint") << "D-12: der Mess-Typ ist der TIEFSTE Haupt-Achsen-Typ";
}

TEST(Lb3BinariesRealm, Abnahme5CebBinariesLiegenImSystemAchsenBlatt) {
    auto const spec = binaries_spec();
    auto const voll = bl::BinariesRealmPolicy::kaskade(spec);
    auto const ceb  = bl::BinariesRealmPolicy::ceb_blatt_ebenen(spec);
    ASSERT_TRUE(ceb.ok());
    ASSERT_EQ(ceb.ebenen.size(), 1u);
    EXPECT_EQ(ceb.pfad(), voll.ebenen.front());
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
    EXPECT_NE(bl::MessdatenRealmPolicy::kaskade(messdaten_spec()).pfad(),
              bl::BinariesRealmPolicy::kaskade(binaries_spec()).pfad());
}

// ===========================================================================
// (6) EINLAGERUNG (depth-first) -- FakeAblage zaehlt die Verben, OE-B-Dummy-Lager schreibt real.
// ===========================================================================
TEST(Lb2Einlagerung, DepthFirstLegtJedenKnotenAnBevorDasBlattGeschriebenWird) {
    FakeAblage f;
    auto const w = bl::make_messdaten_baum_writer(f.naht(), "wurzel");
    auto const e = w.einlagern(messdaten_spec(), "20260812-093011.xlsx", "inhalt");
    ASSERT_TRUE(e.ok()) << bl::to_string(e.fehler);
    ASSERT_EQ(e.knoten.size(), 8u);
    // Von der Wurzel abwaerts, jeder Knoten ein echtes Praefix des naechsten.
    for (std::size_t i = 1; i < e.knoten.size(); ++i)
        EXPECT_TRUE(bl::pfad_praefix_passt(e.knoten[i], e.knoten[i - 1])) << e.knoten[i];
    EXPECT_TRUE(e.knoten.front().starts_with("wurzel/"));
    EXPECT_EQ(e.knoten_pfad, e.knoten.back());
    EXPECT_EQ(e.blatt_pfad, e.knoten.back() + "/20260812-093011.xlsx");
    EXPECT_EQ(f.dateien.at(e.blatt_pfad), "inhalt");
    EXPECT_EQ(f.write_rufe, 1u) << "Genau EIN Blatt geschrieben";
    EXPECT_EQ(f.mkdir_rufe, 9u) << "Wurzel + 8 Ebenen";
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
    TempLager  temp;
    auto const bin  = bl::make_binaries_baum_writer(bl::make_filesystem_ablage(), temp.pfad() + "/binaries");
    auto const mess = bl::make_messdaten_baum_writer(bl::make_filesystem_ablage(), temp.pfad() + "/messdaten");

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

    // Die zwei Realm-Wurzeln sind auch auf der Platte disjunkt.
    EXPECT_FALSE(bl::pfad_praefix_passt(m.blatt_pfad, temp.pfad() + "/binaries"));
    EXPECT_FALSE(bl::pfad_praefix_passt(b.blatt_pfad, temp.pfad() + "/messdaten"));

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
    auto const bin = bl::make_binaries_baum_writer(bl::make_filesystem_ablage(), temp.pfad() + "/binaries");

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
