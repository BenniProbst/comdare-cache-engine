// test_lazy_resume_binary.cpp — Goal-V4 G4 / M-CE-05: dedizierter Unit-Test der BINARY-Resume-Logik
// lazy_try_resume_binary (#139 Mess-Resume-Schutz).
//
// ZWECK: Das PROFIL-Pfad-Resume-Gate (test_profile_roundtrip) ist in CI, aber die BINARY-CSV-Resume-
// Entscheidung selbst — lazy_try_resume_binary aus cache_engine_builder_iterator.hpp — hatte keinen
// eigenen Unit-Test. Diese Funktion entscheidet je per-Binary-Ordner, ob eine bereits geschriebene
// result.csv fuer den AKTUELLEN Lauf VOLLSTAENDIG + KONFIGURATIONS-AKTUELL ist (Stamp-Prefix-Match +
// Header-Identitaet + Zeilenzahl) und daher UEBERSPRUNGEN werden darf (ihre Zeilen unveraendert in die
// globale CSV uebernommen). Jede Abweichung MUSS false liefern → ehrliche Neu-Messung, nie stiller
// Teil-/Stale-Uebernahme. Das gedachte Format (Writer, cache_engine_builder_iterator.hpp:844-867):
//   dir/result.csv       = lazy_csv_header() + je Zeile eine format_csv_row-Zeile (jede mit '\n')
//   dir/result.csv.stamp = "<resume_stamp_prefix>|rows=<N>\n"
//
// Getestete Kern-Resume-Pfade (Aufgabe M-CE-05):
//   (a) valider Binary-Zwischenstand → korrektes Resume: true + korrekte Zeilen in out_rows (Skip).
//   (b) korrupter/partieller/leerer Binary-Stand → sauberer Fallback: false, KEIN Crash/UB (Neustart).
//   (c) veralteter Stand (Version-/Schema-/Format-Mismatch) → false (Neustart, kein Stale-Resume).
//
// Bewusst deterministisch + ohne DLL/Messung/Threads (wie test_harness_compile). Temp-Dateien liegen
// user-eindeutig unter comdare::test::user_tmp_dir() (#278/#24, wie test_config_durability), NIE fix /tmp.
// Include-/Define-Satz gespiegelt von test_harness_compile (gleicher schwerer Host-Treiber-Header).

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <random>
#include <string>
#include <system_error>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace fs = std::filesystem;

namespace {

int g_fail = 0;

void check(char const* what, bool ok) {
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << '\n';
    if (!ok) ++g_fail;
}

// Eindeutiges Temp-Arbeitsverzeichnis je Fall (verhindert Kollisionen bei parallelen Laeufen auf
// demselben Runner). Frisch geleert, damit kein Rest eines Vorlaufs die Entscheidung verfaelscht.
[[nodiscard]] fs::path make_case_dir(char const* tag) {
    std::random_device rd;
    std::string const  token = std::to_string(rd()) + "_" + std::to_string(rd());
    fs::path const     dir   = ::comdare::test::user_tmp_dir() / ("comdare_resume_" + std::string{tag} + "_" + token);
    std::error_code    ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    return dir;
}

// Schreibt Inhalt exakt byteweise (auch ohne/mit CRLF, auch 0 Byte).
void write_file(fs::path const& p, std::string const& content) {
    std::ofstream o{p, std::ios::binary};
    o.write(content.data(), static_cast<std::streamsize>(content.size()));
}

// Baut den result.csv-Inhalt: `header` (lazy_csv_header, endet bereits auf '\n') + n_rows Marker-Zeilen.
// lazy_try_resume_binary parst NUR die Header-Zeile (Schema-Vergleich) + zaehlt die nicht-leeren Daten-
// zeilen; der Zeileninhalt ist fuer die Resume-Entscheidung irrelevant → deterministische Marker genuegen.
// Der von der Funktion zurueckgelieferte out_rows-String (Zeilen OHNE Header, je mit '\n') landet in
// *expected_rows, damit der Positiv-Fall die Byte-Identitaet der uebernommenen Zeilen pruefen kann.
[[nodiscard]] std::string make_csv(std::string const& header, std::size_t n_rows, std::string* expected_rows,
                                   char const* eol = "\n") {
    std::string csv = header;
    std::string rows;
    for (std::size_t i = 0; i < n_rows; ++i) {
        std::string const line = "datarow_" + std::to_string(i);
        csv += line;
        csv += eol;
        rows += line;
        rows += '\n'; // lazy_try_resume_binary normalisiert '\r' weg → out_rows hat immer nur '\n'
    }
    if (expected_rows != nullptr) *expected_rows = rows;
    return csv;
}

[[nodiscard]] std::string stamp_of(std::string const& prefix, std::string const& rows_field) {
    return prefix + "|rows=" + rows_field + "\n";
}

// ── (a) VALIDER Zwischenstand → korrektes Resume (Skip + Zeilen-Uebernahme) ──────────────────────────
void path_a_valid_resume(std::string const& prefix, std::string const& header) {
    std::cout << "== (a) valider Binary-Zwischenstand -> Resume (Skip, Zeilen uebernommen) ==\n";

    // a1: vollstaendig + aktuell (Stamp-Prefix-Match, Header-Identitaet, Zeilenzahl == rows) → true.
    {
        fs::path const dir = make_case_dir("a1");
        std::string    expected_rows;
        write_file(dir / "result.csv", make_csv(header, 3, &expected_rows));
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "3"));
        std::string out = "SENTINEL";
        bool const  ok  = ex::lazy_try_resume_binary(dir, prefix, &out);
        check("a1 valider Stand -> true (Resume)", ok);
        check("a1 out_rows == genau die 3 Datenzeilen (byte-identische Uebernahme)", out == expected_rows);
    }

    // a2: identisch valide, aber out_rows == nullptr → true, kein Crash (Aufrufer will nur das Ja/Nein).
    {
        fs::path const dir = make_case_dir("a2");
        write_file(dir / "result.csv", make_csv(header, 5, nullptr));
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "5"));
        bool const ok = ex::lazy_try_resume_binary(dir, prefix, nullptr);
        check("a2 valider Stand + out_rows=nullptr -> true, kein Crash", ok);
    }

    // a3: CRLF-Zeilenenden in der result.csv (Windows-geschriebener Stand) → '\r' wird normalisiert,
    //     Header-Vergleich + Zeilenzahl stimmen weiterhin → true, out_rows tragen nur '\n'.
    {
        fs::path const dir = make_case_dir("a3");
        std::string    expected_rows;
        std::string    csv = header; // Header selbst mit CRLF: Funktion strippt trailing '\r'
        // Header-Zeile auf CRLF umstellen: das abschliessende '\n' aus lazy_csv_header zu "\r\n".
        if (!csv.empty() && csv.back() == '\n') {
            csv.pop_back();
            csv += "\r\n";
        }
        csv += make_csv(std::string{}, 4, &expected_rows, "\r\n");
        write_file(dir / "result.csv", csv);
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "4"));
        std::string out;
        bool const  ok = ex::lazy_try_resume_binary(dir, prefix, &out);
        check("a3 CRLF-Stand -> true (\\r normalisiert)", ok);
        check("a3 out_rows CRLF-normalisiert (nur \\n)", out == expected_rows);
    }
}

// ── (b) KORRUPTER / PARTIELLER / LEERER Stand → sauberer Fallback (false, kein Crash/UB) ──────────────
void path_b_corrupt_fallback(std::string const& prefix, std::string const& header) {
    std::cout << "== (b) korrupter/partieller/leerer Stand -> sauberer Fallback (false, kein Crash) ==\n";

    // b1: leerer Ordner (weder csv noch stamp) → false (exists()-Gate).
    {
        fs::path const dir = make_case_dir("b1");
        check("b1 leerer Ordner -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b2: result.csv da, Stamp fehlt → false (halber Stand, nie ohne Stamp resumen).
    {
        fs::path const dir = make_case_dir("b2");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        check("b2 csv ohne stamp -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b3: Stamp da, result.csv fehlt → false.
    {
        fs::path const dir = make_case_dir("b3");
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "3"));
        check("b3 stamp ohne csv -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b4: leerer Stamp (0 Byte) + valide csv → false (kein Stamp-Inhalt).
    {
        fs::path const dir = make_case_dir("b4");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        write_file(dir / "result.csv.stamp", std::string{});
        check("b4 leerer Stamp -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b5: zu kurzer Stamp (kann Prefix + "|rows=" gar nicht enthalten) → false (Laengen-Gate).
    {
        fs::path const dir = make_case_dir("b5");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        write_file(dir / "result.csv.stamp", std::string{"x\n"});
        check("b5 zu kurzer Stamp -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b6: NICHT-NUMERISCHE Zeilenzahl im Stamp → false. KERN-No-Crash-Fall: std::stoull wirft, der
    //     Fang (catch(...)) muss sauber false liefern statt den Prozess zu terminieren.
    {
        fs::path const dir = make_case_dir("b6");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "NOTANUMBER"));
        check("b6 nicht-numerische rows -> false (stoull-Wurf gefangen, kein Crash)",
              !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b7: PARTIELLER/abgeschnittener Stand: Stamp verspricht 5 Zeilen, csv hat nur 2 → false.
    {
        fs::path const dir = make_case_dir("b7");
        write_file(dir / "result.csv", make_csv(header, 2, nullptr));
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "5"));
        check("b7 abgeschnittene csv (2<5) -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b8: rows=0 im Stamp (nie einen Null-Zeilen-Stand als 'fertig' resumen) → false.
    {
        fs::path const dir = make_case_dir("b8");
        write_file(dir / "result.csv", header); // nur Header, 0 Datenzeilen
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "0"));
        check("b8 rows=0 -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b9: komplett leere result.csv (0 Byte, keine Header-Zeile) + valider Stamp → false (kein getline).
    {
        fs::path const dir = make_case_dir("b9");
        write_file(dir / "result.csv", std::string{});
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "3"));
        check("b9 leere csv (kein Header) -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // b10: MEHR Zeilen als versprochen: Stamp rows=2, csv hat 4 Datenzeilen → false (Zeilenzahl != erwartet).
    {
        fs::path const dir = make_case_dir("b10");
        write_file(dir / "result.csv", make_csv(header, 4, nullptr));
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "2"));
        check("b10 zu viele Zeilen (4>2) -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }
}

// ── (c) VERALTETER Stand (Version-/Schema-/Format-Mismatch) → false (ehrliche Neu-Messung) ────────────
void path_c_stale_restart(std::string const& prefix, std::string const& stale_prefix, std::string const& header) {
    std::cout << "== (c) veralteter Stand (Version-/Schema-/Format-Mismatch) -> false (Neustart) ==\n";

    // c1: VERSION-/CONFIG-Mismatch: sonst vollstaendig valider Stand, aber der Stamp-Prefix stammt aus
    //     einer anderen Lauf-Konfiguration (z.B. andere build_version) → Prefix-Vergleich schlaegt fehl
    //     → false. Kein stiller Stale-Resume einer fremd-konfigurierten Messung.
    {
        fs::path const dir = make_case_dir("c1");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        write_file(dir / "result.csv.stamp", stamp_of(stale_prefix, "3"));
        check("c1 fremder Stamp-Prefix (Version-Mismatch) -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // c2: SCHEMA-Drift: Stamp-Prefix stimmt + Zeilenzahl stimmt, aber die Header-Zeile der csv ist ein
    //     ALTES/anderes Schema (nicht lazy_csv_header()) → Header-Identitaet scheitert → false.
    {
        fs::path const dir     = make_case_dir("c2");
        std::string    old_csv = "binary_id;setting;repetition;n_ops;total_ns;ns_per_op\n"; // veraltetes Schema
        old_csv += "datarow_0\ndatarow_1\ndatarow_2\n";
        write_file(dir / "result.csv", old_csv);
        write_file(dir / "result.csv.stamp", stamp_of(prefix, "3"));
        check("c2 veraltetes Header-Schema -> false", !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }

    // c3: FORMAT-Drift: Prefix stimmt, aber der Feld-Key nach dem Prefix ist NICHT "|rows=" (z.B. ein
    //     alter Stamp-Formatschluessel) → Format-Vergleich scheitert → false.
    {
        fs::path const dir = make_case_dir("c3");
        write_file(dir / "result.csv", make_csv(header, 3, nullptr));
        write_file(dir / "result.csv.stamp", prefix + "|cols=3\n"); // falscher Schluessel statt |rows=
        check("c3 falscher Format-Schluessel (|cols= statt |rows=) -> false",
              !ex::lazy_try_resume_binary(dir, prefix, nullptr));
    }
}

// Zwei realistische, unterschiedliche Resume-Stamp-Prefixe (via der echten lazy_resume_stamp_prefix-
// Single-Source): identisch bis auf build_version → modelliert exakt den "andere Version"-Stale-Fall.
[[nodiscard]] std::string make_prefix(char const* build_version) {
    ex::LazyRunConfig cfg;
    cfg.build_version = build_version;
    std::vector<ex::DynamicDim> const dims{
        ex::DynamicDim{"repetition", "repetition_index", {"0", "1", "2"}, "repetition"}};
    return ex::lazy_resume_stamp_prefix(cfg, dims);
}

} // namespace

int main() {
    std::cout << "==== M-CE-05 lazy_try_resume_binary Resume-Logik ====\n";

    std::string const header       = ex::lazy_csv_header();
    std::string const prefix       = make_prefix("resume-test-v1");
    std::string const stale_prefix = make_prefix("resume-test-v2"); // andere build_version → anderer Prefix

    // Vorbedingung: die zwei Prefixe MUESSEN sich unterscheiden, sonst testet (c1) nichts.
    check("Vorbedingung: Stamp-Prefixe v1 != v2 (Version-Mismatch modellierbar)", prefix != stale_prefix);
    check("Vorbedingung: lazy_csv_header nicht leer", !header.empty());

    path_a_valid_resume(prefix, header);
    path_b_corrupt_fallback(prefix, header);
    path_c_stale_restart(prefix, stale_prefix, header);

    std::cout << (g_fail == 0 ? "RESUME_OK\n" : "RESUME_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
