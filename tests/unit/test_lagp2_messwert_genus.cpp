// test_lagp2_messwert_genus.cpp -- LAG-P2: das MESSWERT-GENUS des Lagers, am Objekt.
//
// DER BEFUND, DEN DIESE TU SCHLIESST. Die drei Iterator-Felder mess_bestand_doc_key /
// mess_bestand_key_of / mess_bestand_versions hatten NULL externe Zuweiser (gemessen ueber ce
// libs/, apps/, tests/ und super Code/ -- die vier super-Treffer sind die ce-Submodul-Kopie
// derselben Datei). Folge: mess_bestandslog_active wurde NIE true, der Konsum
// (load ~:1958 / observe ~:2896 / flush ~:2162) lief nie, und das Lager fuehrte zwar Binaries,
// aber KEINE Messungen. Der Schreiber existierte (messwert_registrierung.hpp), ihm fehlte der
// Schluessel-Provider und die Belegung.
//
// WAS HIER GEPRUEFT WIRD -- AUSSAGEN, KEINE ANWESENHEIT:
//   (P2-a) Der reale Provider make_messwert_key_fn liefert fuer ein bin_dir mit gueltigem
//          `.fingerprint`-Sidecar einen 128-hex-Schluessel. Geprueft wird der WERT (Laenge + reines
//          Hex), nicht dass die Funktion existiert.
//   (P2-b) KEINE FUSION (Section 66-N3): der Messwert-Schluessel ist NICHT der Binary-Fingerprint.
//          Das ist die Doktrin-Aussage des Genus -- die vermessene Binary wird REFERENZIERT
//          (stempel), nicht in den Schluessel gemischt.
//   (P2-c) Die HARDWARE-IDENTITAET traegt: dieselbe Binary auf zwei Maschinen ergibt ZWEI
//          verschiedene Messwert-Schluessel. Ohne diese Eigenschaft wuerde prod2 eine Messung von
//          prod1 als eigene ausweisen.
//   (P2-d) Die BINARY-IDENTITAET traegt: zwei verschiedene Fingerprints auf derselben Maschine
//          ergeben zwei verschiedene Schluessel.
//   (P2-e) GENAU EIN EINTRAG: nach einer gemessenen Zelle traegt das measurement-Dokument im Store
//          genau einen <eintrag>, mit key_sha512 + Zelle + versions. Der Nenner 1 kommt aus dem
//          Vorgang (EINE gemessene Zelle), nicht aus dem geprueften Dokument.
//   (P2-f) DER OWNER-KERN vom 08.08.2026 woertlich: "Die operation bei Validen Messdaten ist skip
//          fuer die XLSX, sofern von der exakt gleichen binary gemessen wird." Der WIEDERHOLUNGS-
//          LAUF derselben Binary quittiert lager_hit und schreibt KEINEN zweiten Eintrag -- das
//          Dokument steht danach immer noch bei genau EINEM.
//   (P2-g) Das GENUS ist measurement, nicht binary -- sonst waeren die zwei Realms vermischt.
//   (P2-h) Eine ANDERE ZELLE derselben Binary ist KEIN Treffer (der Lager-Schluessel ist das Tupel
//          aus Digest UND Zelle, Section 62-NACHTRAG-4) -- avx2 und avx512 derselben Permutation
//          duerfen nicht wechselseitig wegdedupliziert werden.
//   (P2-i) Fehlendes/formwidriges Sidecar -> nullopt -> no_key: kein Eintrag ohne Anker
//          (fail-closed, dieselbe Doktrin wie bestand_key_of).
//
// DIE DREI-SCHICHT-NAHT (Fassade -> Entry -> Iterator) wird COMPILE-HART geprueft: die
// Feldtypen von ProfileRunArgs/RunProfileArgs muessen EXAKT die Iterator-Feldtypen sein. Ein
// abweichender Typ waere genau die stille Divergenz, die AUF-A4 fuer bestand_key_of beschreibt
// (FingerprintFn ist string->string, key_of ist path->optional<string> -- benachbarte Namen,
// verschiedene Bedeutung).
//
// Deterministisch, ohne minio/mc: FakeStore-Transport in-Memory (Muster
// test_f3_lager_key_provider_iterator). Kein DLL-Bau, keine Messung -- geprueft wird der
// Rueckschrieb-Weg, nicht der Messvorgang. Plain main (kein gtest), Return 0/1.

#include <builder/bestandslog/messwert_key_source.hpp>               // LAG-P2: der reale mess_bestand_key_of
#include <builder/bestandslog/messwert_registrierung.hpp>            // MesswertRunState / messwert_key_hex
#include <builder/build_orchestrator/fingerprint_sidecar.hpp>        // die EINE Suffix-Wahrheit (AUF-A2)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // LazyRunConfig (Typ-Vergleich)
#include <profile_facade/profile_run_entry.hpp>                      // RunProfileArgs (Typ-Vergleich)

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <type_traits>

namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

// ---------------------------------------------------------------------------
// DIE COMPILE-HARTE HAELFTE: die Naht traegt DENSELBEN Typ auf allen Schichten.
// ---------------------------------------------------------------------------
static_assert(std::is_same_v<decltype(ex::LazyRunConfig::mess_bestand_key_of),
                             decltype(tlz::RunProfileArgs::mess_bestand_key_of)>,
              "LAG-P2: RunProfileArgs::mess_bestand_key_of muss EXAKT der Iterator-Feldtyp sein "
              "(std::function<std::optional<std::string>(std::filesystem::path const&)>). Ein "
              "abweichender Typ ist die AUF-A4-Falle: FingerprintFn (string->string) und key_of "
              "(path->optional<string>) sind benachbarte Namen mit verschiedener Bedeutung.");
static_assert(std::is_same_v<decltype(ex::LazyRunConfig::mess_bestand_doc_key),
                             decltype(tlz::RunProfileArgs::mess_bestand_doc_key)>,
              "LAG-P2: RunProfileArgs::mess_bestand_doc_key muss EXAKT der Iterator-Feldtyp sein.");
static_assert(std::is_same_v<decltype(ex::LazyRunConfig::mess_bestand_versions),
                             decltype(tlz::RunProfileArgs::mess_bestand_versions)>,
              "LAG-P2: RunProfileArgs::mess_bestand_versions muss EXAKT der Iterator-Feldtyp sein.");

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == static_cast<A>(want));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

// In-Memory-Transport (Muster test_f3_lager_key_provider_iterator).
struct FakeStore {
    std::map<std::string, std::string> objs;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
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

constexpr char const* kMessDocKey = "bestandslog/test_lagp2_messwert.xml";

// Legt ein bin_dir an, wie es der Iterator nach einer gemessenen Zelle vorfindet: die (hier leere)
// Binary, ihr `.fingerprint`-Sidecar und die result.csv. Der Provider liest ausschliesslich das
// Sidecar -- die Existenz der result.csv ist die Bedingung des ITERATORS (kein Messwert-Eintrag
// ohne Messdatum), nicht die des Providers.
void lege_zelle_an(fs::path const& bin_dir, std::string const& sidecar_inhalt) {
    std::error_code ec;
    fs::create_directories(bin_dir, ec);
    { std::ofstream{bin_dir / "perm.dll", std::ios::trunc} << "// keine echte DLL noetig\n"; }
    {
        std::ofstream f{ex::fingerprint_sidecar_path(bin_dir / "perm.dll"), std::ios::trunc};
        f << sidecar_inhalt; // BYTEGENAU -- ohne abschliessenden Newline, wie write_fingerprint_sidecar
    }
    { std::ofstream{bin_dir / "result.csv", std::ios::trunc} << "kopf;zeile\n1;2\n"; }
}

[[nodiscard]] bool ist_128_hex(std::string const& s) {
    if (s.size() != 128) return false;
    for (char const c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// Zaehlt die <eintrag>-Elemente im Dokument-Text des Stores. Bewusst am ROHTEXT und nicht am
// geparsten Objekt: geprueft werden soll, was WIRKLICH im Store liegt.
[[nodiscard]] std::size_t zaehle_eintraege(std::string const& xml) {
    std::size_t n = 0;
    for (std::size_t p = xml.find("<eintrag"); p != std::string::npos; p = xml.find("<eintrag", p + 1)) ++n;
    return n;
}

} // namespace

int main() {
    std::cout << "==== LAG-P2: das Messwert-Genus des Lagers (Provider + Rueckschrieb + Owner-SKIP) ====\n";

    fs::path const  basis = ::comdare::test::user_tmp_dir() / "comdare_lagp2_messwert";
    std::error_code ec;
    fs::remove_all(basis, ec);

    // Zwei Zellen mit UNTERSCHIEDLICHEN Binary-Fingerprints (die Hex-Werte sind Test-Anker, nicht
    // aus einer Doku abgeschrieben) + eine Zelle mit formwidrigem Sidecar.
    std::string const fp_eins    = std::string(64, 'a') + std::string(64, '1');
    std::string const fp_zwei    = std::string(64, 'b') + std::string(64, '2');
    fs::path const    dir_eins   = basis / "zelle_eins";
    fs::path const    dir_zwei   = basis / "zelle_zwei";
    fs::path const    dir_kaputt = basis / "zelle_kaputt";
    lege_zelle_an(dir_eins, fp_eins);
    lege_zelle_an(dir_zwei, fp_zwei);
    lege_zelle_an(dir_kaputt, std::string(127, 'c')); // laenge_verstoss

    // Die Hardware-Identitaeten der beiden Maschinen des Clusters.
    auto const key_of_prod1 = bl::make_messwert_key_fn("prod1");
    auto const key_of_prod2 = bl::make_messwert_key_fn("prod2");

    // -- (P2-a) der WERT des Providers -------------------------------------------------------
    auto const k1_prod1 = key_of_prod1(dir_eins);
    check_true("(P2-a) Provider liefert einen Schluessel fuer eine Zelle mit gueltigem Sidecar", k1_prod1.has_value());
    check_true("(P2-a) und dieser Schluessel ist 128-stelliges Kleinbuchstaben-Hex",
               k1_prod1.has_value() && ist_128_hex(*k1_prod1));

    // -- (P2-b) KEINE FUSION mit dem Binary-Fingerprint (Section 66-N3) ----------------------
    check_true("(P2-b) der Messwert-Schluessel ist NICHT der Binary-Fingerprint (keine Fusion)",
               k1_prod1.has_value() && *k1_prod1 != fp_eins);

    // -- (P2-c) die Hardware-Identitaet traegt ------------------------------------------------
    auto const k1_prod2 = key_of_prod2(dir_eins);
    check_true("(P2-c) dieselbe Binary auf prod2 ergibt einen ANDEREN Messwert-Schluessel",
               k1_prod2.has_value() && k1_prod1.has_value() && *k1_prod2 != *k1_prod1);

    // -- (P2-d) die Binary-Identitaet traegt --------------------------------------------------
    auto const k2_prod1 = key_of_prod1(dir_zwei);
    check_true("(P2-d) eine andere Binary auf prod1 ergibt einen ANDEREN Messwert-Schluessel",
               k2_prod1.has_value() && k1_prod1.has_value() && *k2_prod1 != *k1_prod1);

    // -- (P2-i) fail-closed bei formwidrigem Sidecar -----------------------------------------
    check_true("(P2-i) formwidriges Sidecar (127 Zeichen) -> nullopt (kein Eintrag ohne Anker)",
               !key_of_prod1(dir_kaputt).has_value());

    // == Der Rueckschrieb: EIN Lager ueber ZWEI Laeufe derselben Binary =======================
    FakeStore                 store;
    bl::ZellKoordinaten const zelle{.combo = "default", .opt = "O2", .simd = "avx2"};
    std::string const         versions = "traversal@1.2.0c;alloc@3.1.0c";

    // -- LAUF 1: die Zelle wird zum ersten Mal gemessen ---------------------------------------
    {
        bl::MesswertRunState lager;
        lager.load(store.transport(), kMessDocKey);
        auto const aus = lager.observe(key_of_prod1(dir_eins).value_or(std::string{}), zelle, "zelle_eins/result.csv",
                                       42, "binary-id-eins", "2026-08-09T00:00:00Z", versions);
        check_true("(P2-e) LAUF 1: die frisch gemessene Zelle wird VORGEMERKT (fresh_register)",
                   aus == bl::MesswertOutcome::fresh_register);
        auto const neu = lager.flush(store.transport(), kMessDocKey, "2026-08-09T00:00:00Z",
                                     bl::make_lock_owner("uuid-lagp2", "prod1"));
        check_true("(P2-e) LAUF 1: der flush persistiert", neu.has_value());
        check_eq("(P2-e) LAUF 1: neu geschriebene Eintraege", neu.value_or(std::size_t{0}), std::size_t{1});
    }

    check_true("(P2-e) das measurement-Dokument liegt im Store", store.objs.count(kMessDocKey) == 1);
    std::string const doc1 = store.objs.count(kMessDocKey) ? store.objs[kMessDocKey] : std::string{};
    check_eq("(P2-e) das Dokument traegt GENAU EINEN <eintrag>", zaehle_eintraege(doc1), std::size_t{1});
    check_true("(P2-g) und es ist das measurement-Genus, nicht binary",
               doc1.find("genus=\"measurement\"") != std::string::npos);
    check_true("(P2-e) der Eintrag traegt den 128-hex key_sha512",
               k1_prod1.has_value() && doc1.find("key_sha512=\"" + *k1_prod1 + "\"") != std::string::npos);
    check_true("(P2-e) der Eintrag traegt die Zell-Koordinaten (combo/opt/simd)",
               doc1.find("O2") != std::string::npos && doc1.find("avx2") != std::string::npos &&
                   doc1.find("default") != std::string::npos);
    check_true("(P2-e) der Eintrag traegt das versions-Tag der Haupt-Achsen",
               doc1.find("versions=\"" + versions + "\"") != std::string::npos);

    // -- LAUF 2: DIESELBE Binary, DIESELBE Maschine, DIESELBE Zelle ---------------------------
    // Der Owner-KERN vom 08.08.2026 am Objekt. Das ist die Aussage dieses Tests.
    {
        bl::MesswertRunState lager;
        lager.load(store.transport(), kMessDocKey); // liest, was LAUF 1 geschrieben hat
        auto const aus = lager.observe(key_of_prod1(dir_eins).value_or(std::string{}), zelle, "zelle_eins/result.csv",
                                       42, "binary-id-eins", "2026-08-09T01:00:00Z", versions);
        check_true("(P2-f) OWNER-KERN: der Wiederholungslauf DERSELBEN Binary quittiert lager_hit",
                   aus == bl::MesswertOutcome::lager_hit);
        check_eq("(P2-f) und das Lager zaehlt genau EINEN Treffer", lager.lager_hits(), std::size_t{1});
        check_eq("(P2-f) es ist NICHTS vorgemerkt (kein zweiter Eintrag entsteht)", lager.pending_fresh(),
                 std::size_t{0});
        auto const neu = lager.flush(store.transport(), kMessDocKey, "2026-08-09T01:00:00Z",
                                     bl::make_lock_owner("uuid-lagp2", "prod1"));
        check_eq("(P2-f) der flush schreibt 0 neue Eintraege", neu.value_or(std::size_t{99}), std::size_t{0});
    }
    check_eq("(P2-f) das Dokument steht UNVERAENDERT bei genau EINEM <eintrag>",
             zaehle_eintraege(store.objs[kMessDocKey]), std::size_t{1});

    // -- (P2-h) eine ANDERE ZELLE derselben Binary ist KEIN Treffer ---------------------------
    {
        bl::MesswertRunState      lager;
        bl::ZellKoordinaten const zelle_avx512{.combo = "default", .opt = "O2", .simd = "avx512"};
        lager.load(store.transport(), kMessDocKey);
        auto const aus =
            lager.observe(key_of_prod1(dir_eins).value_or(std::string{}), zelle_avx512, "zelle_eins_avx512/result.csv",
                          42, "binary-id-eins", "2026-08-09T02:00:00Z", versions);
        check_true("(P2-h) dieselbe Binary in einer ANDEREN Zelle (avx512) ist KEIN Lager-Treffer",
                   aus == bl::MesswertOutcome::fresh_register);
    }

    std::cout << (g_fail == 0 ? "==== LAG-P2: ALLE PRUEFUNGEN GRUEN ====\n" : "==== LAG-P2: FEHLGESCHLAGEN ====\n");
    return g_fail == 0 ? 0 : 1;
}
