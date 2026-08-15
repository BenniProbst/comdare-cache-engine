// -----------------------------------------------------------------------------
// test_lagz1_lager_schluessel_simd.cpp -- LAG-Z1: DER LAGER-SCHLUESSEL MUSS DIE SIMD-STUFE TRENNEN.
//
// SELBSTCHECK dieses Blocks: jede Zahl unten wird von diesem Programm SELBST erhoben und mit Nenner
// gedruckt; keine ist aus einer Doku abgeschrieben. Der Koeder in kKoeder ist frisch gewuerfelt
// (head -c9 /dev/urandom | base32 | tr A-Z a-z) und steht NUR hier -- er beweist beim Lauf, dass die
// ausgefuehrte Binary aus DIESER Quelle stammt und nicht aus einem alten Objekt.
//
// DIE FRAGE, DIE HIER ENTSCHIEDEN WIRD. Der Lager-Schluessel ist das TUPEL
// (Anatomie-Fingerprint, ZellKoordinaten) -- bestandslog_index.hpp:104-111, defaulted <=>; der Index
// ist std::map<LagerKey, BestandEintrag> (:175) und beide Naehte des Bau-Loops schlagen darin nach
// (builder_registration.hpp:588 observe, :613/:620 lager_contains). Die Zelle steht DANEBEN, statt im
// Digest zu stecken. Damit haengt alles an einer Frage: traegt der Fingerprint die SIMD-Stufe schon
// selbst, oder ist die Zelle der EINZIGE Diskriminator? Ist Letzteres wahr, dann kollabieren avx2 und
// avx512 derselben Permutation in jedem Lauf, der die Zelle nicht meldet -- und ein Folgelauf
// ueberspringt Bauten, die nie stattgefunden haben.
//
// ZWEI WEGE, DIE DER ITERATOR WIRKLICH FAHREN KANN (profile_run_entry.hpp:1063-1094):
//   WEG L (LIVE, per Perm)  : perm_fingerprint = make_lazy_adhoc_fingerprint_fn_from_env(
//                             perm_cell_values, perm_toolchain_glied). Greift, wenn a.compile_for_perm
//                             belegt ist (perm_bau_je_zelle). Auf dem Facade-Pfad ist es das immer,
//                             sobald das Profil <system_axes> deklariert -- und nur dann laeuft die
//                             opt x simd-Schleife ueberhaupt (profile_run_facade.cpp:605/668/685).
//   WEG F (FALLBACK, lauf-konstant): perm_fingerprint = lazy_fingerprint, also der ARGUMENTLOSE
//                             Provider (:473, gesetzt bei :1092). Greift, wenn compile_for_perm
//                             unbelegt ist; dann baut JEDE Zelle dieselbe lauf-konstante Binary
//                             (:1046-1057) und derselbe Digest ist auch die ehrliche Antwort.
//
// WARUM DIESE WACHE UEBERHAUPT GEBAUT WIRD. Der Zusammenbruch ist in diesem Baum schon EINMAL
// passiert und steht als Prosa im Code: profile_run_entry.hpp:1074-1082 ("Genau dann fiel der Zwilling
// auf den LAUF-KONSTANTEN Provider zurueck und rechnete fuer JEDE Zelle denselben Digest: O2 und O3
// bekamen wieder EINEN Lager-Schluessel -- das Loch in T2-B, unter der Zeile versteckt, die es zu
// schliessen vorgab"). Geheilt wurde er am 06.08.2026 durch eine BEDINGUNG. Was fehlte, war eine
// Wache, die den Rueckfall FAENGT: der vorhandene Nachbar-Test prueft, dass die GLIEDER verschieden
// sind (test_m_w12_stamp_bausteine.cpp:1881-1897) -- nicht, dass die LAGER-SCHLUESSEL es sind.
//
// BEIDE RICHTUNGEN, IMMER. Ein Schluessel, der IMMER verschieden ist, macht jeden SKIP unmoeglich und
// waere genauso falsch wie einer, der immer gleich ist. Neben jedem "verschieden"-Fall steht deshalb
// ein "gleich"-Fall mit demselben Aufbau, dazu eine Determinismus-Probe gegen jeden Lauf-variablen
// Anteil (Zeit/PID/Host) -- der wuerde jeden SKIP ebenfalls unmoeglich machen.
//
// DREI BETRIEBSARTEN (argv[1]):
//   (ohne)        Die Wache. WEG L, alle Faelle muessen halten. rc=0 bei "alle gehalten".
//   --mutant      Der MUTANT: WEG L wird durch WEG F ersetzt, also exakt der historische Rueckfall.
//                 Bewertet wird normal -> die Trennungs-Faelle MUESSEN reissen, rc=1. Das ist der
//                 literale ROT-Beleg; er wird nicht behauptet, sondern gefahren.
//   --selbstbiss  Derselbe Mutant, aber die ERWARTUNG ist umgedreht: rc=0 nur, wenn GENAU die
//                 Trennungs-Faelle gerissen sind UND die SKIP-/Determinismus-Faelle gehalten haben.
//                 So faehrt der Biss bei jedem CI-Lauf mit, statt hier als Prosa zu verjaehren.
//
// Doktrin: header-only C++23, ASCII-Kommentare, eigener main() (Muster test_lazy_adhoc_source_gen.cpp
// -- kein gtest-Link), Rueckgabe 0 = Betriebsart-Erwartung gehalten.
// -----------------------------------------------------------------------------

#include "lazy_adhoc_source_gen.hpp"    // make_lazy_adhoc_fingerprint_fn_from_env (der EINE Laufzeit-Zwilling)
#include "generated_source_catalog.hpp" // generated_catalog_static_levels (die realen binary_ids)
#include "system_cell_values_naht.hpp"  // compose_system_cell_values / resolve_system_cell_target_isa
#include "toolchain_stamp_naht.hpp"     // PermToolchainAchsen / compose_toolchain_stamp_glied_for_perm

#include <builder/bestandslog/bestandslog_document.hpp> // ZellKoordinaten
#include <builder/bestandslog/bestandslog_index.hpp>    // LagerKey / lager_key_from_hex (DER Vergleich)
#include <builder/experiment_tree/experiment_tree.hpp>  // ExperimentTree / StaticBinaryView
#include <system_axes/simd_sub_axis.hpp>   // SimdNoExtOption (die no_extension-Regel)

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace pfn = ::comdare::cache_engine::profile_facade;
namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace cm  = ::comdare::cache_engine::measurement;

namespace {

// K13: FRISCH GEWUERFELT (head -c9 /dev/urandom | base32 | tr A-Z a-z), nicht abgeschrieben.
constexpr char const* kKoeder = "2e5grq7a6q54hxi=";

// Die Faelle, deren Ausgang die Betriebsart --selbstbiss namentlich erwartet. TRENNUNG muss unter dem
// Mutanten REISSEN, SKIP/DETERMINISMUS muss auch unter ihm HALTEN (sonst faengt die Wache den Mutanten
// aus dem falschen Grund und belegt nichts).
enum class Sorte { Trennung, Skip, Determinismus, Aufbau };

struct Fall {
    std::string name;
    Sorte       sorte    = Sorte::Aufbau; // cppcheck uninitMemberVarNoCtor: jedes Feld traegt seinen Default
    bool        gehalten = false;
};

std::vector<Fall> g_faelle;

void pruefe(char const* was, Sorte sorte, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    g_faelle.push_back(Fall{was, sorte, ok});
}

std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string>   ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

// EINE Perm-Zelle, gebildet wie profile_run_entry.hpp sie in der opt x simd-Schleife bildet
// (:1008-1029). KEINE zweite Ableitung: dieselben Quellen, dieselbe no_extension-Regel.
struct PermZelle {
    std::string opt;       // opt_level-id
    std::string opt_flags; // aufgeloeste opt-Flags
    std::string simd;      // simd-id
};

std::string zellwerte_von(PermZelle const& z) {
    // profile_run_entry.hpp:1008-1011 -- die beiden lauf-konstanten Facade-Zellen + simd_id.
    return pfn::compose_system_cell_values(pfn::resolve_system_cell_target_isa(std::string_view{}),
                                           pfn::kSystemCellBuildOsFamily, z.simd);
}

std::string glied_von(PermZelle const& z) {
    // profile_run_entry.hpp:1018-1029 -- no_extension wird als LEERES Segment gereicht.
    std::string const simd_segment = (z.simd == std::string{cm::SimdNoExtOption::simd_id()}) ? std::string{} : z.simd;
    pfn::PermToolchainAchsen achsen{};
    achsen.opt       = z.opt;
    achsen.opt_flags = z.opt_flags;
    achsen.simd      = simd_segment;
    return pfn::compose_toolchain_stamp_glied_for_perm(achsen);
}

// WEG F: der lauf-konstante Provider, wie ihn :473 baut (argumentlos) und :1092 als Fallback setzt.
std::string fp_weg_f(std::string const& binary_id) { return tlz::make_lazy_adhoc_fingerprint_fn_from_env()(binary_id); }

// WEG L: der per-Perm-Provider, wie ihn :1094 baut. Unter `mutant` wird er durch WEG F ersetzt --
// das ist der historische Rueckfall aus profile_run_entry.hpp:1074-1082, nicht eine erfundene Variante.
bool g_mutant = false;

std::string fp_weg_l(PermZelle const& z, std::string const& binary_id) {
    if (g_mutant) return fp_weg_f(binary_id);
    return tlz::make_lazy_adhoc_fingerprint_fn_from_env(zellwerte_von(z), glied_von(z))(binary_id);
}

bl::ZellKoordinaten zelle_von(PermZelle const& z) {
    bl::ZellKoordinaten k;
    k.opt  = z.opt;
    k.simd = z.simd;
    return k;
}

// Der Lager-Schluessel, gebildet wie builder_registration.hpp:588/613 ihn bildet.
std::optional<bl::LagerKey> schluessel(std::string const& hex, bl::ZellKoordinaten const& zelle) {
    return bl::lager_key_from_hex(hex, zelle);
}

bool schluessel_gleich(std::optional<bl::LagerKey> const& a, std::optional<bl::LagerKey> const& b) {
    return a.has_value() && b.has_value() && *a == *b;
}

} // namespace

int main(int argc, char** argv) {
    std::string const modus      = (argc > 1) ? std::string{argv[1]} : std::string{};
    bool const        selbstbiss = (modus == "--selbstbiss");
    g_mutant                     = selbstbiss || (modus == "--mutant");

    std::cout << "==== LAG-Z1: Lager-Schluessel x SIMD-Stufe  [Modus=" << (modus.empty() ? std::string{"WACHE"} : modus)
              << "]  (Koeder=" << kKoeder << ") ====\n";
    if (g_mutant)
        std::cout << "  MUTANT AKTIV: WEG L ist durch den lauf-konstanten Provider ersetzt "
                     "(der Rueckfall aus profile_run_entry.hpp:1074-1082).\n";

    std::vector<std::string> const ids = binary_ids(tlz::generated_catalog_static_levels());
    std::cout << "  binary_ids im materialisierten Katalog: " << ids.size() << " (Nenner der Flaechen-Faelle)\n";
    pruefe("A0: Katalog liefert binary_ids (Nenner > 0)", Sorte::Aufbau, !ids.empty());
    if (ids.empty()) return 1;
    std::string const id = ids.front();

    PermZelle const p_o2_avx2{"O2", "-O2", "avx2"};
    PermZelle const p_o2_avx512{"O2", "-O2", "avx512"}; // NUR die SIMD-Stufe unterscheidet sich
    PermZelle const p_o3_avx512{"O3", "-O3", "avx512"};

    // ---------------------------------------------------------------------------------------------
    // WEG L mit LEERER Zelle -- genau so erreicht ein Job ohne COMDARE_GN_OPT/COMDARE_GN_SIMD den
    // Leser (profile_run_entry.hpp:514-524 liest beide per getenv; ungesetzt => ZellKoordinaten leer).
    // Wenn die Trennung SCHON HIER haelt, traegt der Fingerprint die Stufe selbst.
    // ---------------------------------------------------------------------------------------------
    std::cout << "\n---- WEG L (per Perm), Abfrage mit LEERER Zelle ----\n";
    std::string const l_a  = fp_weg_l(p_o2_avx2, id);
    std::string const l_b  = fp_weg_l(p_o2_avx512, id);
    std::string const l_c  = fp_weg_l(p_o3_avx512, id);
    std::string const l_a2 = fp_weg_l(p_o2_avx2, id); // Wiederholung ueber einen ZWEITEN Provider
    std::cout << "  fp[O2,avx2]   = " << l_a << "\n";
    std::cout << "  fp[O2,avx512] = " << l_b << "\n";
    std::cout << "  fp[O3,avx512] = " << l_c << "\n";
    pruefe("A1: alle vier Fingerprints sind 128-hex", Sorte::Aufbau,
           l_a.size() == 128 && l_b.size() == 128 && l_c.size() == 128 && l_a2.size() == 128);

    bl::ZellKoordinaten const leer{};
    auto const                kl_a  = schluessel(l_a, leer);
    auto const                kl_b  = schluessel(l_b, leer);
    auto const                kl_c  = schluessel(l_c, leer);
    auto const                kl_a2 = schluessel(l_a2, leer);
    pruefe("L1 (TRENNUNG): gleiche opt, VERSCHIEDENE SIMD-Stufe => VERSCHIEDENE Lager-Schluessel", Sorte::Trennung,
           !schluessel_gleich(kl_a, kl_b));
    pruefe("L2 (TRENNUNG): verschiedene opt UND SIMD => VERSCHIEDENE Lager-Schluessel", Sorte::Trennung,
           !schluessel_gleich(kl_a, kl_c));
    pruefe("L3 (SKIP BLEIBT MOEGLICH): gleiche Stufe zweimal => GLEICHER Lager-Schluessel", Sorte::Skip,
           schluessel_gleich(kl_a, kl_a2));

    // ---------------------------------------------------------------------------------------------
    // WEG F -- der lauf-konstante Fallback. Hier ist der Digest fuer JEDE Zelle derselbe, weil dort
    // auch WIRKLICH dieselbe Binary gebaut wird. Genau dann ist die Zelle der einzige Diskriminator --
    // das ist die Begruendung, mit der messwert_key_source.hpp:26-28 sie danebenstellt.
    // ---------------------------------------------------------------------------------------------
    std::cout << "\n---- WEG F (lauf-konstant): die Zelle ist dort der EINZIGE Diskriminator ----\n";
    std::string const f_hex = fp_weg_f(id);
    std::cout << "  fp[lauf-konstant] = " << f_hex << "\n";
    pruefe("F1 (LEERE ZELLE => KOLLISION): lauf-konstanter Digest + leere Zelle => EIN Schluessel fuer beide Stufen",
           Sorte::Skip, schluessel_gleich(schluessel(f_hex, leer), schluessel(f_hex, leer)));
    pruefe("F2 (DIE ZELLE RETTET): lauf-konstanter Digest + BELEGTE Zelle => VERSCHIEDENE Schluessel", Sorte::Skip,
           !schluessel_gleich(schluessel(f_hex, zelle_von(p_o2_avx2)), schluessel(f_hex, zelle_von(p_o2_avx512))));
    pruefe("F3 (SKIP BLEIBT MOEGLICH): lauf-konstant + gleiche belegte Zelle => GLEICHER Schluessel", Sorte::Skip,
           schluessel_gleich(schluessel(f_hex, zelle_von(p_o2_avx2)), schluessel(f_hex, zelle_von(p_o2_avx2))));

    // ---------------------------------------------------------------------------------------------
    // DETERMINISMUS -- die Gegenprobe zur UEBERSCHAERFUNG. Geht etwas Lauf-Variables in den Schluessel
    // ein (Zeitstempel, PID, Hostname), greift NIE ein SKIP und jeder Lauf baut alles neu. Fuenf
    // vollstaendig getrennte Provider-Bauten desselben Aufbaus muessen denselben 128-hex liefern.
    // ---------------------------------------------------------------------------------------------
    std::cout << "\n---- DETERMINISMUS (Gegenprobe gegen eine ZU scharfe Zelle) ----\n";
    std::size_t gleich = 0;
    for (int i = 0; i < 5; ++i)
        if (fp_weg_l(p_o2_avx2, id) == l_a) ++gleich;
    std::cout << "  identische Wiederholungen: " << gleich << " von 5\n";
    pruefe("D1: fuenf identische Laeufe liefern denselben Schluessel (kein Zeit-/PID-/Host-Anteil)",
           Sorte::Determinismus, gleich == 5);

    // ---------------------------------------------------------------------------------------------
    // FLAECHE -- die Trennung darf nicht an EINER binary_id haengen.
    // ---------------------------------------------------------------------------------------------
    std::cout << "\n---- FLAECHE ueber den ganzen Katalog ----\n";
    std::size_t getrennt = 0;
    for (auto const& bid : ids)
        if (fp_weg_l(p_o2_avx2, bid) != fp_weg_l(p_o2_avx512, bid)) ++getrennt;
    std::cout << "  getrennte binary_ids: " << getrennt << " von " << ids.size() << "\n";
    pruefe("L4 (FLAECHE): JEDE binary_id trennt avx2 von avx512", Sorte::Trennung, getrennt == ids.size());

    // ---------------------------------------------------------------------------------------------
    // AUSWERTUNG je Betriebsart.
    // ---------------------------------------------------------------------------------------------
    std::size_t gerissen = 0, trennung_gerissen = 0, trennung_gesamt = 0, andere_gerissen = 0;
    for (auto const& f : g_faelle) {
        if (f.sorte == Sorte::Trennung) ++trennung_gesamt;
        if (f.gehalten) continue;
        ++gerissen;
        if (f.sorte == Sorte::Trennung)
            ++trennung_gerissen;
        else
            ++andere_gerissen;
    }
    std::cout << "\n  Faelle gesamt=" << g_faelle.size() << "  gerissen=" << gerissen
              << "  davon TRENNUNG=" << trennung_gerissen << " von " << trennung_gesamt << "\n";

    if (selbstbiss) {
        // Der Biss zaehlt NUR, wenn der Mutant an den Trennungs-Faellen stirbt und die SKIP-/
        // Determinismus-Faelle ueberlebt -- sonst haette die Wache ihn aus dem falschen Grund gefangen.
        bool const gebissen = (trennung_gerissen == trennung_gesamt) && (trennung_gesamt > 0) && (andere_gerissen == 0);
        std::cout << "\n==== LAG-Z1 SELBSTBISS: " << (gebissen ? "GEBISSEN" : "NICHT GEBISSEN") << " -- "
                  << trennung_gerissen << " von " << trennung_gesamt
                  << " Trennungs-Faellen gerissen, Nicht-Trennungs-Faelle gerissen=" << andere_gerissen
                  << " (erwartet 0)  (Koeder=" << kKoeder << ") ====\n";
        return gebissen ? 0 : 1;
    }

    // ---------------------------------------------------------------------------------------------
    // TASK #59 -- DIFFERENZTEST MIT FREMDEM NENNER (V-7): ZWEI WEGE, EIN HASH.
    //
    // Der neue Maker make_lazy_adhoc_fingerprint_mit_bvset_fn_from_env nimmt das bvset-Glied als
    // AUFRUF-Parameter, der Bestands-Maker loest es einmal vorab auf. Beide muessen fuer dieselbe
    // binary_id denselben Hash liefern, sobald der Parameter der LIVE-Wert ist -- sonst prueft die
    // Bindungs-Pruefung des Skip-Gates ein anderes Preimage als das, welches die Binary traegt, und
    // der ganze Teilmengen-Pfad waere ein Zirkelschluss ueber die eigene Formel.
    //
    // DER FREMDE NENNER ist hier der Bestands-Weg: er existiert seit NB/CX-4 und ist durch die
    // Faelle oben unabhaengig gedeckt. Der neue Weg wird GEGEN ihn gemessen, nicht gegen sich selbst.
    // BEIDE Hex-Werte werden genannt (V-4) -- eine Gleichheits-Zusage ohne die zwei Zahlen waere
    // genau die Erfolgsmarke ohne literale Ausgabe, die der Vertrag verbietet.
    {
        // `id` ist die AEUSSERE Bindung (:182, ebenfalls ids.front()). Eine eigene Deklaration hier waere
        // eine Verdeckung (-Wshadow) UND eine zweite Wahrheit ueber dieselbe binary_id -- der Differenztest
        // muss beide Wege ueber DENSELBEN Eingang fahren, sonst vergleicht er zwei Dinge statt zwei Wege.
        std::string const bvset_live = pfn::live_build_variant_set_signature_glied();
        std::string const weg_alt    = tlz::make_lazy_adhoc_fingerprint_fn_from_env()(id);
        std::string const weg_neu    = tlz::make_lazy_adhoc_fingerprint_mit_bvset_fn_from_env()(id, bvset_live);
        std::cout << "\n  [#59 V-7] binary_id=" << id << "\n"
                  << "            bvset (" << bvset_live.size() << " Byte) = " << bvset_live << "\n"
                  << "            Bestands-Maker (bvset implizit live) = " << weg_alt << "\n"
                  << "            Neuer Maker    (bvset explizit)      = " << weg_neu << "\n";
        if (weg_alt != weg_neu) {
            std::cout << "  [#59 V-7] FEHLER: die zwei Wege liefern verschiedene Hashes\n";
            ++gerissen;
        }
        // GEGENEINGANG (T-4): ein ANDERES bvset MUSS einen anderen Hash ergeben -- sonst waere der
        // Parameter wirkungslos und die Bindungs-Pruefung eine leere Geste, die jeden Wert bestaetigt.
        std::string const bvset_fremd = bvset_live + "X";
        std::string const weg_fremd   = tlz::make_lazy_adhoc_fingerprint_mit_bvset_fn_from_env()(id, bvset_fremd);
        std::cout << "            Neuer Maker    (bvset VERAENDERT)    = " << weg_fremd << "\n";
        if (weg_fremd == weg_neu) {
            std::cout << "  [#59 V-7] FEHLER: ein veraendertes bvset aendert den Hash NICHT -- der Parameter "
                         "ist wirkungslos\n";
            ++gerissen;
        }
    }

    std::cout << "\n==== LAG-Z1: "
              << (gerissen == 0 ? std::string{"ALLE FAELLE GEHALTEN"} : std::to_string(gerissen) + " FEHLER")
              << "  (Koeder=" << kKoeder << ") ====\n";
    return gerissen == 0 ? 0 : 1;
}
