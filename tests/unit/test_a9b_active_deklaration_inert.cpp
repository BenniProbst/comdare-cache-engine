// tests/unit/test_a9b_active_deklaration_inert.cpp -- Lane A, Paket P6/A9b (26.07.2026). UNREGISTRIERT.
//
// Dieser Test ist ABSICHTLICH NICHT in tests/unit/CMakeLists.txt eingetragen: die Datei ist in der
// laufenden Welle Konflikt-Zone (Sammel-Registrierung am Join, wie bei den Lane-C-Guards C-5). Der
// Voll-ctest-Zaehler bleibt dadurch unveraendert; die Verifikation laeuft ueber einen Hand-Bau. Die
// Include-Wurzeln sind die des Muster-Blocks von test_e4_contract_xml_to_axislevels, erweitert um
// libs/cache_engine/include (validate_profile.hpp zieht cache_engine/measurement/*), die GENERIERTE
// Flags-Wurzel <build>/generated fuer Teil 4 und die vendored boost_mp11-Wurzel. xml_reader ist
// header-only -- die EINZIGE mitzugebende .cpp ist der Parser. Literal gelaufenes Kommando (26.07.):
//
//   g++ -std=c++23 -O0 -I libs/cache_engine -I libs/cache_engine/include \
//       -I libs/common/serialization -I libs/common -I <build>/generated \
//       -I cmake/third_party/boost_mp11/include \
//       tests/unit/test_a9b_active_deklaration_inert.cpp \
//       libs/common/serialization/xml_config_parser/xml_config_parser.cpp -o <build>/test_a9b_guard
//
// WAS HIER BEWIESEN WIRD -- und warum die UMKEHRUNG der ueblichen Beweisrichtung Absicht ist:
// A9b baut einen KANAL, der heute bewusst INERT bleibt. Das Attribut <axis active="true|false"> gibt der
// Anwender-XML zum ersten Mal die dritte Aussage "genannt UND abgewaehlt" (bisher nur "ungenannt" = volles
// Angebot, "genannt" = Override) -- Registry ist ein ANGEBOT (§27), die Abwahl ist eine ERKLAERUNG des
// Anwenders. WIRKSAM darf diese Erklaerung heute nicht werden: ein tatsaechlich entfernter Baustein
// veraenderte die Katalog-Kardinalitaet und damit den golden-CRC (kNewGolden131072Crc64,
// tests/unit/test_lazy_adhoc_source_gen.cpp) -- das waere ein DRITTER Anker in einer Welle, die
// binary_id-NEUTRAL zu bleiben hat. Der auswertende Verbraucher (declared_count) gehoert in das
// Lane-F-Byte-Fenster.
//
// Dieser Guard nagelt deshalb BEIDE Haelften fest:
//   Teil 1  ANGEBOT LESBAR   -- der Parser liest active in BEIDEN Anwender-Kanaelen (Thesis permute_axes
//                               und Experiment axes_default_lookup), inklusive der Abwesenheits-Semantik
//                               (fehlendes Attribut == true) und der xs:boolean-Lexik {true,false,1,0}.
//   Teil 2  WIRKUNG GESPERRT -- zwei Profile, die sich EINZIG im active="false"-Attribut unterscheiden,
//                               liefern denselben AxisLevel-Satz, dieselbe binary_count und Zeichen fuer
//                               Zeichen dieselbe binary_id-Liste. Das ist der eigentliche Kern: der Test
//                               belegt die WIRKUNGSLOSIGKEIT, nicht die Wirkung.
//   Teil 3  RESOLVER TRAEGT  -- die additive Ueberladung resolve_axis_refs_against_trio(DeclaredAxisRef)
//                               fuehrt die abgewaehlte Achse IN die Bausteinmenge (statt Abwahl durch
//                               Abwesenheit) und meldet sie separat; ok/rejects bleiben identisch zur
//                               Ref-String-Fassung, die der Produktiv-Pfad weiter benutzt.
//   Teil 4  UMKEHR-HEBEL     -- der EINZIGE heute wirksame Abschalt-Hebel einer Achse ist der Bau-Weg
//                               CMakeLists.txt:380-381 option() -> axis_persistence_target_flags.hpp.in
//                               -> EnabledTargets = mp_filter. Er ist compile-time und von keiner XML
//                               erreichbar. persistence_target wird hier ausschliesslich GELESEN
//                               (Registry-mp_list = TABU-Klasse dieser Welle).
//
// NEGATIV-BEWEIS (26.07., gegen eine Schatten-Kopie im Scratchpad, der Baum blieb unberuehrt): baut man die
// Abwahl in der Resolver-Ueberladung WIRKSAM ein (`if (d.active) refs.push_back(...)`), faellt Teil 3 mit 3
// Fehlern und der Guard endet mit EXIT=1. Der Guard laeuft also nicht leer. Teil 2 bleibt bei dieser Sabotage
// gruen -- und das ist kein Loch, sondern die Aussage selbst: der Baum-Pfad (build_axis_levels) kennt `active`
// UEBERHAUPT NICHT, er kann vom Resolver aus gar nicht erreicht werden. Genau diese Trennung ist der Grund,
// warum die Deklaration heute byte-neutral landen darf.

#include "builder/experiment_tree/experiment_tree.hpp"
#include "builder/experiment_tree/profile_to_tree.hpp"
#include "profile_facade/validate_profile.hpp"
#include "xml_config_parser/xml_config_parser.hpp"

#include <axes/persistence_target/axis_persistence_target_registry.hpp>

#include <boost/mp11.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace pt  = comdare::cache_engine::persistence_target;
namespace mp  = boost::mp11;
namespace fs  = std::filesystem;

static int g_fail = 0;

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

namespace {

fs::path write_tmp(fs::path const& dir, std::string const& name, std::string const& content) {
    fs::create_directories(dir);
    fs::path const p = dir / name;
    std::ofstream  out{p};
    out << content;
    return p;
}

// Die BEIDEN Thesis-Profile unterscheiden sich in GENAU EINEM Zeichenbereich: dem active-Attribut an der
// Achse memory_layout. Alles andere ist identisch -- nur so ist der Baum-Vergleich in Teil 2 ein Beweis
// UEBER das Attribut und nicht ueber irgendeine andere Profil-Differenz.
//
// Beide Refs sind ECHTE Organ-Kompositions-Achsen (kCompositionAxisNames, axis_path_serialization.hpp) --
// das ist Absicht: nur sie passieren den strukturellen Organ-only-Guard in build_axis_levels und erzeugen
// ein binary_id-Segment. Die Abwahl sitzt bewusst auf memory_layout, also auf einer Achse, die tatsaechlich
// in die binary_id einginge -- eine Abwahl auf einer Achse, die der Guard ohnehin verwirft, waere kein
// Beweis. (Gemessen am 26.07.: is_organ_composition_axis("tier") == false, weshalb "tier" hier NICHT taugt.)
std::string thesis_profile_xml(std::string const& active_attr_on_memory_layout) {
    return std::string{"<comdare_thesis_profile id=\"a9b_guard\" schema_version=\"1\">\n"} +
           "  <permute_axes>\n"
           "    <axis ref=\"search_algo\">\n"
           "      <value>art</value>\n"
           "      <value>hashmap</value>\n"
           "    </axis>\n"
           "    <axis ref=\"memory_layout\"" +
           active_attr_on_memory_layout +
           ">\n"
           "      <value>soa</value>\n"
           "      <value>aos</value>\n"
           "    </axis>\n"
           "  </permute_axes>\n"
           "</comdare_thesis_profile>\n";
}

std::string experiment_profile_xml(std::string const& active_attr_on_isa) {
    return std::string{"<comdare_experiment version=\"1\" id=\"a9b_guard_exp\">\n"} +
           "  <axes_default_lookup enabled=\"true\">\n"
           "    <axis ref=\"memory_layout\" allowed_variants=\"soa\"/>\n"
           "    <axis ref=\"isa\"" +
           active_attr_on_isa +
           "/>\n"
           "  </axes_default_lookup>\n"
           "</comdare_experiment>\n";
}

// Hermetisches 3-Art-Angebot: RegistryTrio/RegistryContents sind Aggregate, der Guard braucht deshalb KEINE
// Registry-Dateien von Platte (und haengt an keinem Pfad, den eine andere Lane gerade umbaut). memory_layout
// = Organ-Achse (korrekt platziert), isa = System-Achse (V-CATEGORY-Route) -- damit traegt der Report in
// Teil 3 sowohl einen Nicht-Reject als auch einen echten Reject, und ein etwaiger Unterschied zwischen den
// beiden Resolver-Fassungen haette Gelegenheit, sich zu zeigen.
tlz::RegistryTrio make_trio() {
    tlz::RegistryTrio trio;
    trio.organ.engine                      = "cache_engine";
    trio.organ.axis_names["tier"]          = {"art", "hashmap"};
    trio.organ.axis_names["memory_layout"] = {"soa", "aos"};
    trio.system.engine                     = "cache_engine_system";
    trio.system.axis_names["isa"]          = {"x86_64", "aarch64"};
    trio.measurement.engine                = "cache_engine_measurement";
    trio.measurement.axis_names["latency"] = {"wallclock"};
    return trio;
}

std::vector<std::string> binary_ids_of(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    std::vector<std::string> ids;
    tree.for_each_binary(
        [&ids](std::string const& binary_id, std::string const&, auto const&) { ids.push_back(binary_id); });
    return ids;
}

bool levels_equal(std::vector<ex::AxisLevel> const& a, std::vector<ex::AxisLevel> const& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].axis != b[i].axis) return false;
        if (a[i].values != b[i].values) return false;
        if (a[i].is_static != b[i].is_static) return false;
        if (a[i].variable != b[i].variable) return false;
        if (a[i].block_id != b[i].block_id) return false;
    }
    return true;
}

} // namespace

int main() {
    fs::path const      dir = fs::temp_directory_path() / "comdare_a9b_guard";
    cx::XmlConfigParser parser;

    // -- Teil 1: ANGEBOT LESBAR ------------------------------------------------------------------------
    std::cout << "\n== Teil 1: das Angebot ist lesbar (Parser liest active in beiden Kanaelen) ==\n";

    auto const tp_off =
        parser.parse_thesis_profile(write_tmp(dir, "thesis_off.xml", thesis_profile_xml(" active=\"false\"")));
    auto const tp_abs = parser.parse_thesis_profile(write_tmp(dir, "thesis_abs.xml", thesis_profile_xml("")));
    check_true("Thesis-Kanal: beide Profile parsen", tp_off.has_value() && tp_abs.has_value());
    if (!tp_off || !tp_abs) return 1;
    check_eq("Thesis: 2 permute_axes (abgewaehlte Achse bleibt in der Liste)", tp_off->permute_axes.size(),
             std::size_t{2});
    check_true("Thesis: active=\"false\" wird GELESEN", tp_off->permute_axes[1].active == false);
    check_true("Thesis: die nicht annotierte Achse bleibt aktiv", tp_off->permute_axes[0].active == true);
    check_true("Thesis: ABWESENDES Attribut == true (Bestands-Profile unveraendert)",
               tp_abs->permute_axes[0].active == true && tp_abs->permute_axes[1].active == true);

    auto const ep_off =
        parser.parse_experiment_profile(write_tmp(dir, "exp_off.xml", experiment_profile_xml(" active=\"false\"")));
    auto const ep_abs = parser.parse_experiment_profile(write_tmp(dir, "exp_abs.xml", experiment_profile_xml("")));
    check_true("Experiment-Kanal: beide Profile parsen", ep_off.has_value() && ep_abs.has_value());
    if (!ep_off || !ep_abs) return 1;
    check_eq("Experiment: 2 axes_default_lookup-Eintraege", ep_off->axes_default_lookup.size(), std::size_t{2});
    check_true("Experiment: active=\"false\" wird GELESEN", ep_off->axes_default_lookup[1].active == false);
    check_true("Experiment: ABWESENDES Attribut == true",
               ep_abs->axes_default_lookup[0].active == true && ep_abs->axes_default_lookup[1].active == true);

    // xs:boolean-Lexik: NUR "false" und "0" schalten ab. Alles andere -- auch ein schema-ungueltiger Wert --
    // bleibt true. Das ist die byte-sichere Richtung: ein Tippfehler kann niemals still eine Achse abwaehlen.
    auto const tp_zero =
        parser.parse_thesis_profile(write_tmp(dir, "thesis_zero.xml", thesis_profile_xml(" active=\"0\"")));
    auto const tp_one =
        parser.parse_thesis_profile(write_tmp(dir, "thesis_one.xml", thesis_profile_xml(" active=\"1\"")));
    auto const tp_junk =
        parser.parse_thesis_profile(write_tmp(dir, "thesis_junk.xml", thesis_profile_xml(" active=\"vielleicht\"")));
    check_true("Lexik: active=\"0\" == false", tp_zero.has_value() && tp_zero->permute_axes[1].active == false);
    check_true("Lexik: active=\"1\" == true", tp_one.has_value() && tp_one->permute_axes[1].active == true);
    check_true("Lexik: ungueltiger Wert bleibt true (Tippfehler waehlt NIE still ab)",
               tp_junk.has_value() && tp_junk->permute_axes[1].active == true);

    // -- Teil 2: WIRKUNG GESPERRT ----------------------------------------------------------------------
    std::cout << "\n== Teil 2: die Wirkung ist gesperrt (Baum und binary_ids sind identisch) ==\n";

    auto const levels_off = ex::build_axis_levels(*tp_off, "ce_only", ex::AxisRegistry{});
    auto const levels_abs = ex::build_axis_levels(*tp_abs, "ce_only", ex::AxisRegistry{});
    check_true("AxisLevel-Satz mit active=\"false\" == Satz ohne das Attribut", levels_equal(levels_off, levels_abs));
    // 3 Ebenen: die zwei STATISCHEN Organ-Ebenen search_algo und memory_layout plus die DYNAMISCHE
    // Wiederholungs-Ebene, die build_axis_levels aus dem Default <repetitions>=3 selbst emittiert (KF-10).
    // Die dynamische Ebene geht in keine binary_id ein -- der Baum filtert sie ueber is_static heraus.
    check_eq("Ebenen-Satz unveraendert (2 statische Organ-Ebenen + 1 dynamische Wiederholungs-Ebene)",
             levels_off.size(), std::size_t{3});
    std::size_t static_levels = 0;
    for (auto const& l : levels_off)
        if (l.is_static) ++static_levels;
    check_eq("die abgewaehlte Achse erzeugt WEITERHIN ihre statische Ebene", static_levels, std::size_t{2});

    auto const ids_off = binary_ids_of(levels_off);
    auto const ids_abs = binary_ids_of(levels_abs);
    check_eq("binary_count mit active=\"false\" (search_algo 2 x memory_layout 2)", ids_off.size(), std::size_t{4});
    check_true("binary_id-Liste Zeichen fuer Zeichen identisch", ids_off == ids_abs);
    check_true("jede binary_id traegt das Segment der abgewaehlten Achse unveraendert",
               !ids_off.empty() && std::all_of(ids_off.begin(), ids_off.end(), [](std::string const& id) {
                   return id.find("memory_layout=") != std::string::npos;
               }));

    // -- Teil 3: RESOLVER TRAEGT, WERTET NICHT ---------------------------------------------------------
    std::cout << "\n== Teil 3: der Resolver traegt die Erklaerung, wertet sie aber nicht aus ==\n";

    tlz::RegistryTrio const trio = make_trio();

    auto const refs_off     = tlz::organ_position_refs(*ep_off);
    auto const refs_abs     = tlz::organ_position_refs(*ep_abs);
    auto const declared_off = tlz::organ_position_declared_axes(*ep_off);
    check_true("die Ref-Liste ist mit und ohne Abwahl dieselbe (keine Abwahl durch Weglassen)", refs_off == refs_abs);
    check_eq("die Ueberladung liefert gleich viele Eintraege wie die Ref-Fassung", declared_off.size(),
             refs_off.size());

    auto const rep_refs     = tlz::resolve_axis_refs_against_trio(refs_off, trio);
    auto const rep_declared = tlz::resolve_axis_refs_against_trio(declared_off, trio);
    check_eq("Ref-Fassung: 1 Reject (isa steht als System-Achse in Organ-Position)", rep_refs.rejects.size(),
             std::size_t{1});
    check_true("beide Fassungen liefern dasselbe ok", rep_refs.ok == rep_declared.ok);
    check_eq("beide Fassungen liefern gleich viele Rejects", rep_declared.rejects.size(), rep_refs.rejects.size());
    bool rejects_identical = rep_refs.rejects.size() == rep_declared.rejects.size();
    for (std::size_t i = 0; rejects_identical && i < rep_refs.rejects.size(); ++i)
        rejects_identical = rep_refs.rejects[i].code == rep_declared.rejects[i].code &&
                            rep_refs.rejects[i].ref == rep_declared.rejects[i].ref &&
                            rep_refs.rejects[i].message == rep_declared.rejects[i].message;
    check_true("die Rejects sind Zeichen fuer Zeichen identisch (Code, Ref, Meldung)", rejects_identical);
    check_true("die Ref-String-Fassung fuellt declared_inactive NICHT (Produktiv-Pfad unveraendert)",
               rep_refs.declared_inactive.empty());
    check_eq("die Ueberladung MELDET die Abwahl separat", rep_declared.declared_inactive.size(), std::size_t{1});
    check_true("und zwar mit der richtigen Koordinate",
               !rep_declared.declared_inactive.empty() && rep_declared.declared_inactive[0] == "isa");

    // -- Teil 4: DER EINZIGE WIRKSAME ABSCHALT-HEBEL IST COMPILE-TIME ----------------------------------
    std::cout << "\n== Teil 4: der wirksame Abschalt-Hebel bleibt der Bau-Weg (read-only geprueft) ==\n";

    // Gelesen, nicht angefasst: AllTargets ist der deklarierte Vollausbau, EnabledTargets das, was der Bau
    // permutiert. Der Filter speist sich aus dem CMake-option() ueber den generierten Flags-Header -- eine
    // XML kann ihn nicht erreichen. Genau das ist die Asymmetrie, die A9b heute NICHT aufhebt.
    static_assert(mp::mp_size<pt::AllTargets>::value == 2, "AllTargets = deklarierter Vollausbau (2 Bausteine)");
    static_assert(mp::mp_size<pt::EnabledTargets>::value == 1, "Q-1 FALL B: disk_writeback ist per option() AUS");
    static_assert(pt::flags::memory_only_enabled, "memory_only ist der golden_wired-Durchreich-Wert");
    static_assert(!pt::flags::disk_writeback_enabled, "disk_writeback ist per option() AUS (Q-1 FALL B)");
    check_eq("AllTargets (deklariert)", std::size_t{mp::mp_size<pt::AllTargets>::value}, std::size_t{2});
    check_eq("EnabledTargets (gebaut)", std::size_t{mp::mp_size<pt::EnabledTargets>::value}, std::size_t{1});
    check_true("die Differenz kommt AUSSCHLIESSLICH aus dem compile-time-Flag, nicht aus einer XML",
               pt::flags::memory_only_enabled && !pt::flags::disk_writeback_enabled);

    fs::remove_all(dir);
    std::cout << "\n==== A9b active-Deklaration INERT: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
