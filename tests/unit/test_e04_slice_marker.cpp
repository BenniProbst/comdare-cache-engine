// tests/unit/test_e04_slice_marker.cpp -- E-04-P1 (Marker-Familie v2): der Guard der Zeilen-Grammatik
// des Live-Fortschritts-Kanals.
//
// TRAGENDE AUSSAGE (Review-Auflage A7-1): jede Zeile der Familie ist FUER SICH ALLEIN zuordenbar --
// lane=, zelle= und fenster= sind PFLICHTFELDER, und der Aggregator-Schluessel ist das TUPEL
// (zelle, fenster). Der Guard weist das negativ nach: zwei Perms EINER Lane liefern IDENTISCHE
// fenster=-Werte; erst die zelle= trennt sie. Ein Aggregator, der auf fenster= allein (oder gar auf
// der Zeilen-Reihenfolge) keyt, verschmilzt sie -- genau der Befund, den die Auflage schliesst.
//
// Zweite Aussage (Lehre "gruene Tests zementieren alte Ordnung"): die Tee-Filter-Erwartung wird aus
// kSliceMarkerTraceMarken ABGELEITET, nie handgelistet -- eine vierte Marke faellt damit automatisch
// in die Filter-Pflicht statt still am Trace vorbeizulaufen.

#include <builder/experiment_tree/slice_marker.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace ex = ::comdare::cache_engine::builder::experiment;

namespace {

int g_fail = 0;

void check_true(char const* was, bool ok) {
    std::cout << (ok ? "  OK   " : "  FAIL ") << was << "\n";
    if (!ok) ++g_fail;
}

void check_eq(char const* was, std::string const& ist, std::string const& soll) {
    bool const ok = ist == soll;
    std::cout << (ok ? "  OK   " : "  FAIL ") << was << "\n";
    if (!ok) std::cout << "         ist='" << ist << "'\n         soll='" << soll << "'\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] bool enthaelt(std::string const& heu, std::string_view nadel) {
    return heu.find(nadel) != std::string::npos;
}

ex::MarkerKontext kontext_amd_o2() { return ex::MarkerKontext{"amd", "[O2,avx2][a,b,c]", "[all]"}; }

} // namespace

int main() {
    std::cout << "==== E-04-P1: Marker-Familie v2 (Pflichtfelder + Aggregator-Key) ====\n";

    // -- Teil 1: die exakte Zeilen-Grammatik des KOPFES (Feld-Ordnung wie die Shell-Testate) -------
    {
        std::string const kopf = ex::marker_kopf(ex::kMarkePlanTestat, kontext_amd_o2(), "2026-08-02T10:11:12Z", "bau",
                                                 ex::marker_fenster(4096, 4096));
        check_eq("(1) KOPF byte-genau", kopf,
                 std::string{"[PLAN-TESTAT] ts=2026-08-02T10:11:12Z lane=amd zelle=[O2,avx2][a,b,c] ceb=[all] "
                             "phase=bau fenster=4096:4096"});
    }

    // -- Teil 2: PFLICHTFELDER verschwinden NIE -- ein leerer Wert wird ehrlich benannt ------------
    {
        std::string const kopf =
            ex::marker_kopf(ex::kMarkeBilanzTestat, ex::MarkerKontext{}, "", "bau", ex::marker_fenster(0, 0));
        check_true("(2) lane= bleibt in der Zeile (Sentinel statt Fehlen)", enthaelt(kopf, " lane=unbelegt"));
        check_true("(2) zelle= bleibt in der Zeile (Sentinel statt Fehlen)", enthaelt(kopf, " zelle=unbelegt"));
        check_true("(2) ceb= bleibt in der Zeile (Sentinel statt Fehlen)", enthaelt(kopf, " ceb=unbelegt"));
        check_true("(2) ts= bleibt in der Zeile (Sentinel statt Fehlen)", enthaelt(kopf, " ts=unbelegt"));
        // fenster= ist immer belegt (0:0 ist eine ehrliche Angabe, kein Sentinel).
        check_true("(2) fenster= traegt das leere Fenster als Zahl-Paar", enthaelt(kopf, " fenster=0:0"));
    }

    // -- Teil 3: Saeuberung -- Trennzeichen falten, Legenden-Zeichen bleiben -----------------------
    {
        check_eq("(3) Leerzeichen faltet auf '_'", ex::marker_wert("a b"), std::string{"a_b"});
        check_eq("(3) '=' faltet auf '_' (Feld-Trenner)", ex::marker_wert("a=b"), std::string{"a_b"});
        check_eq("(3) Zeilenumbruch faltet auf '_'", ex::marker_wert("a\nb"), std::string{"a_b"});
        check_eq("(3) Legenden-Zeichen bleiben erhalten", ex::marker_wert("[O2,avx2][a,b,c]"),
                 std::string{"[O2,avx2][a,b,c]"});
        check_eq("(3) leerer Wert wird zum Sentinel", ex::marker_wert(""), std::string{"unbelegt"});
    }

    // -- Teil 4 (TRAGEND): der Aggregator-Key ist (zelle, fenster), NICHT fenster allein -----------
    {
        std::string const       fenster = ex::marker_fenster(8192, 4096);
        ex::MarkerKontext const perm0{"amd", "[O2,avx2][a,b,c]", "[all]"};
        ex::MarkerKontext const perm1{"amd", "[O3,avx512][a,b,c]", "[all]"};
        std::string const       z0 = ex::marker_kopf(ex::kMarkeBilanzTestat, perm0, "t", "bau", fenster);
        std::string const       z1 = ex::marker_kopf(ex::kMarkeBilanzTestat, perm1, "t", "bau", fenster);
        check_true("(4) zwei Perms EINER Lane wiederholen denselben fenster=-Wert",
                   enthaelt(z0, " fenster=8192:4096") && enthaelt(z1, " fenster=8192:4096"));
        check_true("(4) erst zelle= trennt sie -- (zelle, fenster) ist der Key", z0 != z1);
        check_true("(4) die Zelle steht IN der Zeile (keine Ableitung aus dem Zeilen-Kontext noetig)",
                   enthaelt(z0, " zelle=[O2,avx2][a,b,c]") && enthaelt(z1, " zelle=[O3,avx512][a,b,c]"));
        // Ebene-Trennung (Section 62-B-NACHTRAG): [a,b,c] steht in ceb=, NIE in der zelle=-Klammer.
        check_true("(4) Layer nie verschmolzen: ceb= ist ein EIGENES Feld",
                   enthaelt(z0, " ceb=[all]") && !enthaelt(z0, "zelle=[all]"));
    }

    // -- Teil 5: Tee-Filter-Liste ABGELEITET, nicht handgelistet ----------------------------------
    {
        // Der Filter der Stufe-2-Emission fuehrt genau diese Marken; wer eine Marke ergaenzt, ohne
        // sie hier einzureihen, wuerde sie stumm am Job-Trace vorbeischicken.
        check_true("(5) drei Marken in der Trace-Liste", ex::kSliceMarkerTraceMarken.size() == std::size_t{3});
        bool alle_gerendert = true;
        for (std::string_view const marke : ex::kSliceMarkerTraceMarken) {
            std::string const kopf = ex::marker_kopf(marke, kontext_amd_o2(), "t", "bau", ex::marker_fenster(0, 4));
            std::string const erwartet_praefix = "[" + std::string{marke} + "] ";
            if (kopf.rfind(erwartet_praefix, 0) != 0) alle_gerendert = false;
        }
        check_true("(5) jede Marke der Liste rendert ihren eigenen Klammer-Praefix", alle_gerendert);
        check_true("(5) die drei Marken sind paarweise verschieden",
                   ex::kMarkePlanTestat != ex::kMarkeBilanzTestat && ex::kMarkeBilanzTestat != ex::kMarkePruefBilanz &&
                       ex::kMarkePlanTestat != ex::kMarkePruefBilanz);
    }

    std::cout << (g_fail == 0 ? "==== E-04-P1 Marker-Familie v2: ALLE OK ====\n"
                              : "==== E-04-P1 Marker-Familie v2: FEHLER ====\n");
    return g_fail == 0 ? 0 : 1;
}
