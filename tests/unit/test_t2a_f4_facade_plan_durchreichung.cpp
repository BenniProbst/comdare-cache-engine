// test_t2a_f4_facade_plan_durchreichung.cpp -- T2-A/F4, Fable-Review-Befund MITTEL-1 (2026-08-06).
//
// DIE LUECKE, DIE HIER GESCHLOSSEN WIRD. Die F4-Mechanik (Batch-Plan als Dokument VOR dem Lauf +
// Phasen-Zaehler dagegen) war gebaut und auf Iterator-Ebene belegt (test_tp1_planer_filter_iterator,
// Fall 11) -- aber im PRODUKTIVEN Lauf unerreichbar: kein Host baut eine LazyRunConfig selbst, der
// produktive Weg laeuft ausschliesslich ueber die run_profile-Fassade, und die reichte das Feld
// `batch_plan_datei` nicht durch. Unbemerkt blieb das, weil KEIN Test die DURCHREICHUNG prueft: die
// Iterator-Tests setzen cfg.batch_plan_datei selbst und springen damit ueber genau das Glied hinweg,
// das fehlte. Diese Wache prueft deshalb nicht die Mechanik (die ist dort belegt), sondern die KETTE
// RunProfileArgs::batch_plan_datei -> make_cfg -> LazyRunConfig::batch_plan_datei -- und zwar an ihrer
// WIRKUNG, nicht an einer Zuweisung: liegt der Plan nach dem Lauf auf der Platte, ist er angekommen.
//
// WAS BEWIESEN WIRD (gegen das ECHTE run_profile, Stub-Compile, FakeTransport, kein minio):
//   (1) ARG GESETZT -> PLAN DA: der Lauf legt Plan UND Zaehler unter dem uebergebenen Pfad ab, der
//       Plan-Leser nimmt das Dokument gegen den Stempel DIESER Selektion an, und der Zaehler weist
//       sich gegen genau diesen Plan aus.
//   (2) ARG LEER -> NICHTS: derselbe Lauf ohne den Pfad legt weder Plan noch Zaehler an -- das
//       Opt-in-Muster (PlanPersistenz::aktiv()==false => inert) bleibt erhalten.
//   (3) DIE WIRKUNG IST DER RESUME, nicht nur eine Datei: ein zweiter Lauf gegen dieselbe Ablage mit
//       FRISCHEM Ausgabe-Verzeichnis KOMPILIERT nichts mehr (das Verzeichnis bleibt leer) und endet
//       trotzdem REGULAER -- er fuehrt die geerbten Atome als bereitgestellt und weist sie in der
//       Bilanz aus. Ohne die Durchreichung baut er die volle Selektion erneut -- das ist der
//       Unterschied, den der Owner-KERN meint.
//
// WAS HIER GEHEILT WURDE (T2-A/F4-BILANZ, 2026-08-06). Fall (3) stand bis heute als STOLPERDRAHT im
// Baum: ein VOLLSTAENDIG plan-resumierter provision-only-Lauf endete mit exit_code=1. Der Grund lag eine
// Ebene tiefer -- die vom Plan-Resume uebersprungenen FUEHRENDEN Faecher erreichen den Slice-Loop nie,
// also buchte niemand ihre Atome in die BuildStats, LazyRunResult::built blieb 0, und genau daran haengt
// der Erfolgs-Test des provision-only-Modus ("mind. 1 DLL bereitgestellt"). Die LAGER-Skips wurden an
// derselben Stelle sehr wohl gebucht: eine Asymmetrie gegen die eigene Definition von `built`. Sie ist
// geheilt (Buchung + [BILANZ-TESTAT]-Zeile mit dem neuen Feld plan_skip=), und der Stolperdraht ist
// hier zur ZUSAGE gedreht -- genau wie sein Kommentar es verlangt hat. Der TEILWEISE Resume (einige
// Faecher gedeckt, einige nicht) ist ein FACH-Phaenomen und bei Korn 4096 nur mit >4096 Binaries
// auszuloesen; er ist deshalb dort belegt, wo das Korn eine Naht hat: test_tp1_planer_filter_iterator
// Fall (11c2), am echten run_planer_driven_provision.
//
// PROFIL: planner_thesis_min.profile.xml -- ohne <axis_sweeps> und ohne <sota_series>, also GENAU EIN
// Selektions-Pass. Das ist Absicht: eine Ablage je Lauf traegt heute genau dort (s. die Grenze am
// RunProfileArgs-Feld); ein Mehr-Pass-Profil wuerde die Ablage zwischen den Paessen umschreiben und
// dieser Test wuerde eine Eigenschaft messen, die die Kette nicht zusagt.
//
// Build: plain main (KEIN gtest), Return 0/1 -- Muster test_tp1_planer_filter_iterator.

#include "profile_run_entry.hpp" // die geprueften Glieder: RunProfileArgs + make_cfg (run_profile)

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_F4_THESIS_MIN
#error "COMDARE_F4_THESIS_MIN must point to tests/unit/thesis_tiere/planner_thesis_min.profile.xml"
#endif

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace fs  = std::filesystem;

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

// In-Memory-Transport (Muster test_tp1_planer_filter_iterator): das Bestandslog muss AKTIV sein, sonst
// bleibt planer_driven_active false und der planer-getriebene Pfad -- der einzige, der den Plan legt --
// wird gar nicht betreten.
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

constexpr char const* kDocKey = "bestandslog/test_f4_facade.xml";

// Die Lauf-Args der Fassade. NUR `batch_plan_datei` und das Ausgabe-Verzeichnis unterscheiden die Faelle
// -- alles andere ist zwischen ihnen gleich, damit die Zurechnung eindeutig bleibt.
tlz::RunProfileArgs mach_args(FakeStore& store, fs::path const& lauf, fs::path const& plan) {
    // Das Lauf-Verzeichnis legt der HOST an, nicht run_profile (es oeffnet die CSV und bricht sonst mit
    // "CSV nicht oeffenbar" ab) -- hier steht der Test in der Rolle des Hosts.
    std::error_code ec;
    fs::create_directories(lauf, ec);
    tlz::RunProfileArgs a;
    a.profile_path = fs::path{COMDARE_F4_THESIS_MIN};
    a.out_csv      = lauf / "measurements.csv";
    a.src_dir      = lauf / "src";
    a.dll_dir      = lauf / "dll";
    // Stub-Compile: der Bau-Erfolg ist hier nicht die Frage, die Ablage ist es. 0 == Erfolg.
    a.compile           = [](ex::BuildJob const&) -> int { return 0; };
    a.provision_only    = true; // planer_driven_active = bestandslog_active UND provision_only
    a.run_sota_series   = false;
    a.max_binaries      = 1;
    a.cores_per_build   = 1;
    a.build_parallelism = 1;
    // Das Bestandslog scharf, aber ohne Lager-Inhalt: der Key-Provider antwortet nie -> jede Binary gilt
    // als fehlend -> die Selektion wird wirklich gebaut (und der Zaehler traegt sie).
    a.bestand_transport = store.transport();
    a.bestand_doc_key   = kDocKey;
    a.bestand_key_of    = [](fs::path const&) -> std::optional<std::string> { return std::nullopt; };
    a.batch_plan_datei  = plan; // DAS gepruefte Glied (leer im Negativ-Fall)
    return a;
}

} // namespace

int main() {
    std::error_code ec;
    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_t2a_f4_facade";
    fs::remove_all(base, ec); // Reste eines Vorlaufs -- (2) prueft ABWESENHEIT und darf nichts erben
    fs::create_directories(base, ec);

    std::cout << "== T2-A/F4: die Plan-Ablage reist durch die run_profile-Fassade ==\n";

    // -- (1) ARG GESETZT -> PLAN UND ZAEHLER LIEGEN DA.
    fs::path const plan_datei = base / "ablage" / "batch_plan.txt";
    fs::path const z_datei    = fs::path{plan_datei.string() + ".zaehler"};
    FakeStore      store1;
    std::uint64_t  lauf1_bereitgestellt = 0;
    {
        tlz::RunProfileArgs const   a = mach_args(store1, base / "lauf1", plan_datei);
        tlz::RunProfileResult const r = tlz::run_profile(a);
        lauf1_bereitgestellt          = r.any_provisioned;
        check_eq("(1) der Lauf endet regulaer", r.exit_code, 0);
        check_true("(1) und hat wirklich etwas bereitgestellt (sonst waere (3) trivial)",
                   lauf1_bereitgestellt > 0);
    }
    check_true("(1) der Batch-Plan liegt unter dem uebergebenen Pfad", fs::exists(plan_datei, ec));
    check_true("(1) und der Phasen-Zaehler daneben", fs::exists(z_datei, ec));

    // Der Stempel der Selektion DIESES Laufs: max_binaries=1 => Basis-Selektion {0}. Nimmt der Leser das
    // Dokument gegen genau diesen Stempel an, ist der Pfad nicht irgendwie, sondern richtig angekommen.
    std::vector<std::size_t> const eins{0};
    std::string const              stamp   = bl::slice_plan_stamp(eins, bl::kBuildSliceGrain);
    auto const                     faecher = bl::read_batch_plan(plan_datei, stamp, ex::kLazyResumeRowsKey);
    check_true("(1) der Plan-Leser nimmt das Dokument gegen den Stempel dieser Selektion an",
               faecher.has_value());
    check_eq("(1) und es traegt genau ein Fach", faecher.has_value() ? faecher->size() : std::size_t{0},
             std::size_t{1});
    auto const zaehler = faecher.has_value()
                             ? bl::read_phasen_zaehler(z_datei, stamp, *faecher, ex::kLazyResumeRowsKey)
                             : std::nullopt;
    check_true("(1) der Zaehler weist sich gegen GENAU diesen Plan aus", zaehler.has_value());
    check_eq("(1) die Bau-Front traegt die Atome des Fensters",
             zaehler.has_value() ? zaehler->kompiliert : std::uint64_t{0}, std::uint64_t{1});
    check_eq("(1) die Mess-Front bleibt 0 (dieser Lauf hat nicht gemessen)",
             zaehler.has_value() ? zaehler->gemessen : std::uint64_t{99}, std::uint64_t{0});

    // -- (2) ARG LEER -> NICHTS. Derselbe Lauf, nur ohne Ablage-Pfad: das Opt-in-Muster bleibt inert.
    {
        fs::path const      plan_leer = base / "ablage_leer" / "batch_plan.txt";
        FakeStore           store2;
        tlz::RunProfileArgs a = mach_args(store2, base / "lauf2", plan_leer);
        a.batch_plan_datei.clear(); // DER Unterschied zu (1)
        tlz::RunProfileResult const r = tlz::run_profile(a);
        check_eq("(2) auch dieser Lauf endet regulaer", r.exit_code, 0);
        check_true("(2) KEIN Plan -- leerer Pfad heisst inert, nicht 'irgendwohin'",
                   !fs::exists(plan_leer, ec));
        check_true("(2) und kein Zaehler", !fs::exists(fs::path{plan_leer.string() + ".zaehler"}, ec));
        check_true("(2) das Ablage-Verzeichnis wird gar nicht erst angelegt",
                   !fs::exists(plan_leer.parent_path(), ec));
    }

    // -- (3) DIE WIRKUNG: der zweite Lauf gegen DIESELBE Ablage setzt hinter dem gedeckten Fach auf.
    //    Frisches Ausgabe-Verzeichnis, damit ein etwaiger Bau SICHTBAR waere -- ohne die Durchreichung
    //    stuende hier wieder die volle Selektion.
    {
        fs::path const              lauf3 = base / "lauf3";
        tlz::RunProfileArgs const   a     = mach_args(store1, lauf3, plan_datei);
        tlz::RunProfileResult const r     = tlz::run_profile(a);
        // (3a) DER RESUME GREIFT: es wird NICHTS mehr kompiliert. Der Beleg ist die Platte und nicht eine
        //      Zahl -- das Ausgabe-Verzeichnis dieses Laufs ist frisch, ein Bau haette es gefuellt.
        check_true("(3a) der Folgelauf kompiliert nichts mehr -- sein Ausgabe-Verzeichnis bleibt leer",
                   !fs::exists(lauf3 / "dll", ec) || fs::is_empty(lauf3 / "dll", ec));
        // (3b) UND ER IST TROTZDEM EIN GUELTIGER LAUF. Das ist die geheilte Asymmetrie (s. Kopf): die vom
        //      Plan-Zaehler gedeckten Atome sind BEREITGESTELLT und werden seit T2-A/F4-BILANZ genauso
        //      gebucht wie ein Lager-Skip. Bis zur Heilung stand hier exit_code=1 als STOLPERDRAHT --
        //      diese Zeile ist seine bewusste Nachfuehrung, nicht sein stilles Verschwinden.
        check_eq("(3b) voll plan-resumiert == exit 0 (geheilt; frueher TRIPWIRE auf exit 1)", r.exit_code, 0);
        // (3c) DIE BILANZ WEIST DIE RESUMIERTEN ATOME AUS -- und zwar genau die eine des einen Fachs.
        //      Ohne diese Zahl waere (3b) nur ein anders begruendetes Gruen.
        check_eq("(3c) die Bilanz fuehrt das geerbte Atom als bereitgestellt", r.any_provisioned,
                 std::uint64_t{1});
        check_eq("(3c) und es ist dasselbe eine Atom, das Lauf 1 gebaut hat", r.any_provisioned,
                 lauf1_bereitgestellt);
        // (3d) GEGENPROBE ZUR HERKUNFT: gemessen hat dieser Lauf nichts, resumiert im MESS-Sinn auch
        //      nichts. Das Gruen kommt AUSSCHLIESSLICH aus provision_ok -- also aus der neuen Buchung.
        check_eq("(3d) nichts gemessen (provision-only)", r.any_measured, std::uint64_t{0});
        check_eq("(3d) und nichts mess-resumiert -- das Gruen traegt allein die Bereitstellung",
                 r.any_resumed, std::uint64_t{0});
        // (3e) DER ZAEHLER IST UNVERAENDERT GEBLIEBEN: ein Lauf, der nichts vollzieht, schreibt auch
        //      nichts fort. Die Bau-Front steht weiter bei dem einen Atom, die Mess-Front weiter bei 0.
        auto const faecher3 = bl::read_batch_plan(plan_datei, stamp, ex::kLazyResumeRowsKey);
        auto const zaehler3 = faecher3.has_value()
                                  ? bl::read_phasen_zaehler(z_datei, stamp, *faecher3, ex::kLazyResumeRowsKey)
                                  : std::nullopt;
        check_true("(3e) der Zaehler steht unveraendert gegen denselben Plan", zaehler3.has_value());
        check_eq("(3e) Bau-Front unveraendert", zaehler3.has_value() ? zaehler3->kompiliert : std::uint64_t{99},
                 std::uint64_t{1});
        check_eq("(3e) Mess-Front unveraendert", zaehler3.has_value() ? zaehler3->gemessen : std::uint64_t{99},
                 std::uint64_t{0});
    }

    fs::remove_all(base, ec);
    std::cout << (g_fail == 0 ? "== ALLE PRUEFUNGEN BESTANDEN ==\n" : "== FEHLGESCHLAGEN ==\n");
    return g_fail == 0 ? 0 : 1;
}
