// Phase 7 (Parser-/Gating-Konsolidierung): Regressions-Gate fuer die DOM-Fassung der
// frueheren Regex-Pfade (parse_profile / load_messreihen / parse_one). Erwartungswerte sind
// literal gegen die Regex-Referenz eingefroren (art.profile.xml = committete SOTA-Akte).

#include "xml_config_parser/xml_config_parser.hpp"

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen (analog M-SU-04)

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <process.h> // ::_getpid
#else
#include <unistd.h> // ::getpid
#endif

namespace cx = ::comdare::builder::xml;
namespace fs = ::std::filesystem;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// analog M-SU-04 (#278/#24): feste Namen direkt unter temp_directory_path() kollidieren auf host-weit
// geteiltem /tmp mit Resten FREMDER User (Owner-Mismatch + Sticky-Bit -> ofstream/remove scheitern STILL,
// Test liest Alt-/Fremdinhalte). Basis = uid-eindeutiges user_tmp_dir() (verbindliche Loesung, wie die
// uebrigen ce-Unit-Tests); der zusaetzliche getpid()+Zaehler-Praefix macht jeden comdare_p7_-Namen
// prozess-eindeutig gegen parallele Laeufe desselben Users. POSIX/Windows portabel.
[[nodiscard]] long long comdare_pid() {
#if defined(_WIN32)
    return static_cast<long long>(::_getpid());
#else
    return static_cast<long long>(::getpid());
#endif
}

fs::path temp_path(char const* name) {
    static int counter = 0;
    return ::comdare::test::user_tmp_dir() /
           (std::to_string(comdare_pid()) + "_" + std::to_string(counter++) + "_" + name);
}

fs::path write_temp(char const* name, std::string const& content) {
    fs::path const p = temp_path(name);
    // M-CE-26 (2026-07-15): den Schreiberfolg pruefen — ein still gescheiterter ofstream
    // (Owner-/Sticky-/Platz-Fehler) darf nicht als „leere Datei erfolgreich geschrieben" durchgehen.
    std::ofstream os{p};
    os << content;
    if (!os) {
        std::cout << "  [ERR] write_temp nicht schreibbar: " << p << "\n";
        ++g_fail;
    }
    return p;
}

} // namespace

int main(int argc, char** argv) {
    fs::path const sota_dir =
        (argc >= 2) ? fs::path(argv[1]) : (fs::path("libs") / "cache_engine" / "algorithm_profiles" / "sota");

    std::cout << "==== Phase 7: Parser-Konsolidierung (DOM statt Regex) ====\n";
    cx::XmlConfigParser parser;

    // ── (1) parse_profile gegen die committete SOTA-Akte art.profile.xml (Regex-Referenzwerte) ──
    cx::AlgorithmProfile const art = parser.parse_profile(sota_dir / "art.profile.xml");
    check_eq("parse_profile: id", art.id, std::string{"art"});
    check_eq("parse_profile: paper_ref", art.paper_ref, std::string{"P01"});
    check_eq("parse_profile: pruefling_type leer (full)", art.pruefling_type, std::string{});
    check_eq("parse_profile: axes[page]", art.axes.at("page"), std::string{"DENSEBYTE_ART256"});
    check_eq("parse_profile: axes[allocator]", art.axes.at("allocator"), std::string{"MIMALLOC"});
    check_eq("parse_profile: axes-Anzahl", art.axes.size(), std::size_t{11});
    check_eq("parse_profile: key_types", art.key_types, std::string{"std::uint64_t"});
    check_eq("parse_profile: value_types", art.value_types, std::string{"std::string"});

    // masstree = einzige committete Akte mit XML-Entities in key_types. BEWUSSTE KORREKTUR
    // (Review wf_8508f98c): der DOM dekodiert &lt;/&gt; — die Regex-Fassung lieferte den
    // Escape-Rohtext in generierte C++-Kommentare. Dekodierter Wert hier EINGEFROREN.
    cx::AlgorithmProfile const mt = parser.parse_profile(sota_dir / "masstree.profile.xml");
    check_eq("parse_profile: masstree key_types (Entities dekodiert)", mt.key_types,
             std::string{"std::uint64_t, std::array<char, 8>"});
    check_eq("parse_profile: masstree id", mt.id, std::string{"masstree"});

    // Fremde Wurzel => leeres Profil (id=="" bleibt Kein-Algorithm-Profil-Sentinel).
    fs::path const foreign_xml    = write_temp("comdare_p7_foreign.xml", "<comdare_thesis_profile id=\"nicht_algo\">"
                                                                         "<x>y</x></comdare_thesis_profile>\n");
    cx::AlgorithmProfile const fp = parser.parse_profile(foreign_xml);
    check_true("parse_profile: fremde Wurzel => leeres Profil (Sentinel id==\"\")", fp.id.empty());

    // ── (2) optionale Overrides (V19.1/V29.A) ueber Mini-Fixture ──
    fs::path const prof_xml =
        write_temp("comdare_p7_profile.xml", "<comdare_algorithm_profile id=\"t\" paper_ref=\"P99\" "
                                             "pruefling_type=\"abstract\">\n"
                                             "  <axes><page>A</page><node>B</node></axes>\n"
                                             "  <key_value_signature><key_types>k</key_types>"
                                             "<value_types>v</value_types></key_value_signature>\n"
                                             "  <expected_workload>YCSB_C</expected_workload>\n"
                                             "  <allocator_override>mimalloc</allocator_override>\n"
                                             "</comdare_algorithm_profile>\n");
    cx::AlgorithmProfile const tp = parser.parse_profile(prof_xml);
    check_eq("overrides: pruefling_type", tp.pruefling_type, std::string{"abstract"});
    check_eq("overrides: expected_workload", tp.expected_workload, std::string{"YCSB_C"});
    check_eq("overrides: allocator_override", tp.allocator_override, std::string{"mimalloc"});
    check_eq("overrides: axes-Anzahl", tp.axes.size(), std::size_t{2});

    // ── (3) load_messreihen: defined/full + profile-Reihenfolge (Regex-Referenzverhalten) ──
    fs::path const mr_xml = write_temp("comdare_p7_messreihen.xml", "<comdare_messreihen version=\"1\">\n"
                                                                    "  <messreihe id=\"A_defined\">\n"
                                                                    "    <mode>defined</mode>\n"
                                                                    "    <profile>art</profile>\n"
                                                                    "    <profile>hot</profile>\n"
                                                                    "  </messreihe>\n"
                                                                    "  <messreihe id=\"A_full\">\n"
                                                                    "    <mode>full</mode>\n"
                                                                    "  </messreihe>\n"
                                                                    "</comdare_messreihen>\n");
    auto const     reihen = parser.load_messreihen(mr_xml);
    check_eq("messreihen: Anzahl", reihen.size(), std::size_t{2});
    if (reihen.size() == 2) {
        check_eq("messreihen[0]: id", reihen[0].id, std::string{"A_defined"});
        check_true("messreihen[0]: mode defined", reihen[0].mode == cx::MessreihenMode::Defined);
        check_eq("messreihen[0]: profile-Anzahl", reihen[0].sota_profile_refs.size(), std::size_t{2});
        check_eq("messreihen[0]: profile[0]", reihen[0].sota_profile_refs[0], std::string{"art"});
        check_eq("messreihen[1]: id", reihen[1].id, std::string{"A_full"});
        check_true("messreihen[1]: mode full", reihen[1].mode == cx::MessreihenMode::Full);
    }

    // ── (4) parse_one: {} bleibt {} (R2-Gate), aber diagnostiziert ──
    auto const missing = parser.parse_one(temp_path("comdare_p7_gibt_es_nicht.xml"));
    check_true("parse_one: fehlende Datei => leer", missing.empty());
    fs::path const perm_xml = write_temp("comdare_p7_perm.xml", "<root><allocator_permutation id=\"a1\">\n"
                                                                "  <family>hoard</family><variant>v1</variant>\n"
                                                                "</allocator_permutation></root>\n");
    auto const     perms    = parser.parse_one(perm_xml);
    check_eq("parse_one: 1 Eintrag", perms.size(), std::size_t{1});
    if (!perms.empty()) {
        check_eq("parse_one: id", perms[0].id, std::string{"a1"});
        check_eq("parse_one: attributes[family]", perms[0].attributes.at("family"), std::string{"hoard"});
    }
    fs::path const junk_xml = write_temp("comdare_p7_junk.xml", "kein xml hier\n");
    check_true("parse_one: Muell-Datei => leer (mit WARNUNG)", parser.parse_one(junk_xml).empty());

    std::error_code ec;
    fs::remove(prof_xml, ec);
    fs::remove(foreign_xml, ec);
    fs::remove(mr_xml, ec);
    fs::remove(perm_xml, ec);
    fs::remove(junk_xml, ec);

    std::cout << "\n==== Phase 7 Parser-Konsolidierung: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
